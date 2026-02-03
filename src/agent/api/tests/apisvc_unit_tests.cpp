/**
 * @file apisvc_unit_tests.cpp
 * @brief Unit Tests for apisvc library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/aducsdk.h"
#include "aduc/apiproto.h"
#include "aduc/apisvc.h"
#include "aduc/config_utils.h"
#include "aduc/defer.hpp"
#include "aduc/logging.h"
#include "aduc/result.h"
#include "aduc/viewstatemgr.h"

#include <arpa/inet.h>
#include <catch2/catch_all.hpp>
#include <chrono>
#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <poll.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

// Timeout in milliseconds for FIFO operations
constexpr int FIFO_OPEN_TIMEOUT_MS = 10000; // 10 seconds
constexpr int FIFO_POLL_INTERVAL_MS = 50;   // 50 ms polling interval

// Test data directory for FIFO files - use compile-time ADUC_TEST_DATA_FOLDER
static const std::string TEST_DATA_DIR = std::string{ ADUC_TEST_DATA_FOLDER } + "/apisvc";

/**
 * @brief Opens a FIFO with a timeout using non-blocking I/O and polling.
 * @param path The path to the FIFO
 * @param flags O_RDONLY or O_WRONLY (O_NONBLOCK will be added internally)
 * @param timeout_ms Timeout in milliseconds
 * @return File descriptor on success, -1 on timeout or error
 */
static int open_fifo_with_timeout(const char* path, int flags, int timeout_ms)
{
    auto start = std::chrono::steady_clock::now();
    int fd = -1;

    while (true)
    {
        fd = open(path, flags | O_NONBLOCK);
        if (fd >= 0)
        {
            // Successfully opened, remove O_NONBLOCK for normal blocking read/write
            int current_flags = fcntl(fd, F_GETFL);
            if (current_flags != -1)
            {
                fcntl(fd, F_SETFL, current_flags & ~O_NONBLOCK);
            }
            return fd;
        }

        if (errno != ENXIO && errno != EAGAIN)
        {
            // Real error, not just "no reader/writer yet"
            return -1;
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if (elapsed_ms >= timeout_ms)
        {
            // Timeout
            errno = ETIMEDOUT;
            return -1;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(FIFO_POLL_INTERVAL_MS));
    }
}

using Catch::Matchers::Equals;

ViewStateManager g_vsm = { 0 };

TEST_CASE("apisvc crossproc tests")
{
    // Ensure test data directory exists
    std::filesystem::create_directories(TEST_DATA_DIR);
    aduc::Defer defer_cleanup([]() { std::filesystem::remove_all(TEST_DATA_DIR); });

    REQUIRE(0 == viewstatemgr_create(&g_vsm));
    aduc::Defer defer_destroy([&]() { viewstatemgr_destroy(&g_vsm); });

    ADUC_Logging_Init(ADUC_LOG_DEBUG, "apisvc_unit_tests");
    aduc::Defer defer_logging([]() { ADUC_Logging_Uninit(); });

    SECTION("GETSTATE")
    {
        REQUIRE(viewstatemgr_svcstatus_set(&g_vsm, ADUC_ServiceStatus_Installing));

        // Use test data directory for FIFO files
        std::string reqFifoPath = TEST_DATA_DIR + "/test_req_fifo";
        std::string respFifoPath = TEST_DATA_DIR + "/test_resp_fifo";

        REQUIRE(init_api_svc(reqFifoPath.c_str()));

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        {
            std::filesystem::path p{ respFifoPath };
            if (std::filesystem::exists(p))
            {
                REQUIRE(std::filesystem::remove(p));
            }
        }
        // Use 0660 permissions to match FIFO_FILE_MODE expected by verify_fifo_security()
        int ret_resp_fifo = mkfifo(respFifoPath.c_str(), 0660);
        bool cond = ret_resp_fifo == 0 || errno == EEXIST;
        REQUIRE(cond);

        aduc::Defer defer_rm_fifo([&respFifoPath]() { unlink(respFifoPath.c_str()); });

        // open request fifo for writing (with timeout to avoid indefinite blocking in CI)
        int reqFifo = open_fifo_with_timeout(reqFifoPath.c_str(), O_WRONLY, FIFO_OPEN_TIMEOUT_MS);
        REQUIRE(reqFifo != -1);

        aduc::Defer defer_close_req_fifo([reqFifo]() -> void { close(reqFifo); });

        // Write GET_STATE request
        size_t slen = respFifoPath.length();
        REQUIRE(slen <= MAX_BUF_LEN);
        ApiWireRequestMsg req = { .ver = 1, .type = ApiRequestType_GETSTATE, .len = (uint16_t)slen };
        strncpy(req.data, respFifoPath.c_str(), slen);

        ssize_t n = msg_send_req(reqFifo, &req);
        REQUIRE(n == 3 * sizeof(uint16_t) + slen);

        // open response fifo for reading (with timeout to avoid indefinite blocking in CI)
        int respFifo = open_fifo_with_timeout(respFifoPath.c_str(), O_RDONLY, FIFO_OPEN_TIMEOUT_MS);
        REQUIRE(respFifo != -1);

        aduc::Defer defer_close_resp_fifo([respFifo]() -> void { close(respFifo); });

        // Read response
        ApiWireResponseMsg resp = { 0 };
        n = msg_recv_resp(respFifo, (ApiWireResponseMsg*)&resp);
        REQUIRE(n == 2 * sizeof(uint16_t));
        CHECK(resp.code == (uint16_t)ApiRequestType_GETSTATE);
        CHECK(resp.ret_val == (uint16_t)ADUC_ServiceStatus_Installing);

        CHECK(uninit_api_svc());
    }
}
