/**
 * @file workflow_data_utils.h
 * @brief Util functions for working with ADUC_WorkflowData objects.
 *
 * @copyright Copyright (c) Microsoft Corp.
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
 * @brief Gets a copy of the sandbox work folder path from workflowData persistenceState, or else from the workflowHandle.
 *
 * @param workflowData The workflow data.
 * @return char* On success, a copy of the string that must be freed with workflow_free_string; else, NULL on failure.
 */
char* ADUC_WorkflowData_GetWorkFolder(const ADUC_WorkflowData* workflowData);

/**
 * @brief Gets the workflow Id from workflowData persistenceState, or else from the WorkflowHandle.
 *
 * @param workflowData The workflow data.
 * @return char* On success, a copy of the string that must be freed with workflow_free_string; else, NULL on failure.
 */
char* ADUC_WorkflowData_GetWorkflowId(const ADUC_WorkflowData* workflowData);

/**
 * @brief Gets the update type of the workflow from workflowData persistenceState, or else from the WorkflowHandle.
 *
 * @param workflowData The workflow data.
 * @return char* On success, a copy of the string that must be freed with workflow_free_string; else, NULL on failure.
 */
char* ADUC_WorkflowData_GetUpdateType(const ADUC_WorkflowData* workflowData);

/**
 * @brief Gets the installed criteria of the workflow from workflowData persistenceState, or else from the WorkflowHandle.
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

EXTERN_C_END

#endif // ADUC_DATA_WORKFLOW_UTILS_H
