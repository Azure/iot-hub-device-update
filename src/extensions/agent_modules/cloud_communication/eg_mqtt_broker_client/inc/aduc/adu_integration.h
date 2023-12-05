/**
 * @file adu_integration.h
 * @brief The header for code from gen1 but initially necessary for gen2
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADU_INTEGRATION_H
#define ADU_INTEGRATION_H

#include <aduc/types/workflow.h>
#include <aduc/result.h>
#include <aduc/content_handler.hpp>

#include <parson.h>

void ADU_Integration_CallDownloadHandlerOnUpdateWorkflowCompleted(const ADUC_WorkflowHandle workflowHandle);
void ADU_Integration_SetInstalledUpdateIdAndGoToIdle(ADUC_WorkflowData* workflowData, const char* updateId);
ContentHandler* ADU_Integration_GetUpdateManifestHandler(const ADUC_WorkflowData* workflowData, ADUC_Result* result);
void ADU_Integration_SandboxDestroy(const char* workflowId, const char* workFolder);

#endif // ADU_INTEGRATION_H
