/**
 * @file agent_workflow.h
 * @brief Handles workflow requests coming in from the hub.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_AGENT_WORKFLOW_H
#define ADUC_AGENT_WORKFLOW_H

#include "aduc/types/workflow.h"
#include <stdbool.h> // for _Bool

EXTERN_C_BEGIN

void ADUC_Workflow_DoWork(ADUC_WorkflowData* workflowData);

void ADUC_Workflow_HandlePropertyUpdate(ADUC_WorkflowData* currentWorkflowData, const unsigned char* propertyUpdateValue, bool forceDeferral);

void ADUC_Workflow_HandleComponentChanged(ADUC_WorkflowData* workflowData);

void ADUC_Workflow_HandleUpdateAction(ADUC_WorkflowData* workflowData);

void ADUC_Workflow_TransitionWorkflow(ADUC_WorkflowData* workflowData);

void ADUC_Workflow_HandleStartupWorkflowData(ADUC_WorkflowData* currentWorkflowData);

const char* ADUC_Workflow_CancellationTypeToString(ADUC_WorkflowCancellationType cancellationType);

//
// Device Update Action data type and methods.
//
typedef struct tagADUC_MethodCall_Data
{
    ADUC_WorkCompletionData WorkCompletionData;
    ADUC_WorkflowData* WorkflowData;
} ADUC_MethodCall_Data;

void ADUC_Workflow_MethodCall_Idle(ADUC_WorkflowData* workflowData);

ADUC_Result ADUC_Workflow_MethodCall_ProcessDeployment(ADUC_MethodCall_Data* methodCallData);
void ADUC_Workflow_MethodCall_ProcessDeployment_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result);

ADUC_Result ADUC_Workflow_MethodCall_Download(ADUC_MethodCall_Data* methodCallData);
void ADUC_Workflow_MethodCall_Download_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result);

ADUC_Result ADUC_Workflow_MethodCall_Install(ADUC_MethodCall_Data* methodCallData);
void ADUC_Workflow_MethodCall_Install_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result);

ADUC_Result ADUC_Workflow_MethodCall_Apply(ADUC_MethodCall_Data* methodCallData);
void ADUC_Workflow_MethodCall_Apply_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result);

void ADUC_Workflow_MethodCall_Cancel(const ADUC_WorkflowData* workflowData);

ADUC_Result ADUC_Workflow_MethodCall_IsInstalled(const ADUC_WorkflowData* workflowData);


//
// State transition
//

void ADUC_Workflow_SetUpdateState(ADUC_WorkflowData* workflowData, ADUCITF_State updateState);
void ADUC_Workflow_SetUpdateStateWithResult(ADUC_WorkflowData* workflowData, ADUCITF_State updateState, ADUC_Result result);
void ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle(ADUC_WorkflowData* workflowData, const char* updateId);

void ADUC_Workflow_DefaultDownloadProgressCallback(
    const char* workflowId,
    const char* fileId,
    ADUC_DownloadProgressState state,
    uint64_t bytesTransferred,
    uint64_t bytesTotal);

EXTERN_C_END

#endif // ADUC_AGENT_WORKFLOW_H
