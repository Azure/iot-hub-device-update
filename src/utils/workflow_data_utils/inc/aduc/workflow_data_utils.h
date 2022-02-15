/**
 * @file workflow_data_utils.h
 * @brief Util functions for working with ADUC_WorkflowData objects.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_WORKFLOW_DATA_UTILS_H
#define ADUC_WORKFLOW_DATA_UTILS_H

#include "aduc/adu_types.h"
#include "aduc/types/workflow.h"

EXTERN_C_BEGIN

/**
 * @brief  Gets the current action of the workflow being processed.
 *
 * @param workflowData The workflow data.
 * @return ADUCITF_UpdateAction The current update action.
 */
ADUCITF_UpdateAction ADUC_WorkflowData_GetCurrentAction(const ADUC_WorkflowData* workflowData);

/**
 * @brief Sets the current update action for the workflow being processed.
 *
 * @param newAction The new update action for the workflow.
 * @param workflowData The workflow data.
 */
void ADUC_WorkflowData_SetCurrentAction(ADUCITF_UpdateAction newAction, ADUC_WorkflowData* workflowData);

/**
 * @brief Gets the last reported workflow state.
 *
 * @param workflowData The workflow data.
 * @return ADUCITF_State The state.
 */
ADUCITF_State ADUC_WorkflowData_GetLastReportedState(const ADUC_WorkflowData* workflowData);

/**
 * @brief Sets the last reported workflow state.
 *
 * @param newState The new workflow state.
 * @param workflowData The workflow data.
 */
void ADUC_WorkflowData_SetLastReportedState(ADUCITF_State newState, ADUC_WorkflowData* workflowData);

/**
 * @brief Sets the last successful deployment details.
 *
 * @param[in] completedWorkflowId The completed workflow id, normally from calling workflow_peek_id(workflowData). The ownership of this string's memory is NOT transferred to the callee, which will make a copy.
 * @param[out] workflowData The workflow data from which the workflow id will be read(from its WorkflowHandle opaque object) and to which the workflow id will be written.
 * @return true if succeeded in setting the last completed workflow id.
 */
_Bool ADUC_WorkflowData_SetLastCompletedWorkflowId(const char* completedWorkflowId, ADUC_WorkflowData* workflowData);

/**
 * @brief Gets a copy of the sandbox work folder path
 *
 * @param workflowData The workflow data.
 * @return char* On success, a copy of the string that must be freed with workflow_free_string; else, NULL on failure.
 */
char* ADUC_WorkflowData_GetWorkFolder(const ADUC_WorkflowData* workflowData);

/**
 * @brief Gets the workflow Id from workflowData
 *
 * @param workflowData The workflow data.
 * @return char* On success, a copy of the string that must be freed with workflow_free_string; else, NULL on failure.
 */
char* ADUC_WorkflowData_GetWorkflowId(const ADUC_WorkflowData* workflowData);

/**
 * @brief Gets the update type of the workflow
 *
 * @param workflowData The workflow data.
 * @return char* On success, a copy of the string that must be freed with workflow_free_string; else, NULL on failure.
 */
char* ADUC_WorkflowData_GetUpdateType(const ADUC_WorkflowData* workflowData);

/**
 * @brief Gets the installed criteria of the workflow
 *
 * @param workflowData The workflow data.
 * @return char* On success, a copy of the string that must be freed with workflow_free_string; else, NULL on failure.
 */
char* ADUC_WorkflowData_GetInstalledCriteria(const ADUC_WorkflowData* workflowData);

/**
 * @brief Gets the function that reboots the system.
 *
 * @param workflowData The workflow data.
 * @return RebootSystemFunc The function that reboots the system.
 */
RebootSystemFunc ADUC_WorkflowData_GetRebootSystemFunc(const ADUC_WorkflowData* workflowData);

/**
 * @brief Gets the function for restarting the agent process.
 *
 * @param workflowData The workflow data.
 * @return RestartAgentFunc The restart agent function.
 */
RestartAgentFunc ADUC_WorkflowData_GetRestartAgentFunc(const ADUC_WorkflowData* workflowData);

/**
 * @brief Gets the function for updating the workflow state machine state with result.
 *
 * @param workflowData The workflow data.
 * @return SetUpdateStateWithResultFunc The function for updating the workflow state with result.
 */
SetUpdateStateWithResultFunc ADUC_WorkflowData_GetSetUpdateStateWithResultFunc(const ADUC_WorkflowData* workflowData);

/**
 * @brief Gets the function for handling a new incoming update action
 *
 * @param workflowData The workflow data.
 * @return HandleUpdateActionFunc The function for handling update action.
 */
HandleUpdateActionFunc ADUC_WorkflowData_GetHandleUpdateActionFunc(const ADUC_WorkflowData* workflowData);

/**
 * @brief Save the goal state json string used (re-process), as needed, after deployment is completed.
 *
 * @param workflowData The workflow data.
 * @param goalStateJson A serialized json string containing the last Goal State data.
 */
void ADUC_WorkflowData_SaveLastGoalStateJson(ADUC_WorkflowData* workflowData, const char* goalStateJson);

EXTERN_C_END

#endif // ADUC_DATA_WORKFLOW_UTILS_H
