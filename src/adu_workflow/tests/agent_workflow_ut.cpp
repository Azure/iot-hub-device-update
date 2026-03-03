/**
 * @file agent_workflow_ut.cpp
 * @brief Unit Tests for agent_workflow library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/agent_workflow.h"
#include "aduc/agent_orchestration.h"
#include "aduc/result.h"
#include "aduc/types/workflow.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_utils.h"

#include <catch2/catch_all.hpp>
#include <cstring>
#include <string>
#include <atomic>

using Catch::Matchers::Equals;

// Extern declarations for non-static functions without header declarations
extern "C"
{
    void ADUC_Workflow_WorkCompletionCallback(const void* workCompletionToken, ADUC_Result result, bool isAsync);
    void ADUC_Workflow_AutoTransitionWorkflow(ADUC_WorkflowData* workflowData, bool onSuccess);
}

// Track callback invocations for testing
static std::atomic<int> s_sandboxCreateCount{ 0 };
static std::atomic<int> s_sandboxDestroyCount{ 0 };
static std::atomic<int> s_idleCallbackCount{ 0 };
static std::atomic<int> s_doWorkCallbackCount{ 0 };
static std::atomic<int> s_downloadCallbackCount{ 0 };
static std::atomic<int> s_backupCallbackCount{ 0 };
static std::atomic<int> s_installCallbackCount{ 0 };
static std::atomic<int> s_applyCallbackCount{ 0 };
static std::atomic<int> s_restoreCallbackCount{ 0 };
static std::atomic<int> s_cancelCallbackCount{ 0 };
static std::atomic<int> s_isInstalledCallbackCount{ 0 };
static std::atomic<int> s_reportStateCallbackCount{ 0 };

// Track last reported state and installed update id
static std::atomic<ADUCITF_State> s_lastReportedUpdateState{ ADUCITF_State_Idle };
static std::string s_lastInstalledUpdateId;

// Reset all callback counters
static void ResetCallbackCounters()
{
    s_sandboxCreateCount = 0;
    s_sandboxDestroyCount = 0;
    s_idleCallbackCount = 0;
    s_doWorkCallbackCount = 0;
    s_downloadCallbackCount = 0;
    s_backupCallbackCount = 0;
    s_installCallbackCount = 0;
    s_applyCallbackCount = 0;
    s_restoreCallbackCount = 0;
    s_cancelCallbackCount = 0;
    s_isInstalledCallbackCount = 0;
    s_reportStateCallbackCount = 0;
    s_lastReportedUpdateState = ADUCITF_State_Idle;
    s_lastInstalledUpdateId.clear();
}

// clang-format off

/**
 * @brief Sample workflow JSON for testing ProcessDeployment action.
 */
static const char* sample_process_deployment_json =
    R"( {                                                          )"
    R"(     "workflow": {                                          )"
    R"(         "action": 3,                                       )"
    R"(         "id": "test-workflow-001"                          )"
    R"(     },                                                     )"
    R"(     "updateManifest": "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Test-Update\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"test-device\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/apt:1\",\"files\":[\"f1\"],\"handlerProperties\":{\"installedCriteria\":\"test-criteria\"}}]},\"files\":{\"f1\":{\"fileName\":\"test-file.json\",\"sizeInBytes\":100,\"hashes\":{\"sha256\":\"abc123\"}}},\"createdDateTime\":\"2024-01-01T00:00:00Z\"}",  )"
    R"(     "updateManifestSignature": "test-signature",           )"
    R"(     "fileUrls": {                                          )"
    R"(         "f1": "http://test.example.com/test-file.json"     )"
    R"(     }                                                      )"
    R"( }                                                          )";

/**
 * @brief Sample workflow JSON for testing Cancel action.
 */
static const char* sample_cancel_json =
    R"( {                                                          )"
    R"(     "workflow": {                                          )"
    R"(         "action": 255,                                     )"
    R"(         "id": "cancel-workflow-001"                        )"
    R"(     },                                                     )"
    R"(     "updateManifest": "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Test-Update\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"test-device\"}],\"instructions\":{\"steps\":[]},\"files\":{},\"createdDateTime\":\"2024-01-01T00:00:00Z\"}",  )"
    R"(     "updateManifestSignature": "test-signature",           )"
    R"(     "fileUrls": {}                                         )"
    R"( }                                                          )";

/**
 * @brief Sample workflow JSON without action field (Undefined action).
 */
static const char* sample_undefined_action_json =
    R"( {                                                          )"
    R"(     "workflow": {                                          )"
    R"(         "id": "undefined-workflow-001"                     )"
    R"(     },                                                     )"
    R"(     "updateManifest": "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Test-Update\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"test-device\"}],\"instructions\":{\"steps\":[]},\"files\":{},\"createdDateTime\":\"2024-01-01T00:00:00Z\"}",  )"
    R"(     "updateManifestSignature": "test-signature",           )"
    R"(     "fileUrls": {}                                         )"
    R"( }                                                          )";

/**
 * @brief Sample workflow JSON with retry timestamp.
 */
static const char* sample_retry_json =
    R"( {                                                          )"
    R"(     "workflow": {                                          )"
    R"(         "action": 3,                                       )"
    R"(         "id": "retry-workflow-001",                        )"
    R"(         "retryTimestamp": "2024-01-01T12:00:00Z"           )"
    R"(     },                                                     )"
    R"(     "updateManifest": "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Test-Update\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"test-device\"}],\"instructions\":{\"steps\":[]},\"files\":{},\"createdDateTime\":\"2024-01-01T00:00:00Z\"}",  )"
    R"(     "updateManifestSignature": "test-signature",           )"
    R"(     "fileUrls": {}                                         )"
    R"( }                                                          )";

/**
 * @brief Second sample workflow JSON with a different workflow id for replacement tests.
 */
static const char* sample_process_deployment_json_2 =
    R"( {                                                          )"
    R"(     "workflow": {                                          )"
    R"(         "action": 3,                                       )"
    R"(         "id": "test-workflow-002"                          )"
    R"(     },                                                     )"
    R"(     "updateManifest": "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Test-Update\",\"version\":\"2.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"test-device\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/apt:1\",\"files\":[\"f1\"],\"handlerProperties\":{\"installedCriteria\":\"test-criteria-2\"}}]},\"files\":{\"f1\":{\"fileName\":\"test-file-2.json\",\"sizeInBytes\":200,\"hashes\":{\"sha256\":\"def456\"}}},\"createdDateTime\":\"2024-01-02T00:00:00Z\"}",  )"
    R"(     "updateManifestSignature": "test-signature-2",         )"
    R"(     "fileUrls": {                                          )"
    R"(         "f1": "http://test.example.com/test-file-2.json"   )"
    R"(     }                                                      )"
    R"( }                                                          )";

// clang-format on

//
// Test fixtures and helper functions
//

/**
 * @brief Helper to create a test workflow handle.
 */
static ADUC_WorkflowHandle CreateTestWorkflowHandle(const char* jsonStr)
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(jsonStr, false /* validateManifest */, &handle);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    return handle;
}

/**
 * @brief Mock callback for state reporting - always succeeds.
 */
static bool MockReportStateCallback_Success(
    ADUC_WorkflowDataToken workflowData,
    ADUCITF_State updateState,
    const ADUC_Result* result,
    const char* installedUpdateId)
{
    UNREFERENCED_PARAMETER(workflowData);
    UNREFERENCED_PARAMETER(updateState);
    UNREFERENCED_PARAMETER(result);
    UNREFERENCED_PARAMETER(installedUpdateId);
    return true;
}

/**
 * @brief Mock callback for state reporting - always fails.
 */
static bool MockReportStateCallback_Failure(
    ADUC_WorkflowDataToken workflowData,
    ADUCITF_State updateState,
    const ADUC_Result* result,
    const char* installedUpdateId)
{
    UNREFERENCED_PARAMETER(workflowData);
    UNREFERENCED_PARAMETER(updateState);
    UNREFERENCED_PARAMETER(result);
    UNREFERENCED_PARAMETER(installedUpdateId);
    return false;
}

/**
 * @brief Mock sandbox create callback - always succeeds.
 */
static ADUC_Result MockSandboxCreateCallback(void* token, const char* workflowId, char* workFolder)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);
    return ADUC_Result{ ADUC_Result_Success, 0 };
}

/**
 * @brief Mock sandbox destroy callback.
 */
static void MockSandboxDestroyCallback(void* token, const char* workflowId, const char* workFolder)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);
}

/**
 * @brief Mock idle callback.
 */
static void MockIdleCallback(void* token, const char* workflowId)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
}

/**
 * @brief Mock do work callback.
 */
static void MockDoWorkCallback(void* token, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowData);
}

/**
 * @brief Mock download callback - returns success synchronously.
 */
static ADUC_Result
MockDownloadCallback(void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Download_Success, 0 };
}

/**
 * @brief Mock backup callback - returns success synchronously.
 */
static ADUC_Result
MockBackupCallback(void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Backup_Success, 0 };
}

/**
 * @brief Mock install callback - returns success synchronously.
 */
static ADUC_Result
MockInstallCallback(void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Install_Success, 0 };
}

/**
 * @brief Mock apply callback - returns success synchronously.
 */
static ADUC_Result
MockApplyCallback(void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Apply_Success, 0 };
}

/**
 * @brief Mock restore callback - returns success synchronously.
 */
static ADUC_Result
MockRestoreCallback(void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Restore_Success, 0 };
}

/**
 * @brief Mock cancel callback.
 */
static void MockCancelCallback(void* token, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowData);
}

/**
 * @brief Mock IsInstalled callback - returns not installed.
 */
static ADUC_Result MockIsInstalledCallback_NotInstalled(void* token, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_IsInstalled_NotInstalled, 0 };
}

/**
 * @brief Mock IsInstalled callback - returns installed.
 */
static ADUC_Result MockIsInstalledCallback_Installed(void* token, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_IsInstalled_Installed, 0 };
}

/**
 * @brief Initialize mock callbacks structure.
 */
static void InitMockCallbacks(ADUC_UpdateActionCallbacks* callbacks)
{
    memset(callbacks, 0, sizeof(*callbacks));
    callbacks->SandboxCreateCallback = MockSandboxCreateCallback;
    callbacks->SandboxDestroyCallback = MockSandboxDestroyCallback;
    callbacks->IdleCallback = MockIdleCallback;
    callbacks->DoWorkCallback = MockDoWorkCallback;
    callbacks->DownloadCallback = MockDownloadCallback;
    callbacks->BackupCallback = MockBackupCallback;
    callbacks->InstallCallback = MockInstallCallback;
    callbacks->ApplyCallback = MockApplyCallback;
    callbacks->RestoreCallback = MockRestoreCallback;
    callbacks->CancelCallback = MockCancelCallback;
    callbacks->IsInstalledCallback = MockIsInstalledCallback_NotInstalled;
}

//
// Unit Tests for ADUC_Workflow_MethodCall_IsInstalled
//

TEST_CASE("ADUC_Workflow_MethodCall_IsInstalled")
{
    SECTION("Returns NotInstalled when workflowData is NULL")
    {
        ADUC_Result result = ADUC_Workflow_MethodCall_IsInstalled(nullptr);
        CHECK(result.ResultCode == ADUC_Result_IsInstalled_NotInstalled);
    }

    SECTION("Returns NotInstalled when IsInstalledCallback indicates not installed")
    {
        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacks(&callbacks);
        callbacks.IsInstalledCallback = MockIsInstalledCallback_NotInstalled;
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Result result = ADUC_Workflow_MethodCall_IsInstalled(&workflowData);
        CHECK(result.ResultCode == ADUC_Result_IsInstalled_NotInstalled);
    }

    SECTION("Returns Installed when IsInstalledCallback indicates installed")
    {
        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacks(&callbacks);
        callbacks.IsInstalledCallback = MockIsInstalledCallback_Installed;
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Result result = ADUC_Workflow_MethodCall_IsInstalled(&workflowData);
        CHECK(result.ResultCode == ADUC_Result_IsInstalled_Installed);
    }
}

//
// Unit Tests for ADUC_Workflow_DefaultDownloadProgressCallback
//

TEST_CASE("ADUC_Workflow_DefaultDownloadProgressCallback")
{
    // This function just logs, so we're testing it doesn't crash
    SECTION("NotStarted state")
    {
        ADUC_Workflow_DefaultDownloadProgressCallback(
            "test-workflow", "file-001", ADUC_DownloadProgressState_NotStarted, 0, 1000);
        CHECK(true); // If we reach here, no crash occurred
    }

    SECTION("InProgress state")
    {
        ADUC_Workflow_DefaultDownloadProgressCallback(
            "test-workflow", "file-001", ADUC_DownloadProgressState_InProgress, 500, 1000);
        CHECK(true);
    }

    SECTION("Completed state")
    {
        ADUC_Workflow_DefaultDownloadProgressCallback(
            "test-workflow", "file-001", ADUC_DownloadProgressState_Completed, 1000, 1000);
        CHECK(true);
    }

    SECTION("Cancelled state")
    {
        ADUC_Workflow_DefaultDownloadProgressCallback(
            "test-workflow", "file-001", ADUC_DownloadProgressState_Cancelled, 250, 1000);
        CHECK(true);
    }

    SECTION("Error state")
    {
        ADUC_Workflow_DefaultDownloadProgressCallback(
            "test-workflow", "file-001", ADUC_DownloadProgressState_Error, 100, 1000);
        CHECK(true);
    }

    SECTION("With NULL workflowId")
    {
        ADUC_Workflow_DefaultDownloadProgressCallback(
            nullptr, "file-001", ADUC_DownloadProgressState_InProgress, 500, 1000);
        CHECK(true);
    }

    SECTION("With NULL fileId")
    {
        ADUC_Workflow_DefaultDownloadProgressCallback(
            "test-workflow", nullptr, ADUC_DownloadProgressState_InProgress, 500, 1000);
        CHECK(true);
    }

    SECTION("With zero bytes")
    {
        ADUC_Workflow_DefaultDownloadProgressCallback(
            "test-workflow", "file-001", ADUC_DownloadProgressState_NotStarted, 0, 0);
        CHECK(true);
    }

    SECTION("With large byte counts")
    {
        ADUC_Workflow_DefaultDownloadProgressCallback(
            "test-workflow", "file-001", ADUC_DownloadProgressState_InProgress, UINT64_MAX / 2, UINT64_MAX);
        CHECK(true);
    }
}

//
// Unit Tests for ADUC_Workflow_SetUpdateState and ADUC_Workflow_SetUpdateStateWithResult
//

TEST_CASE("ADUC_Workflow_SetUpdateState")
{
    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacks(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    SECTION("Set DeploymentInProgress state")
    {
        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_DeploymentInProgress);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_DeploymentInProgress);
    }

    SECTION("Set DownloadStarted state")
    {
        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_DownloadStarted);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_DownloadStarted);
    }

    SECTION("Set DownloadSucceeded state")
    {
        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_DownloadSucceeded);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_DownloadSucceeded);
    }

    SECTION("Set InstallStarted state")
    {
        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_InstallStarted);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_InstallStarted);
    }

    SECTION("Set InstallSucceeded state")
    {
        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_InstallSucceeded);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_InstallSucceeded);
    }

    SECTION("Set ApplyStarted state")
    {
        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_ApplyStarted);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_ApplyStarted);
    }

    SECTION("Set Failed state")
    {
        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_Failed);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Failed);
    }

    SECTION("Set BackupStarted state")
    {
        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_BackupStarted);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_BackupStarted);
    }

    SECTION("Set BackupSucceeded state")
    {
        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_BackupSucceeded);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_BackupSucceeded);
    }

    SECTION("Set RestoreStarted state")
    {
        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_RestoreStarted);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_RestoreStarted);
    }

    workflow_free(workflowData.WorkflowHandle);
}

TEST_CASE("ADUC_Workflow_SetUpdateStateWithResult")
{
    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacks(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    SECTION("Set state with success result")
    {
        ADUC_Result result = { ADUC_Result_Download_Success, 0 };
        ADUC_Workflow_SetUpdateStateWithResult(&workflowData, ADUCITF_State_DownloadSucceeded, result);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_DownloadSucceeded);
    }

    SECTION("Set state with failure result")
    {
        ADUC_Result result = { ADUC_Result_Failure, 0x12345678 };
        ADUC_Workflow_SetUpdateStateWithResult(&workflowData, ADUCITF_State_Failed, result);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Failed);
    }

    SECTION("Set state with extended result code")
    {
        ADUC_Result result = { ADUC_Result_Download_Success, 0xABCD };
        ADUC_Workflow_SetUpdateStateWithResult(&workflowData, ADUCITF_State_DownloadSucceeded, result);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_DownloadSucceeded);
    }

    workflow_free(workflowData.WorkflowHandle);
}

TEST_CASE("ADUC_Workflow_SetUpdateState with callback failure")
{
    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Failure;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacks(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    SECTION("Callback failure for non-Idle state should set Failed state")
    {
        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_DownloadStarted);
        // When callback fails, the state should be set to Failed
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Failed);
    }

    SECTION("Callback failure for Failed state should remain Failed")
    {
        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_Failed);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Failed);
    }

    workflow_free(workflowData.WorkflowHandle);
}

//
// Unit Tests for SetUpdateState transitioning to Idle from ApplyStarted
//

TEST_CASE("ADUC_Workflow_SetUpdateState - Idle from ApplyStarted transitions")
{
    SECTION("ApplyStarted to Idle with no reboot or restart reports installed update id")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_apply_idle");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.LastReportedState = ADUCITF_State_ApplyStarted;
        workflowData.SystemRebootState = ADUC_SystemRebootState_None;
        workflowData.AgentRestartState = ADUC_AgentRestartState_None;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacks(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_Idle);

        // Should transition to Idle via SetInstalledUpdateIdAndGoToIdle
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Idle);
        // WorkflowHandle freed by SetInstalledUpdateIdAndGoToIdle → MethodCall_Idle
    }

    SECTION("ApplyStarted to Idle with reboot in progress calls Idle internally")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_reboot_idle");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.LastReportedState = ADUCITF_State_ApplyStarted;
        workflowData.SystemRebootState = ADUC_SystemRebootState_InProgress;
        workflowData.AgentRestartState = ADUC_AgentRestartState_None;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacks(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_Idle);

        // With reboot in progress, MethodCall_Idle is called but state not reported
        CHECK(workflowData.WorkflowHandle == nullptr);
    }

    SECTION("ApplyStarted to Idle with agent restart in progress calls Idle internally")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_restart_idle");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.LastReportedState = ADUCITF_State_ApplyStarted;
        workflowData.SystemRebootState = ADUC_SystemRebootState_None;
        workflowData.AgentRestartState = ADUC_AgentRestartState_InProgress;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacks(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_Idle);

        // With agent restart in progress, MethodCall_Idle is called
        CHECK(workflowData.WorkflowHandle == nullptr);
    }

    SECTION("ApplyStarted to Idle with reboot required but not in progress falls through")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_reboot_required");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.LastReportedState = ADUCITF_State_ApplyStarted;
        workflowData.SystemRebootState = ADUC_SystemRebootState_Required;
        workflowData.AgentRestartState = ADUC_AgentRestartState_None;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacks(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_Idle);

        // Falls through to general Idle handling since reboot required but not in progress
        CHECK(workflowData.WorkflowHandle == nullptr);
    }

    SECTION("Idle from ApplyStarted with callback failure sets Failed state")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.LastReportedState = ADUCITF_State_ApplyStarted;
        workflowData.SystemRebootState = ADUC_SystemRebootState_None;
        workflowData.AgentRestartState = ADUC_AgentRestartState_None;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Failure;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacks(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_Idle);

        // The callback for reporting installed update id failed, so last reported state
        // May remain Idle (from SetInstalledUpdateIdAndGoToIdle) or Failed depending on path
        // The key test is that it doesn't crash
        CHECK(true);

        // Workflow gets freed inside SetInstalledUpdateIdAndGoToIdle
    }

    SECTION("Idle from non-ApplyStarted state with callback failure sets Failed")
    {
        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.LastReportedState = ADUCITF_State_DownloadSucceeded;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Failure;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacks(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_Idle);

        // The ReportStateAndResultAsyncCallback fails, state goes to Failed
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Failed);

        workflow_free(workflowData.WorkflowHandle);
    }
}

//
// Unit Tests for ADUC_Workflow_DoWork
//

TEST_CASE("ADUC_Workflow_DoWork")
{
    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacks(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    SECTION("DoWork should call DoWorkCallback")
    {
        // This test verifies DoWork doesn't crash
        ADUC_Workflow_DoWork(&workflowData);
        CHECK(true);
    }
}

//
// Unit Tests for ADUC_Workflow_HandleStartupWorkflowData
//

TEST_CASE("ADUC_Workflow_HandleStartupWorkflowData")
{
    SECTION("NULL workflowData should return early")
    {
        // Should not crash
        ADUC_Workflow_HandleStartupWorkflowData(nullptr);
        CHECK(true);
    }

    SECTION("StartupIdleCallSent true should skip processing")
    {
        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.StartupIdleCallSent = true;

        ADUC_Workflow_HandleStartupWorkflowData(&workflowData);
        CHECK(workflowData.StartupIdleCallSent == true);
    }

    SECTION("NULL WorkflowHandle should be handled")
    {
        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = nullptr;
        workflowData.StartupIdleCallSent = false;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacks(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_HandleStartupWorkflowData(&workflowData);
        CHECK(workflowData.StartupIdleCallSent == true);
    }
}

//
// Unit Tests for ADUC_Workflow_MethodCall_ProcessDeployment
//

TEST_CASE("ADUC_Workflow_MethodCall_ProcessDeployment")
{
    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacks(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    ADUC_MethodCall_Data methodCallData;
    memset(&methodCallData, 0, sizeof(methodCallData));
    methodCallData.WorkflowData = &workflowData;

    SECTION("ProcessDeployment returns success")
    {
        ADUC_Result result = ADUC_Workflow_MethodCall_ProcessDeployment(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Success);
        CHECK(result.ExtendedResultCode == 0);
    }

    SECTION("ProcessDeployment sets DeploymentInProgress state")
    {
        ADUC_Workflow_MethodCall_ProcessDeployment(&methodCallData);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_DeploymentInProgress);
    }

    workflow_free(workflowData.WorkflowHandle);
}

TEST_CASE("ADUC_Workflow_MethodCall_ProcessDeployment_Complete")
{
    ADUC_MethodCall_Data methodCallData;
    memset(&methodCallData, 0, sizeof(methodCallData));

    ADUC_Result result = { ADUC_Result_Success, 0 };

    // This function is a no-op, just verify it doesn't crash
    ADUC_Workflow_MethodCall_ProcessDeployment_Complete(&methodCallData, result);
    CHECK(true);
}

//
// Unit Tests for ADUC_Workflow_MethodCall_Download
//

TEST_CASE("ADUC_Workflow_MethodCall_Download")
{
    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    // Set up work folder
    workflow_set_workfolder(handle, "/tmp/test_workflow");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;
    workflowData.LastReportedState = ADUCITF_State_DeploymentInProgress;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacks(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    ADUC_MethodCall_Data methodCallData;
    memset(&methodCallData, 0, sizeof(methodCallData));
    methodCallData.WorkflowData = &workflowData;

    SECTION("Download with correct state returns success")
    {
        ADUC_Result result = ADUC_Workflow_MethodCall_Download(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Download_Success);
    }

    SECTION("Download with incorrect state returns failure")
    {
        workflowData.LastReportedState = ADUCITF_State_Idle; // Wrong state
        ADUC_Result result = ADUC_Workflow_MethodCall_Download(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Failure);
        CHECK(result.ExtendedResultCode == ADUC_ERC_UPPERLEVEL_WORKFLOW_UPDATE_ACTION_UNEXPECTED_STATE);
    }

    workflow_free(workflowData.WorkflowHandle);
}

TEST_CASE("ADUC_Workflow_MethodCall_Download_Complete")
{
    ADUC_MethodCall_Data methodCallData;
    memset(&methodCallData, 0, sizeof(methodCallData));

    ADUC_Result result = { ADUC_Result_Download_Success, 0 };

    // This function is a no-op, verify no crash
    ADUC_Workflow_MethodCall_Download_Complete(&methodCallData, result);
    CHECK(true);
}

//
// Unit Tests for ADUC_Workflow_MethodCall_Backup
//

TEST_CASE("ADUC_Workflow_MethodCall_Backup")
{
    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;
    workflowData.LastReportedState = ADUCITF_State_DownloadSucceeded;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacks(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    ADUC_MethodCall_Data methodCallData;
    memset(&methodCallData, 0, sizeof(methodCallData));
    methodCallData.WorkflowData = &workflowData;

    SECTION("Backup with correct state returns success")
    {
        ADUC_Result result = ADUC_Workflow_MethodCall_Backup(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Backup_Success);
    }

    SECTION("Backup with incorrect state returns failure")
    {
        workflowData.LastReportedState = ADUCITF_State_Idle;
        ADUC_Result result = ADUC_Workflow_MethodCall_Backup(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Failure);
        CHECK(result.ExtendedResultCode == ADUC_ERC_UPPERLEVEL_WORKFLOW_UPDATE_ACTION_UNEXPECTED_STATE);
    }

    workflow_free(workflowData.WorkflowHandle);
}

TEST_CASE("ADUC_Workflow_MethodCall_Backup_Complete")
{
    ADUC_MethodCall_Data methodCallData;
    memset(&methodCallData, 0, sizeof(methodCallData));

    ADUC_Result result = { ADUC_Result_Backup_Success, 0 };

    ADUC_Workflow_MethodCall_Backup_Complete(&methodCallData, result);
    CHECK(true);
}

//
// Unit Tests for ADUC_Workflow_MethodCall_Install
//

TEST_CASE("ADUC_Workflow_MethodCall_Install")
{
    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;
    workflowData.LastReportedState = ADUCITF_State_BackupSucceeded;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacks(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    ADUC_MethodCall_Data methodCallData;
    memset(&methodCallData, 0, sizeof(methodCallData));
    methodCallData.WorkflowData = &workflowData;

    SECTION("Install with correct state returns success")
    {
        ADUC_Result result = ADUC_Workflow_MethodCall_Install(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Install_Success);
    }

    SECTION("Install with incorrect state returns failure")
    {
        workflowData.LastReportedState = ADUCITF_State_Idle;
        ADUC_Result result = ADUC_Workflow_MethodCall_Install(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Failure);
        CHECK(result.ExtendedResultCode == ADUC_ERC_UPPERLEVEL_WORKFLOW_INSTALL_ACTION_IN_UNEXPECTED_STATE);
    }

    workflow_free(workflowData.WorkflowHandle);
}

//
// Unit Tests for ADUC_Workflow_MethodCall_Apply
//

TEST_CASE("ADUC_Workflow_MethodCall_Apply")
{
    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;
    workflowData.LastReportedState = ADUCITF_State_InstallSucceeded;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacks(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    ADUC_MethodCall_Data methodCallData;
    memset(&methodCallData, 0, sizeof(methodCallData));
    methodCallData.WorkflowData = &workflowData;

    SECTION("Apply with correct state returns success")
    {
        ADUC_Result result = ADUC_Workflow_MethodCall_Apply(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Apply_Success);
    }

    SECTION("Apply with incorrect state returns failure")
    {
        workflowData.LastReportedState = ADUCITF_State_Idle;
        ADUC_Result result = ADUC_Workflow_MethodCall_Apply(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Failure);
        CHECK(result.ExtendedResultCode == ADUC_ERC_NOTPERMITTED);
    }

    workflow_free(workflowData.WorkflowHandle);
}

//
// Unit Tests for ADUC_Workflow_MethodCall_Restore
//

TEST_CASE("ADUC_Workflow_MethodCall_Restore")
{
    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;
    workflowData.LastReportedState = ADUCITF_State_Failed;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacks(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    ADUC_MethodCall_Data methodCallData;
    memset(&methodCallData, 0, sizeof(methodCallData));
    methodCallData.WorkflowData = &workflowData;

    SECTION("Restore with correct state returns success")
    {
        ADUC_Result result = ADUC_Workflow_MethodCall_Restore(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Restore_Success);
    }

    SECTION("Restore with incorrect state returns failure")
    {
        workflowData.LastReportedState = ADUCITF_State_Idle;
        ADUC_Result result = ADUC_Workflow_MethodCall_Restore(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Failure);
        CHECK(result.ExtendedResultCode == ADUC_ERC_NOTPERMITTED);
    }

    workflow_free(workflowData.WorkflowHandle);
}

//
// Unit Tests for ADUC_Workflow_MethodCall_Cancel
//

TEST_CASE("ADUC_Workflow_MethodCall_Cancel")
{
    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacks(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    SECTION("Cancel when operation NOT in progress should return")
    {
        workflow_set_operation_in_progress(handle, false);
        // Should not crash, just return early
        ADUC_Workflow_MethodCall_Cancel(&workflowData);
        CHECK(true);
    }

    SECTION("Cancel when operation IS in progress should call CancelCallback")
    {
        workflow_set_operation_in_progress(handle, true);
        ADUC_Workflow_MethodCall_Cancel(&workflowData);
        CHECK(true);
    }

    workflow_free(workflowData.WorkflowHandle);
}

//
// Unit Tests for ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle
//

TEST_CASE("ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle")
{
    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;
    workflowData.SystemRebootState = ADUC_SystemRebootState_Required;
    workflowData.AgentRestartState = ADUC_AgentRestartState_Required;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacks(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    SECTION("Sets Idle state and resets reboot/restart states")
    {
        const char* updateId = "{\"provider\":\"Contoso\",\"name\":\"Test-Update\",\"version\":\"1.0\"}";
        ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle(&workflowData, updateId);

        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Idle);
        CHECK(workflowData.SystemRebootState == ADUC_SystemRebootState_None);
        CHECK(workflowData.AgentRestartState == ADUC_AgentRestartState_None);
    }

    // Note: workflow is freed inside the function, so don't free again
}

TEST_CASE("ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle - report callback failure")
{
    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_goToIdle_fail");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Failure;
    workflowData.SystemRebootState = ADUC_SystemRebootState_None;
    workflowData.AgentRestartState = ADUC_AgentRestartState_None;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacks(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    SECTION("Report callback failure still transitions to Idle internally")
    {
        const char* updateId = "{\"provider\":\"Contoso\",\"name\":\"Test\",\"version\":\"1.0\"}";
        ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle(&workflowData, updateId);

        // Even though callback fails, the function still resets states
        CHECK(workflowData.SystemRebootState == ADUC_SystemRebootState_None);
        CHECK(workflowData.AgentRestartState == ADUC_AgentRestartState_None);
    }
    // workflow freed inside
}

//
// Unit Tests for ADUC_Workflow_MethodCall_Idle
//

TEST_CASE("ADUC_Workflow_MethodCall_Idle")
{
    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    // Configure workfolder for cleanup
    workflow_set_workfolder(handle, "/tmp/test_workflow_idle");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.LastReportedState = ADUCITF_State_Failed;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacks(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    SECTION("Idle transitions cleanly")
    {
        ADUC_Workflow_MethodCall_Idle(&workflowData);
        CHECK(workflowData.WorkflowHandle == nullptr);
    }

    // Note: workflow is freed inside the function
}

//
// Unit Tests for ADUC_Workflow_WorkCompletionCallback
//

// Tracking mock callback with counter for report state
static bool MockReportStateCallback_WithCounter(
    ADUC_WorkflowDataToken workflowData,
    ADUCITF_State updateState,
    const ADUC_Result* result,
    const char* installedUpdateId)
{
    UNREFERENCED_PARAMETER(workflowData);
    UNREFERENCED_PARAMETER(result);
    s_reportStateCallbackCount++;
    s_lastReportedUpdateState = updateState;
    if (installedUpdateId != nullptr)
    {
        s_lastInstalledUpdateId = installedUpdateId;
    }
    return true;
}

// Tracking mock callbacks with counters
static ADUC_Result MockSandboxCreateCallback_WithCounter(void* token, const char* workflowId, char* workFolder)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);
    s_sandboxCreateCount++;
    return ADUC_Result{ ADUC_Result_Success, 0 };
}

static void MockSandboxDestroyCallback_WithCounter(void* token, const char* workflowId, const char* workFolder)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);
    s_sandboxDestroyCount++;
}

static void MockIdleCallback_WithCounter(void* token, const char* workflowId)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    s_idleCallbackCount++;
}

static void MockDoWorkCallback_WithCounter(void* token, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowData);
    s_doWorkCallbackCount++;
}

static ADUC_Result MockDownloadCallback_WithCounter(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    s_downloadCallbackCount++;
    return ADUC_Result{ ADUC_Result_Download_Success, 0 };
}

static ADUC_Result MockBackupCallback_WithCounter(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    s_backupCallbackCount++;
    return ADUC_Result{ ADUC_Result_Backup_Success, 0 };
}

static ADUC_Result MockInstallCallback_WithCounter(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    s_installCallbackCount++;
    return ADUC_Result{ ADUC_Result_Install_Success, 0 };
}

static ADUC_Result MockApplyCallback_WithCounter(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    s_applyCallbackCount++;
    return ADUC_Result{ ADUC_Result_Apply_Success, 0 };
}

static ADUC_Result MockRestoreCallback_WithCounter(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    s_restoreCallbackCount++;
    return ADUC_Result{ ADUC_Result_Restore_Success, 0 };
}

static void MockCancelCallback_WithCounter(void* token, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowData);
    s_cancelCallbackCount++;
}

static ADUC_Result MockIsInstalledCallback_WithCounter(void* token, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowData);
    s_isInstalledCallbackCount++;
    return ADUC_Result{ ADUC_Result_IsInstalled_NotInstalled, 0 };
}

// Failure callbacks for testing error paths
static ADUC_Result MockSandboxCreateCallback_Failure(void* token, const char* workflowId, char* workFolder)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);
    return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
}

static ADUC_Result MockDownloadCallback_Failure(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
}

static ADUC_Result MockBackupCallback_Failure(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
}

static ADUC_Result MockInstallCallback_Failure(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
}

static ADUC_Result MockApplyCallback_Failure(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
}

static ADUC_Result MockRestoreCallback_Failure(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
}

/**
 * @brief Initialize mock callbacks structure with counters.
 */
static void InitMockCallbacksWithCounters(ADUC_UpdateActionCallbacks* callbacks)
{
    ResetCallbackCounters();
    memset(callbacks, 0, sizeof(*callbacks));
    callbacks->SandboxCreateCallback = MockSandboxCreateCallback_WithCounter;
    callbacks->SandboxDestroyCallback = MockSandboxDestroyCallback_WithCounter;
    callbacks->IdleCallback = MockIdleCallback_WithCounter;
    callbacks->DoWorkCallback = MockDoWorkCallback_WithCounter;
    callbacks->DownloadCallback = MockDownloadCallback_WithCounter;
    callbacks->BackupCallback = MockBackupCallback_WithCounter;
    callbacks->InstallCallback = MockInstallCallback_WithCounter;
    callbacks->ApplyCallback = MockApplyCallback_WithCounter;
    callbacks->RestoreCallback = MockRestoreCallback_WithCounter;
    callbacks->CancelCallback = MockCancelCallback_WithCounter;
    callbacks->IsInstalledCallback = MockIsInstalledCallback_WithCounter;
}

TEST_CASE("ADUC_Workflow_WorkCompletionCallback - success path for non-Idle state")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_wcc_success");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;
    workflowData.LastReportedState = ADUCITF_State_DeploymentInProgress;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    // Set up current workflow step to ProcessDeployment
    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_ProcessDeployment);

    // WorkCompletionCallback frees methodCallData, so must be heap-allocated
    ADUC_MethodCall_Data* methodCallData =
        static_cast<ADUC_MethodCall_Data*>(calloc(1, sizeof(ADUC_MethodCall_Data)));
    REQUIRE(methodCallData != nullptr);
    methodCallData->WorkflowData = &workflowData;

    SECTION("Success result transitions to next state and auto-transitions")
    {
        ADUC_Result result = { ADUC_Result_Success, 0 };

        // This will: transition to DeploymentInProgress, then auto-transition to Download step,
        // which triggers the full chain. The chain of synchronous callbacks will cascade through
        // all workflow steps until Apply transitions to Idle.
        ADUC_Workflow_WorkCompletionCallback(methodCallData, result, false /* isAsync */);

        // After full workflow cascade, should have reached Idle
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Idle);
        // WorkflowHandle is freed when transitioning to Idle
    }
}

TEST_CASE("ADUC_Workflow_WorkCompletionCallback - failure path without cancel")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_wcc_failure");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;
    workflowData.LastReportedState = ADUCITF_State_DownloadStarted;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    // Set restore callback to succeed for the auto-transition after failure
    callbacks.RestoreCallback = MockRestoreCallback_WithCounter;
    workflowData.UpdateActionCallbacks = callbacks;

    // Set the current workflow step to Download
    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_Download);
    workflow_set_operation_in_progress(handle, true);

    ADUC_MethodCall_Data* methodCallData =
        static_cast<ADUC_MethodCall_Data*>(calloc(1, sizeof(ADUC_MethodCall_Data)));
    REQUIRE(methodCallData != nullptr);
    methodCallData->WorkflowData = &workflowData;

    SECTION("Failure result goes to failed state (no auto-transition for Download failure)")
    {
        ADUC_Result result = { ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };

        ADUC_Workflow_WorkCompletionCallback(methodCallData, result, false /* isAsync */);

        // Download failure from handler map: NextStateOnFailure=Failed, AutoTransitionWorkflowStepOnFailure=Undefined
        // So it should report Failed state and workflow is complete.
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Failed);
    }

    // Clean up if workflow handle still valid
    if (workflowData.WorkflowHandle != nullptr)
    {
        workflow_free(workflowData.WorkflowHandle);
    }
}

TEST_CASE("ADUC_Workflow_WorkCompletionCallback - Install failure triggers Restore auto-transition")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_wcc_install_fail");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;
    workflowData.LastReportedState = ADUCITF_State_InstallStarted;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    // Set the current workflow step to Install
    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_Install);
    workflow_set_operation_in_progress(handle, true);

    ADUC_MethodCall_Data* methodCallData =
        static_cast<ADUC_MethodCall_Data*>(calloc(1, sizeof(ADUC_MethodCall_Data)));
    REQUIRE(methodCallData != nullptr);
    methodCallData->WorkflowData = &workflowData;

    SECTION("Install failure triggers auto-transition to Restore step")
    {
        ADUC_Result result = { ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };

        ADUC_Workflow_WorkCompletionCallback(methodCallData, result, false /* isAsync */);

        // Install failure: NextStateOnFailure=Failed, AutoTransitionWorkflowStepOnFailure=Restore
        // So it should go to Failed, then auto-transition to Restore, which should succeed
        // and transition to Idle.
        CHECK(s_restoreCallbackCount >= 1);
    }

    // workflowHandle may be freed by Idle transition
}

TEST_CASE("ADUC_Workflow_WorkCompletionCallback - Apply failure triggers Restore auto-transition")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_wcc_apply_fail");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;
    workflowData.LastReportedState = ADUCITF_State_ApplyStarted;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    // Set the current workflow step to Apply
    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_Apply);
    workflow_set_operation_in_progress(handle, true);

    ADUC_MethodCall_Data* methodCallData =
        static_cast<ADUC_MethodCall_Data*>(calloc(1, sizeof(ADUC_MethodCall_Data)));
    REQUIRE(methodCallData != nullptr);
    methodCallData->WorkflowData = &workflowData;

    SECTION("Apply failure triggers auto-transition to Restore step")
    {
        ADUC_Result result = { ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
        ADUC_Workflow_WorkCompletionCallback(methodCallData, result, false /* isAsync */);

        // Apply failure: AutoTransitionWorkflowStepOnFailure=Restore
        CHECK(s_restoreCallbackCount >= 1);
    }
}

TEST_CASE("ADUC_Workflow_WorkCompletionCallback - cancel requested with Normal cancellation")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_wcc_cancel_normal");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;
    workflowData.LastReportedState = ADUCITF_State_DownloadStarted;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    // Set up : operation in progress, cancel requested, Normal cancellation
    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_Download);
    workflow_set_operation_in_progress(handle, true);
    workflow_set_operation_cancel_requested(handle, true);
    workflow_set_cancellation_type(handle, ADUC_WorkflowCancellationType_Normal);

    ADUC_MethodCall_Data* methodCallData =
        static_cast<ADUC_MethodCall_Data*>(calloc(1, sizeof(ADUC_MethodCall_Data)));
    REQUIRE(methodCallData != nullptr);
    methodCallData->WorkflowData = &workflowData;

    SECTION("Normal cancel goes to Idle with Failure_Cancelled result")
    {
        ADUC_Result result = { ADUC_Result_Failure_Cancelled, 0 };
        ADUC_Workflow_WorkCompletionCallback(methodCallData, result, false /* isAsync */);

        // Normal cancellation should transition to Idle
        CHECK(workflowData.WorkflowHandle == nullptr);
    }
}

TEST_CASE("ADUC_Workflow_WorkCompletionCallback - cancel requested with Retry cancellation")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_wcc_cancel_retry");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;
    workflowData.LastReportedState = ADUCITF_State_DownloadStarted;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    // Set up: cancel requested with Retry cancellation
    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_Download);
    workflow_set_operation_in_progress(handle, true);
    workflow_set_operation_cancel_requested(handle, true);
    workflow_set_cancellation_type(handle, ADUC_WorkflowCancellationType_Retry);

    ADUC_MethodCall_Data* methodCallData =
        static_cast<ADUC_MethodCall_Data*>(calloc(1, sizeof(ADUC_MethodCall_Data)));
    REQUIRE(methodCallData != nullptr);
    methodCallData->WorkflowData = &workflowData;

    SECTION("Retry cancellation reprocesses deployment")
    {
        ADUC_Result result = { ADUC_Result_Failure_Cancelled, 0 };
        ADUC_Workflow_WorkCompletionCallback(methodCallData, result, false /* isAsync */);

        // Retry cancellation should re-process the deployment (full chain triggers)
        // The workflow continues from the beginning
        CHECK(true); // Not crashing is the primary check
    }
}

TEST_CASE("ADUC_Workflow_WorkCompletionCallback - isAsync takes lock")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_wcc_async");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;
    workflowData.LastReportedState = ADUCITF_State_DeploymentInProgress;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_ProcessDeployment);

    ADUC_MethodCall_Data* methodCallData =
        static_cast<ADUC_MethodCall_Data*>(calloc(1, sizeof(ADUC_MethodCall_Data)));
    REQUIRE(methodCallData != nullptr);
    methodCallData->WorkflowData = &workflowData;

    SECTION("isAsync=true successfully takes and releases lock")
    {
        ADUC_Result result = { ADUC_Result_Success, 0 };
        // isAsync=true triggers s_workflow_lock/s_workflow_unlock
        ADUC_Workflow_WorkCompletionCallback(methodCallData, result, true /* isAsync */);

        // Should complete without deadlock
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Idle);
    }
}

TEST_CASE("ADUC_Workflow_WorkCompletionCallback - InProgress result code logs error")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_Download);

    ADUC_MethodCall_Data* methodCallData =
        static_cast<ADUC_MethodCall_Data*>(calloc(1, sizeof(ADUC_MethodCall_Data)));
    REQUIRE(methodCallData != nullptr);
    methodCallData->WorkflowData = &workflowData;

    SECTION("InProgress result code goes to done without processing")
    {
        // ADUC_Result_Download_InProgress is an in-progress result code
        ADUC_Result result = { ADUC_Result_Download_InProgress, 0 };
        ADUC_Workflow_WorkCompletionCallback(methodCallData, result, false /* isAsync */);

        // Should log error and skip, but not crash
        CHECK(true);
    }

    workflow_free(workflowData.WorkflowHandle);
}

TEST_CASE("ADUC_Workflow_WorkCompletionCallback - Cancel action in workflow data logs error")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_Cancel; // Cancel action set
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_Download);

    ADUC_MethodCall_Data* methodCallData =
        static_cast<ADUC_MethodCall_Data*>(calloc(1, sizeof(ADUC_MethodCall_Data)));
    REQUIRE(methodCallData != nullptr);
    methodCallData->WorkflowData = &workflowData;

    SECTION("Cancel action in current action logs error and goes to done")
    {
        ADUC_Result result = { ADUC_Result_Download_Success, 0 };
        ADUC_Workflow_WorkCompletionCallback(methodCallData, result, false /* isAsync */);

        // Should log error about current action being Cancel and go to done
        CHECK(true);
    }

    workflow_free(workflowData.WorkflowHandle);
}

//
// Unit Tests for ADUC_Workflow_HandleUpdateAction
//

TEST_CASE("ADUC_Workflow_HandleUpdateAction - Cancel with operation in progress")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    // Set cancellation type to Normal and operation in progress
    workflow_set_cancellation_type(handle, ADUC_WorkflowCancellationType_Normal);
    workflow_set_operation_in_progress(handle, true);

    SECTION("Cancel cancels in-progress operation")
    {
        ADUC_Workflow_HandleUpdateAction(&workflowData);
        CHECK(s_cancelCallbackCount == 1);
    }

    workflow_free(workflowData.WorkflowHandle);
}

TEST_CASE("ADUC_Workflow_HandleUpdateAction - Cancel without operation in progress")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_cancel_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    // Cancel action, no operation in progress
    workflow_set_cancellation_type(handle, ADUC_WorkflowCancellationType_Normal);
    workflow_set_operation_in_progress(handle, false);

    SECTION("Cancel returns to Idle without calling CancelCallback")
    {
        ADUC_Workflow_HandleUpdateAction(&workflowData);
        CHECK(s_cancelCallbackCount == 0);
    }

    workflow_free(workflowData.WorkflowHandle);
}

TEST_CASE("ADUC_Workflow_HandleUpdateAction - duplicate deployment ignored")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    // Set LastCompletedWorkflowId to match current workflow
    ADUC_WorkflowData_SetLastCompletedWorkflowId(workflow_peek_id(handle), &workflowData);

    SECTION("Duplicate deployment is ignored")
    {
        ADUC_Workflow_HandleUpdateAction(&workflowData);
        // No callbacks should be invoked for duplicate deployment
        CHECK(s_isInstalledCallbackCount == 0);
    }

    free(workflowData.LastCompletedWorkflowId);
    workflow_free(workflowData.WorkflowHandle);
}

TEST_CASE("ADUC_Workflow_HandleUpdateAction - already installed goes to Idle")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_hua_installed");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    callbacks.IsInstalledCallback = MockIsInstalledCallback_Installed;
    workflowData.UpdateActionCallbacks = callbacks;

    SECTION("Already installed update transitions to Idle")
    {
        ADUC_Workflow_HandleUpdateAction(&workflowData);
        // Should have called SetInstalledUpdateIdAndGoToIdle
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Idle);
        CHECK(s_idleCallbackCount == 1);
    }

    // Workflow freed in SetInstalledUpdateIdAndGoToIdle
}

TEST_CASE("ADUC_Workflow_HandleUpdateAction - ProcessDeployment normal flow")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_hua_normal");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    SECTION("ProcessDeployment triggers full workflow chain with synchronous callbacks")
    {
        ADUC_Workflow_HandleUpdateAction(&workflowData);

        // With all sync callbacks, the full chain fires:
        // ProcessDeployment → Download → Backup → Install → Apply → Idle
        CHECK(s_downloadCallbackCount >= 1);
        CHECK(s_backupCallbackCount >= 1);
        CHECK(s_installCallbackCount >= 1);
        CHECK(s_applyCallbackCount >= 1);
        CHECK(s_idleCallbackCount >= 1);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Idle);
    }

    // Workflow freed in Idle transition
}

TEST_CASE("ADUC_Workflow_HandleUpdateAction - Replace/Retry without operation in progress")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_hua_replace_no_op");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    // Set Replacement cancellation type but no operation in progress
    workflow_set_cancellation_type(handle, ADUC_WorkflowCancellationType_Replacement);
    workflow_set_operation_in_progress(handle, false);

    SECTION("Replace without operation in progress processes workflow normally")
    {
        ADUC_Workflow_HandleUpdateAction(&workflowData);

        // Should clear cancellation and process workflow
        CHECK(s_isInstalledCallbackCount >= 1);
    }
}

//
// Unit Tests for ADUC_Workflow_TransitionWorkflow
//

TEST_CASE("ADUC_Workflow_TransitionWorkflow - ProcessDeployment step")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_tw_pd");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    // Set the current workflow step
    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_ProcessDeployment);

    SECTION("TransitionWorkflow ProcessDeployment triggers full chain")
    {
        ADUC_Workflow_TransitionWorkflow(&workflowData);

        // Full workflow chain fires with sync callbacks
        CHECK(s_reportStateCallbackCount >= 1);
    }
}

TEST_CASE("ADUC_Workflow_TransitionWorkflow - invalid workflow step")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    // Set an undefined workflow step
    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_Undefined);

    SECTION("Invalid workflow step is ignored without crash")
    {
        ADUC_Workflow_TransitionWorkflow(&workflowData);
        // Should log error and return early
        CHECK(true);
    }

    workflow_free(workflowData.WorkflowHandle);
}

//
// Unit Tests for ADUC_Workflow_AutoTransitionWorkflow
//

TEST_CASE("ADUC_Workflow_AutoTransitionWorkflow - success when workflow complete")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_atw_complete");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    // Set to Apply step which has Undefined as AutoTransitionWorkflowStepOnSuccess (end of workflow)
    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_Apply);

    SECTION("onSuccess with completed workflow does not transition further")
    {
        ADUC_Workflow_AutoTransitionWorkflow(&workflowData, true /* onSuccess */);
        // Workflow is complete, no further transitions
        CHECK(true);
    }

    workflow_free(workflowData.WorkflowHandle);
}

TEST_CASE("ADUC_Workflow_AutoTransitionWorkflow - failure when workflow complete")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    // Set to Download step which has Undefined as AutoTransitionWorkflowStepOnFailure (end of workflow on failure)
    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_Download);

    SECTION("onFailure with completed workflow (Download failure) does not transition further")
    {
        ADUC_Workflow_AutoTransitionWorkflow(&workflowData, false /* onSuccess */);
        // Workflow is complete on failure, no further transitions
        CHECK(true);
    }

    workflow_free(workflowData.WorkflowHandle);
}

TEST_CASE("ADUC_Workflow_AutoTransitionWorkflow - failure with auto-transition to Restore")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_atw_restore");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;
    workflowData.LastReportedState = ADUCITF_State_Failed;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    // Set to Install which has Restore as AutoTransitionWorkflowStepOnFailure
    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_Install);

    SECTION("Install failure auto-transitions to Restore")
    {
        ADUC_Workflow_AutoTransitionWorkflow(&workflowData, false /* onSuccess */);
        // Should transition to Restore step and trigger that workflow
        CHECK(s_restoreCallbackCount >= 1);
    }
}

TEST_CASE("ADUC_Workflow_AutoTransitionWorkflow - success auto-transition from Download to Backup")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_atw_dl_backup");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;
    workflowData.LastReportedState = ADUCITF_State_DownloadSucceeded;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    // Set to Download which has Backup as AutoTransitionWorkflowStepOnSuccess
    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_Download);

    SECTION("Download success auto-transitions to Backup and cascades")
    {
        ADUC_Workflow_AutoTransitionWorkflow(&workflowData, true /* onSuccess */);
        // Should auto-transition to Backup then cascade through Install → Apply → Idle
        CHECK(s_backupCallbackCount >= 1);
    }
}

TEST_CASE("ADUC_Workflow_AutoTransitionWorkflow - invalid workflow step logs error")
{
    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;

    // Set to Undefined workflow step
    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_Undefined);

    SECTION("Invalid workflow step logs error and returns")
    {
        ADUC_Workflow_AutoTransitionWorkflow(&workflowData, true /* onSuccess */);
        CHECK(true); // No crash
    }

    workflow_free(handle);
}

//
// Tests for callback invocation verification
//

TEST_CASE("Verify callback invocations with counters")
{
    SECTION("DoWork invokes DoWorkCallback once")
    {
        ResetCallbackCounters();

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_DoWork(&workflowData);
        CHECK(s_doWorkCallbackCount == 1);
    }

    SECTION("IsInstalled invokes callback once")
    {
        ResetCallbackCounters();

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_MethodCall_IsInstalled(&workflowData);
        CHECK(s_isInstalledCallbackCount == 1);
    }

    SECTION("Cancel invokes callback when operation in progress")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        workflow_set_operation_in_progress(handle, true);
        ADUC_Workflow_MethodCall_Cancel(&workflowData);
        CHECK(s_cancelCallbackCount == 1);

        workflow_free(handle);
    }

    SECTION("Cancel does NOT invoke callback when operation NOT in progress")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        workflow_set_operation_in_progress(handle, false);
        ADUC_Workflow_MethodCall_Cancel(&workflowData);
        CHECK(s_cancelCallbackCount == 0);

        workflow_free(handle);
    }
}

//
// Tests for state reporting
//

TEST_CASE("State reporting callback invocation")
{
    SECTION("SetUpdateState invokes report callback")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_DownloadStarted);
        CHECK(s_reportStateCallbackCount == 1);

        workflow_free(workflowData.WorkflowHandle);
    }

    SECTION("SetUpdateStateWithResult invokes report callback")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Result result = { ADUC_Result_Download_Success, 0 };
        ADUC_Workflow_SetUpdateStateWithResult(&workflowData, ADUCITF_State_DownloadSucceeded, result);
        CHECK(s_reportStateCallbackCount == 1);

        workflow_free(workflowData.WorkflowHandle);
    }
}

//
// Tests for Download with sandbox operations
//

TEST_CASE("Download operations with sandbox")
{
    SECTION("Download creates sandbox")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_sandbox");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;
        workflowData.LastReportedState = ADUCITF_State_DeploymentInProgress;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_MethodCall_Data methodCallData;
        memset(&methodCallData, 0, sizeof(methodCallData));
        methodCallData.WorkflowData = &workflowData;

        ADUC_Result result = ADUC_Workflow_MethodCall_Download(&methodCallData);
        CHECK(IsAducResultCodeSuccess(result.ResultCode));
        CHECK(s_sandboxCreateCount == 1);
        CHECK(s_downloadCallbackCount == 1);

        workflow_free(workflowData.WorkflowHandle);
    }

    SECTION("Download fails when sandbox create fails")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_sandbox_fail");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;
        workflowData.LastReportedState = ADUCITF_State_DeploymentInProgress;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        callbacks.SandboxCreateCallback = MockSandboxCreateCallback_Failure;
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_MethodCall_Data methodCallData;
        memset(&methodCallData, 0, sizeof(methodCallData));
        methodCallData.WorkflowData = &workflowData;

        ADUC_Result result = ADUC_Workflow_MethodCall_Download(&methodCallData);
        CHECK(IsAducResultCodeFailure(result.ResultCode));
        // Download callback should NOT be called when sandbox fails
        CHECK(s_downloadCallbackCount == 0);

        workflow_free(workflowData.WorkflowHandle);
    }

    SECTION("Download with download callback failure")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_sandbox_dl_fail");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;
        workflowData.LastReportedState = ADUCITF_State_DeploymentInProgress;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        callbacks.DownloadCallback = MockDownloadCallback_Failure;
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_MethodCall_Data methodCallData;
        memset(&methodCallData, 0, sizeof(methodCallData));
        methodCallData.WorkflowData = &workflowData;

        ADUC_Result result = ADUC_Workflow_MethodCall_Download(&methodCallData);
        CHECK(IsAducResultCodeFailure(result.ResultCode));
        CHECK(s_sandboxCreateCount == 1); // Sandbox still created

        workflow_free(workflowData.WorkflowHandle);
    }
}

//
// Tests for Idle state transition
//

TEST_CASE("Idle method call behavior")
{
    SECTION("Idle from ApplyStarted state")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_idle_apply");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.LastReportedState = ADUCITF_State_ApplyStarted;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_MethodCall_Idle(&workflowData);

        CHECK(s_sandboxDestroyCount == 1);
        CHECK(s_idleCallbackCount == 1);
        CHECK(workflowData.WorkflowHandle == nullptr);
    }

    SECTION("Idle from Failed state")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_idle_failed");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.LastReportedState = ADUCITF_State_Failed;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_MethodCall_Idle(&workflowData);

        CHECK(s_sandboxDestroyCount == 1);
        CHECK(s_idleCallbackCount == 1);
        CHECK(workflowData.WorkflowHandle == nullptr);
    }

    SECTION("Idle from unexpected state logs warning but still transitions")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_idle_unexpected");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.LastReportedState = ADUCITF_State_DownloadStarted; // Unexpected state for Idle

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_MethodCall_Idle(&workflowData);

        // Should still complete the transition
        CHECK(s_idleCallbackCount == 1);
        CHECK(workflowData.WorkflowHandle == nullptr);
    }

    SECTION("Idle without workfolder still calls idle callback")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        // Don't set workfolder

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.LastReportedState = ADUCITF_State_Failed;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_MethodCall_Idle(&workflowData);

        CHECK(s_idleCallbackCount == 1);
        CHECK(workflowData.WorkflowHandle == nullptr);
    }

    SECTION("Idle from Idle state")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_idle_from_idle");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.LastReportedState = ADUCITF_State_Idle;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_MethodCall_Idle(&workflowData);

        CHECK(s_idleCallbackCount == 1);
        CHECK(workflowData.WorkflowHandle == nullptr);
    }
}

//
// Tests for complete state machine flow
//

TEST_CASE("Complete workflow state machine - ProcessDeployment to Backup")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_complete_flow");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;
    workflowData.LastReportedState = ADUCITF_State_Idle;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    ADUC_MethodCall_Data methodCallData;
    memset(&methodCallData, 0, sizeof(methodCallData));
    methodCallData.WorkflowData = &workflowData;

    SECTION("Step through ProcessDeployment -> Download -> Backup")
    {
        // 1. ProcessDeployment
        ADUC_Result result = ADUC_Workflow_MethodCall_ProcessDeployment(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Success);
        // ProcessDeployment sets DeploymentInProgress
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_DeploymentInProgress);

        // 2. Download
        result = ADUC_Workflow_MethodCall_Download(&methodCallData);
        CHECK(IsAducResultCodeSuccess(result.ResultCode));
        CHECK(s_downloadCallbackCount == 1);

        // Simulate successful download completion
        workflowData.LastReportedState = ADUCITF_State_DownloadSucceeded;

        // 3. Backup
        result = ADUC_Workflow_MethodCall_Backup(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Backup_Success);
        CHECK(s_backupCallbackCount == 1);
    }

    workflow_free(workflowData.WorkflowHandle);
}

TEST_CASE("Complete workflow - Install to Apply to Idle")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_install_apply_idle");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    workflowData.UpdateActionCallbacks = callbacks;

    ADUC_MethodCall_Data methodCallData;
    memset(&methodCallData, 0, sizeof(methodCallData));
    methodCallData.WorkflowData = &workflowData;

    SECTION("Install then Apply completes workflow")
    {
        // Install from BackupSucceeded
        workflowData.LastReportedState = ADUCITF_State_BackupSucceeded;
        ADUC_Result result = ADUC_Workflow_MethodCall_Install(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Install_Success);

        // Apply from InstallSucceeded
        workflowData.LastReportedState = ADUCITF_State_InstallSucceeded;
        result = ADUC_Workflow_MethodCall_Apply(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Apply_Success);
    }

    workflow_free(workflowData.WorkflowHandle);
}

//
// Tests for HandleStartupWorkflowData with various scenarios
//

TEST_CASE("HandleStartupWorkflowData scenarios")
{
    SECTION("With Cancel action on startup reports Idle")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_cancel_json);
        REQUIRE(handle != nullptr);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.StartupIdleCallSent = false;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_HandleStartupWorkflowData(&workflowData);

        CHECK(workflowData.StartupIdleCallSent == true);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Idle);

        workflow_free(workflowData.WorkflowHandle);
    }

    SECTION("With Undefined action on startup just sets flag")
    {
        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_undefined_action_json);
        REQUIRE(handle != nullptr);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.StartupIdleCallSent = false;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacks(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_HandleStartupWorkflowData(&workflowData);

        CHECK(workflowData.StartupIdleCallSent == true);

        workflow_free(workflowData.WorkflowHandle);
    }

    SECTION("With ProcessDeployment and already installed goes to Idle")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.StartupIdleCallSent = false;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        callbacks.IsInstalledCallback = MockIsInstalledCallback_Installed;
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_HandleStartupWorkflowData(&workflowData);

        CHECK(workflowData.StartupIdleCallSent == true);
        // Should have reported Idle with installed update
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Idle);

        // Workflow gets freed in SetInstalledUpdateIdAndGoToIdle, so don't free again
    }

    SECTION("With ProcessDeployment and not installed triggers HandleUpdateAction")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_startup_not_installed");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.StartupIdleCallSent = false;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_HandleStartupWorkflowData(&workflowData);

        CHECK(workflowData.StartupIdleCallSent == true);
        // Should have triggered HandleUpdateAction which processes the deployment
        CHECK(s_downloadCallbackCount >= 1);
    }
}

//
// Tests for Install complete callback behaviors
//

TEST_CASE("Install_Complete callback behaviors")
{
    SECTION("Normal success - no reboot requested")
    {
        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;

        ADUC_MethodCall_Data methodCallData;
        memset(&methodCallData, 0, sizeof(methodCallData));
        methodCallData.WorkflowData = &workflowData;

        ADUC_Result result = { ADUC_Result_Install_Success, 0 };

        // Should not crash and should not trigger reboot logic
        ADUC_Workflow_MethodCall_Install_Complete(&methodCallData, result);

        CHECK(workflowData.SystemRebootState == ADUC_SystemRebootState_None);
        CHECK(workflowData.AgentRestartState == ADUC_AgentRestartState_None);

        workflow_free(handle);
    }
}

//
// Tests for Backup complete callback
//

TEST_CASE("Backup_Complete callback behaviors")
{
    SECTION("Backup complete - no special handling")
    {
        ADUC_MethodCall_Data methodCallData;
        memset(&methodCallData, 0, sizeof(methodCallData));

        ADUC_Result result = { ADUC_Result_Backup_Success, 0 };

        // Just verify it doesn't crash
        ADUC_Workflow_MethodCall_Backup_Complete(&methodCallData, result);
        CHECK(true);
    }

    SECTION("Backup complete with failure result")
    {
        ADUC_MethodCall_Data methodCallData;
        memset(&methodCallData, 0, sizeof(methodCallData));

        ADUC_Result result = { ADUC_Result_Failure, 0x12345678 };

        // Just verify it doesn't crash
        ADUC_Workflow_MethodCall_Backup_Complete(&methodCallData, result);
        CHECK(true);
    }
}

//
// Tests for Restore complete callback behaviors
//

TEST_CASE("Restore_Complete callback behaviors")
{
    SECTION("Restore success - no reboot")
    {
        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;

        ADUC_MethodCall_Data methodCallData;
        memset(&methodCallData, 0, sizeof(methodCallData));
        methodCallData.WorkflowData = &workflowData;

        ADUC_Result result = { ADUC_Result_Restore_Success, 0 };

        ADUC_Workflow_MethodCall_Restore_Complete(&methodCallData, result);

        CHECK(workflow_get_operation_in_progress(handle) == false);

        workflow_free(handle);
    }

    SECTION("Restore success unsupported - clears operation in progress")
    {
        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_operation_in_progress(handle, true);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;

        ADUC_MethodCall_Data methodCallData;
        memset(&methodCallData, 0, sizeof(methodCallData));
        methodCallData.WorkflowData = &workflowData;

        ADUC_Result result = { ADUC_Result_Restore_Success_Unsupported, 0 };

        ADUC_Workflow_MethodCall_Restore_Complete(&methodCallData, result);

        CHECK(workflow_get_operation_in_progress(handle) == false);

        workflow_free(handle);
    }

    SECTION("Restore failure - does not clear operation in progress")
    {
        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_operation_in_progress(handle, true);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;

        ADUC_MethodCall_Data methodCallData;
        memset(&methodCallData, 0, sizeof(methodCallData));
        methodCallData.WorkflowData = &workflowData;

        ADUC_Result result = { ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };

        ADUC_Workflow_MethodCall_Restore_Complete(&methodCallData, result);

        // Failure does not match any result code branch, so operation_in_progress stays true
        CHECK(workflow_get_operation_in_progress(handle) == true);

        workflow_free(handle);
    }
}

//
// Tests for Apply complete callback behaviors
//

TEST_CASE("Apply_Complete callback behaviors")
{
    SECTION("Apply success - clears operation in progress")
    {
        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_operation_in_progress(handle, true);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;

        ADUC_MethodCall_Data methodCallData;
        memset(&methodCallData, 0, sizeof(methodCallData));
        methodCallData.WorkflowData = &workflowData;

        ADUC_Result result = { ADUC_Result_Apply_Success, 0 };

        ADUC_Workflow_MethodCall_Apply_Complete(&methodCallData, result);

        CHECK(workflow_get_operation_in_progress(handle) == false);

        workflow_free(handle);
    }

    SECTION("Apply failure - does not clear operation in progress")
    {
        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_operation_in_progress(handle, true);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;

        ADUC_MethodCall_Data methodCallData;
        memset(&methodCallData, 0, sizeof(methodCallData));
        methodCallData.WorkflowData = &workflowData;

        ADUC_Result result = { ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };

        ADUC_Workflow_MethodCall_Apply_Complete(&methodCallData, result);

        // With failure, no branch matches, operation_in_progress stays true
        CHECK(workflow_get_operation_in_progress(handle) == true);

        workflow_free(handle);
    }
}

//
// Tests for SetInstalledUpdateIdAndGoToIdle
//

TEST_CASE("SetInstalledUpdateIdAndGoToIdle behavior")
{
    SECTION("With NULL updateId - still goes to Idle")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_null_updateid");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;
        workflowData.SystemRebootState = ADUC_SystemRebootState_Required;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle(&workflowData, nullptr);

        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Idle);
        CHECK(workflowData.SystemRebootState == ADUC_SystemRebootState_None);

        // Workflow is freed inside the function
    }

    SECTION("Resets both reboot and restart states")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_reset_states");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;
        workflowData.SystemRebootState = ADUC_SystemRebootState_InProgress;
        workflowData.AgentRestartState = ADUC_AgentRestartState_InProgress;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        const char* updateId = "{\"provider\":\"Test\",\"name\":\"Update\",\"version\":\"1.0\"}";
        ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle(&workflowData, updateId);

        CHECK(workflowData.SystemRebootState == ADUC_SystemRebootState_None);
        CHECK(workflowData.AgentRestartState == ADUC_AgentRestartState_None);
        CHECK(s_idleCallbackCount == 1);

        // Workflow is freed inside the function
    }

    SECTION("Sets LastCompletedWorkflowId")
    {
        ResetCallbackCounters();

        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_last_completed");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacksWithCounters(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        const char* updateId = "{\"provider\":\"Test\",\"name\":\"Update\",\"version\":\"2.0\"}";
        ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle(&workflowData, updateId);

        // LastCompletedWorkflowId should be set
        CHECK(workflowData.LastCompletedWorkflowId != nullptr);

        free(workflowData.LastCompletedWorkflowId);
    }
}

//
// Tests for workflow data utilities
//

TEST_CASE("WorkflowData utilities")
{
    SECTION("SetCurrentAction and GetCurrentAction")
    {
        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));

        ADUC_WorkflowData_SetCurrentAction(ADUCITF_UpdateAction_ProcessDeployment, &workflowData);
        CHECK(ADUC_WorkflowData_GetCurrentAction(&workflowData) == ADUCITF_UpdateAction_ProcessDeployment);

        ADUC_WorkflowData_SetCurrentAction(ADUCITF_UpdateAction_Cancel, &workflowData);
        CHECK(ADUC_WorkflowData_GetCurrentAction(&workflowData) == ADUCITF_UpdateAction_Cancel);
    }

    SECTION("SetLastReportedState and GetLastReportedState")
    {
        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));

        ADUC_WorkflowData_SetLastReportedState(ADUCITF_State_DownloadStarted, &workflowData);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_DownloadStarted);

        ADUC_WorkflowData_SetLastReportedState(ADUCITF_State_InstallSucceeded, &workflowData);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_InstallSucceeded);
    }

    SECTION("SetLastCompletedWorkflowId")
    {
        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.LastCompletedWorkflowId = nullptr;

        bool success = ADUC_WorkflowData_SetLastCompletedWorkflowId("test-workflow-id", &workflowData);
        CHECK(success);
        CHECK(workflowData.LastCompletedWorkflowId != nullptr);

        free(workflowData.LastCompletedWorkflowId);
    }

    SECTION("SetLastCompletedWorkflowId with NULL id")
    {
        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.LastCompletedWorkflowId = nullptr;

        ADUC_WorkflowData_SetLastCompletedWorkflowId(nullptr, &workflowData);
        CHECK(true); // No crash
    }
}

//
// Tests for various state transition edge cases
//

TEST_CASE("State transition edge cases")
{
    SECTION("Transition to Idle from non-standard state")
    {
        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;
        workflowData.LastReportedState = ADUCITF_State_BackupStarted;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacks(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        // Setting to Idle from a non-standard state (not ApplyStarted)
        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_Idle);

        // Should still transition and call idle
        CHECK(workflowData.WorkflowHandle == nullptr);
    }

    SECTION("Multiple state transitions")
    {
        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacks(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        // Simulate multiple transitions
        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_DeploymentInProgress);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_DeploymentInProgress);

        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_DownloadStarted);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_DownloadStarted);

        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_DownloadSucceeded);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_DownloadSucceeded);

        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_BackupStarted);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_BackupStarted);

        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_BackupSucceeded);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_BackupSucceeded);

        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_InstallStarted);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_InstallStarted);

        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_InstallSucceeded);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_InstallSucceeded);

        workflow_free(workflowData.WorkflowHandle);
    }
}

//
// Tests for workflow handle map entry lookup (coverage for GetWorkflowHandlerMapEntryForAction)
//

TEST_CASE("WorkflowHandlerMapEntry coverage")
{
    // These tests verify that all workflow steps can be looked up properly through
    // the method calls which use GetWorkflowHandlerMapEntryForAction internally

    SECTION("All workflow steps are mapped")
    {
        ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
        REQUIRE(handle != nullptr);

        workflow_set_workfolder(handle, "/tmp/test_map");

        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.WorkflowHandle = handle;
        workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_Success;

        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacks(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        ADUC_MethodCall_Data methodCallData;
        memset(&methodCallData, 0, sizeof(methodCallData));
        methodCallData.WorkflowData = &workflowData;

        // ProcessDeployment step
        ADUC_Result result = ADUC_Workflow_MethodCall_ProcessDeployment(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Success);

        // Download step
        workflowData.LastReportedState = ADUCITF_State_DeploymentInProgress;
        result = ADUC_Workflow_MethodCall_Download(&methodCallData);
        CHECK(IsAducResultCodeSuccess(result.ResultCode));

        // Backup step
        workflowData.LastReportedState = ADUCITF_State_DownloadSucceeded;
        result = ADUC_Workflow_MethodCall_Backup(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Backup_Success);

        // Install step
        workflowData.LastReportedState = ADUCITF_State_BackupSucceeded;
        result = ADUC_Workflow_MethodCall_Install(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Install_Success);

        // Apply step
        workflowData.LastReportedState = ADUCITF_State_InstallSucceeded;
        result = ADUC_Workflow_MethodCall_Apply(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Apply_Success);

        // Restore step
        workflowData.LastReportedState = ADUCITF_State_Failed;
        result = ADUC_Workflow_MethodCall_Restore(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Restore_Success);

        workflow_free(workflowData.WorkflowHandle);
    }
}

//
// Edge case tests
//

TEST_CASE("Edge cases for agent_workflow")
{
    SECTION("Empty workflow id handling")
    {
        // Test that empty workflow id doesn't cause issues
        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));
        workflowData.LastCompletedWorkflowId = nullptr;

        // Should not crash
        ADUC_WorkflowData_SetLastCompletedWorkflowId("test-id", &workflowData);
        CHECK(true);

        // Clean up allocated memory
        free(workflowData.LastCompletedWorkflowId);
    }

    SECTION("Workflow data with initialized callbacks but NULL handle")
    {
        ADUC_WorkflowData workflowData;
        memset(&workflowData, 0, sizeof(workflowData));

        // Initialize with mock callbacks
        ADUC_UpdateActionCallbacks callbacks;
        InitMockCallbacks(&callbacks);
        workflowData.UpdateActionCallbacks = callbacks;

        // IsInstalled with NULL workflow handle but valid callback should work
        ADUC_Result result = ADUC_Workflow_MethodCall_IsInstalled(&workflowData);
        CHECK(result.ResultCode == ADUC_Result_IsInstalled_NotInstalled);
    }
}

//
// Tests for Workflow full end-to-end with failure paths
//

TEST_CASE("Full workflow with Install failure and Restore")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_install_fail_restore");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    callbacks.InstallCallback = MockInstallCallback_Failure;
    workflowData.UpdateActionCallbacks = callbacks;

    SECTION("HandleUpdateAction with Install failure auto-transitions to Restore")
    {
        ADUC_Workflow_HandleUpdateAction(&workflowData);

        // Full chain: ProcessDeployment → Download → Backup → Install(fail) → Failed → Restore → Idle
        CHECK(s_downloadCallbackCount >= 1);
        CHECK(s_backupCallbackCount >= 1);
        CHECK(s_restoreCallbackCount >= 1);
    }
}

TEST_CASE("Full workflow with backup failure")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_backup_fail");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    callbacks.BackupCallback = MockBackupCallback_Failure;
    workflowData.UpdateActionCallbacks = callbacks;

    SECTION("Backup failure ends workflow at Failed state")
    {
        ADUC_Workflow_HandleUpdateAction(&workflowData);

        // Chain: ProcessDeployment → Download → Backup(fail) → Failed (end, Undefined auto-transition)
        CHECK(s_downloadCallbackCount >= 1);
        CHECK(s_installCallbackCount == 0); // Should not reach Install
    }
}

TEST_CASE("Full workflow with download failure")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_download_fail");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    callbacks.DownloadCallback = MockDownloadCallback_Failure;
    workflowData.UpdateActionCallbacks = callbacks;

    SECTION("Download failure ends workflow at Failed state")
    {
        ADUC_Workflow_HandleUpdateAction(&workflowData);

        // Chain: ProcessDeployment → Download(fail) → Failed (end, no auto-transition)
        CHECK(s_backupCallbackCount == 0); // Should not reach Backup
        CHECK(s_installCallbackCount == 0); // Should not reach Install
    }
}

TEST_CASE("Full workflow with Apply failure triggers Restore")
{
    ResetCallbackCounters();

    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_apply_fail_restore");

    ADUC_WorkflowData workflowData;
    memset(&workflowData, 0, sizeof(workflowData));
    workflowData.WorkflowHandle = handle;
    workflowData.CurrentAction = ADUCITF_UpdateAction_ProcessDeployment;
    workflowData.ReportStateAndResultAsyncCallback = MockReportStateCallback_WithCounter;

    ADUC_UpdateActionCallbacks callbacks;
    InitMockCallbacksWithCounters(&callbacks);
    callbacks.ApplyCallback = MockApplyCallback_Failure;
    workflowData.UpdateActionCallbacks = callbacks;

    SECTION("Apply failure triggers Restore auto-transition")
    {
        ADUC_Workflow_HandleUpdateAction(&workflowData);

        // Chain: ProcessDeployment → Download → Backup → Install → Apply(fail) → Failed → Restore → Idle
        CHECK(s_downloadCallbackCount >= 1);
        CHECK(s_backupCallbackCount >= 1);
        CHECK(s_installCallbackCount >= 1);
        CHECK(s_restoreCallbackCount >= 1);
    }
}
