/**
 * @file command_helper_ut.cpp
 * @brief Unit Tests for command_helper module
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>
#include <cstring>
#include <ctime> // nanosleep
#include <string>
#include <unistd.h> // unlink

extern "C"
{
#include "aduc/command_helper.h"
#include <stdbool.h>

// Undefine logging macros so we can provide mock function implementations
#undef log_debug
#undef log_info
#undef log_warn
#undef log_error
#undef Log_Debug
#undef Log_Info
#undef Log_Warn
#undef Log_Error

    // Mock logging functions
    void Log_Error(const char* fmt, ...)
    {
        (void)fmt;
    }
    void Log_Warn(const char* fmt, ...)
    {
        (void)fmt;
    }
    void Log_Info(const char* fmt, ...)
    {
        (void)fmt;
    }
    void Log_Debug(const char* fmt, ...)
    {
        (void)fmt;
    }

    // Mock permission utils
    bool PermissionUtils_CheckOwnership(const char* path, const char* expectedUser, const char* expectedGroup)
    {
        (void)path;
        (void)expectedUser;
        (void)expectedGroup;
        return true;
    }

    bool PermissionUtils_VerifyProcessEffectiveGroup(const char* groupName)
    {
        (void)groupName;
        return true;
    }
}

// Test callback tracking
static bool g_callbackInvoked = false;
static std::string g_lastCommand;

extern "C"
{
    static bool TestCommandCallback(const char* command, void* context)
    {
        (void)context;
        g_callbackInvoked = true;
        if (command != NULL)
        {
            g_lastCommand = command;
        }
        return true;
    }
}

class CommandHelperTestFixture
{
public:
    CommandHelperTestFixture()
    {
        g_callbackInvoked = false;
        g_lastCommand.clear();
    }

    ~CommandHelperTestFixture()
    {
        // Remove any leftover FIFO to prevent state leaking between tests.
        // Without this, a FIFO created by the listener thread test can cause
        // SendCommand tests to block indefinitely on open(O_WRONLY).
        unlink(ADUC_COMMANDS_FIFO_NAME);
    }
};

TEST_CASE_METHOD(CommandHelperTestFixture, "RegisterCommand", "[command_helper]")
{
    SECTION("Successfully registers a command")
    {
        ADUC_Command cmd = { "test-command", TestCommandCallback };
        int result = RegisterCommand(&cmd);
        REQUIRE(result >= 0);

        // Clean up
        UnregisterCommand(&cmd);
    }

    SECTION("Returns valid index on register")
    {
        ADUC_Command cmd = { "test-command", TestCommandCallback };
        int index = RegisterCommand(&cmd);
        REQUIRE(index == 0);

        // Clean up
        UnregisterCommand(&cmd);
    }

    SECTION("Fails when max commands exceeded")
    {
        ADUC_Command cmd1 = { "command-1", TestCommandCallback };
        int result1 = RegisterCommand(&cmd1);
        REQUIRE(result1 >= 0);

        // Second registration should fail (MAX_COMMAND_ARRAY_SIZE is 1)
        ADUC_Command cmd2 = { "command-2", TestCommandCallback };
        int result2 = RegisterCommand(&cmd2);
        REQUIRE(result2 == -1);

        // Clean up
        UnregisterCommand(&cmd1);
    }
}

TEST_CASE_METHOD(CommandHelperTestFixture, "UnregisterCommand", "[command_helper]")
{
    SECTION("Successfully unregisters a registered command")
    {
        ADUC_Command cmd = { "test-unregister", TestCommandCallback };
        int regResult = RegisterCommand(&cmd);
        REQUIRE(regResult >= 0);

        bool unregResult = UnregisterCommand(&cmd);
        REQUIRE(unregResult == true);
    }

    SECTION("Fails to unregister non-existent command")
    {
        ADUC_Command cmd = { "nonexistent", TestCommandCallback };
        bool result = UnregisterCommand(&cmd);
        REQUIRE(result == false);
    }

    SECTION("Can re-register after unregister")
    {
        ADUC_Command cmd = { "test-reregister", TestCommandCallback };
        REQUIRE(RegisterCommand(&cmd) >= 0);
        REQUIRE(UnregisterCommand(&cmd) == true);
        REQUIRE(RegisterCommand(&cmd) >= 0);
        UnregisterCommand(&cmd);
    }
}

TEST_CASE_METHOD(CommandHelperTestFixture, "SendCommand guard paths", "[command_helper]")
{
    SECTION("SendCommand returns false for empty command")
    {
        CHECK(SendCommand("") == false);
    }

    SECTION("SendCommand returns false for command over max length")
    {
        std::string tooLong(80, 'A');
        CHECK(SendCommand(tooLong.c_str()) == false);
    }

    SECTION("SendCommand with valid short command fails without FIFO pipe")
    {
        // Valid command passes null/length checks, then hits SecurityChecks or open() failure.
        // Either way returns false because there is no real FIFO to write to.
        bool result = SendCommand("reprocess");
        CHECK(result == false);
    }

    SECTION("SendCommand with maximum-length command fails without FIFO pipe")
    {
        // 63 chars is the max (COMMAND_MAX_LEN - 1 = 64 - 1 = 63)
        std::string maxLen(63, 'X');
        bool result = SendCommand(maxLen.c_str());
        CHECK(result == false);
    }
}

TEST_CASE_METHOD(CommandHelperTestFixture, "Command listener thread lifecycle", "[command_helper]")
{
    SECTION("Uninitialize is safe to call repeatedly")
    {
        UninitializeCommandListenerThread();
        UninitializeCommandListenerThread();
        CHECK(true);
    }

    SECTION("InitializeCommandListenerThread and immediate UninitializeCommandListenerThread")
    {
        // Start the listener thread, then immediately cancel it.
        // The thread will attempt TryCreateFIFOPipe + SecurityChecks;
        // if either fails (likely in a test env without 'adu' group) the thread exits on its own.
        bool initResult = InitializeCommandListenerThread();
        CHECK(initResult == true);

        // Small sleep to give the thread a chance to start.
        // Use usleep or nanosleep-style: this is sufficient for thread startup.
        struct timespec ts = { 0, 50000000 }; // 50ms
        nanosleep(&ts, nullptr);

        // Calling Init again while the thread is running should return false.
        bool secondInit = InitializeCommandListenerThread();
        CHECK(secondInit == false);

        // Clean up the thread.
        UninitializeCommandListenerThread();
    }
}
