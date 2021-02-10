/**
 * @file agent_workflow.h
 * @brief Handles workflow requests coming in from the hub.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#ifndef ADUC_AGENT_WORKFLOW_H
#define ADUC_AGENT_WORKFLOW_H

#include <limits.h>
#include <stdbool.h>

#include <parson.h>

#include "aduc/adu_core_exports.h"
#include "aduc/adu_core_json.h"
#include "aduc/result.h"

/**
 * @brief Content specific data for the workflow
 */
typedef struct tagADUC_ContentData
{
    ADUC_UpdateId* ExpectedUpdateId; /**< The expected/desired update Id. Required. */
    char* InstalledCriteria; /**< The installed criteria string used to evaluate if content is installed. Required. */
    char* UpdateType; /**< The content type string. Required. */
} ADUC_ContentData;

typedef enum tagADUC_AgentRestartState
{
    ADUC_AgentRestartState_None = 0, /** Agent restart not required after Apply completed. */
    ADUC_AgentRestartState_Required = 1, /** Agent restart required, but not initiated yet. */
    ADUC_AgentRestartState_InProgress = 2 /** Agent restart is in progress. */
} ADUC_AgentRestartState;

typedef enum tagADUC_SystemRebootState
{
    ADUC_SystemRebootState_None = 0, /** System reboot not required after Apply completed. */
    ADUC_SystemRebootState_Required = 1, /** System reboot required, but not initiated yet. */
    ADUC_SystemRebootState_InProgress = 2 /** System reboot is in progress. */
} ADUC_SystemRebootState;

/**
 * @brief Data shared across the agent workflow.
 */
typedef struct tagADUC_WorkflowData
{
    // In strftime("%y%m%d%H%M%S") format - see GenerateUniqueId().
    char WorkflowId[sizeof("191121010203")]; /**< Unique workflow identifier. */

    ADUCITF_State LastReportedState; /**< Previously reported state (for state validation). */

    JSON_Value* UpdateActionJson; /**< Root of AzureDeviceUpdateCore metadata. */

    ADUCITF_UpdateAction CurrentAction; /**< Value of "action" from UpdateActionJson. */

    ADUC_RegisterData RegisterData; /**< Upper-level registration data; function pointers, etc. */

    _Bool IsRegistered; /**< True if RegisterData is valid and needs to be ultimately unregistered. */

    _Bool StartupIdleCallSent; /**< True once the initial Idle call is sent to the orchestrator on agent startup. */

    _Bool OperationInProgress; /**< Is an upper-level method currently in progress? */

    ADUC_SystemRebootState SystemRebootState; /**< The system reboot state */

    ADUC_AgentRestartState AgentRestartState; /**< The agent restart state */

    _Bool OperationCancelled; /**< Was the operation in progress requested to cancel? */

    char* WorkFolder; /**< Download/Install/Apply sandbox folder */

    ADUC_DownloadProgressCallback DownloadProgressCallback; /**< Callback for download progress */

    ADUC_ContentData* ContentData; /**< The content specific data for this workflow */
} ADUC_WorkflowData;

_Bool ADUC_WorkflowData_Init(ADUC_WorkflowData* workflowData, int argc, char** argv);

_Bool ADUC_WorkflowData_UpdateContentData(ADUC_WorkflowData* workflowData);

void ADUC_WorkflowData_DoWork(ADUC_WorkflowData* workflowData);

void ADUC_WorkflowData_Free(ADUC_WorkflowData* workflowData);

void ADUC_Workflow_HandlePropertyUpdate(ADUC_WorkflowData* workflowData, const unsigned char* propertyUpdateValue);

void ADUC_Workflow_HandleUpdateAction(ADUC_WorkflowData* workflowData);

void ADUC_Workflow_HandleStartupWorkflowData(ADUC_WorkflowData* workflowData);

#endif // ADUC_AGENT_WORKFLOW_H
