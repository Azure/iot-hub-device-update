/**
 * @file agent_orchestration.c
 * @brief Contains business logic implementation for agent-driven workflow orchestration processing.
 *
 * @copyright Copyright (c) Microsoft Corp.
 * Licensed under the MIT License.
 */

#include "aduc/agent_orchestration.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h"
#include "aduc/types/update_content.h"
#include "aduc/types/workflow.h"
#include "aduc/workflow_utils.h"

/**
 * @brief Maps the desired update action from the twin to a workflow step
 * @param desiredUpdateAction The update action from the desired section of the twin
 * @return The mapped workflow step
 */
ADUCITF_WorkflowStep AgentOrchestration_GetWorkflowStep(const ADUCITF_UpdateAction desiredUpdateAction)
{
    switch (desiredUpdateAction)
    {
    case ADUCITF_UpdateAction_ProcessDeployment:
        return ADUCITF_WorkflowStep_ProcessDeployment;

    case ADUCITF_UpdateAction_Cancel:
        // Should not get here as Cancel should have just signaled the cancel request
        // to the current ongoing operation, or just went to idle.
        Log_Warn("GetWorkflowStep called with Cancel action; expected Cancel to be handled before reaching here");
        return ADUCITF_WorkflowStep_Undefined;

    default:
        Log_Warn("GetWorkflowStep: unrecognized desiredUpdateAction %d, returning Undefined step", (int)desiredUpdateAction);
        return ADUCITF_WorkflowStep_Undefined;
    }
}

/**
 * @brief Returns whether the workflow is complete.
 * @param entryAutoTransitionWorkflowStep The auto transition workflow step from the entry in the workflow handler map.
 * @return true if workflow is complete.
 */
bool AgentOrchestration_IsWorkflowComplete(ADUCITF_WorkflowStep entryAutoTransitionWorkflowStep)
{
    return entryAutoTransitionWorkflowStep == ADUCITF_WorkflowStep_Undefined;
}

/**
 * @brief Returns whether reporting to the cloud should be done.
 * @param updateState the current update state.
 * @return true if it should not report.
 */
bool AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State updateState)
{
    return updateState != ADUCITF_State_DeploymentInProgress && updateState != ADUCITF_State_Idle
        && updateState != ADUCITF_State_Failed;
}

/**
 * @brief Returns whether a retry should be done or not.
 * @param currentToken The timestamp token of the current workflow; can be NULL if it does not have one (first try).
 * @param newToken The timestamp token of the new workflow request.
 * @return true if a retry should be done.
 */
bool AgentOrchestration_IsRetryApplicable(const char* currentToken, const char* newToken)
{
    if (currentToken == NULL || newToken == NULL)
    {
        if (currentToken == NULL && newToken != NULL)
        {
            // canonical retry
            Log_Debug("IsRetryApplicable: currentToken is NULL, newToken is non-NULL — canonical retry");
            return true;
        }

        if (currentToken != NULL && newToken == NULL)
        {
            Log_Warn("IsRetryApplicable: newToken is NULL while currentToken is '%s' — skipping retry", currentToken);
        }

        return false;
    }

    return (strcmp(currentToken, newToken) != 0);
}
