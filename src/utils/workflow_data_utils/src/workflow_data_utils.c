/**
 * @file workflow_utils.c
 * @brief Utility functions for workflow data.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/workflow_data_utils.h"
#include <aduc/adu_core_export_helpers.h> //
#include <aduc/workflow_utils.h>
#include <azure_c_shared_utility/crt_abstractions.h> // for mallocAndStrcpy_s
#include <stdlib.h>

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

EXTERN_C_END
