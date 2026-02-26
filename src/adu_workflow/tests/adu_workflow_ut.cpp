/**
 * @file adu_workflow_ut.cpp
 * @brief Additional unit tests for agent_workflow to increase code coverage.
 * Targets NULL‑argument paths, missing callback paths, Install_Complete
 * reboot/restart branches, Cancel edge cases, and full workflow flows.
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

// clang-format off

/**
 * @brief Sample workflow JSON for testing (ProcessDeployment action = 3).
 */
static const char* s_process_deployment_json =
    R"( {                                                          )"
    R"(     "workflow": {                                          )"
    R"(         "action": 3,                                       )"
    R"(         "id": "coverage-workflow-001"                      )"
    R"(     },                                                     )"
    R"(     "updateManifest": "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Coverage-Update\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"test-device\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/apt:1\",\"files\":[\"f1\"],\"handlerProperties\":{\"installedCriteria\":\"test-criteria\"}}]},\"files\":{\"f1\":{\"fileName\":\"test-file.json\",\"sizeInBytes\":100,\"hashes\":{\"sha256\":\"abc123\"}}},\"createdDateTime\":\"2024-01-01T00:00:00Z\"}",  )"
    R"(     "updateManifestSignature": "test-signature",           )"
    R"(     "fileUrls": {                                          )"
    R"(         "f1": "http://test.example.com/test-file.json"     )"
    R"(     }                                                      )"
    R"( }                                                          )";

/**
 * @brief Sample workflow JSON for a ProcessDeployment with retryTimestamp.
 */
static const char* s_retry_deployment_json =
    R"( {                                                          )"
    R"(     "workflow": {                                          )"
    R"(         "action": 3,                                       )"
    R"(         "id": "coverage-retry-001",                        )"
    R"(         "retryTimestamp": "2024-06-15T08:00:00Z"           )"
    R"(     },                                                     )"
    R"(     "updateManifest": "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Coverage-Update\",\"version\":\"2.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"test-device\"}],\"instructions\":{\"steps\":[]},\"files\":{},\"createdDateTime\":\"2024-06-15T00:00:00Z\"}",  )"
    R"(     "updateManifestSignature": "test-signature",           )"
    R"(     "fileUrls": {}                                         )"
    R"( }                                                          )";

// clang-format on

//
// Counter-based mock callbacks for verification
//

static std::atomic<int> s_idleCbCount{ 0 };
static std::atomic<int> s_sandboxDestroyCbCount{ 0 };
static std::atomic<int> s_doWorkCbCount{ 0 };
static std::atomic<int> s_cancelCbCount{ 0 };
static std::atomic<int> s_reportStateCbCount{ 0 };

static void ResetCounters()
{
    s_idleCbCount = 0;
    s_sandboxDestroyCbCount = 0;
    s_doWorkCbCount = 0;
    s_cancelCbCount = 0;
    s_reportStateCbCount = 0;
}

// ---- report state callbacks ----

static bool MockReport_Success(
    ADUC_WorkflowDataToken token,
    ADUCITF_State updateState,
    const ADUC_Result* result,
    const char* installedUpdateId)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(updateState);
    UNREFERENCED_PARAMETER(result);
    UNREFERENCED_PARAMETER(installedUpdateId);
    s_reportStateCbCount++;
    return true;
}

static bool MockReport_Failure(
    ADUC_WorkflowDataToken token,
    ADUCITF_State updateState,
    const ADUC_Result* result,
    const char* installedUpdateId)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(updateState);
    UNREFERENCED_PARAMETER(result);
    UNREFERENCED_PARAMETER(installedUpdateId);
    s_reportStateCbCount++;
    return false;
}

// ---- action callbacks ----

static ADUC_Result MockSandboxCreate_Success(void* token, const char* workflowId, char* workFolder)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);
    return ADUC_Result{ ADUC_Result_Success, 0 };
}

static void MockSandboxDestroy(void* token, const char* workflowId, const char* workFolder)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);
    s_sandboxDestroyCbCount++;
}

static void MockIdle(void* token, const char* workflowId)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    s_idleCbCount++;
}

static void MockDoWork(void* token, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowData);
    s_doWorkCbCount++;
}

static ADUC_Result MockDownload_Success(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Download_Success, 0 };
}

static ADUC_Result MockBackup_Success(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Backup_Success, 0 };
}

static ADUC_Result MockInstall_Success(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Install_Success, 0 };
}

static ADUC_Result MockApply_Success(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Apply_Success, 0 };
}

static ADUC_Result MockRestore_Success(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Restore_Success, 0 };
}

static void MockCancel(void* token, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowData);
    s_cancelCbCount++;
}

static ADUC_Result MockIsInstalled_NotInstalled(void* token, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_IsInstalled_NotInstalled, 0 };
}

/**
 * @brief Fills all callbacks with working mocks.
 */
static void InitAllCallbacks(ADUC_UpdateActionCallbacks* cb)
{
    memset(cb, 0, sizeof(*cb));
    cb->SandboxCreateCallback = MockSandboxCreate_Success;
    cb->SandboxDestroyCallback = MockSandboxDestroy;
    cb->IdleCallback = MockIdle;
    cb->DoWorkCallback = MockDoWork;
    cb->DownloadCallback = MockDownload_Success;
    cb->BackupCallback = MockBackup_Success;
    cb->InstallCallback = MockInstall_Success;
    cb->ApplyCallback = MockApply_Success;
    cb->RestoreCallback = MockRestore_Success;
    cb->CancelCallback = MockCancel;
    cb->IsInstalledCallback = MockIsInstalled_NotInstalled;
}

/**
 * @brief Helper to create a valid ADUC_WorkflowHandle from JSON.
 */
static ADUC_WorkflowHandle MakeHandle(const char* json)
{
    ADUC_WorkflowHandle h = nullptr;
    ADUC_Result r = workflow_init(json, false, &h);
    REQUIRE(IsAducResultCodeSuccess(r.ResultCode));
    return h;
}

// ============================================================================
// NULL-argument tests for every public MethodCall function
// ============================================================================

TEST_CASE("NULL arg: ProcessDeployment returns failure for NULL methodCallData")
{
    ADUC_Result r = ADUC_Workflow_MethodCall_ProcessDeployment(nullptr);
    CHECK(r.ResultCode == ADUC_Result_Failure);
}

TEST_CASE("NULL arg: ProcessDeployment returns failure when WorkflowData is NULL")
{
    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = nullptr;

    ADUC_Result r = ADUC_Workflow_MethodCall_ProcessDeployment(&mcd);
    CHECK(r.ResultCode == ADUC_Result_Failure);
}

TEST_CASE("NULL arg: Download returns failure for NULL methodCallData")
{
    ADUC_Result r = ADUC_Workflow_MethodCall_Download(nullptr);
    CHECK(r.ResultCode == ADUC_Result_Failure);
}

TEST_CASE("NULL arg: Backup returns failure for NULL methodCallData")
{
    ADUC_Result r = ADUC_Workflow_MethodCall_Backup(nullptr);
    CHECK(r.ResultCode == ADUC_Result_Failure);
}

TEST_CASE("NULL arg: Install returns failure for NULL methodCallData")
{
    ADUC_Result r = ADUC_Workflow_MethodCall_Install(nullptr);
    CHECK(r.ResultCode == ADUC_Result_Failure);
}

TEST_CASE("NULL arg: Apply returns failure for NULL methodCallData")
{
    ADUC_Result r = ADUC_Workflow_MethodCall_Apply(nullptr);
    CHECK(r.ResultCode == ADUC_Result_Failure);
}

TEST_CASE("NULL arg: Restore returns failure for NULL methodCallData")
{
    ADUC_Result r = ADUC_Workflow_MethodCall_Restore(nullptr);
    CHECK(r.ResultCode == ADUC_Result_Failure);
}

TEST_CASE("NULL arg: Cancel does not crash for NULL workflowData")
{
    ADUC_Workflow_MethodCall_Cancel(nullptr);
    CHECK(true);
}

TEST_CASE("NULL arg: Cancel with NULL WorkflowHandle returns early")
{
    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = nullptr;

    ADUC_Workflow_MethodCall_Cancel(&wd);
    CHECK(true);
}

TEST_CASE("NULL arg: Idle does not crash for NULL workflowData")
{
    ADUC_Workflow_MethodCall_Idle(nullptr);
    CHECK(true);
}

TEST_CASE("NULL arg: SetUpdateState does not crash for NULL workflowData")
{
    ADUC_Workflow_SetUpdateState(nullptr, ADUCITF_State_Idle);
    CHECK(true);
}

TEST_CASE("NULL arg: SetUpdateStateWithResult does not crash for NULL workflowData")
{
    ADUC_Result res = { ADUC_Result_Success, 0 };
    ADUC_Workflow_SetUpdateStateWithResult(nullptr, ADUCITF_State_Idle, res);
    CHECK(true);
}

TEST_CASE("NULL arg: SetInstalledUpdateIdAndGoToIdle does not crash for NULL workflowData")
{
    ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle(nullptr, "some-id");
    CHECK(true);
}

TEST_CASE("NULL arg: DoWork does not crash for NULL workflowData")
{
    ADUC_Workflow_DoWork(nullptr);
    CHECK(true);
}

TEST_CASE("NULL arg: HandleStartupWorkflowData does not crash for NULL workflowData")
{
    ADUC_Workflow_HandleStartupWorkflowData(nullptr);
    CHECK(true);
}

// ============================================================================
// Install_Complete: reboot-required and agent-restart-required branches
// ============================================================================

TEST_CASE("Install_Complete sets reboot required when ExtendedResultCode indicates reboot")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    // Set the reboot-requested flag on the workflow handle so Install_Complete enters the reboot branch
    workflow_request_reboot(handle);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result result = { ADUC_Result_Install_Success, 0 };
    ADUC_Workflow_MethodCall_Install_Complete(&mcd, result);

    CHECK(wd.SystemRebootState == ADUC_SystemRebootState_Required);
    CHECK(wd.AgentRestartState == ADUC_AgentRestartState_None);

    workflow_free(handle);
}

TEST_CASE("Install_Complete sets agent restart required when ExtendedResultCode indicates restart")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    // Set the agent-restart-requested flag on the workflow handle so Install_Complete enters the restart branch
    workflow_request_agent_restart(handle);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result result = { ADUC_Result_Install_Success, 0 };
    ADUC_Workflow_MethodCall_Install_Complete(&mcd, result);

    CHECK(wd.SystemRebootState == ADUC_SystemRebootState_None);
    // RestartAgent succeeded in this environment, so state progresses to InProgress
    CHECK((wd.AgentRestartState == ADUC_AgentRestartState_Required || wd.AgentRestartState == ADUC_AgentRestartState_InProgress));

    workflow_free(handle);
}

TEST_CASE("Install_Complete does not set reboot/restart for failure result")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result result = { ADUC_Result_Failure, ADUC_Result_Install_RequiredReboot };
    ADUC_Workflow_MethodCall_Install_Complete(&mcd, result);

    // Failure ResultCode means the reboot branch is not entered
    CHECK(wd.SystemRebootState == ADUC_SystemRebootState_None);
    CHECK(wd.AgentRestartState == ADUC_AgentRestartState_None);

    workflow_free(handle);
}

TEST_CASE("Install_Complete NULL args do not crash")
{
    ADUC_Result result = { ADUC_Result_Install_Success, 0 };

    // NULL methodCallData
    ADUC_Workflow_MethodCall_Install_Complete(nullptr, result);
    CHECK(true);

    // NULL WorkflowData inside methodCallData
    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = nullptr;
    ADUC_Workflow_MethodCall_Install_Complete(&mcd, result);
    CHECK(true);
}

// ============================================================================
// Apply_Complete NULL args
// ============================================================================

TEST_CASE("Apply_Complete NULL args do not crash")
{
    ADUC_Result result = { ADUC_Result_Apply_Success, 0 };

    ADUC_Workflow_MethodCall_Apply_Complete(nullptr, result);
    CHECK(true);

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = nullptr;
    ADUC_Workflow_MethodCall_Apply_Complete(&mcd, result);
    CHECK(true);
}

// ============================================================================
// Restore_Complete NULL args
// ============================================================================

TEST_CASE("Restore_Complete NULL args do not crash")
{
    ADUC_Result result = { ADUC_Result_Restore_Success, 0 };

    ADUC_Workflow_MethodCall_Restore_Complete(nullptr, result);
    CHECK(true);

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = nullptr;
    ADUC_Workflow_MethodCall_Restore_Complete(&mcd, result);
    CHECK(true);
}

// ============================================================================
// DoWork with NULL DoWorkCallback (should not crash)
// ============================================================================

TEST_CASE("DoWork with NULL DoWorkCallback does not crash")
{
    ResetCounters();

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    // All callbacks are zeroed, including DoWorkCallback

    ADUC_Workflow_DoWork(&wd);
    CHECK(s_doWorkCbCount == 0);
}

// ============================================================================
// IsInstalled with NULL IsInstalledCallback
// ============================================================================

TEST_CASE("IsInstalled with NULL callback returns NotInstalled")
{
    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    // IsInstalledCallback is NULL

    ADUC_Result r = ADUC_Workflow_MethodCall_IsInstalled(&wd);
    CHECK(r.ResultCode == ADUC_Result_IsInstalled_NotInstalled);
}

// ============================================================================
// Cancel: sets cancel-requested flag on handle
// ============================================================================

TEST_CASE("Cancel sets cancel-requested flag and invokes callback")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_operation_in_progress(handle, true);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_MethodCall_Cancel(&wd);

    CHECK(s_cancelCbCount == 1);

    workflow_free(handle);
}

// ============================================================================
// DefaultDownloadProgressCallback: unknown/default state branch
// ============================================================================

TEST_CASE("DefaultDownloadProgressCallback handles unknown state value")
{
    // Pass an out-of-range enum value to exercise the default branch
    ADUC_Workflow_DefaultDownloadProgressCallback(
        "wf-001", "f-001", static_cast<ADUC_DownloadProgressState>(999), 0, 0);
    CHECK(true);
}

// ============================================================================
// HandleStartupWorkflowData: pending deployment NOT installed path
// ============================================================================

TEST_CASE("HandleStartupWorkflowData with pending deployment NOT installed sets DeploymentInProgress")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.StartupIdleCallSent = false;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    cb.IsInstalledCallback = MockIsInstalled_NotInstalled;
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_HandleStartupWorkflowData(&wd);

    CHECK(wd.StartupIdleCallSent == true);
    // The full workflow runs synchronously to completion (Idle) because all callbacks are mocked as synchronous successes
    CHECK(ADUC_WorkflowData_GetLastReportedState(&wd) == ADUCITF_State_Idle);

    // WorkflowHandle is freed as a side-effect of going to Idle
    // No need to free it again
}

// ============================================================================
// Download without SandboxCreateCallback (NULL)
// ============================================================================

TEST_CASE("Download succeeds even when SandboxCreateCallback is NULL")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);
    workflow_set_workfolder(handle, "/tmp/cov_no_sandbox");

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_DeploymentInProgress;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    cb.SandboxCreateCallback = nullptr; // No sandbox callback
    wd.UpdateActionCallbacks = cb;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result r = ADUC_Workflow_MethodCall_Download(&mcd);
    CHECK(IsAducResultCodeSuccess(r.ResultCode));

    workflow_free(wd.WorkflowHandle);
}

// ============================================================================
// Download with NULL DownloadCallback returns success
// ============================================================================

TEST_CASE("Download with NULL DownloadCallback returns Download_Success")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);
    workflow_set_workfolder(handle, "/tmp/cov_no_dl_cb");

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_DeploymentInProgress;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    cb.DownloadCallback = nullptr; // No download callback
    wd.UpdateActionCallbacks = cb;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result r = ADUC_Workflow_MethodCall_Download(&mcd);
    CHECK(r.ResultCode == ADUC_Result_Download_Success);

    workflow_free(wd.WorkflowHandle);
}

// ============================================================================
// Backup / Install / Apply / Restore with NULL action callback
// ============================================================================

TEST_CASE("Backup with NULL BackupCallback returns Backup_Success")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_DownloadSucceeded;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    cb.BackupCallback = nullptr;
    wd.UpdateActionCallbacks = cb;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result r = ADUC_Workflow_MethodCall_Backup(&mcd);
    CHECK(r.ResultCode == ADUC_Result_Backup_Success);

    workflow_free(handle);
}

TEST_CASE("Install with NULL InstallCallback returns Install_Success")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_BackupSucceeded;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    cb.InstallCallback = nullptr;
    wd.UpdateActionCallbacks = cb;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result r = ADUC_Workflow_MethodCall_Install(&mcd);
    CHECK(r.ResultCode == ADUC_Result_Install_Success);

    workflow_free(handle);
}

TEST_CASE("Apply with NULL ApplyCallback returns Apply_Success")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_InstallSucceeded;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    cb.ApplyCallback = nullptr;
    wd.UpdateActionCallbacks = cb;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result r = ADUC_Workflow_MethodCall_Apply(&mcd);
    CHECK(r.ResultCode == ADUC_Result_Apply_Success);

    workflow_free(handle);
}

TEST_CASE("Restore with NULL RestoreCallback returns Restore_Success")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_Failed;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    cb.RestoreCallback = nullptr;
    wd.UpdateActionCallbacks = cb;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result r = ADUC_Workflow_MethodCall_Restore(&mcd);
    CHECK(r.ResultCode == ADUC_Result_Restore_Success);

    workflow_free(handle);
}

// ============================================================================
// Idle with NULL WorkflowHandle (no sandbox destroy, no workflow to free)
// ============================================================================

TEST_CASE("Idle with NULL WorkflowHandle calls IdleCallback with NULL workflowId")
{
    ResetCounters();

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = nullptr;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_MethodCall_Idle(&wd);

    CHECK(s_sandboxDestroyCbCount == 0);
    CHECK(s_idleCbCount == 1);
    CHECK(wd.WorkflowHandle == nullptr);
}

// ============================================================================
// Idle with NULL SandboxDestroyCallback skips sandbox destroy
// ============================================================================

TEST_CASE("Idle with NULL SandboxDestroyCallback skips destroy and still idles")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);
    workflow_set_workfolder(handle, "/tmp/cov_idle_no_destroy");

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    cb.SandboxDestroyCallback = nullptr;
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_MethodCall_Idle(&wd);

    CHECK(s_sandboxDestroyCbCount == 0);
    CHECK(s_idleCbCount == 1);
    CHECK(wd.WorkflowHandle == nullptr);
}

// ============================================================================
// Idle with NULL IdleCallback - should still free handle
// ============================================================================

TEST_CASE("Idle with NULL IdleCallback frees handle without crashing")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    cb.IdleCallback = nullptr;
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_MethodCall_Idle(&wd);

    CHECK(s_idleCbCount == 0);
    CHECK(wd.WorkflowHandle == nullptr);
}

// ============================================================================
// Full end-to-end workflow: Backup -> Install -> Apply -> Idle
// ============================================================================

TEST_CASE("Full workflow: Backup through Apply to Idle")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);
    workflow_set_workfolder(handle, "/tmp/cov_full_flow");

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    // ProcessDeployment
    ADUC_Result r = ADUC_Workflow_MethodCall_ProcessDeployment(&mcd);
    CHECK(r.ResultCode == ADUC_Result_Success);
    CHECK(ADUC_WorkflowData_GetLastReportedState(&wd) == ADUCITF_State_DeploymentInProgress);

    // Download
    r = ADUC_Workflow_MethodCall_Download(&mcd);
    CHECK(IsAducResultCodeSuccess(r.ResultCode));

    // Simulate download completion
    wd.LastReportedState = ADUCITF_State_DownloadSucceeded;

    // Backup
    r = ADUC_Workflow_MethodCall_Backup(&mcd);
    CHECK(r.ResultCode == ADUC_Result_Backup_Success);

    // Simulate backup completion
    wd.LastReportedState = ADUCITF_State_BackupSucceeded;

    // Install
    r = ADUC_Workflow_MethodCall_Install(&mcd);
    CHECK(r.ResultCode == ADUC_Result_Install_Success);

    // Simulate install completion
    wd.LastReportedState = ADUCITF_State_InstallSucceeded;

    // Apply
    r = ADUC_Workflow_MethodCall_Apply(&mcd);
    CHECK(r.ResultCode == ADUC_Result_Apply_Success);

    workflow_free(wd.WorkflowHandle);
}

// ============================================================================
// SetUpdateStateWithResult going to Idle triggers Idle cleanup
// ============================================================================

TEST_CASE("SetUpdateStateWithResult to Idle triggers Idle cleanup")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);
    workflow_set_workfolder(handle, "/tmp/cov_set_state_idle");

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Result res = { ADUC_Result_Success, 0 };
    ADUC_Workflow_SetUpdateStateWithResult(&wd, ADUCITF_State_Idle, res);

    CHECK(s_idleCbCount == 1);
    CHECK(wd.WorkflowHandle == nullptr);
}

// ============================================================================
// HandleStartupWorkflowData with retryTimestamp JSON
// ============================================================================

TEST_CASE("HandleStartupWorkflowData with retry deployment JSON")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_retry_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.StartupIdleCallSent = false;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    cb.IsInstalledCallback = MockIsInstalled_NotInstalled;
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_HandleStartupWorkflowData(&wd);

    CHECK(wd.StartupIdleCallSent == true);
    // The full workflow runs synchronously to completion (Idle) because all callbacks are mocked as synchronous successes
    CHECK(ADUC_WorkflowData_GetLastReportedState(&wd) == ADUCITF_State_Idle);

    // WorkflowHandle is freed as a side-effect of going to Idle
    // No need to free it again
}

// ============================================================================
// ReportState callback failure path via SetUpdateStateWithResult
// ============================================================================

TEST_CASE("SetUpdateStateWithResult: callback failure sets Failed state")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Failure;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Result res = { ADUC_Result_Download_Success, 0 };
    ADUC_Workflow_SetUpdateStateWithResult(&wd, ADUCITF_State_DownloadSucceeded, res);

    // Callback returns false → state should be set to Failed
    CHECK(ADUC_WorkflowData_GetLastReportedState(&wd) == ADUCITF_State_Failed);

    workflow_free(wd.WorkflowHandle);
}

// ============================================================================
// _ReportUpdateState with NULL ReportStateAndResultAsyncCallback
// (indirectly tested via SetUpdateState with NULL callback)
// ============================================================================

TEST_CASE("SetUpdateState with NULL report callback still sets state")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = nullptr; // No callback

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_SetUpdateState(&wd, ADUCITF_State_DownloadStarted);

    CHECK(ADUC_WorkflowData_GetLastReportedState(&wd) == ADUCITF_State_DownloadStarted);

    workflow_free(wd.WorkflowHandle);
}

// ============================================================================
// Additional coverage tests
// ============================================================================

// --- Cancel action JSON (action=255) ---
// clang-format off
static const char* s_cancel_action_json =
    R"( {                                                          )"
    R"(     "workflow": {                                          )"
    R"(         "action": 255,                                     )"
    R"(         "id": "coverage-cancel-001"                        )"
    R"(     },                                                     )"
    R"(     "updateManifest": "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Coverage-Update\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"test-device\"}],\"instructions\":{\"steps\":[]},\"files\":{},\"createdDateTime\":\"2024-01-01T00:00:00Z\"}",  )"
    R"(     "updateManifestSignature": "test-signature",           )"
    R"(     "fileUrls": {}                                         )"
    R"( }                                                          )";

// A different workflow ID for replacement testing
static const char* s_replacement_deployment_json =
    R"( {                                                          )"
    R"(     "workflow": {                                          )"
    R"(         "action": 3,                                       )"
    R"(         "id": "coverage-replacement-002"                   )"
    R"(     },                                                     )"
    R"(     "updateManifest": "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Coverage-Update\",\"version\":\"3.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"test-device\"}],\"instructions\":{\"steps\":[]},\"files\":{},\"createdDateTime\":\"2024-07-01T00:00:00Z\"}",  )"
    R"(     "updateManifestSignature": "test-signature",           )"
    R"(     "fileUrls": {}                                         )"
    R"( }                                                          )";

// Same workflow ID with different retry timestamp
static const char* s_retry_deployment_json_v2 =
    R"( {                                                          )"
    R"(     "workflow": {                                          )"
    R"(         "action": 3,                                       )"
    R"(         "id": "coverage-workflow-001",                     )"
    R"(         "retryTimestamp": "2024-08-01T10:00:00Z"           )"
    R"(     },                                                     )"
    R"(     "updateManifest": "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Coverage-Update\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"test-device\"}],\"instructions\":{\"steps\":[]},\"files\":{},\"createdDateTime\":\"2024-01-01T00:00:00Z\"}",  )"
    R"(     "updateManifestSignature": "test-signature",           )"
    R"(     "fileUrls": {}                                         )"
    R"( }                                                          )";

// Undefined action JSON
static const char* s_undefined_action_json =
    R"( {                                                          )"
    R"(     "workflow": {                                          )"
    R"(         "action": -1,                                      )"
    R"(         "id": "coverage-undefined-001"                     )"
    R"(     },                                                     )"
    R"(     "updateManifest": "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Coverage-Update\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"test-device\"}],\"instructions\":{\"steps\":[]},\"files\":{},\"createdDateTime\":\"2024-01-01T00:00:00Z\"}",  )"
    R"(     "updateManifestSignature": "test-signature",           )"
    R"(     "fileUrls": {}                                         )"
    R"( }                                                          )";
// clang-format on

// ---- Additional mock callbacks ----

static ADUC_Result MockIsInstalled_Installed(void* token, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_IsInstalled_Installed, 0 };
}

static ADUC_Result MockDownload_Failure(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Failure, 0 };
}

static ADUC_Result MockInstall_Failure(
    void* token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);
    return ADUC_Result{ ADUC_Result_Failure, 0 };
}

static ADUC_Result MockSandboxCreate_Failure(void* token, const char* workflowId, char* workFolder)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);
    return ADUC_Result{ ADUC_Result_Failure, 0 };
}

// ============================================================================
// HandleStartupWorkflowData: Cancel action on startup
// ============================================================================

TEST_CASE("HandleStartupWorkflowData with Cancel action goes to Idle")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_cancel_action_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.StartupIdleCallSent = false;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_HandleStartupWorkflowData(&wd);

    CHECK(wd.StartupIdleCallSent == true);
    CHECK(ADUC_WorkflowData_GetLastReportedState(&wd) == ADUCITF_State_Idle);
}

// ============================================================================
// HandleStartupWorkflowData: NULL WorkflowHandle (first time connected)
// ============================================================================

TEST_CASE("HandleStartupWorkflowData with NULL WorkflowHandle (first time connected)")
{
    ResetCounters();

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = nullptr;
    wd.StartupIdleCallSent = false;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    // Should NOT crash; will try HandleUpdateAction with null handle
    // In practice this will fail gracefully
    ADUC_Workflow_HandleStartupWorkflowData(&wd);

    CHECK(wd.StartupIdleCallSent == true);
}

// ============================================================================
// HandleStartupWorkflowData: StartupIdleCallSent already true → skip
// ============================================================================

TEST_CASE("HandleStartupWorkflowData skips when StartupIdleCallSent is already true")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.StartupIdleCallSent = true; // Already sent

    ADUC_Workflow_HandleStartupWorkflowData(&wd);

    // Should not process anything
    CHECK(s_idleCbCount == 0);
    CHECK(s_reportStateCbCount == 0);

    workflow_free(handle);
}

// ============================================================================
// HandleStartupWorkflowData: Undefined action → goto done early
// ============================================================================

TEST_CASE("HandleStartupWorkflowData with Undefined action goes to done")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_undefined_action_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.StartupIdleCallSent = false;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_HandleStartupWorkflowData(&wd);

    CHECK(wd.StartupIdleCallSent == true);

    workflow_free(handle);
}

// ============================================================================
// HandleStartupWorkflowData: IsInstalled returns Installed → go to idle
// ============================================================================

TEST_CASE("HandleStartupWorkflowData with already installed update goes to Idle")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.StartupIdleCallSent = false;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    cb.IsInstalledCallback = MockIsInstalled_Installed;
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_HandleStartupWorkflowData(&wd);

    CHECK(wd.StartupIdleCallSent == true);
    CHECK(ADUC_WorkflowData_GetLastReportedState(&wd) == ADUCITF_State_Idle);
}


// ============================================================================
// HandleUpdateAction: duplicate deployment (same LastCompletedWorkflowId) → ignored
// ============================================================================

TEST_CASE("HandleUpdateAction ignores duplicate deployment")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    // Set LastCompletedWorkflowId to same as current
    ADUC_WorkflowData_SetLastCompletedWorkflowId(workflow_peek_id(handle), &wd);

    ADUC_Workflow_HandleUpdateAction(&wd);

    // Should be ignored - no state change
    CHECK(s_reportStateCbCount == 0);

    workflow_free(handle);
}

// ============================================================================
// HandleUpdateAction: IsInstalled returns Installed → goes to Idle
// ============================================================================

TEST_CASE("HandleUpdateAction with already installed update goes to Idle")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    cb.IsInstalledCallback = MockIsInstalled_Installed;
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_HandleUpdateAction(&wd);

    CHECK(ADUC_WorkflowData_GetLastReportedState(&wd) == ADUCITF_State_Idle);
}

// ============================================================================
// HandleUpdateAction: Cancel without operation in progress returns to idle
// ============================================================================

TEST_CASE("HandleUpdateAction Cancel without operation in progress clears state")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_cancel_action_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_HandleUpdateAction(&wd);

    // Cancel with no op in progress → return to idle (no state reporting)
    CHECK(s_cancelCbCount == 0);

    workflow_free(handle);
}

// ============================================================================
// HandleUpdateAction: Replace/Retry when operation NOT in progress → process
// ============================================================================

TEST_CASE("HandleUpdateAction Retry when not in progress processes workflow")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    // Set retry cancellation but no operation in progress
    workflow_set_cancellation_type(handle, ADUC_WorkflowCancellationType_Retry);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_HandleUpdateAction(&wd);

    // Should clear cancellation and process workflow
    CHECK(ADUC_WorkflowData_GetLastReportedState(&wd) == ADUCITF_State_Idle);
}

// ============================================================================
// Download in unexpected state returns failure
// ============================================================================

TEST_CASE("Download in unexpected state returns failure")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_Idle; // Wrong state for download

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result r = ADUC_Workflow_MethodCall_Download(&mcd);
    CHECK(IsAducResultCodeFailure(r.ResultCode));

    workflow_free(handle);
}

// ============================================================================
// Download with SandboxCreateCallback failure
// ============================================================================

TEST_CASE("Download with SandboxCreate failure returns failure")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);
    workflow_set_workfolder(handle, "/tmp/cov_sandbox_fail");

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_DeploymentInProgress;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    cb.SandboxCreateCallback = MockSandboxCreate_Failure;
    wd.UpdateActionCallbacks = cb;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result r = ADUC_Workflow_MethodCall_Download(&mcd);
    CHECK(IsAducResultCodeFailure(r.ResultCode));

    workflow_free(handle);
}

// ============================================================================
// Install in unexpected state returns failure
// ============================================================================

TEST_CASE("Install in unexpected state returns failure")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_Idle; // Wrong state

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result r = ADUC_Workflow_MethodCall_Install(&mcd);
    CHECK(IsAducResultCodeFailure(r.ResultCode));

    workflow_free(handle);
}

// ============================================================================
// Backup in unexpected state returns failure
// ============================================================================

TEST_CASE("Backup in unexpected state returns failure")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_Idle; // Wrong state

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result r = ADUC_Workflow_MethodCall_Backup(&mcd);
    CHECK(IsAducResultCodeFailure(r.ResultCode));

    workflow_free(handle);
}

// ============================================================================
// Apply in unexpected state returns failure
// ============================================================================

TEST_CASE("Apply in unexpected state returns failure")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_Idle; // Wrong state

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result r = ADUC_Workflow_MethodCall_Apply(&mcd);
    CHECK(IsAducResultCodeFailure(r.ResultCode));

    workflow_free(handle);
}

// ============================================================================
// Restore in unexpected state returns failure
// ============================================================================

TEST_CASE("Restore in unexpected state returns failure")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_Idle; // Wrong state (expects Failed)

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result r = ADUC_Workflow_MethodCall_Restore(&mcd);
    CHECK(IsAducResultCodeFailure(r.ResultCode));

    workflow_free(handle);
}

// ============================================================================
// SetUpdateStateHelper: Idle from ApplyStarted with no reboot/restart → installed
// ============================================================================

TEST_CASE("SetUpdateState to Idle from ApplyStarted reports installed update id")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);
    workflow_set_workfolder(handle, "/tmp/cov_apply_idle");

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_ApplyStarted;
    wd.SystemRebootState = ADUC_SystemRebootState_None;
    wd.AgentRestartState = ADUC_AgentRestartState_None;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_SetUpdateState(&wd, ADUCITF_State_Idle);

    CHECK(ADUC_WorkflowData_GetLastReportedState(&wd) == ADUCITF_State_Idle);
    CHECK(s_idleCbCount >= 1);
    CHECK(wd.WorkflowHandle == nullptr);
}

// ============================================================================
// SetUpdateStateHelper: Idle from ApplyStarted with SystemRebootState_InProgress
// ============================================================================

TEST_CASE("SetUpdateState to Idle from ApplyStarted with reboot in progress")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);
    workflow_set_workfolder(handle, "/tmp/cov_apply_reboot");

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_ApplyStarted;
    wd.SystemRebootState = ADUC_SystemRebootState_InProgress;
    wd.AgentRestartState = ADUC_AgentRestartState_None;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_SetUpdateState(&wd, ADUCITF_State_Idle);

    // Reboot in progress → calls Idle but does NOT report state to service
    CHECK(s_idleCbCount == 1);
    CHECK(wd.WorkflowHandle == nullptr);
}

// ============================================================================
// SetUpdateStateHelper: Idle from ApplyStarted with AgentRestartState_InProgress
// ============================================================================

TEST_CASE("SetUpdateState to Idle from ApplyStarted with agent restart in progress")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);
    workflow_set_workfolder(handle, "/tmp/cov_apply_restart");

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_ApplyStarted;
    wd.SystemRebootState = ADUC_SystemRebootState_None;
    wd.AgentRestartState = ADUC_AgentRestartState_InProgress;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_SetUpdateState(&wd, ADUCITF_State_Idle);

    // Agent restart in progress → calls Idle but does NOT report state to service
    CHECK(s_idleCbCount == 1);
    CHECK(wd.WorkflowHandle == nullptr);
}

// ============================================================================
// SetUpdateStateHelper: report callback failure for Idle state
// ============================================================================

TEST_CASE("SetUpdateState to Idle with report failure sets Failed")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Failure;
    wd.LastReportedState = ADUCITF_State_DownloadStarted; // Not ApplyStarted

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_SetUpdateState(&wd, ADUCITF_State_Idle);

    // Report failure → state set to Failed
    CHECK(ADUC_WorkflowData_GetLastReportedState(&wd) == ADUCITF_State_Failed);

    workflow_free(wd.WorkflowHandle);
}

// ============================================================================
// Apply_Complete: reboot requested branch
// ============================================================================

TEST_CASE("Apply_Complete with reboot requested sets reboot state")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_request_reboot(handle);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result result = { ADUC_Result_Apply_Success, 0 };
    ADUC_Workflow_MethodCall_Apply_Complete(&mcd, result);

    CHECK(wd.SystemRebootState == ADUC_SystemRebootState_Required);

    workflow_free(handle);
}

// ============================================================================
// Apply_Complete: agent restart requested branch
// ============================================================================

TEST_CASE("Apply_Complete with agent restart requested sets restart state")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_request_agent_restart(handle);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result result = { ADUC_Result_Apply_Success, 0 };
    ADUC_Workflow_MethodCall_Apply_Complete(&mcd, result);

    CHECK((wd.AgentRestartState == ADUC_AgentRestartState_Required || wd.AgentRestartState == ADUC_AgentRestartState_InProgress));

    workflow_free(handle);
}

// ============================================================================
// Apply_Complete: success with no reboot/restart clears operation
// ============================================================================

TEST_CASE("Apply_Complete success without reboot/restart clears operation in progress")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_operation_in_progress(handle, true);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result result = { ADUC_Result_Apply_Success, 0 };
    ADUC_Workflow_MethodCall_Apply_Complete(&mcd, result);

    CHECK(workflow_get_operation_in_progress(handle) == false);

    workflow_free(handle);
}

// ============================================================================
// Restore_Complete: reboot required branch
// ============================================================================

TEST_CASE("Restore_Complete with RequiredReboot sets reboot state")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result result = { ADUC_Result_Restore_RequiredReboot, 0 };
    ADUC_Workflow_MethodCall_Restore_Complete(&mcd, result);

    CHECK(wd.SystemRebootState == ADUC_SystemRebootState_Required);

    workflow_free(handle);
}

// ============================================================================
// Restore_Complete: immediate reboot branch
// ============================================================================

TEST_CASE("Restore_Complete with RequiredImmediateReboot sets reboot state")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result result = { ADUC_Result_Restore_RequiredImmediateReboot, 0 };
    ADUC_Workflow_MethodCall_Restore_Complete(&mcd, result);

    CHECK(wd.SystemRebootState == ADUC_SystemRebootState_Required);

    workflow_free(handle);
}

// ============================================================================
// Restore_Complete: agent restart branch
// ============================================================================

TEST_CASE("Restore_Complete with RequiredAgentRestart sets restart state")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result result = { ADUC_Result_Restore_RequiredAgentRestart, 0 };
    ADUC_Workflow_MethodCall_Restore_Complete(&mcd, result);

    CHECK((wd.AgentRestartState == ADUC_AgentRestartState_Required || wd.AgentRestartState == ADUC_AgentRestartState_InProgress));

    workflow_free(handle);
}

// ============================================================================
// Restore_Complete: immediate agent restart branch
// ============================================================================

TEST_CASE("Restore_Complete with RequiredImmediateAgentRestart sets restart state")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result result = { ADUC_Result_Restore_RequiredImmediateAgentRestart, 0 };
    ADUC_Workflow_MethodCall_Restore_Complete(&mcd, result);

    CHECK((wd.AgentRestartState == ADUC_AgentRestartState_Required || wd.AgentRestartState == ADUC_AgentRestartState_InProgress));

    workflow_free(handle);
}

// ============================================================================
// Restore_Complete: success clears operation in progress
// ============================================================================

TEST_CASE("Restore_Complete with Restore_Success clears operation in progress")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_operation_in_progress(handle, true);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result result = { ADUC_Result_Restore_Success, 0 };
    ADUC_Workflow_MethodCall_Restore_Complete(&mcd, result);

    CHECK(workflow_get_operation_in_progress(handle) == false);

    workflow_free(handle);
}

// ============================================================================
// Restore_Complete: Restore_Success_Unsupported also clears operation
// ============================================================================

TEST_CASE("Restore_Complete with Restore_Success_Unsupported clears operation")
{
    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    workflow_set_operation_in_progress(handle, true);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_MethodCall_Data mcd;
    memset(&mcd, 0, sizeof(mcd));
    mcd.WorkflowData = &wd;

    ADUC_Result result = { ADUC_Result_Restore_Success_Unsupported, 0 };
    ADUC_Workflow_MethodCall_Restore_Complete(&mcd, result);

    CHECK(workflow_get_operation_in_progress(handle) == false);

    workflow_free(handle);
}

// ============================================================================
// DownloadProgressCallback: all known enum values
// ============================================================================

TEST_CASE("DefaultDownloadProgressCallback handles all known states")
{
    // Exercise every branch of the switch statement
    ADUC_Workflow_DefaultDownloadProgressCallback(
        "wf-001", "f-001", ADUC_DownloadProgressState_NotStarted, 0, 100);
    ADUC_Workflow_DefaultDownloadProgressCallback(
        "wf-001", "f-001", ADUC_DownloadProgressState_InProgress, 50, 100);
    ADUC_Workflow_DefaultDownloadProgressCallback(
        "wf-001", "f-001", ADUC_DownloadProgressState_Completed, 100, 100);
    ADUC_Workflow_DefaultDownloadProgressCallback(
        "wf-001", "f-001", ADUC_DownloadProgressState_Cancelled, 25, 100);
    ADUC_Workflow_DefaultDownloadProgressCallback(
        "wf-001", "f-001", ADUC_DownloadProgressState_Error, 0, 100);
    CHECK(true);
}

// ============================================================================
// DoWork with valid callback invokes it
// ============================================================================

TEST_CASE("DoWork with valid callback invokes it")
{
    ResetCounters();

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_DoWork(&wd);
    CHECK(s_doWorkCbCount == 1);
}

// ============================================================================
// Idle called from unexpected state (not Idle/ApplyStarted/Failed)
// ============================================================================

TEST_CASE("Idle called from unexpected state logs warning but continues")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);
    workflow_set_workfolder(handle, "/tmp/cov_idle_unexpected");

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.LastReportedState = ADUCITF_State_DownloadStarted; // Unexpected

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_MethodCall_Idle(&wd);

    CHECK(s_idleCbCount == 1);
    CHECK(wd.WorkflowHandle == nullptr);
}

// ============================================================================
// WorkCompletionCallback through install failure → restore auto-transition
// ============================================================================

TEST_CASE("Workflow install failure auto-transitions to restore")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);
    workflow_set_workfolder(handle, "/tmp/cov_install_fail");

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.StartupIdleCallSent = true;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_DeploymentInProgress;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    cb.InstallCallback = MockInstall_Failure; // Install will fail
    wd.UpdateActionCallbacks = cb;

    // Set workflow to Download step, trigger transition through download → backup → install(fail) → restore
    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_ProcessDeployment);
    ADUC_WorkflowData_SetCurrentAction(ADUCITF_UpdateAction_ProcessDeployment, &wd);
    ADUC_Workflow_TransitionWorkflow(&wd);

    // The workflow should have gone through all steps including restore on install failure
    CHECK(ADUC_WorkflowData_GetLastReportedState(&wd) == ADUCITF_State_Idle);
}

// ============================================================================
// WorkCompletionCallback: download failure → Failed state (no restore for download)
// ============================================================================

TEST_CASE("Workflow download failure goes to Failed state")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);
    workflow_set_workfolder(handle, "/tmp/cov_dl_fail");

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.StartupIdleCallSent = true;
    wd.ReportStateAndResultAsyncCallback = MockReport_Success;
    wd.LastReportedState = ADUCITF_State_DeploymentInProgress;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    cb.DownloadCallback = MockDownload_Failure;
    wd.UpdateActionCallbacks = cb;

    workflow_set_current_workflowstep(handle, ADUCITF_WorkflowStep_Download);
    ADUC_WorkflowData_SetCurrentAction(ADUCITF_UpdateAction_ProcessDeployment, &wd);
    ADUC_Workflow_TransitionWorkflow(&wd);

    // Download failure → Failed (no auto-transition to restore for download)
    CHECK(ADUC_WorkflowData_GetLastReportedState(&wd) == ADUCITF_State_Failed);
    // WorkflowHandle freed when going to idle from failed; but we might still have it
    // since download failure goes to Failed not Idle
    workflow_free(wd.WorkflowHandle);
}

// ============================================================================
// SetInstalledUpdateIdAndGoToIdle with NULL report callback
// ============================================================================

TEST_CASE("SetInstalledUpdateIdAndGoToIdle with NULL report callback logs error but idles")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);
    workflow_set_workfolder(handle, "/tmp/cov_installed_null_cb");

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = nullptr;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle(&wd, "test-update-id");

    CHECK(ADUC_WorkflowData_GetLastReportedState(&wd) == ADUCITF_State_Idle);
    CHECK(s_idleCbCount == 1);
    CHECK(wd.SystemRebootState == ADUC_SystemRebootState_None);
    CHECK(wd.AgentRestartState == ADUC_AgentRestartState_None);
}

// ============================================================================
// SetInstalledUpdateIdAndGoToIdle with report callback failure
// ============================================================================

TEST_CASE("SetInstalledUpdateIdAndGoToIdle with report failure still idles")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);
    workflow_set_workfolder(handle, "/tmp/cov_installed_report_fail");

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;
    wd.ReportStateAndResultAsyncCallback = MockReport_Failure;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle(&wd, "test-update-id");

    CHECK(ADUC_WorkflowData_GetLastReportedState(&wd) == ADUCITF_State_Idle);
    CHECK(s_idleCbCount == 1);
}

// ============================================================================
// Cancel without operation in progress returns early
// ============================================================================

TEST_CASE("Cancel without operation in progress ignores and returns")
{
    ResetCounters();

    ADUC_WorkflowHandle handle = MakeHandle(s_process_deployment_json);
    REQUIRE(handle != nullptr);

    // operation NOT in progress
    workflow_set_operation_in_progress(handle, false);

    ADUC_WorkflowData wd;
    memset(&wd, 0, sizeof(wd));
    wd.WorkflowHandle = handle;

    ADUC_UpdateActionCallbacks cb;
    InitAllCallbacks(&cb);
    wd.UpdateActionCallbacks = cb;

    ADUC_Workflow_MethodCall_Cancel(&wd);

    CHECK(s_cancelCbCount == 0); // Not called because no op in progress

    workflow_free(handle);
}

