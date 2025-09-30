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

        const char* fifoPath = "/tmp/test_req_fifo";
        REQUIRE(init_api_svc(fifoPath));

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        const char* respFifoPath = "/tmp/test_resp_fifo";
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
        ApiWireRequestMsg req = { .ver = htons(1),
                                  .type = htons(ApiRequestType_GETSTATE),
                                  .len = htons((uint16_t)slen) };
        strncpy(req.data, respFifoPath, slen);
        ssize_t n = write(reqFifo, &req, sizeof(req));
        REQUIRE(n == sizeof(req));

        // open response fifo for reading--open will block until data is available in fifo queue
        int respFifo = open(respFifoPath, O_RDONLY);
        REQUIRE(respFifo != -1);

        aduc::Defer defer_close_resp_fifo([respFifo]() -> void { close(respFifo); });

        // Read response
        ApiWireResponseMsg resp = { 0 };
        n = read(respFifo, &resp, sizeof(resp));
        REQUIRE(n == sizeof(resp));
        CHECK(ntohs(resp.code) == (uint16_t)ApiRequestType_GETSTATE);
        CHECK(ntohs(resp.ret_val) == (uint16_t)ADUC_ServiceStatus_Installing);

        CHECK(uninit_api_svc());
    }
}
