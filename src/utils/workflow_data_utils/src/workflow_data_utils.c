/**
 * @file workflow_utils.c
 * @brief Utility functions for workflow data.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_utils.h"

#include <azure_c_shared_utility/crt_abstractions.h> // for mallocAndStrcpy_s
#include <stdlib.h>

// Forward declarations
int ADUC_MethodCall_RebootSystem();
int ADUC_MethodCall_RestartAgent();
void ADUC_Workflow_SetUpdateStateWithResult(
    ADUC_WorkflowData* workflowData, ADUCITF_State updateState, ADUC_Result result);
void ADUC_Workflow_HandleUpdateAction(ADUC_WorkflowData* workflowData);

EXTERN_C_BEGIN

/**
 * @brief  Gets the current action of the workflow being processed.
 *
 * @param workflowData The workflow data.
 * @return ADUCITF_UpdateAction The current update action.
 */
ADUCITF_UpdateAction ADUC_WorkflowData_GetCurrentAction(const ADUC_WorkflowData* workflowData)
{
    return workflowData->CurrentAction;
}

/**
 * @brief Sets the current update action for the workflow being processed.
 *
 * @param newAction The new update action for the workflow.
 * @param workflowData The workflow data.
 */
void ADUC_WorkflowData_SetCurrentAction(ADUCITF_UpdateAction newAction, ADUC_WorkflowData* workflowData)
{
    workflowData->CurrentAction = newAction;
}

/**
 * @brief Gets the last reported workflow state.
 *
 * @param workflowData The workflow data.
 * @return ADUCITF_State The state.
 */
ADUCITF_State ADUC_WorkflowData_GetLastReportedState(const ADUC_WorkflowData* workflowData)
{
    return workflowData->LastReportedState;
}

/**
 * @brief Sets the last reported workflow state.
 *
 * @param newState The new workflow state.
 * @param workflowData The workflow data.
 */
void ADUC_WorkflowData_SetLastReportedState(ADUCITF_State newState, ADUC_WorkflowData* workflowData)
{
    workflowData->LastReportedState = newState;
}

/**
 * @brief Sets the last successful deployment details.
 *
 * @param[in]  completedWorkflowId The completed workflow id, normally from calling workflow_peek_id(workflowData). The ownership of this string's memory is NOT transferred to the callee, which will make a copy.
 * @param[out] workflowData The workflow data from which the workflow id will be read(from its WorkflowHandle opaque object) and to which the workflow id will be written.
 * @return true if succeeded in setting the last completed workflow id.
 */
_Bool ADUC_WorkflowData_SetLastCompletedWorkflowId(const char* completedWorkflowId, ADUC_WorkflowData* workflowData)
{
    char* copy = workflow_copy_string(completedWorkflowId);
    if (copy == NULL)
    {
        return false;
    }

    workflow_free_string(workflowData->LastCompletedWorkflowId);
    workflowData->LastCompletedWorkflowId = copy;

    return true;
}

/**
 * @brief Gets a copy of the sandbox work folder path
 *
 * @param workflowData The workflow data.
 * @return char* On success, a copy of the string that must be freed with workflow_free_string; else, NULL on failure.
 */
char* ADUC_WorkflowData_GetWorkFolder(const ADUC_WorkflowData* workflowData)
{
    return workflow_get_workfolder(workflowData->WorkflowHandle);
}

/**
 * @brief Gets the workflow Id
 *
 * @param workflowData The workflow data.
 * @return char* On success, a copy of the string that must be freed with workflow_free_string; else, NULL on failure.
 */
char* ADUC_WorkflowData_GetWorkflowId(const ADUC_WorkflowData* workflowData)
{
    return workflow_get_id(workflowData->WorkflowHandle);
}

/**
 * @brief Gets the update type of the workflow
 *
 * @param workflowData The workflow data.
 * @return char* On success, a copy of the string that must be freed with workflow_free_string; else, NULL on failure.
 */
char* ADUC_WorkflowData_GetUpdateType(const ADUC_WorkflowData* workflowData)
{
    return workflow_get_update_type(workflowData->WorkflowHandle);
}

/**
 * @brief Gets the installed criteria of the workflow
 *
 * @param workflowData The workflow data.
 * @return char* On success, a copy of the string that must be freed with workflow_free_string; else, NULL on failure.
 */
char* ADUC_WorkflowData_GetInstalledCriteria(const ADUC_WorkflowData* workflowData)
{
    return workflow_get_installed_criteria(workflowData->WorkflowHandle);
}

/**
 * @brief Gets the function that reboots the system.
 *
 * @param workflowData The workflow data.
 * @return RebootSystemFunc The function that reboots the system.
 */
RebootSystemFunc ADUC_WorkflowData_GetRebootSystemFunc(const ADUC_WorkflowData* workflowData)
{
    RebootSystemFunc fn = ADUC_MethodCall_RebootSystem;

#ifdef ADUC_BUILD_UNIT_TESTS
    if (workflowData->TestOverrides && workflowData->TestOverrides->RebootSystemFunc_TestOverride)
    {
        fn = workflowData->TestOverrides->RebootSystemFunc_TestOverride;
    }
#endif

    return fn;
}

/**
 * @brief Gets the function for restarting the agent process.
 *
 * @param workflowData The workflow data.
 * @return RestartAgentFunc The restart agent function.
 */
RestartAgentFunc ADUC_WorkflowData_GetRestartAgentFunc(const ADUC_WorkflowData* workflowData)
{
    RestartAgentFunc fn = ADUC_MethodCall_RestartAgent;

#ifdef ADUC_BUILD_UNIT_TESTS
    if (workflowData->TestOverrides && workflowData->TestOverrides->RestartAgentFunc_TestOverride)
    {
        fn = workflowData->TestOverrides->RestartAgentFunc_TestOverride;
    }
#endif

    return fn;
}

/**
 * @brief Gets the function for updating the workflow state machine state with result.
 *
 * @param workflowData The workflow data.
 * @return SetUpdateStateWithResultFunc The function for updating the workflow state with result.
 */
SetUpdateStateWithResultFunc ADUC_WorkflowData_GetSetUpdateStateWithResultFunc(const ADUC_WorkflowData* workflowData)
{
    SetUpdateStateWithResultFunc fn = ADUC_Workflow_SetUpdateStateWithResult;

#ifdef ADUC_BUILD_UNIT_TESTS
    if (workflowData->TestOverrides && workflowData->TestOverrides->SetUpdateStateWithResultFunc_TestOverride)
    {
        fn = workflowData->TestOverrides->SetUpdateStateWithResultFunc_TestOverride;
    }
#endif

    return fn;
}

/**
 * @brief Gets the function for handling a new incoming update action
 *
 * @param workflowData The workflow data.
 * @return HandleUpdateActionFunc The function for handling update action.
 */
HandleUpdateActionFunc ADUC_WorkflowData_GetHandleUpdateActionFunc(const ADUC_WorkflowData* workflowData)
{
    HandleUpdateActionFunc fn = ADUC_Workflow_HandleUpdateAction;

#ifdef ADUC_BUILD_UNIT_TESTS
    if (workflowData->TestOverrides && workflowData->TestOverrides->HandleUpdateActionFunc_TestOverride)
    {
        fn = workflowData->TestOverrides->HandleUpdateActionFunc_TestOverride;
    }
#endif

    return fn;
}

/**
 * @brief Save the goal state json string used (re-process), as needed, after deployment is completed.
 *
 * @param workflowData The workflow data.
 * @param goalStateJson A serialized json string containing the last Goal State data.
 */
void ADUC_WorkflowData_SaveLastGoalStateJson(ADUC_WorkflowData* workflowData, const char* goalStateJson)
{
    if (workflowData->LastGoalStateJson != (char*)goalStateJson)
    {
        free(workflowData->LastGoalStateJson);
        if (mallocAndStrcpy_s(&workflowData->LastGoalStateJson, (const char*)goalStateJson) != 0)
        {
            workflowData->LastGoalStateJson = NULL;
        }
    }
}

EXTERN_C_END
