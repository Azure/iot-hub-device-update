/**
 * @file shutdown_service_ut.cpp
 * @brief Unit Tests for shutdown_service module
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

extern "C"
{
#include "aduc/shutdown_service.h"
}

/**
 * @brief Test that ADUC_ShutdownService_ShouldKeepRunning returns true initially
 *
 * Note: Due to the static state in shutdown_service.c, these tests must be run
 * in a specific order. The initial state test must run first.
 */
TEST_CASE("ADUC_ShutdownService - Initial State", "[shutdown_service]")
{
    // Note: This test assumes fresh process start where shutdown hasn't been requested yet.
    // In a real-world scenario, the shutdown service state persists across tests within
    // the same test run after RequestShutdown is called.

    SECTION("ShouldKeepRunning returns true before shutdown is requested")
    {
        // We can only verify the behavior - if shutdown was already requested in a previous
        // test, this would fail. In production, this would be true at startup.
        // For test isolation, we document this limitation.
        bool keepRunning = ADUC_ShutdownService_ShouldKeepRunning();
        // Initial state should be "keep running" (true)
        // If this fails, it means shutdown was already requested in another test
        CHECK(keepRunning == true);
    }
}

/**
 * @brief Test the shutdown request functionality
 */
TEST_CASE("ADUC_ShutdownService - Shutdown Request", "[shutdown_service]")
{
    SECTION("RequestShutdown changes state to not keep running")
    {
        // Request shutdown
        ADUC_ShutdownService_RequestShutdown();

        // After requesting shutdown, ShouldKeepRunning should return false
        bool keepRunning = ADUC_ShutdownService_ShouldKeepRunning();
        CHECK(keepRunning == false);
    }

    SECTION("Multiple shutdown requests are idempotent")
    {
        // Request shutdown multiple times
        ADUC_ShutdownService_RequestShutdown();
        ADUC_ShutdownService_RequestShutdown();
        ADUC_ShutdownService_RequestShutdown();

        // Should still return false
        bool keepRunning = ADUC_ShutdownService_ShouldKeepRunning();
        CHECK(keepRunning == false);
    }
}

/**
 * @brief Test that demonstrates the state is persistent
 */
TEST_CASE("ADUC_ShutdownService - State Persistence", "[shutdown_service]")
{
    SECTION("State persists across multiple calls to ShouldKeepRunning")
    {
        // Get the current state multiple times
        bool state1 = ADUC_ShutdownService_ShouldKeepRunning();
        bool state2 = ADUC_ShutdownService_ShouldKeepRunning();
        bool state3 = ADUC_ShutdownService_ShouldKeepRunning();

        // All calls should return the same value
        CHECK(state1 == state2);
        CHECK(state2 == state3);
    }
}
