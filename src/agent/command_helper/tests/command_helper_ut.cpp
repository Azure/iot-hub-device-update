/**
 * @file command_helper_ut.cpp
 * @brief Unit Tests for command_helper module
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>
#include <cstring>
#include <string>

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
        // Clean up any registered commands
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
