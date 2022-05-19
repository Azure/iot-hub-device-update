/**
 * @file adu_core_export_helpers_ut.cpp
 * @brief Unit tests for adu_core_export_helpers.h
 *
 * @copyright Copyright (c) Microsoft Corp.
 * Licensed under the MIT License.
 */
#include <atomic>
#include <catch2/catch.hpp>
using Catch::Matchers::Equals;
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <sys/stat.h> // mkdir
#include <thread>

#include "aduc/adu_core_export_helpers.h"
#include "aduc/adu_core_interface.h"
#include "aduc/agent_workflow.h"
#include "aduc/c_utils.h"
#include "aduc/content_handler.hpp"
#include "aduc/result.h"
#include "aduc/types/workflow.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_internal.h"
#include "aduc/workflow_utils.h"
#include "workflow_test_utils.h"

// clang-format off

EXTERN_C_BEGIN
// fwd-decl to completion signal handler fn.
void ADUC_Workflow_WorkCompletionCallback(const void* workCompletionToken, ADUC_Result result, bool isAsync);
EXTERN_C_END

// Note: g_iotHubClientHandleForADUComponent declared in adu_core_intefaace.h

//
// Test Helpers
//

static std::mutex cv_mutex;
static std::condition_variable workCompletionCallbackCV;

static bool s_workflowComplete = false;
static std::string s_expectedWorkflowIdWhenIdle;

#define ADUC_ClientHandle_Invalid (-1)

static unsigned s_mockWorkCompletionCallbackCallCount = 0;

static IdleCallbackFunc s_platform_idle_callback = nullptr;

static ADUC_ResultCode s_download_result_code = ADUC_Result_Download_Success;
static ADUC_ResultCode s_install_result_code = ADUC_Result_Install_Success;
static ADUC_ResultCode s_apply_result_code = ADUC_Result_Apply_Success;
static ADUC_ResultCode s_cancel_result_code = ADUC_Result_Cancel_Success;
static ADUC_ResultCode s_isInstalled_result_code = ADUC_Result_IsInstalled_NotInstalled;
static int s_reboot_system_return_code = 0;
static unsigned s_Mock_RebootSystem_call_count = 0;


bool s_workflowBeforeRebootIsDone = false;
bool s_idleDone = false;

static void Mock_Idle_Callback(ADUC_Token token, const char* workflowId)
{
    CHECK(token != nullptr);
    CHECK(workflowId != nullptr);
    CHECK(strcmp(workflowId, "e99c69ca-3188-43a3-80af-310616c7751d") == 0);

    // call the original update callback
    REQUIRE(s_platform_idle_callback != nullptr);
    (*s_platform_idle_callback)(token, workflowId);

    // notify now so that test can cleanup
    {
        std::unique_lock<std::mutex> lock(cv_mutex);

        s_idleDone = true; // update condition

        // need to manually unlock before notifying to prevent waking wait
        // thread to only block again because lock was already taken
        lock.unlock();
        workCompletionCallbackCV.notify_one();
    }
}

extern "C"
{
    static void Mock_DownloadProgressCallback(
        const char* /*workflowId*/,
        const char* /*fileId*/,
        ADUC_DownloadProgressState /*state*/,
        uint64_t /*bytesTransferred*/,
        uint64_t /*bytesTotal*/)
    {
    }

    static void Mock_WorkCompletionCallback(const void* workCompletionToken, ADUC_Result result, bool isAsync)
    {
        CHECK(workCompletionToken != nullptr);
        CHECK(IsAducResultCodeSuccess(result.ResultCode));
        CHECK(result.ExtendedResultCode == 0);

        auto methodCallData = (ADUC_MethodCall_Data*)workCompletionToken; // NOLINT
        auto workflowData = methodCallData->WorkflowData;

        switch (s_mockWorkCompletionCallbackCallCount)
        {
            case 0:
                //
                // Process Deployment
                //
                REQUIRE_FALSE(isAsync);
                REQUIRE(ADUC_WorkflowData_GetLastReportedState(workflowData) == ADUCITF_State_Idle);
                REQUIRE(ADUC_WorkflowData_GetCurrentAction(workflowData) == ADUCITF_UpdateAction_ProcessDeployment);
                REQUIRE(workflow_get_current_workflowstep(workflowData->WorkflowHandle) == ADUCITF_WorkflowStep_ProcessDeployment);
                REQUIRE(workflow_get_operation_in_progress(workflowData->WorkflowHandle) == true);
                REQUIRE(workflow_get_operation_cancel_requested(workflowData->WorkflowHandle) == false);
                break;

            case 1:
                //
                // Download
                //
                REQUIRE(isAsync);
                REQUIRE(ADUC_WorkflowData_GetLastReportedState(workflowData) == ADUCITF_State_DownloadStarted);
                REQUIRE(ADUC_WorkflowData_GetCurrentAction(workflowData) == ADUCITF_UpdateAction_ProcessDeployment);
                REQUIRE(workflow_get_current_workflowstep(workflowData->WorkflowHandle) == ADUCITF_WorkflowStep_Download);

                REQUIRE(workflowData->IsRegistered == false);
                REQUIRE(workflow_get_operation_in_progress(workflowData->WorkflowHandle) == true);
                REQUIRE(workflow_get_operation_cancel_requested(workflowData->WorkflowHandle) == false);
                break;

            case 2:
                //
                // Install
                //
                REQUIRE(isAsync);
                REQUIRE(ADUC_WorkflowData_GetLastReportedState(workflowData) == ADUCITF_State_InstallStarted);
                REQUIRE(ADUC_WorkflowData_GetCurrentAction(workflowData) == ADUCITF_UpdateAction_ProcessDeployment);
                REQUIRE(workflow_get_current_workflowstep(workflowData->WorkflowHandle) == ADUCITF_WorkflowStep_Install);

                REQUIRE(workflowData->IsRegistered == false);
                REQUIRE(workflow_get_operation_in_progress(workflowData->WorkflowHandle) == true);
                REQUIRE(workflow_get_operation_cancel_requested(workflowData->WorkflowHandle) == false);
                break;

            case 3:
                //
                // Apply
                //
                REQUIRE(isAsync);
                REQUIRE(ADUC_WorkflowData_GetLastReportedState(workflowData) == ADUCITF_State_ApplyStarted);
                REQUIRE(ADUC_WorkflowData_GetCurrentAction(workflowData) == ADUCITF_UpdateAction_ProcessDeployment);
                REQUIRE(workflow_get_current_workflowstep(workflowData->WorkflowHandle) == ADUCITF_WorkflowStep_Apply);

                REQUIRE(workflowData->IsRegistered == false);
                REQUIRE(workflow_get_operation_in_progress(workflowData->WorkflowHandle) == true);
                REQUIRE(workflow_get_operation_cancel_requested(workflowData->WorkflowHandle) == false);

                s_platform_idle_callback = workflowData->UpdateActionCallbacks.IdleCallback;
                workflowData->UpdateActionCallbacks.IdleCallback = Mock_Idle_Callback;

                {
                    std::unique_lock<std::mutex> lock(cv_mutex);

                    s_workflowBeforeRebootIsDone = true; // update condition

                    // need to manually unlock before notifying to prevent waking wait
                    // thread to only block again because lock was already taken
                    lock.unlock();
                    workCompletionCallbackCV.notify_one();
                }
                break;

            default:
                REQUIRE(false);
                break;
        }

        ++s_mockWorkCompletionCallbackCallCount;

        // Call the normal work completion callback to continue the workflow processing
        ADUC_Workflow_WorkCompletionCallback(workCompletionToken, result, isAsync);
    }
}

// Mock the content handler so that these tests do not require the simulator platform or simulator content handler.
// A pointer to mock content handler instance will be set in the workflowData in the below tests.
class MockContentHandler : public ContentHandler
{
public:
    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    MockContentHandler(const MockContentHandler&) = delete;
    MockContentHandler& operator=(const MockContentHandler&) = delete;
    MockContentHandler(MockContentHandler&&) = delete;
    MockContentHandler& operator=(MockContentHandler&&) = delete;

    MockContentHandler() = default;
    ~MockContentHandler() override = default;

    ADUC_Result Download(const ADUC_WorkflowData* workflowData) override {
        UNREFERENCED_PARAMETER(workflowData);

        ADUC_Result result = { s_download_result_code, 0 };
        return result;
    }

    ADUC_Result Install(const ADUC_WorkflowData* workflowData) override {
        UNREFERENCED_PARAMETER(workflowData);

        ADUC_Result result = { s_install_result_code, 0 };

        switch (result.ResultCode)
        {
            case ADUC_Result_Install_RequiredImmediateAgentRestart:
                workflow_request_immediate_agent_restart(workflowData->WorkflowHandle);
                break;
            case ADUC_Result_Install_RequiredAgentRestart:
                workflow_request_agent_restart(workflowData->WorkflowHandle);
                break;
            case ADUC_Result_Install_RequiredImmediateReboot:
                workflow_request_immediate_reboot(workflowData->WorkflowHandle);
                break;
            case ADUC_Result_Install_RequiredReboot:
                workflow_request_reboot(workflowData->WorkflowHandle);
                break;
        }

        return result;
    }

    ADUC_Result Apply(const ADUC_WorkflowData* workflowData) override {
        UNREFERENCED_PARAMETER(workflowData);

        ADUC_Result result = { s_apply_result_code, 0 };

        switch (result.ResultCode)
        {
            case ADUC_Result_Apply_RequiredImmediateAgentRestart:
                workflow_request_immediate_agent_restart(workflowData->WorkflowHandle);
                break;
            case ADUC_Result_Apply_RequiredAgentRestart:
                workflow_request_agent_restart(workflowData->WorkflowHandle);
                break;
            case ADUC_Result_Apply_RequiredImmediateReboot:
                workflow_request_immediate_reboot(workflowData->WorkflowHandle);
                break;
            case ADUC_Result_Apply_RequiredReboot:
                workflow_request_reboot(workflowData->WorkflowHandle);
                break;
        }

        return result;
    }

    ADUC_Result Cancel(const ADUC_WorkflowData* workflowData) override {
        UNREFERENCED_PARAMETER(workflowData);

        ADUC_Result result = { s_cancel_result_code, 0 };
        return result;
    }

    ADUC_Result IsInstalled(const ADUC_WorkflowData* workflowData) override {
        UNREFERENCED_PARAMETER(workflowData);

        ADUC_Result result = { s_isInstalled_result_code, 0 };
        return result;
    }
};

class ADUC_Test_ReportPropertyAsyncValues
{
public:
    void
    set(void* deviceHandleIn,
        const unsigned char* reportedStateIn,
        size_t reportedStateLenIn,
        IOTHUB_CLIENT_REPORTED_STATE_CALLBACK_TYPE reportedStateCallbackIn,
        void* userContextCallbackIn)
    {
        deviceHandle = deviceHandleIn;

        // we interpret the octets to be bytes of a string as it is a json string (utf-8 or ascii is fine).
        // copy the bytes into the std::string wholesale which is bitwise correct.
        std::string reportedState;
        reportedState.clear();
        reportedState.reserve(reportedStateLenIn);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        std::for_each(reportedStateIn, reportedStateIn + reportedStateLenIn, [&](unsigned char c) {
            reportedState.push_back(c);
        });

        reportedStates.push_back(reportedState);

        reportedStateCallback = reportedStateCallbackIn;
        userContextCallback = userContextCallbackIn;
    }

    std::vector<std::string> reportedStates;
    void* deviceHandle{ nullptr };
    IOTHUB_CLIENT_REPORTED_STATE_CALLBACK_TYPE reportedStateCallback{ nullptr };
    const void* userContextCallback{ nullptr };
} s_SendReportedStateValues;

// NOLINTNEXTLINE(readability-non-const-parameter)
static ADUC_Result Mock_SandboxCreateCallback(ADUC_Token token, const char* workflowId, char* workFolder)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);

    ADUC_Result result = { ADUC_Result_Success, 0 };
    return result;
}

static void Mock_SandboxDestroyCallback(ADUC_Token token, const char* workflowId, const char* workFolder)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);
}

static void Mock_IdleCallback(ADUC_Token token, const char* workflowId)
{
    UNREFERENCED_PARAMETER(token);
    CHECK(s_expectedWorkflowIdWhenIdle == workflowId);
    s_workflowComplete = true;
}

static ADUC_Result Mock_IsInstalledCallback(ADUC_Token token, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowData);

    ADUC_Result result = { ADUC_Result_IsInstalled_Installed, 0 };
    return result;
}

static int Mock_RebootSystem()
{
    ++s_Mock_RebootSystem_call_count;
    return s_reboot_system_return_code;
}

static IOTHUB_CLIENT_RESULT_TYPE mockClientHandle_SendReportedState(
    ADUC_CLIENT_HANDLE_TYPE deviceHandle,
    const unsigned char* reportedState,
    size_t reportedStateLen,
    IOTHUB_CLIENT_REPORTED_STATE_CALLBACK_TYPE reportedStateCallback,
    void* userContextCallback)
{
    s_SendReportedStateValues.set(
        deviceHandle, reportedState, reportedStateLen, reportedStateCallback, userContextCallback);

    return ADUC_IOTHUB_CLIENT_OK;
}

static void wait_for_workflow_complete()
{
    const unsigned MaxIterations = 100;
    const unsigned SleepIntervalInMs = 10;

    unsigned count = 0;
    while (count++ < MaxIterations && !s_workflowComplete)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(SleepIntervalInMs));
    }
    CHECK(s_workflowComplete);
}

static void Reset_Mocks_State()
{
    s_mockWorkCompletionCallbackCallCount = 0;
    s_workflowComplete = false;
    s_expectedWorkflowIdWhenIdle = "";

    s_download_result_code = ADUC_Result_Download_Success;
    s_install_result_code = ADUC_Result_Install_Success;
    s_apply_result_code = ADUC_Result_Apply_Success;
    s_cancel_result_code = ADUC_Result_Cancel_Success;
    s_isInstalled_result_code = ADUC_Result_IsInstalled_NotInstalled;

    s_reboot_system_return_code = 0;
    s_Mock_RebootSystem_call_count = 0;

    s_workflowBeforeRebootIsDone = false;
    s_idleDone = false;

    s_SendReportedStateValues.reportedStates.clear();
}

class TestCaseFixture
{
public:
    TestCaseFixture()
    {
        m_previousDeviceHandle = g_iotHubClientHandleForADUComponent;
        g_iotHubClientHandleForADUComponent = (void*)ADUC_ClientHandle_Invalid; // NOLINT
    }

    ~TestCaseFixture()
    {
        g_iotHubClientHandleForADUComponent = m_previousDeviceHandle;
    }

    TestCaseFixture(const TestCaseFixture&) = delete;
    TestCaseFixture& operator=(const TestCaseFixture&) = delete;
    TestCaseFixture(TestCaseFixture&&) = delete;
    TestCaseFixture& operator=(TestCaseFixture&&) = delete;

private:
    void* m_previousDeviceHandle;
};

class WorkflowRebootManagedWorkflowData
{
    ADUC_WorkflowData m_workflowData{};

public:
    WorkflowRebootManagedWorkflowData()
    {
        //
        // Setup test hooks
        //

        auto hooks = (ADUC_TestOverride_Hooks*)malloc(sizeof(ADUC_TestOverride_Hooks)); // NOLINT
        memset(hooks, 0, sizeof(*hooks));

        // Intercept the operation function completion callbacks to make assertions and
        // then pass thru to ADUC_Workflow_WorkCompletionCallback to continue workflow processing.
        hooks->WorkCompletionCallbackFunc_TestOverride = Mock_WorkCompletionCallback;

        // Use a mock content handler
        hooks->ContentHandler_TestOverride = new MockContentHandler;

        // Don't actually reboot the system
        hooks->RebootSystemFunc_TestOverride = Mock_RebootSystem;

        // Mock the low level actual client reporting so we can verify the client reporting string
        hooks->ClientHandle_SendReportedStateFunc_TestOverride = (void*)mockClientHandle_SendReportedState; // NOLINT

        m_workflowData.TestOverrides = hooks;

        //
        // Setup UpdateActionCallbacks
        //

        ADUC_Result result = ADUC_MethodCall_Register(&(m_workflowData.UpdateActionCallbacks), 0 /*argc*/, nullptr /*argv*/);
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
        REQUIRE(result.ExtendedResultCode == 0);
        m_workflowData.IsRegistered = false; // we will cleanup in dtor instead of AzureDeviceUpdateCoreInterface_Destroy

        m_workflowData.UpdateActionCallbacks.SandboxCreateCallback = Mock_SandboxCreateCallback;
        m_workflowData.UpdateActionCallbacks.SandboxDestroyCallback = Mock_SandboxDestroyCallback;
        m_workflowData.UpdateActionCallbacks.IdleCallback = Mock_IdleCallback;

        // setup other workflow state
        m_workflowData.DownloadProgressCallback = Mock_DownloadProgressCallback;
        m_workflowData.ReportStateAndResultAsyncCallback = AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync;
        m_workflowData.LastReportedState = ADUCITF_State_Idle;
    }

    ~WorkflowRebootManagedWorkflowData()
    {
        free(m_workflowData.TestOverrides->ContentHandler_TestOverride); // NOLINT
        delete m_workflowData.TestOverrides;
        m_workflowData.TestOverrides = nullptr;

        ADUC_MethodCall_Unregister(&m_workflowData.UpdateActionCallbacks);
    }

    ADUC_WorkflowData& GetWorkflowData()
    {
        return m_workflowData;
    }

    WorkflowRebootManagedWorkflowData(const WorkflowRebootManagedWorkflowData&) = delete;
    WorkflowRebootManagedWorkflowData& operator=(const WorkflowRebootManagedWorkflowData&) = delete;
    WorkflowRebootManagedWorkflowData(WorkflowRebootManagedWorkflowData&&) = delete;
    WorkflowRebootManagedWorkflowData& operator=(WorkflowRebootManagedWorkflowData&&) = delete;
};

// This test processes a workflow that requires reboot during Apply phase.
// The actual reboot action is mocked and startup is simulated afterwards by
// calling ADUC_Workflow_HandleStartupWorkflowData and then HandlePropertyUpdate.
// It asserts that proper reporting occurs after the reboot.
// The mock content handler above controls the ResultCode of Apply phase
// to result in reboot.
TEST_CASE_METHOD(TestCaseFixture, "Process Workflow Apply - Reboot Success")
{
    Reset_Mocks_State();

    const mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
    if (::mkdir("/tmp/adu", mode) == -1)
    {
        REQUIRE(errno == EEXIST);
    }

    if (::mkdir("/tmp/adu/workflow_reboot_ut", mode) == -1)
    {
        REQUIRE(errno == EEXIST);
    }

    s_expectedWorkflowIdWhenIdle = "e99c69ca-3188-43a3-80af-310616c7751d";

    std::mutex workCompletionCallbackMTX;
    std::unique_lock<std::mutex> lock(workCompletionCallbackMTX);

    WorkflowRebootManagedWorkflowData managedWorkflowDataBeforeReboot;
    auto workflowData = managedWorkflowDataBeforeReboot.GetWorkflowData();

    //
    // Setup reboot
    //
    s_apply_result_code = ADUC_Result_Apply_RequiredImmediateReboot; // require reboot during Apply
    s_reboot_system_return_code = 0; // Reboot operation will succeed

    // Initiate workflow processing due to PnP property change
    workflowData.WorkflowHandle = nullptr;
    workflowData.StartupIdleCallSent = true;

    std::string workflow_test_process_deployment = slurpTextFile(std::string{ ADUC_TEST_DATA_FOLDER } + "/workflow_reboot/updateActionForActionBundle.json");
    ADUC_Workflow_HandlePropertyUpdate(&workflowData, reinterpret_cast<const unsigned char*>(workflow_test_process_deployment.c_str()), false /* forceDeferral */); // NOLINT

    {
        std::unique_lock<std::mutex> lock(cv_mutex);
        workCompletionCallbackCV.wait(lock, [] { return s_workflowBeforeRebootIsDone; });
    }

    // Wait again for it to go to Idle
    {
        std::unique_lock<std::mutex> lock(cv_mutex);
        workCompletionCallbackCV.wait(lock, [] { return s_idleDone; });
    }

    wait_for_workflow_complete();

    // Assert that reboot occurred
    CHECK(s_Mock_RebootSystem_call_count == 1);
    CHECK(workflowData.SystemRebootState == ADUC_SystemRebootState_InProgress);

    //
    // Simulate post reboot after this line
    //

    Reset_Mocks_State();
    s_expectedWorkflowIdWhenIdle = "e99c69ca-3188-43a3-80af-310616c7751d";

    // This simulates when workflowdata is created when adu interface has just connected
    WorkflowRebootManagedWorkflowData managedStartupWorkflowDataAfterReboot;
    auto startupWorkflowDataAfterReboot = managedStartupWorkflowDataAfterReboot.GetWorkflowData();

    // After reboot, have mock content handler report the update was installed successfully.
    startupWorkflowDataAfterReboot.UpdateActionCallbacks.IsInstalledCallback = Mock_IsInstalledCallback;

    // Do Startup after reboot now.
    // Call HandleStartupWorkflowData with NULL workflowHandle and
    // then call HandlePropertyUpdate with latest twin JSON.
    // Ensure that was in progress properly when it goes to idle
    ADUC_Workflow_HandleStartupWorkflowData(&startupWorkflowDataAfterReboot);
    ADUC_Workflow_HandlePropertyUpdate(&startupWorkflowDataAfterReboot, reinterpret_cast<const unsigned char*>(workflow_test_process_deployment.c_str()), false /* forceDeferral */);

    CHECK(s_SendReportedStateValues.reportedStates.size() == 1);

    std::string expectedClientReportingString = slurpTextFile(std::string{ ADUC_TEST_DATA_FOLDER } + "/workflow_reboot/expectedClientReportingStringAfterReboot.json");
    JSON_Value* value = json_parse_string((s_SendReportedStateValues.reportedStates[0]).c_str());
    REQUIRE(value != nullptr);

    std::string actualClientReportingString_formatted = json_serialize_to_string_pretty(value);
    REQUIRE_THAT(actualClientReportingString_formatted + "\n", Equals(expectedClientReportingString));

    REQUIRE_THAT(startupWorkflowDataAfterReboot.LastCompletedWorkflowId, Equals("e99c69ca-3188-43a3-80af-310616c7751d"));

    wait_for_workflow_complete();

    // Now simulate a duplicate workflow request due to token expiry connection refresh
    s_SendReportedStateValues.reportedStates.clear();
    ADUC_Workflow_HandlePropertyUpdate(&startupWorkflowDataAfterReboot, reinterpret_cast<const unsigned char*>(workflow_test_process_deployment.c_str()), false /* forceDeferral */);
    CHECK(s_SendReportedStateValues.reportedStates.empty()); // did not do a duplicate report but ignored it

    wait_for_workflow_complete();
}
