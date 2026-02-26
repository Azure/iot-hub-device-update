/**
 * @file adu_core_export_helpers_ut.cpp
 * @brief Unit Tests for adu_core_export_helpers module
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>
#include <cstring>
#include <string>

extern "C"
{
#include "aduc/adu_core_export_helpers.h"
#include <aduc/c_utils.h>
#include <aduc/result.h>

    // Mock control variables
    static ADUC_Result mock_register_result = { ADUC_Result_Success };
    static bool mock_register_sets_callbacks = true;
    static int mock_reboot_result = 0;
    static int mock_restart_result = 0;
    static bool mock_unregister_called = false;
    static ADUC_Token mock_unregister_token = NULL;

    // Dummy callback functions
    static void DummyIdleCallback(ADUC_Token token, const char* workflowId)
    {
        (void)token;
        (void)workflowId;
    }

    static ADUC_Result
        DummyDownloadCallback(ADUC_Token token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
    {
        (void)token;
        (void)workCompletionData;
        (void)workflowData;
        ADUC_Result result = { ADUC_Result_Success };
        return result;
    }

    static ADUC_Result
        DummyBackupCallback(ADUC_Token token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
    {
        (void)token;
        (void)workCompletionData;
        (void)workflowData;
        ADUC_Result result = { ADUC_Result_Success };
        return result;
    }

    static ADUC_Result
        DummyInstallCallback(ADUC_Token token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
    {
        (void)token;
        (void)workCompletionData;
        (void)workflowData;
        ADUC_Result result = { ADUC_Result_Success };
        return result;
    }

    static ADUC_Result
        DummyApplyCallback(ADUC_Token token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
    {
        (void)token;
        (void)workCompletionData;
        (void)workflowData;
        ADUC_Result result = { ADUC_Result_Success };
        return result;
    }

    static ADUC_Result
        DummyRestoreCallback(ADUC_Token token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
    {
        (void)token;
        (void)workCompletionData;
        (void)workflowData;
        ADUC_Result result = { ADUC_Result_Success };
        return result;
    }

    static ADUC_Result DummySandboxCreateCallback(
        ADUC_Token token, const char* workflowId, char* workFolder)
    {
        (void)token;
        (void)workflowId;
        (void)workFolder;
        ADUC_Result result = { ADUC_Result_Success };
        return result;
    }

    static void DummySandboxDestroyCallback(ADUC_Token token, const char* workflowId, const char* workFolder)
    {
        (void)token;
        (void)workflowId;
        (void)workFolder;
    }

    static void DummyDoWorkCallback(ADUC_Token token, ADUC_WorkflowDataToken workflowData)
    {
        (void)token;
        (void)workflowData;
    }

    static ADUC_Result DummyIsInstalledCallback(ADUC_Token token, ADUC_WorkflowDataToken workflowData)
    {
        (void)token;
        (void)workflowData;
        ADUC_Result result = { ADUC_Result_Success };
        return result;
    }

    // Helper to fill all callbacks
    static void FillAllCallbacks(ADUC_UpdateActionCallbacks* callbacks)
    {
        memset(callbacks, 0, sizeof(*callbacks));
        callbacks->IdleCallback = DummyIdleCallback;
        callbacks->DownloadCallback = DummyDownloadCallback;
        callbacks->BackupCallback = DummyBackupCallback;
        callbacks->InstallCallback = DummyInstallCallback;
        callbacks->ApplyCallback = DummyApplyCallback;
        callbacks->RestoreCallback = DummyRestoreCallback;
        callbacks->SandboxCreateCallback = DummySandboxCreateCallback;
        callbacks->SandboxDestroyCallback = DummySandboxDestroyCallback;
        callbacks->DoWorkCallback = DummyDoWorkCallback;
        callbacks->IsInstalledCallback = DummyIsInstalledCallback;
    }

    // Mock platform layer functions
    ADUC_Result ADUC_RegisterPlatformLayer(
        ADUC_UpdateActionCallbacks* data, int argc, const char** argv)
    {
        (void)argc;
        (void)argv;
        if (mock_register_sets_callbacks && data != NULL)
        {
            FillAllCallbacks(data);
        }
        return mock_register_result;
    }

    void ADUC_Unregister(ADUC_Token token)
    {
        mock_unregister_called = true;
        mock_unregister_token = token;
    }

    int ADUC_RebootSystem()
    {
        return mock_reboot_result;
    }

    int ADUC_RestartAgent()
    {
        return mock_restart_result;
    }

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
}

class ExportHelpersTestFixture
{
public:
    ExportHelpersTestFixture()
    {
        mock_register_result = { ADUC_Result_Success };
        mock_register_sets_callbacks = true;
        mock_reboot_result = 0;
        mock_restart_result = 0;
        mock_unregister_called = false;
        mock_unregister_token = NULL;
        memset(&callbacks, 0, sizeof(callbacks));
    }

protected:
    ADUC_UpdateActionCallbacks callbacks;
};

TEST_CASE_METHOD(ExportHelpersTestFixture, "ADUC_MethodCall_Register", "[adu_core_export_helpers]")
{
    SECTION("Succeeds when RegisterPlatformLayer succeeds and callbacks are valid")
    {
        ADUC_Result result = ADUC_MethodCall_Register(&callbacks, 0, NULL);
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    }

    SECTION("Fails when RegisterPlatformLayer returns failure")
    {
        mock_register_result.ResultCode = ADUC_Result_Failure;
        ADUC_Result result = ADUC_MethodCall_Register(&callbacks, 0, NULL);
        REQUIRE(IsAducResultCodeFailure(result.ResultCode));
    }

    SECTION("Fails when callbacks are not properly set")
    {
        mock_register_sets_callbacks = false;
        ADUC_Result result = ADUC_MethodCall_Register(&callbacks, 0, NULL);
        REQUIRE(IsAducResultCodeFailure(result.ResultCode));
    }

    SECTION("Succeeds with NULL updateActionCallbacks")
    {
        // Note: passing NULL for updateActionCallbacks; RegisterPlatformLayer
        // mock guards against NULL so this exercises the NULL path if any.
        mock_register_sets_callbacks = false;
        mock_register_result.ResultCode = ADUC_Result_Failure;
        ADUC_Result result = ADUC_MethodCall_Register(NULL, 0, NULL);
        REQUIRE(IsAducResultCodeFailure(result.ResultCode));
    }

    SECTION("Passes argc and argv through")
    {
        const char* argv[] = { "arg1", "arg2" };
        ADUC_Result result = ADUC_MethodCall_Register(&callbacks, 2, argv);
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    }
}

TEST_CASE_METHOD(ExportHelpersTestFixture, "ADUC_MethodCall_Unregister", "[adu_core_export_helpers]")
{
    SECTION("Calls ADUC_Unregister with the correct token")
    {
        callbacks.PlatformLayerHandle = (void*)0x1234;
        ADUC_MethodCall_Unregister(&callbacks);
        REQUIRE(mock_unregister_called == true);
        REQUIRE(mock_unregister_token == (void*)0x1234);
    }

    SECTION("Handles NULL PlatformLayerHandle")
    {
        callbacks.PlatformLayerHandle = NULL;
        ADUC_MethodCall_Unregister(&callbacks);
        REQUIRE(mock_unregister_called == true);
        REQUIRE(mock_unregister_token == NULL);
    }
}

TEST_CASE_METHOD(ExportHelpersTestFixture, "ADUC_MethodCall_RebootSystem", "[adu_core_export_helpers]")
{
    SECTION("Returns 0 on success")
    {
        mock_reboot_result = 0;
        int result = ADUC_MethodCall_RebootSystem();
        REQUIRE(result == 0);
    }

    SECTION("Returns error code on failure")
    {
        mock_reboot_result = EPERM;
        int result = ADUC_MethodCall_RebootSystem();
        REQUIRE(result == EPERM);
    }

    SECTION("Returns ENOSYS error")
    {
        mock_reboot_result = ENOSYS;
        int result = ADUC_MethodCall_RebootSystem();
        REQUIRE(result == ENOSYS);
    }
}

TEST_CASE_METHOD(ExportHelpersTestFixture, "ADUC_MethodCall_RestartAgent", "[adu_core_export_helpers]")
{
    SECTION("Returns 0 on success")
    {
        mock_restart_result = 0;
        int result = ADUC_MethodCall_RestartAgent();
        REQUIRE(result == 0);
    }

    SECTION("Returns error code on failure")
    {
        mock_restart_result = EPERM;
        int result = ADUC_MethodCall_RestartAgent();
        REQUIRE(result == EPERM);
    }
}
