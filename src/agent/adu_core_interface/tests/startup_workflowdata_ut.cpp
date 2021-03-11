/**
 * @file startup_workflowdata_ut.cpp
 * @brief Unit tests for ADUC_Workflow_HandleStartupWorkflowData
 * @copyright Copyright (c) Microsoft Corp.
 * Licensed under the MIT License.
 */

#include "workflow_test_utils.h"
#include <aduc/agent_workflow.h>
#include <aduc/c_utils.h>
#include <aduc/workflow_internal.h>
#include <aduc/workflow_utils.h>
#include <catch2/catch.hpp>

static std::string get_update_manifest_json_path() {
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/startupworkflowdata/updateManifest.json";
    return path;
}

static unsigned int s_handleUpdateAction_call_count = 0;
static void Mock_ADUC_Workflow_HandleUpdateAction(ADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);

    ++s_handleUpdateAction_call_count;
}

static unsigned int s_setUpdateStateWithResult_call_count = 0;
static void Mock_ADUC_Workflow_SetUpdateStateWithResult(
    ADUC_WorkflowData* workflowData, ADUCITF_State updateState, ADUC_Result result)
{
    UNREFERENCED_PARAMETER(workflowData);
    UNREFERENCED_PARAMETER(updateState);
    UNREFERENCED_PARAMETER(result);

    ++s_setUpdateStateWithResult_call_count;
}

static ADUC_Result Mock_IsInstalledCallback(ADUC_Token token, ADUC_WorkflowDataToken workflowDataToken)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowDataToken);

    ADUC_Result result = { ADUC_Result_Success };
    return result;
}

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

static void Reset_MockStats()
{
    s_handleUpdateAction_call_count = 0;
    s_setUpdateStateWithResult_call_count = 0;
}

TEST_CASE("ADUC_Workflow_HandleStartupWorkflowData")
{
    SECTION("It should exit early when workflowData is NULL")
    {
        Reset_MockStats();

        ADUC_Workflow_HandleStartupWorkflowData(nullptr /* currentWorkflowData */);
        CHECK(s_handleUpdateAction_call_count == 0);
        CHECK(s_setUpdateStateWithResult_call_count == 0);
    }

    SECTION("It should exit early when StartupIdleCallSent is true")
    {
        Reset_MockStats();

        ADUC_WorkflowData workflowData{};
        workflowData.StartupIdleCallSent = true;

        ADUC_Workflow_HandleStartupWorkflowData(&workflowData);

        CHECK(s_handleUpdateAction_call_count == 0);
        CHECK(s_setUpdateStateWithResult_call_count == 0);
        CHECK(workflowData.StartupIdleCallSent == true);
    }

    SECTION("It should call HandleUpdateAction for ProcessDeployment workflow action")
    {
        Reset_MockStats();

        auto nextWorkflow = (ADUC_Workflow*)malloc(sizeof(ADUC_Workflow)); // NOLINT
        REQUIRE(nextWorkflow != nullptr);

        ADUC_Result result = workflow_init_from_file(get_update_manifest_json_path().c_str(), true, (void**)&nextWorkflow); // NOLINT
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        ADUC_WorkflowData workflowData{};
        ADUC_TestOverride_Hooks hooks{};
        hooks.HandleUpdateActionFunc_TestOverride = Mock_ADUC_Workflow_HandleUpdateAction;
        hooks.SetUpdateStateWithResultFunc_TestOverride = Mock_ADUC_Workflow_SetUpdateStateWithResult;
        workflowData.TestOverrides = &hooks;

        workflowData.WorkflowHandle = nextWorkflow;
        workflowData.StartupIdleCallSent = false;
        workflowData.UpdateActionCallbacks.IsInstalledCallback = Mock_IsInstalledCallback;
        workflowData.UpdateActionCallbacks.SandboxDestroyCallback = Mock_SandboxDestroyCallback;
        workflowData.UpdateActionCallbacks.SandboxCreateCallback = Mock_SandboxCreateCallback;

        ADUC_Workflow_HandleStartupWorkflowData(&workflowData);
        CHECK(s_handleUpdateAction_call_count == 1);
        CHECK(s_setUpdateStateWithResult_call_count == 0);
        CHECK(workflowData.StartupIdleCallSent == true);
    }

    SECTION("It should exit early and transition to Idle(success) for null workflowData")
    {
        Reset_MockStats();

        auto nextWorkflow = (ADUC_Workflow*)malloc(sizeof(ADUC_Workflow)); // NOLINT
        REQUIRE(nextWorkflow != nullptr);

        ADUC_Result result = workflow_init_from_file(get_update_manifest_json_path().c_str(), true, (void**)&nextWorkflow); // NOLINT
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        ADUC_Workflow_HandleStartupWorkflowData(nullptr);
        CHECK(s_handleUpdateAction_call_count == 0);
        CHECK(s_setUpdateStateWithResult_call_count == 0);
    }
}

// workflowData->WorkflowHandle will be NULL in this case
TEST_CASE("HandleStartupWorkflowData from ADUCoreInterface_Connected")
{
    SECTION("It should set StartupIdleCallSent")
    {
        // Arrange
        ADUC_WorkflowData workflowData{};
        ADUC_TestOverride_Hooks hooks{};
        workflowData.TestOverrides = &hooks;

        // Act
        ADUC_Workflow_HandleStartupWorkflowData(&workflowData);

        // Assert
        CHECK(workflowData.StartupIdleCallSent);
    }

    SECTION("It should handle workflow persistence")
    {
        // Arrange
        ADUC_WorkflowData workflowData{};
        ADUC_TestOverride_Hooks hooks{};
        workflowData.TestOverrides = &hooks;

        // Act
        ADUC_Workflow_HandleStartupWorkflowData(&workflowData);

        // Assert
        CHECK(workflowData.StartupIdleCallSent);
    }
}
