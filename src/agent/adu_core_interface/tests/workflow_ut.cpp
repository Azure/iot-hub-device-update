/**
 * @file adu_core_export_helpers_ut.cpp
 * @brief Unit tests for adu_core_export_helpers.h
 *
 * @copyright Copyright (c) Microsoft Corp.
 * Licensed under the MIT License.
 */
#include <atomic>
#include <catch2/catch.hpp>
#include <chrono>
#include <condition_variable>
#include <iothub_device_client.h>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <thread>

#include "aduc/adu_core_export_helpers.h"
#include "aduc/adu_core_interface.h"
#include "aduc/agent_workflow.h"
#include "aduc/c_utils.h"
#include "aduc/client_handle.h"
#include "aduc/content_handler.hpp"
#include "aduc/result.h"
#include "aduc/types/workflow.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_internal.h"
#include "aduc/workflow_utils.h"

// clang-format off
static const char* workflow_test_process_deployment =
    R"( {                       )"
    R"(     "workflow": {       )"
    R"(            "action": 3, )"
    R"(            "id": "action_bundle" )"
    R"(        },   )"
    R"(        "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"20.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/apt:1\",\"files\":[\"f483750ebb885d32c\"],\"handlerProperties\":{\"installedCriteria\":\"apt-update-tree-1.0\"}},{\"type\":\"reference\",\"detachedManifestFileId\":\"f222b9ffefaaac577\"}]},\"files\":{\"f483750ebb885d32c\":{\"fileName\":\"apt-manifest-tree-1.0.json\",\"sizeInBytes\":136,\"hashes\":{\"sha256\":\"Uk1vsEL/nT4btMngo0YSJjheOL2aqm6/EAFhzPb0rXs=\"}},\"f222b9ffefaaac577\":{\"fileName\":\"contoso.contoso-virtual-motors.1.1.updatemanifest.json\",\"sizeInBytes\":1031,\"hashes\":{\"sha256\":\"9Rnjw7ThZhGacOGn3uvvVq0ccQTHc/UFSL9khR2oKsc=\"}}},\"createdDateTime\":\"2022-01-27T13:45:05.8993329Z\"}",  )"
    R"(        "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJqSW12eGpsc2pqZ29JeUJuYThuZTk2d0RYYlVsU3N6eGFoM0NibkF6STFJPSJ9.PzpvU13h6VhN8VHXUTYKAlpDW5t3JaQ-gs895_Q10XshKPYpeZUtViXGHGC-aQSQAYPhhYV-lLia9niXzZz4Qs4ehwFLHJfkmKR8eRwWvoOgJtAY0IIUA_8SeShmoOc9cdpC35N3OeaM4hV9shxvvrphDib5sLpkrv3LQrt3DHvK_L2n0HsybC-pwS7MzaSUIYoU-fXwZo6x3z7IbSaSNwS0P-50qeV99Mc0AUSIvB26GjmjZ2gEH5R3YD9kp0DOrYvE5tIymVHPTqkmunv2OrjKu2UOhNj8Om3RoVzxIkVM89cVGb1u1yB2kxEmXogXPz64cKqQWm22tV-jalS4dAc_1p9A9sKzZ632HxnlavOBjTKDGFgM95gg8M5npXBP3QIvkwW3yervCukViRUKIm-ljpDmnBJsZTMx0uzTaAk5XgoCUCADuLLol8EXB-0V4m2w-6tV6kAzRiwkqw1PRrGqplf-gmfU7TuFlQ142-EZLU5rK_dAiQRXx-f7LxNH",  )"
    R"(        "fileUrls": {    )"
    R"(            "f483750ebb885d32c": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/e5cc19d5e9174c93ada35cc315f1fb1d/apt-manifest-tree-1.0.json",      )"
    R"(            "f222b9ffefaaac577": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/31c38c3340a84e38ae8d30ce340f4a49/contoso.contoso-virtual-motors.1.1.updatemanifest.json",  )"
    R"(            "f2c5d1f3b0295db0f": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/9ff068f7c2bf43eb9561da14a7cbcecd/motor-firmware-1.1.json",         )"
    R"(            "f13b5435aab7c18da": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/c02058a476a242d7bc0e3c576c180051/contoso-motor-installscript.sh"   )"
    R"(        }    )"
    R"( } )";

EXTERN_C_BEGIN
// fwd-decl to completion signal handler fn.
void ADUC_Workflow_WorkCompletionCallback(const void* workCompletionToken, ADUC_Result result, bool isAsync);
EXTERN_C_END

// Note: g_iotHubClientHandleForADUComponent declared in adu_core_intefaace.h

//
// Test Helpers
//

static std::condition_variable workCompletionCallbackCV;

static bool s_workflowComplete = false;
static std::string s_expectedWorkflowIdWhenIdle;

#define ADUC_ClientHandle_Invalid (-1)

static unsigned int s_mockWorkCompletionCallbackCallCount = 0;

static IdleCallbackFunc s_platform_idle_callback = nullptr;

static void Mock_Idle_Callback(ADUC_Token token, const char* workflowId)
{
    CHECK(token != nullptr);
    CHECK(workflowId != nullptr);
    CHECK(strcmp(workflowId, "action_bundle") == 0);

    // call the original update callback
    REQUIRE(s_platform_idle_callback != nullptr);
    (*s_platform_idle_callback)(token, workflowId);

    // notify now so that test can cleanup
    workCompletionCallbackCV.notify_all();
}

extern "C"
{
    void Mock_DownloadProgressCallback(
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

                workCompletionCallbackCV.notify_all();
                break;

            default:
                workCompletionCallbackCV.notify_all();
        }

        ++s_mockWorkCompletionCallbackCallCount;

        // Call the normal work completion callback to continue the workflow processing
        ADUC_Workflow_WorkCompletionCallback(workCompletionToken, result, isAsync);
    }
}

static ADUC_ResultCode s_download_result_code = ADUC_Result_Download_Success;
static ADUC_ResultCode s_install_result_code = ADUC_Result_Install_Success;
static ADUC_ResultCode s_apply_result_code = ADUC_Result_Apply_Success;
static ADUC_ResultCode s_cancel_result_code = ADUC_Result_Cancel_Success;
static ADUC_ResultCode s_isInstalled_result_code = ADUC_Result_IsInstalled_NotInstalled;

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

    ADUC_Result Download(const ADUC_WorkflowData* workflowData) override
    {
        UNREFERENCED_PARAMETER(workflowData);
        ADUC_Result result = { s_download_result_code };
        return result;
    }

    ADUC_Result Install(const ADUC_WorkflowData* workflowData) override
    {
        UNREFERENCED_PARAMETER(workflowData);
        ADUC_Result result = { s_install_result_code };
        return result;
    }

    ADUC_Result Apply(const ADUC_WorkflowData* workflowData) override
    {
        UNREFERENCED_PARAMETER(workflowData);
        ADUC_Result result = { s_apply_result_code };
        return result;
    }

    ADUC_Result Cancel(const ADUC_WorkflowData* workflowData) override
    {
        UNREFERENCED_PARAMETER(workflowData);
        ADUC_Result result = { s_cancel_result_code };
        return result;
    }

    ADUC_Result IsInstalled(const ADUC_WorkflowData* workflowData) override
    {
        UNREFERENCED_PARAMETER(workflowData);
        ADUC_Result result = { s_isInstalled_result_code };
        return result;
    }
};

// NOLINTNEXTLINE(readability-non-const-parameter)
static ADUC_Result Mock_SandboxCreateCallback(ADUC_Token token, const char* workflowId, char* workFolder)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);

    ADUC_Result result = { ADUC_Result_Success };
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

class TestCaseFixture
{
public:
    TestCaseFixture()
    {
        m_previousDeviceHandle = g_iotHubClientHandleForADUComponent;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        g_iotHubClientHandleForADUComponent = reinterpret_cast<ADUC_ClientHandle>(ADUC_ClientHandle_Invalid);
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
    ADUC_ClientHandle m_previousDeviceHandle;
};

static IOTHUB_CLIENT_RESULT mockClientHandle_SendReportedState(
    ADUC_CLIENT_HANDLE_TYPE deviceHandle,
    const unsigned char* reportedState,
    size_t reportedStateLen,
    IOTHUB_CLIENT_REPORTED_STATE_CALLBACK_TYPE reportedStateCallback,
    void* userContextCallback)
{
    UNREFERENCED_PARAMETER(deviceHandle);
    UNREFERENCED_PARAMETER(reportedState);
    UNREFERENCED_PARAMETER(reportedStateLen);
    UNREFERENCED_PARAMETER(reportedStateCallback);
    UNREFERENCED_PARAMETER(userContextCallback);
    return IOTHUB_CLIENT_OK;
}

// This test exercises the happy path for the entire agent-orchestrated workflow
// via HandlePropertyUpdate, but with mocked
// ADUC_Workflow_WorkCompletionCallback and mocked ContentHandler layer.
// It does not depend on simulator platform / simulator content handler and
// exercises the platform layer as well, including the async worker threads for
// the individual operations (i.e. download, install, apply).
TEST_CASE_METHOD(TestCaseFixture, "Process Workflow E2E Functional")
{
    Reset_Mocks_State();
    s_expectedWorkflowIdWhenIdle = "action_bundle";

    std::mutex workCompletionCallbackMTX;
    std::unique_lock<std::mutex> lock(workCompletionCallbackMTX);

    MockContentHandler mockContentHandler;

    ADUC_WorkflowData workflowData{};

    // set test overrides
    ADUC_TestOverride_Hooks hooks{};
    hooks.WorkCompletionCallbackFunc_TestOverride = Mock_WorkCompletionCallback;
    hooks.ContentHandler_TestOverride = &mockContentHandler;
    hooks.ClientHandle_SendReportedStateFunc_TestOverride = (void*)mockClientHandle_SendReportedState; // NOLINT
    workflowData.TestOverrides = &hooks;

    ADUC_Result result =
        ADUC_MethodCall_Register(&(workflowData.UpdateActionCallbacks), 0 /*argc*/, nullptr /*argv*/);

    workflowData.UpdateActionCallbacks.SandboxCreateCallback = Mock_SandboxCreateCallback;
    workflowData.UpdateActionCallbacks.SandboxDestroyCallback = Mock_SandboxDestroyCallback;
    workflowData.UpdateActionCallbacks.IdleCallback = Mock_IdleCallback;

    workflowData.DownloadProgressCallback = Mock_DownloadProgressCallback;
    workflowData.ReportStateAndResultAsyncCallback = AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync;
    workflowData.LastReportedState = ADUCITF_State_Idle;

    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    REQUIRE(result.ExtendedResultCode == 0);

    // simulating non-startup processing of twin
    workflowData.WorkflowHandle = nullptr;
    workflowData.StartupIdleCallSent = true;
    ADUC_Workflow_HandlePropertyUpdate(&workflowData, (const unsigned char*)workflow_test_process_deployment, false /* forceDeferral */); // NOLINT

    // Wait for entire workflow to complete
    workCompletionCallbackCV.wait(lock);

    // Wait again for it to go to Idle so we don't trash the workflowData by going out of scope
    workCompletionCallbackCV.wait(lock);

    wait_for_workflow_complete();

    ADUC_MethodCall_Unregister(&workflowData.UpdateActionCallbacks);
}
