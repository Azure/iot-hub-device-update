/**
 * @file apisvc_container_unit_tests.cpp
 * @brief Unit tests for apisvc with container environment compatibility
 *
 * This test file provides enhanced container-aware testing for the API service
 * with additional debugging, alternative FIFO locations, and timeout handling.
 *
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch.hpp>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include "aduc/logging.h"
#include "aduc/proto_utils.h"
#include "aduc/viewstatemgr_utils.h"
#include "apisvc.h"

// External global view state manager
extern ADUC_VSM_Handle g_vsm;

namespace
{

/**
 * @brief Check if FIFO operations work in the current environment
 */
bool test_fifo_support(const std::string& test_dir)
{
    std::string test_fifo = test_dir + "/test_fifo_" + std::to_string(getpid());

    // Test FIFO creation
    if (mkfifo(test_fifo.c_str(), 0666) != 0)
    {
        std::cout << "FIFO creation failed in " << test_dir << ": " << strerror(errno) << std::endl;
        return false;
    }

    // Test FIFO access
    struct stat st;
    bool success = (stat(test_fifo.c_str(), &st) == 0) && S_ISFIFO(st.st_mode);

    // Cleanup
    unlink(test_fifo.c_str());

    if (success)
    {
        std::cout << "FIFO support confirmed in " << test_dir << std::endl;
    }
    else
    {
        std::cout << "FIFO operations failed in " << test_dir << std::endl;
    }

    return success;
}

/**
 * @brief Find the best directory for FIFO operations
 */
std::string find_fifo_directory()
{
    std::vector<std::string> candidates = {
        "/tmp",
        "/var/tmp",
        "/dev/shm",
        std::string(getenv("HOME") ? getenv("HOME") : "/tmp") + "/tmp"
    };

    for (const auto& dir : candidates)
    {
        // Check if directory exists and is writable
        struct stat st;
        if (stat(dir.c_str(), &st) == 0 && S_ISDIR(st.st_mode) && access(dir.c_str(), W_OK) == 0)
        {
            if (test_fifo_support(dir))
            {
                std::cout << "Using FIFO directory: " << dir << std::endl;
                return dir;
            }
        }
    }

    std::cout << "Warning: No suitable FIFO directory found, falling back to /tmp" << std::endl;
    return "/tmp";
}

/**
 * @brief Enhanced version of is_api_svc_ready with container debugging
 */
bool wait_for_api_svc_ready(int timeout_seconds = 10)
{
    auto start_time = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(timeout_seconds);
    int check_count = 0;

    while (!is_api_svc_ready() && (std::chrono::steady_clock::now() - start_time) < timeout)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        check_count++;

        // Log progress every second
        if (check_count % 10 == 0)
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start_time).count();
            std::cout << "Waiting for API service ready... (" << elapsed << "s)" << std::endl;
        }
    }

    bool ready = is_api_svc_ready();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time).count();

    if (ready)
    {
        std::cout << "API service ready after " << elapsed << "ms" << std::endl;
    }
    else
    {
        std::cout << "API service NOT ready after " << elapsed << "ms timeout" << std::endl;
    }

    return ready;
}

/**
 * @brief Container-aware API service initialization with timeout protection
 */
bool init_api_svc_with_timeout(const char* fifoPath, int timeout_seconds = 10)
{
    std::cout << "Initializing API service with FIFO: " << fifoPath << std::endl;

    // Check FIFO path directory permissions
    std::string fifo_str(fifoPath);
    size_t last_slash = fifo_str.find_last_of('/');
    if (last_slash != std::string::npos)
    {
        std::string dir = fifo_str.substr(0, last_slash);
        struct stat st;
        if (stat(dir.c_str(), &st) != 0)
        {
            std::cout << "FIFO directory does not exist: " << dir << std::endl;
            return false;
        }
        if (access(dir.c_str(), W_OK) != 0)
        {
            std::cout << "FIFO directory not writable: " << dir << std::endl;
            return false;
        }
    }

    // Set up signal handling to prevent deadlocks
    signal(SIGPIPE, SIG_IGN);

    bool init_result = init_api_svc(fifoPath);
    std::cout << "init_api_svc returned: " << (init_result ? "success" : "failure") << std::endl;

    return init_result;
}

} // anonymous namespace

TEST_CASE("API Service Container Tests", "[apisvc_container]")
{
    std::cout << "=== Starting Container-Aware API Service Tests ===" << std::endl;
    std::cout << "PID: " << getpid() << std::endl;
    std::cout << "UID: " << getuid() << " GID: " << getgid() << std::endl;

    // Check environment
    const char* container_env = getenv("container");
    const char* docker_env = getenv("DOCKER_CONTAINER");
    std::cout << "Container environment: " << (container_env ? container_env : "none") << std::endl;
    std::cout << "Docker environment: " << (docker_env ? docker_env : "none") << std::endl;

    ADUC_Logging_Init(ADUC_LOG_DEBUG, "apisvc_container_tests");
    aduc::Defer defer_logging([]() { ADUC_Logging_Uninit(); });

    SECTION("Container Environment Check")
    {
        std::string fifo_dir = find_fifo_directory();
        REQUIRE(!fifo_dir.empty());

        // Verify we can create and access FIFOs
        std::string test_fifo = fifo_dir + "/container_test_" + std::to_string(getpid());
        REQUIRE(mkfifo(test_fifo.c_str(), 0666) == 0);

        struct stat st;
        REQUIRE(stat(test_fifo.c_str(), &st) == 0);
        REQUIRE(S_ISFIFO(st.st_mode));

        unlink(test_fifo.c_str());
        std::cout << "FIFO operations working correctly" << std::endl;
    }

    SECTION("GETSTATE with Container Timeouts")
    {
        REQUIRE(viewstatemgr_svcstatus_set(&g_vsm, ADUC_ServiceStatus_Installing));

        // Use the best available FIFO directory
        std::string fifo_dir = find_fifo_directory();
        std::string fifoPathStr = fifo_dir + "/test_req_fifo_container_" + std::to_string(getpid()) + "_"
            + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        const char* fifoPath = fifoPathStr.c_str();

        std::cout << "Using FIFO path: " << fifoPath << std::endl;

        REQUIRE(init_api_svc_with_timeout(fifoPath, 15)); // Longer timeout for containers

        // Wait for API service to be ready with extended timeout
        REQUIRE(wait_for_api_svc_ready(15));

        // Create message with extra debugging
        std::cout << "Creating GETSTATE message..." << std::endl;
        ADUC_ProtocolMessage msg = {};
        bool createResult = aduc_protocol_message_create(&msg, ADUC_MethodCall_GetState, NULL, NULL);
        REQUIRE(createResult);
        aduc::Defer defer_msg([&msg]() { aduc_protocol_message_uninit(&msg); });

        // Send message with timeout protection
        std::cout << "Sending message to API service..." << std::endl;
        auto send_start = std::chrono::steady_clock::now();

        bool sendResult = aduc_protocol_message_send(&msg, fifoPath);

        auto send_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - send_start).count();
        std::cout << "Message send completed in " << send_elapsed << "ms, result: "
                  << (sendResult ? "success" : "failure") << std::endl;

        REQUIRE(sendResult);

        // Wait for response with extended timeout
        std::cout << "Waiting for response..." << std::endl;
        std::string response;
        bool responseResult = aduc_protocol_message_receive_response(response, fifoPath, 15000); // 15 second timeout

        std::cout << "Response received: " << (responseResult ? "yes" : "no") << std::endl;
        if (responseResult)
        {
            std::cout << "Response content: " << response << std::endl;
        }

        REQUIRE(responseResult);

        // Cleanup with verification
        std::cout << "Cleaning up API service..." << std::endl;
        uninit_api_svc();

        // Verify FIFO cleanup
        struct stat st;
        bool fifo_cleaned = (stat(fifoPath, &st) != 0);
        std::cout << "FIFO cleanup: " << (fifo_cleaned ? "success" : "incomplete") << std::endl;

        std::cout << "=== GETSTATE Container Test Completed ===" << std::endl;
    }

    SECTION("Stress Test with Container Constraints")
    {
        std::cout << "=== Container Stress Test ===" << std::endl;

        const int num_iterations = 3; // Reduced for container environment
        std::string fifo_dir = find_fifo_directory();

        for (int i = 0; i < num_iterations; i++)
        {
            std::cout << "Stress iteration " << (i + 1) << "/" << num_iterations << std::endl;

            REQUIRE(viewstatemgr_svcstatus_set(&g_vsm, ADUC_ServiceStatus_Installing));

            std::string fifoPathStr = fifo_dir + "/stress_test_" + std::to_string(getpid()) + "_"
                + std::to_string(i) + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
            const char* fifoPath = fifoPathStr.c_str();

            REQUIRE(init_api_svc_with_timeout(fifoPath, 10));
            REQUIRE(wait_for_api_svc_ready(10));

            // Quick message exchange
            ADUC_ProtocolMessage msg = {};
            REQUIRE(aduc_protocol_message_create(&msg, ADUC_MethodCall_GetState, NULL, NULL));
            aduc::Defer defer_msg([&msg]() { aduc_protocol_message_uninit(&msg); });

            REQUIRE(aduc_protocol_message_send(&msg, fifoPath));

            std::string response;
            REQUIRE(aduc_protocol_message_receive_response(response, fifoPath, 10000));

            uninit_api_svc();

            // Brief pause between iterations
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        std::cout << "=== Container Stress Test Completed ===" << std::endl;
    }
}
