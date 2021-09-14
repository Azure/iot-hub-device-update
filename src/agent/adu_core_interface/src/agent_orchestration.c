
#include "aduc/agent_orchestration.h"
#include "aduc/types/workflow.h"

/*
 * C99 extern inlining model
 */
// NOLINTNEXTLINE(readability-redundant-declaration)
extern /* inline */ _Bool AgentOrchestration_IsWorkflowOrchestratedByAgent(ADUC_WorkflowData* workflowData);

ADUCITF_WorkflowStep AgentOrchestration_GetWorkflowStep(const ADUCITF_UpdateAction desiredUpdateAction)
{
    switch (desiredUpdateAction)
    {
        case ADUCITF_UpdateAction_Download:
            return ADUCITF_WorkflowStep_Download;

        case ADUCITF_UpdateAction_Install:
            return ADUCITF_WorkflowStep_Install;

        case ADUCITF_UpdateAction_Apply:
            return ADUCITF_WorkflowStep_Apply;

        // TODO(jewelden): Remove the above cases and handle Cancel

        case ADUCITF_UpdateAction_ProcessDeployment:
            return ADUCITF_WorkflowStep_ProcessDeployment;

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