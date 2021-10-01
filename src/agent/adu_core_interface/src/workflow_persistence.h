/**
 * @file workflow_persistence.h
 * @brief The workflow persistence interface for saving, restoring, and freeing
 *            the state necessary for startup decisions and reporting.
 *
 * @copyright Copyright (c) Microsoft Corp.
 */
#ifndef __WORKFLOW_PERSISTENCE_H__
#define __WORKFLOW_PERSISTENCE_H__

#include <stdbool.h>
#include "aduc/types/update_content.h"
#include "aduc/result.h"

typedef struct tagWorkflowPersistenceState
{
    ADUCITF_WorkflowStep WorkflowStep;
    ADUC_Result Result;
    ADUCITF_State ReportedState;
    ADUC_SystemRebootState SystemRebootState;
    ADUC_AgentRestartState AgentRestartState;
    char* ExpectedUpdateId;
    char* WorkflowId;
    char* WorkFolder;
    char* ReportingJson;
} WorkflowPersistenceState;

bool WorkflowPersistence_Serialize(ADUC_WorkflowData*workflowData);
WorkflowPersistenceState* WorkflowPersistence_Deserialize();
void WorkflowPersistence_Free(WorkflowPersistenceState* persistenceState);

#endif // #ifndef __WORKFLOW_PERSISTENCE_H__