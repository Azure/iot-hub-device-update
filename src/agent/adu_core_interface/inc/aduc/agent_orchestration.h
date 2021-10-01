#ifndef __AGENT_ORCHESTRATION_H__
#define __AGENT_ORCHESTRATION_H__

#include <aduc/types/workflow.h>
#include <stdbool.h>

ADUCITF_WorkflowStep AgentOrchestration_GetWorkflowStep(
    ADUC_WorkflowData* workflowData,
    const ADUCITF_UpdateAction desiredUpdateAction);

_Bool AgentOrchestration_IsWorkflowComplete(
    ADUCITF_UpdateAction entryUpdateAction,
    ADUCITF_UpdateAction workflowDataUpdateAction,
    ADUCITF_WorkflowStep entryAutoTransitionWorkflowStep);

_Bool AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State updateState);

_Bool AgentOrchestration_IsRetryApplicable(const char* currentTimestamp, const char* newTimestamp);

#endif // __AGENT_ORCHESTRATION_H__