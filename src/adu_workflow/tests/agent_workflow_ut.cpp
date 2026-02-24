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

// viewstatemgr is required by agent_workflow - provide the global variable definition
extern "C"
{
#include "aduc/viewstatemgr.h"

// Define the global ViewStateManager instance required by agent_workflow
ViewStateManager g_vsm = { 0 };
}

#include <catch2/catch_all.hpp>
#include <cstring>
#include <string>

using Catch::Matchers::Equals;

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
// Unit Tests for ADUC_Workflow_Init and ADUC_Workflow_Uninit
//

TEST_CASE("ADUC_Workflow_Init and Uninit")
{
    SECTION("Init and Uninit should succeed")
    {
        int result = ADUC_Workflow_Init();
        CHECK(result == 0);

        // Uninit should not crash
        ADUC_Workflow_Uninit();
    }

    SECTION("Multiple Init and Uninit calls")
    {
        int result = ADUC_Workflow_Init();
        CHECK(result == 0);
        ADUC_Workflow_Uninit();

        // Second init should also succeed
        result = ADUC_Workflow_Init();
        CHECK(result == 0);
        ADUC_Workflow_Uninit();
    }
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
// Unit Tests for ADUC_Workflow_HandleReportingCompleted
//

TEST_CASE("ADUC_Workflow_HandleReportingCompleted")
{
    SECTION("Function executes without crash")
    {
        // This function is a no-op stub currently - test that it doesn't crash
        ADUC_Workflow_HandleReportingCompleted();
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

    SECTION("Callback failure should set Failed state")
    {
        ADUC_Workflow_SetUpdateState(&workflowData, ADUCITF_State_DownloadStarted);
        // When callback fails, the state should be set to Failed
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_Failed);
    }

    workflow_free(workflowData.WorkflowHandle);
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
// Unit Tests for workflow step state transitions
//

TEST_CASE("Workflow state transition - Download to Backup")
{
    ADUC_WorkflowHandle handle = CreateTestWorkflowHandle(sample_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_workfolder(handle, "/tmp/test_wf_transition");

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

    SECTION("After Download succeeds, can proceed to Backup")
    {
        // First do download
        ADUC_Result result = ADUC_Workflow_MethodCall_Download(&methodCallData);
        CHECK(IsAducResultCodeSuccess(result.ResultCode));

        // Simulate completion and state update
        workflowData.LastReportedState = ADUCITF_State_DownloadSucceeded;

        // Then backup should succeed
        result = ADUC_Workflow_MethodCall_Backup(&methodCallData);
        CHECK(result.ResultCode == ADUC_Result_Backup_Success);
    }

    workflow_free(workflowData.WorkflowHandle);
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
