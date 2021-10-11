/**
 * @file startup_workflowdata_ut.cpp
 * @brief Unit tests for ADUC_Workflow_HandleStartupWorkflowData
 * @copyright Copyright (c), Microsoft Corp.
 */

#include <aduc/agent_workflow.h>
#include <aduc/c_utils.h>
#include <aduc/workflow_utils.h>
#include <workflow_internal.h>
#include <catch2/catch.hpp>

std::string get_update_manifest_json_path() {
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/startupworkflowdata/updateManifest.json";
    return path;
}

static unsigned int s_handleUpdateAction_call_count = 0;
void Mock_ADUC_Workflow_HandleUpdateAction(ADUC_WorkflowData* workflowData)
{
    ++s_handleUpdateAction_call_count;
}

static unsigned int s_setUpdateStateWithResult_call_count = 0;
void Mock_ADUC_SetUpdateStateWithResult(
    ADUC_WorkflowData* workflowData, ADUCITF_State updateState, ADUC_Result result)
{
    ++s_setUpdateStateWithResult_call_count;
}

static ADUC_Result Mock_IsInstalledCallback(ADUC_Token, ADUC_WorkflowDataToken)
{
    ADUC_Result result = { ADUC_Result_Success };
    return result;
}

void Reset_MockStats()
{
    s_handleUpdateAction_call_count = 0;
    s_setUpdateStateWithResult_call_count = 0;
}

TEST_CASE("ADUC_Workflow_HandleStartupWorkflowData")
{
    SECTION("It should call HandleUpdateAction for ProcessDeployment workflow action")
    {
        Reset_MockStats();

        ADUC_Workflow* nextWorkflow = (ADUC_Workflow*)malloc(sizeof(ADUC_Workflow));
        REQUIRE(nextWorkflow != nullptr);

        ADUC_Result result = workflow_init_from_file(get_update_manifest_json_path().c_str(), true, (void**)&nextWorkflow);
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        ADUC_WorkflowData workflowData{};
        ADUC_TestOverride_Hooks hooks{};
        hooks.HandleUpdateActionFunc_TestOverride = Mock_ADUC_Workflow_HandleUpdateAction;
        hooks.SetUpdateStateWithResultFunc_TestOverride = Mock_ADUC_SetUpdateStateWithResult;
        workflowData.TestOverrides = &hooks;

        workflowData.WorkflowHandle = nextWorkflow;
        workflowData.StartupIdleCallSent = false;
        workflowData.UpdateActionCallbacks.IsInstalledCallback = Mock_IsInstalledCallback;

        ADUC_Workflow_HandleStartupWorkflowData(&workflowData);
        CHECK(s_handleUpdateAction_call_count == 1);
        CHECK(s_setUpdateStateWithResult_call_count == 0);
        CHECK(workflowData.StartupIdleCallSent == true);
    }

    SECTION("It should exit early and transition to Idle(success) for null workflowData")
    {
        Reset_MockStats();

        ADUC_Workflow* nextWorkflow = (ADUC_Workflow*)malloc(sizeof(ADUC_Workflow));
        REQUIRE(nextWorkflow != nullptr);

        ADUC_Result result = workflow_init_from_file(get_update_manifest_json_path().c_str(), true, (void**)&nextWorkflow);
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        ADUC_Workflow_HandleStartupWorkflowData(nullptr);
        CHECK(s_handleUpdateAction_call_count == 0);
        CHECK(s_setUpdateStateWithResult_call_count == 0);
    }
}