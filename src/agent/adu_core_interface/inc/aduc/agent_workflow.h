/**
 * @file agent_workflow.h
 * @brief Handles workflow requests coming in from the hub.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#ifndef ADUC_AGENT_WORKFLOW_H
#define ADUC_AGENT_WORKFLOW_H

#include "aduc/types/workflow.h"
#include <stdbool.h> // for _Bool

EXTERN_C_BEGIN

_Bool ADUC_WorkflowData_Init(ADUC_WorkflowData* workflowData, int argc, char** argv);

void ADUC_WorkflowData_Uninit(ADUC_WorkflowData* workflowData);

void ADUC_WorkflowData_DoWork(ADUC_WorkflowData* workflowData);

void ADUC_Workflow_HandlePropertyUpdate(ADUC_WorkflowData* currentWorkflowData, const unsigned char* propertyUpdateValue);

void ADUC_Workflow_HandleUpdateAction(ADUC_WorkflowData* workflowData);

void ADUC_Workflow_TransitionWorkflow(ADUC_WorkflowData* workflowData);

void ADUC_Workflow_HandleStartupWorkflowData(ADUC_WorkflowData* currentWorkflowData);

EXTERN_C_END

#endif // ADUC_AGENT_WORKFLOW_H
