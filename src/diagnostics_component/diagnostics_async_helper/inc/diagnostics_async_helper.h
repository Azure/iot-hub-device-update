/**
 * @file diagnostics_async_helper.h
 * @brief Header file for the asynchronous runner for the DiagnosticsWorkflow log upload
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/c_utils.h>
#include <stdlib.h>

#ifndef DIAGNOSTICS_ASYNC_HELPER_H
#    define DIAGNOSTICS_ASYNC_HELPER_H

EXTERN_C_BEGIN

typedef struct tagDiagnosticWorkflowData DiagnosticsWorkflowData;

void DiagnosticsWorkflow_DiscoverAndUploadLogsAsync(
    const DiagnosticsWorkflowData* workflowData, const char* jsonString);

EXTERN_C_END

#endif // DIAGNOSTICS_ASYNC_HELPER_H
