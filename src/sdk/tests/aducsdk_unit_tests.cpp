/**
 * @file aducsdk_unit_tests.cpp
 * @brief Unit tests for the public ADU Status SDK API (GetAduServiceStatus).
 *
 * These tests drive the real public SDK ABI end-to-end. To make that possible
 * outside of a privileged production environment, the SDK source (aducsdk.c)
 * is compiled privately into this test target with overridden values for
 * ADUC_API_DEFAULT_FIFO_PATH and ADUC_DATA_FOLDER (see CMakeLists.txt).
 *
 * Bug regressions covered:
 *   1. get_rnd_suffix used to embed '\0' bytes in the random suffix, which
 *      truncated the response FIFO path and intermittently broke the call.
 *      Verified by stress-running GetAduServiceStatus() many times against a
 *      real init_api_svc() server.
 *   2. The SDK used to leave orphaned resp_*.fifo files on disk when the
 *      request-FIFO open failed after the response FIFO had been created.
 *   3. The SDK used to accept any resp.code from the agent without validating
 *      it matched the request type, allowing a spoofed response to be
 *      returned as a status enum.
 */
#include "aduc/aducsdk.h"
#include "aduc/apiproto.h"
#include "aduc/apisvc.h"
#include "aduc/viewstatemgr.h"

#include <arpa/inet.h>
#include <atomic>
#include <catch2/catch_all.hpp>
#include <chrono>
#include <cstring>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

namespace fs = std::filesystem;

// apisvc.c references this global ViewStateManager symbol; the test owner
// is responsible for defining it (matches the pattern in apisvc_unit_tests).
ViewStateManager g_vsm = { 0 };

// SDK test data subtree (must match SDK_TEST_DATA_DIR in CMakeLists.txt).
static const std::string TEST_DATA_DIR = std::string{ ADUC_TEST_DATA_FOLDER } + "/aducsdk";
static const std::string API_DIR = TEST_DATA_DIR + "/api";
static const std::string REQ_FIFO_PATH = API_DIR + "/apireq.fifo";

static int count_resp_fifos()
{
    int n = 0;
    DIR* d = opendir(API_DIR.c_str());
    if (d == nullptr)
    {
        return 0;
    }
    while (struct dirent* e = readdir(d))
    {
        if (std::strncmp(e->d_name, "resp_", 5) == 0)
        {
            ++n;
        }
    }
    closedir(d);
    return n;
}

static void reset_test_dir()
{
    std::error_code ec;
    fs::remove_all(TEST_DATA_DIR, ec);
    fs::create_directories(API_DIR, ec);
}

TEST_CASE("aducsdk: GetAduServiceStatus end-to-end via init_api_svc")
{
    reset_test_dir();

    ViewStateManager vsm = { 0 };
    REQUIRE(viewstatemgr_create(&vsm) == 0);
    REQUIRE(viewstatemgr_svcstatus_set(&vsm, ADUC_ServiceStatus_Idle));
    // apisvc.c reads from a global g_vsm - mirror the local one into it.
    g_vsm = vsm;

    REQUIRE(init_api_svc(REQ_FIFO_PATH.c_str()));
    // Give the apisvc thread a moment to open the request FIFO for read.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    SECTION("happy path returns the configured status")
    {
        ADUC_ServiceStatus s = GetAduServiceStatus();
        CHECK(s == ADUC_ServiceStatus_Idle);
    }

    SECTION("bug #1 regression: random suffix never truncates the response path")
    {
        // With the original bug (modulus = sizeof(charset) including the
        // trailing NUL), each of the 12 suffix chars had a ~1.6% chance of
        // being '\0'. With a NUL embedded, req.data is truncated server-side
        // and the agent fails to open the response FIFO -> the SDK errors
        // out. Probability of seeing AT LEAST one failure across 50 calls
        // with the bug present is >99%. With the fix in place, all 50 must
        // succeed.
        const int N = 50;
        int failures = 0;
        for (int i = 0; i < N; ++i)
        {
            ADUC_ServiceStatus s = GetAduServiceStatus();
            if (s != ADUC_ServiceStatus_Idle)
            {
                ++failures;
            }
        }
        CHECK(failures == 0);
    }

    SECTION("bug #2 regression: no orphan resp_*.fifo files left on disk")
    {
        // Sanity check: stress-call the SDK and ensure cleanup happens after
        // each successful call.
        for (int i = 0; i < 20; ++i)
        {
            (void)GetAduServiceStatus();
        }
        CHECK(count_resp_fifos() == 0);
    }

    CHECK(uninit_api_svc());
    viewstatemgr_destroy(&vsm);
}

namespace
{
struct fake_agent_args
{
    std::string req_fifo;
    uint16_t bogus_code = 0;
    uint16_t bogus_ret_val = 0;
    std::atomic<int> ready{ 0 }; // 1 = req fifo opened, -1 = open failed
};

void fake_agent_thread(fake_agent_args* a)
{
    // O_RDWR so the open never blocks regardless of whether the writer is
    // attached yet (avoids a startup race with O_RDONLY).
    int rfd = open(a->req_fifo.c_str(), O_RDWR);
    if (rfd < 0)
    {
        a->ready.store(-1);
        return;
    }
    a->ready.store(1);

    uint16_t hdr_net[3] = { 0 };
    size_t total = 0;
    while (total < sizeof(hdr_net))
    {
        ssize_t n = read(rfd, reinterpret_cast<char*>(hdr_net) + total, sizeof(hdr_net) - total);
        if (n <= 0)
        {
            close(rfd);
            return;
        }
        total += static_cast<size_t>(n);
    }
    uint16_t len = ntohs(hdr_net[2]);

    char data[PIPE_BUF] = { 0 };
    if (len > 0 && len < (uint16_t)sizeof(data))
    {
        total = 0;
        while (total < len)
        {
            ssize_t n = read(rfd, data + total, len - total);
            if (n <= 0)
            {
                break;
            }
            total += static_cast<size_t>(n);
        }
        data[len] = '\0';
    }
    close(rfd);

    int wfd = open(data, O_WRONLY);
    if (wfd < 0)
    {
        return;
    }
    uint16_t resp_net[2] = { htons(a->bogus_code), htons(a->bogus_ret_val) };
    (void)write(wfd, resp_net, sizeof(resp_net));
    close(wfd);
}
} // namespace

TEST_CASE("aducsdk bug regressions (no apisvc)")
{
    reset_test_dir();

    SECTION("bug #2 regression: orphan resp_*.fifo not left when reqFifo open fails")
    {
        // Create the request FIFO with the right mode but DO NOT have a
        // reader attached. open(reqFifo, O_WRONLY|O_NONBLOCK) in the SDK
        // will fail with ENXIO, exercising the early-exit cleanup path
        // AFTER the response FIFO has been created.
        REQUIRE(mkfifo(REQ_FIFO_PATH.c_str(), 0660) == 0);
        REQUIRE(chmod(REQ_FIFO_PATH.c_str(), 0660) == 0);

        ADUC_ServiceStatus s = GetAduServiceStatus();
        // Some FIFO-side error is expected here.
        CHECK(s >= ADUC_ServiceStatus_ERROR_UnsupportedApiVersion);
        CHECK(count_resp_fifos() == 0);

        unlink(REQ_FIFO_PATH.c_str());
    }

    SECTION("bug #3 regression: bogus response code rejected")
    {
        REQUIRE(mkfifo(REQ_FIFO_PATH.c_str(), 0660) == 0);
        REQUIRE(chmod(REQ_FIFO_PATH.c_str(), 0660) == 0);

        fake_agent_args args;
        args.req_fifo = REQ_FIFO_PATH;
        args.bogus_code = 0xBEEF; // NOT ApiRequestType_GETSTATE (0x0001)
        args.bogus_ret_val = ADUC_ServiceStatus_Idle;

        std::thread th(fake_agent_thread, &args);

        // Wait for the fake agent to attach to the request FIFO.
        for (int i = 0; i < 200 && args.ready.load() == 0; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        REQUIRE(args.ready.load() == 1);

        ADUC_ServiceStatus s = GetAduServiceStatus();
        th.join();

        // The defensive SDK must NOT silently return the injected ret_val
        // when resp.code does not match the request type.
        CHECK(s != ADUC_ServiceStatus_Idle);
        CHECK(s == ADUC_ServiceStatus_ERROR_AgentServiceInternal);

        unlink(REQ_FIFO_PATH.c_str());
    }
}
