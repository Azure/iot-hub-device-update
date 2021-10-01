
#include "aduc/agent_orchestration.h"
#include "aduc/types/update_content.h"
#include "aduc/types/workflow.h"
#include "aduc/workflow_utils.h"
#include "aduc/string_c_utils.h"

ADUCITF_WorkflowStep AgentOrchestration_GetWorkflowStep(
    ADUC_WorkflowData* workflowData,
    const ADUCITF_UpdateAction desiredUpdateAction)
{
    switch (desiredUpdateAction)
    {
        case ADUCITF_UpdateAction_ProcessDeployment:
            return ADUCITF_WorkflowStep_ProcessDeployment;

        case ADUCITF_UpdateAction_Cancel:
            // Should not get here as Cancel should have just signaled the cancel request
            // to the current ongoing operation, or just went to idle.
            return ADUCITF_WorkflowStep_Undefined;

        default:
            return ADUCITF_WorkflowStep_Undefined;
    }
}

_Bool AgentOrchestration_IsWorkflowComplete(
    ADUCITF_UpdateAction entryUpdateAction,
    ADUCITF_UpdateAction workflowDataUpdateAction,
    ADUCITF_WorkflowStep entryAutoTransitionWorkflowStep)
{
    // TODO(jewelden): Remove the next line before shipping PPR once disabling support for cloud-driven orchestration.
    // That is, auto-transitioning will always occur when agent is driving orchestration via ProcessDeployment update action.
    _Bool isAutoTransitionApplicable = entryUpdateAction == workflowDataUpdateAction;

    _Bool isWorkflowOnLastStep = entryAutoTransitionWorkflowStep == ADUCITF_WorkflowStep_Undefined;

    return !isAutoTransitionApplicable || isWorkflowOnLastStep;
}

_Bool AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State updateState) {
    return updateState != ADUCITF_State_DeploymentInProgress
        && updateState != ADUCITF_State_Idle
        && updateState != ADUCITF_State_Failed;
}

_Bool AgentOrchestration_IsRetryApplicable(const char* currentToken, const char* newToken)
{
    if (currentToken == NULL || newToken == NULL)
    {
        if (currentToken == NULL && newToken != NULL)
        {
            // canonical retry
            return true;
        }

        return false;
    }

    return (strcmp(currentToken, newToken) != 0);
}