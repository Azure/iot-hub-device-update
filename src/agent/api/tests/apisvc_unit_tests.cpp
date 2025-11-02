/**
 * @file apisvc_unit_tests.cpp
 * @brief Unit Tests for apisvc library
 *
 * These tests verify cross-process communication between API clients and the
 * Device Update agent service. The tests have been designed to handle race
 * conditions that can occur when multiple test instances run simultaneously.
 *
 * Race condition fixes implemented:
 * 1. Unique FIFO paths using PID + timestamp to prevent conflicts
 * 2. Proper synchronization waiting for service readiness
 * 3. Robust timeout handling with multiple protection layers
 *
 * To verify race condition fixes, run parallel tests:
 *   for i in {1..5}; do timeout 10s ./bin/apisvc_unit_tests & done; wait
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
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

using Catch::Matchers::Equals;

ViewStateManager g_vsm = { 0 };

TEST_CASE("apisvc crossproc tests")
{
    REQUIRE(0 == viewstatemgr_create(&g_vsm));
    aduc::Defer defer_destroy([&]() { viewstatemgr_destroy(&g_vsm); });

    ADUC_Logging_Init(ADUC_LOG_DEBUG, "apisvc_unit_tests");
    aduc::Defer defer_logging([]() { ADUC_Logging_Uninit(); });

    SECTION("GETSTATE")
    {
        REQUIRE(viewstatemgr_svcstatus_set(&g_vsm, ADUC_ServiceStatus_Installing));

        // Use unique FIFO paths to avoid conflicts when tests run in parallel
        std::string fifoPathStr = "/tmp/test_req_fifo_" + std::to_string(getpid()) + "_"
            + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        const char* fifoPath = fifoPathStr.c_str();
        REQUIRE(init_api_svc(fifoPath));

        // Wait for API service to be ready with timeout
        auto start_time = std::chrono::steady_clock::now();
        const auto timeout = std::chrono::seconds(5);
        while (!is_api_svc_ready() && (std::chrono::steady_clock::now() - start_time) < timeout)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        REQUIRE(is_api_svc_ready()); // Ensure the service is actually ready

        std::string respFifoPathStr = "/tmp/test_resp_fifo_" + std::to_string(getpid()) + "_"
            + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        const char* respFifoPath = respFifoPathStr.c_str();
        {
            std::filesystem::path p{ respFifoPath };
            if (std::filesystem::exists(p))
            {
                REQUIRE(std::filesystem::remove(p));
            }
        }
        int ret_resp_fifo = mkfifo(respFifoPath, 0666);
        bool cond = ret_resp_fifo == 0 || errno == EEXIST;
        REQUIRE(cond);

        aduc::Defer defer_rm_fifo([respFifoPath]() { unlink(respFifoPath); });

        // open request fifo for writing
        int reqFifo = open(fifoPath, O_WRONLY);
        REQUIRE(reqFifo != -1);

        aduc::Defer defer_close_req_fifo([reqFifo]() -> void { close(reqFifo); });

        // Write GET_STATE request
        size_t slen = strlen(respFifoPath);
        REQUIRE(slen <= MAX_BUF_LEN);
        ApiWireRequestMsg req = { .ver = 1, .type = ApiRequestType_GETSTATE, .len = (uint16_t)slen };
        strncpy(req.data, respFifoPath, slen);

        ssize_t n = msg_send_req(reqFifo, &req);
        REQUIRE(n == 3 * sizeof(uint16_t) + slen);

        // open response fifo for reading--open will block until data is available in fifo queue
        int respFifo = open(respFifoPath, O_RDONLY);
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
