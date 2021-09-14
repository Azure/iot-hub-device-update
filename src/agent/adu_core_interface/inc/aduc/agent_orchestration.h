#ifndef __AGENT_ORCHESTRATION_H__
#define __AGENT_ORCHESTRATION_H__

#include <aduc/types/workflow.h>
#include <stdbool.h>

inline _Bool AgentOrchestration_IsWorkflowOrchestratedByAgent(ADUC_WorkflowData* workflowData)
{
    return workflowData != NULL
        && workflowData->CurrentAction == ADUCITF_UpdateAction_ProcessDeployment;
}

ADUCITF_WorkflowStep AgentOrchestration_GetWorkflowStep(const ADUCITF_UpdateAction desiredUpdateAction);

_Bool AgentOrchestration_IsWorkflowComplete(
    ADUCITF_UpdateAction entryUpdateAction,
    ADUCITF_UpdateAction workflowDataUpdateAction,
    ADUCITF_WorkflowStep entryAutoTransitionWorkflowStep);


_Bool AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State updateState);

#endif // __AGENT_ORCHESTRATION_H__