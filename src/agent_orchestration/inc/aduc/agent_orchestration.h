/**
 * @file agent_orchestration.h
 * @brief The header declarations for business logic for agent-driven workflow orchestration processing.
 *
 * @copyright Copyright (c) Microsoft Corp.
 * Licensed under the MIT License.
 */

#ifndef __AGENT_ORCHESTRATION_H__
#define __AGENT_ORCHESTRATION_H__

#include <aduc/types/workflow.h>
#include <stdbool.h>

/**
 * @brief Maps the desired update action from the twin to a workflow step
 * @param desiredUpdateAction The update action from the desired section of the twin
 * @return The mapped workflow step
 */
ADUCITF_WorkflowStep AgentOrchestration_GetWorkflowStep(const ADUCITF_UpdateAction desiredUpdateAction);

/**
 * @brief Returns whether the workflow is complete.
 * @param entryAutoTransitionWorkflowStep The auto transition workflow step from the entry in the workflow handler map.
 * @return true if workflow is complete.
 */
bool AgentOrchestration_IsWorkflowComplete(ADUCITF_WorkflowStep entryAutoTransitionWorkflowStep);

/**
 * @brief Returns whether reporting to the cloud should be done.
 * @param updateState the current update state.
 * @return true if it should not report.
 */
bool AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State updateState);

/**
 * @brief Returns whether a retry should be done or not.
 * @param currentToken The timestamp token of the current workflow; can be NULL if it does not have one (first try).
 * @param newToken The timestamp token of the new workflow request.
 * @return true if a retry should be done.
 */
bool AgentOrchestration_IsRetryApplicable(const char* currentToken, const char* newToken);

#endif // __AGENT_ORCHESTRATION_H__
