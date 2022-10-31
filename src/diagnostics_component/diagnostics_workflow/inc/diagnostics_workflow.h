/**
 * @file diagnostics_workflow.h
 * @brief Header for functions handling the Diagnostics Log Upload workflow
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef DIAGNOSTICS_WORKFLOW_H
#define DIAGNOSTICS_WORKFLOW_H

#include <aduc/c_utils.h>
#include <diagnostics_config_utils.h>

EXTERN_C_BEGIN

/**
 * @brief Uploads the diagnostic logs described by @p workflowData
 * @param workflowData the workflowData structure describing the log components
 * @param jsonString the string from the diagnostics_interface describing where to upload the logs
 */
void DiagnosticsWorkflow_DiscoverAndUploadLogs(const DiagnosticsWorkflowData* workflowData, const char* jsonString);

EXTERN_C_END

#endif // DIAGNOSTICS_WORKFLOW_H
