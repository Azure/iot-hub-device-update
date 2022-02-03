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
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_internal.h"
#include "aduc/workflow_utils.h"

extern ADUC_ClientHandle g_iotHubClientHandleForADUComponent;

//NOLINTNEXTLINE(performance-unnecessary-value-param)
static ADUC_Workflow* get_workflow_from_test_data(const std::string path_under_testdata_folder)
{
    auto wf = (ADUC_Workflow*)malloc(sizeof(ADUC_Workflow)); // NOLINT
    REQUIRE(wf != nullptr);

    ADUC_Result result = workflow_init_from_file(
        (std::string{ ADUC_TEST_DATA_FOLDER } + "/" + path_under_testdata_folder).c_str(),
        true,
        (void**)&wf); // NOLINT

    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    REQUIRE(wf != nullptr);
    return wf;
}

class AduCoreExportHelpersTestCaseFixture
{
public:
    AduCoreExportHelpersTestCaseFixture()
    {
        const ADUC_Result result{ ADUC_MethodCall_Register(&m_updateActionCallbacks, 0 /*argc*/, nullptr /*argv*/) };
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
        REQUIRE(result.ExtendedResultCode == 0);

        m_wf = get_workflow_from_test_data("adu_core_export_helpers/updateManifest.json");
        m_workflowData.WorkflowHandle = m_wf;
        m_workflowData.UpdateActionCallbacks = m_updateActionCallbacks;
        m_workflowData.ReportStateAndResultAsyncCallback = AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync;

        m_previousDeviceHandle = g_iotHubClientHandleForADUComponent;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        g_iotHubClientHandleForADUComponent = reinterpret_cast<ADUC_ClientHandle>(42);
    }

    ~AduCoreExportHelpersTestCaseFixture()
    {
        if (m_workflowData.WorkflowHandle != nullptr)
        {
            workflow_free(m_workflowData.WorkflowHandle);
            m_workflowData.WorkflowHandle = nullptr;
        }

        ADUC_MethodCall_Unregister(&m_updateActionCallbacks);

        CHECK(g_iotHubClientHandleForADUComponent != nullptr);
        g_iotHubClientHandleForADUComponent = m_previousDeviceHandle;
    }

    AduCoreExportHelpersTestCaseFixture(const AduCoreExportHelpersTestCaseFixture&) = delete;
    AduCoreExportHelpersTestCaseFixture& operator=(const AduCoreExportHelpersTestCaseFixture&) = delete;
    AduCoreExportHelpersTestCaseFixture(AduCoreExportHelpersTestCaseFixture&&) = delete;
    AduCoreExportHelpersTestCaseFixture& operator=(AduCoreExportHelpersTestCaseFixture&&) = delete;

    ADUC_WorkflowData* GetWorkflowData()
    {
        return &m_workflowData;
    }

private:
    ADUC_ClientHandle m_previousDeviceHandle;

    ADUC_UpdateActionCallbacks m_updateActionCallbacks{};
    ADUC_WorkflowData m_workflowData{};
    ADUC_Workflow* m_wf = nullptr;
};

TEST_CASE("ADUC_MethodCall_Register and Unregister: Valid")
{
    ADUC_UpdateActionCallbacks updateActionCallbacks{};
    const ADUC_Result result{ ADUC_MethodCall_Register(&updateActionCallbacks, 0 /*argc*/, nullptr /*argv*/) };
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    REQUIRE(result.ExtendedResultCode == 0);

    ADUC_MethodCall_Unregister(&updateActionCallbacks);
}

TEST_CASE("ADUC_Workflow_MethodCall_Idle - Calls SandboxDestroyCallback")
{
    AduCoreExportHelpersTestCaseFixture fixture;
    ADUC_WorkflowData* workflowData = fixture.GetWorkflowData();

    auto Mock_IdleCallback = [](ADUC_Token token, const char* workflowId)
    {
        CHECK(std::string("test-workflow-id") == workflowId);
    };

    auto Mock_SandboxDestroyCallback = [](ADUC_Token token, const char* workflowId, const char* workFolder)
    {
        CHECK(std::string("test-workflow-id") == workflowId);
    };

    workflowData->UpdateActionCallbacks.SandboxDestroyCallback = Mock_SandboxDestroyCallback;
    workflowData->UpdateActionCallbacks.IdleCallback = Mock_IdleCallback;

    ADUC_Workflow_MethodCall_Idle(workflowData);

    // ADUC_Workflow_MethodCall_Idle frees and Nulls-out WorkflowHandle
    CHECK(workflowData->WorkflowHandle == nullptr);
}

TEST_CASE("ADUC_Workflow_MethodCall_Download - Fail if not DeploymentInProgress state")
{
    AduCoreExportHelpersTestCaseFixture fixture;
    ADUC_WorkflowData* workflowData = fixture.GetWorkflowData();

    ADUC_WorkflowData_SetLastReportedState(ADUCITF_State_Idle, workflowData); // something other than DeploymentInProgress

    ADUC_MethodCall_Data methodCallData{};
    methodCallData.WorkflowData = workflowData;
    ADUC_Result result = ADUC_Workflow_MethodCall_Download(&methodCallData);
    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_UPPERLEVEL_WORKFLOW_UPDATE_ACTION_UNEXPECTED_STATE);
}

// NOLINTNEXTLINE(readability-non-const-parameter)
static ADUC_Result Mock_SandboxCreateCallback(ADUC_Token token, const char* workflowId, char* workFolder)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);

    ADUC_Result result{ADUC_Result_Success};
    return result;
}

static ADUC_Result Mock_DownloadCallback(ADUC_Token token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workCompletionData);
    UNREFERENCED_PARAMETER(workflowData);

    ADUC_Result result{ADUC_Result_Success};
    return result;
}

TEST_CASE("ADUC_Workflow_MethodCall_Download - Calls SandboxCreate and DownloadCallback")
{
    AduCoreExportHelpersTestCaseFixture fixture;
    ADUC_WorkflowData* workflowData = fixture.GetWorkflowData();

    workflowData->UpdateActionCallbacks.SandboxCreateCallback = Mock_SandboxCreateCallback;
    workflowData->UpdateActionCallbacks.DownloadCallback = Mock_DownloadCallback;

    ADUC_WorkflowData_SetLastReportedState(ADUCITF_State_DeploymentInProgress, workflowData); // Must be DeploymentInProgress to succeed.

    ADUC_MethodCall_Data methodCallData{};
    methodCallData.WorkflowData = workflowData;
    ADUC_Result result = ADUC_Workflow_MethodCall_Download(&methodCallData);

    CHECK(result.ResultCode == ADUC_Result_Success);
}
