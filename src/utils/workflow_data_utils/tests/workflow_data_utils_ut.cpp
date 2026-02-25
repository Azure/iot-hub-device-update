/**
 * @file workflow_data_utils_ut.cpp
 * @brief Unit Tests for workflow_data_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch_all.hpp>
#include <aduc/workflow_data_utils.h>
#include <aduc/workflow_utils.h>
#include <cstring>

TEST_CASE("ADUC_WorkflowData_GetCurrentAction and SetCurrentAction")
{
    ADUC_WorkflowData workflowData = {};
    
    SECTION("Get initial action (should be default/zero)")
    {
        ADUCITF_UpdateAction action = ADUC_WorkflowData_GetCurrentAction(&workflowData);
        CHECK(action == ADUCITF_UpdateAction_Invalid_Download); // Default is 0
    }
    
    SECTION("Set and get action")
    {
        ADUC_WorkflowData_SetCurrentAction(ADUCITF_UpdateAction_ProcessDeployment, &workflowData);
        ADUCITF_UpdateAction action = ADUC_WorkflowData_GetCurrentAction(&workflowData);
        CHECK(action == ADUCITF_UpdateAction_ProcessDeployment);
    }
    
    SECTION("Update action multiple times")
    {
        ADUC_WorkflowData_SetCurrentAction(ADUCITF_UpdateAction_ProcessDeployment, &workflowData);
        CHECK(ADUC_WorkflowData_GetCurrentAction(&workflowData) == ADUCITF_UpdateAction_ProcessDeployment);
        
        ADUC_WorkflowData_SetCurrentAction(ADUCITF_UpdateAction_Cancel, &workflowData);
        CHECK(ADUC_WorkflowData_GetCurrentAction(&workflowData) == ADUCITF_UpdateAction_Cancel);
    }
}

TEST_CASE("ADUC_WorkflowData_GetLastReportedState and SetLastReportedState")
{
    ADUC_WorkflowData workflowData = {};
    
    SECTION("Get initial state (should be default/zero)")
    {
        ADUCITF_State state = ADUC_WorkflowData_GetLastReportedState(&workflowData);
        CHECK(state == ADUCITF_State_Idle);
    }
    
    SECTION("Set and get state")
    {
        ADUC_WorkflowData_SetLastReportedState(ADUCITF_State_DownloadStarted, &workflowData);
        ADUCITF_State state = ADUC_WorkflowData_GetLastReportedState(&workflowData);
        CHECK(state == ADUCITF_State_DownloadStarted);
    }
    
    SECTION("Update state multiple times")
    {
        ADUC_WorkflowData_SetLastReportedState(ADUCITF_State_InstallStarted, &workflowData);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_InstallStarted);
        
        ADUC_WorkflowData_SetLastReportedState(ADUCITF_State_InstallSucceeded, &workflowData);
        CHECK(ADUC_WorkflowData_GetLastReportedState(&workflowData) == ADUCITF_State_InstallSucceeded);
    }
}

TEST_CASE("ADUC_WorkflowData_SetLastCompletedWorkflowId")
{
    ADUC_WorkflowData workflowData = {};
    workflowData.LastCompletedWorkflowId = nullptr;
    
    SECTION("Set workflow ID successfully")
    {
        const char* testId = "workflow-123-abc";
        bool result = ADUC_WorkflowData_SetLastCompletedWorkflowId(testId, &workflowData);
        CHECK(result == true);
        CHECK(workflowData.LastCompletedWorkflowId != nullptr);
        CHECK(strcmp(workflowData.LastCompletedWorkflowId, testId) == 0);
        
        // Cleanup
        workflow_free_string(workflowData.LastCompletedWorkflowId);
    }
    
    SECTION("Replace existing workflow ID")
    {
        const char* firstId = "first-workflow-id";
        const char* secondId = "second-workflow-id";
        
        ADUC_WorkflowData_SetLastCompletedWorkflowId(firstId, &workflowData);
        CHECK(strcmp(workflowData.LastCompletedWorkflowId, firstId) == 0);
        
        ADUC_WorkflowData_SetLastCompletedWorkflowId(secondId, &workflowData);
        CHECK(strcmp(workflowData.LastCompletedWorkflowId, secondId) == 0);
        
        // Cleanup
        workflow_free_string(workflowData.LastCompletedWorkflowId);
    }
    
    SECTION("Handle NULL workflow ID")
    {
        bool result = ADUC_WorkflowData_SetLastCompletedWorkflowId(nullptr, &workflowData);
        CHECK(result == false);
    }
}

TEST_CASE("ADUC_WorkflowData wrapper APIs handle null workflow handle")
{
    ADUC_WorkflowData workflowData = {};
    workflowData.WorkflowHandle = nullptr;

    SECTION("GetWorkFolder returns null for null workflow handle")
    {
        char* workFolder = ADUC_WorkflowData_GetWorkFolder(&workflowData);
        CHECK(workFolder == nullptr);
    }

    SECTION("GetWorkflowId returns null for null workflow handle")
    {
        char* workflowId = ADUC_WorkflowData_GetWorkflowId(&workflowData);
        CHECK(workflowId == nullptr);
    }

    SECTION("GetUpdateType returns null for null workflow handle")
    {
        char* updateType = ADUC_WorkflowData_GetUpdateType(&workflowData);
        CHECK(updateType == nullptr);
    }

    SECTION("GetInstalledCriteria returns null for null workflow handle")
    {
        char* installedCriteria = ADUC_WorkflowData_GetInstalledCriteria(&workflowData);
        CHECK(installedCriteria == nullptr);
    }
}

TEST_CASE("ADUC_WorkflowData_InitWorkflowHandle validates input")
{
    SECTION("Uninitialized workflow data initializes workflow handle")
    {
        ADUC_WorkflowData workflowData = {};
        CHECK(ADUC_WorkflowData_InitWorkflowHandle(&workflowData));
        workflow_free(workflowData.WorkflowHandle);
        workflowData.WorkflowHandle = nullptr;
    }
}