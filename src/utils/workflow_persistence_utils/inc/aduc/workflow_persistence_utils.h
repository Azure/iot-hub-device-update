/**
 * @file workflow_persistence_utils.h
 * @brief The workflow persistence interface for saving, restoring, and freeing
 *            the state necessary for startup decisions and reporting.
 *
 * @copyright Copyright (c) Microsoft Corp.
 */
#ifndef __WORKFLOW_PERSISTENCE_UTILS_H__
#define __WORKFLOW_PERSISTENCE_UTILS_H__

#include <stdbool.h>
#include "aduc/types/update_content.h"
#include "aduc/types/workflow.h"
#include "aduc/result.h"

/**
 * @brief The workflow persistence minimal state needed for completing workflow processing after a reboot of the
 *     system or a restart of the agent process.
 */
typedef struct tagWorkflowPersistenceState
{
    ADUCITF_WorkflowStep WorkflowStep;
    ADUC_Result Result;
    ADUCITF_State ReportedState;
    ADUC_SystemRebootState SystemRebootState;
    ADUC_AgentRestartState AgentRestartState;
    char* ExpectedUpdateId;
    char* WorkflowId;
    char* UpdateType;
    char* InstalledCriteria;
    char* WorkFolder;
    char* ReportingJson;
} WorkflowPersistenceState;

/**
 * @brief Serializes workflow persistence state needed for startup logic and reporting extracted to the file system
 * @param workflowData[in] The workflow data used.
 * @param systemRebootState The system reboot state to be persisted
 * @param agentRestartState The agent restart state to be persisted
 * @return boolean success.
 */
bool WorkflowPersistence_Serialize(ADUC_WorkflowData* workflowData, ADUC_SystemRebootState systemRebootState, ADUC_AgentRestartState agentRestartState);

/**
 * @brief deserializes workflow persistence state from the file system.
 * @param workflowData[in] The workflow data used.
 * @return The pointer to WorkflowPersistenceState. It must be freed by caller by calling WorkflowPersistence_Free
 */
WorkflowPersistenceState* WorkflowPersistence_Deserialize(ADUC_WorkflowData* workflowData);

/**
 * @brief Frees the persistence state
 * @param persistenceState[in,out] The @p WorkflowPersistenceState pointer to be freed.
 */
void WorkflowPersistence_Free(WorkflowPersistenceState* persistenceState);

/**
 * @brief Deletes the persisted workflow data.
 * @param workflowData[in] The workflow data for test override of persistence path.
 */
void WorkflowPersistence_Delete(const ADUC_WorkflowData* workflowData);

#endif // #ifndef __WORKFLOW_PERSISTENCE_UTILS_H__