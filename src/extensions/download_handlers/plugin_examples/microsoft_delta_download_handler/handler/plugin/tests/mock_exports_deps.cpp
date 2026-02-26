/**
 * @file mock_exports_deps.cpp
 * @brief Link-time mock implementations for EXPORTS.c dependencies.
 *
 * Mocks:
 *   - ADUC_Logging_Init / ADUC_Logging_Uninit
 *   - MicrosoftDeltaDownloadHandler_ProcessUpdate
 *   - MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "mock_exports_deps.h"

#include <aduc/logging.h>
#include <aduc/result.h>
#include <aduc/types/update_content.h>
#include <cstring>

typedef void* ADUC_WorkflowHandle;

MockExportsDepsState g_mockExportsDeps{};

void ResetExportsMocks(void)
{
    memset(&g_mockExportsDeps, 0, sizeof(g_mockExportsDeps));
}

extern "C" {

void ADUC_Logging_Init(ADUC_LOG_SEVERITY logLevel, const char* /* filePrefix */)
{
    g_mockExportsDeps.loggingInitCallCount++;
    g_mockExportsDeps.lastLogLevel = static_cast<int>(logLevel);
}

void ADUC_Logging_Uninit(void)
{
    g_mockExportsDeps.loggingUninitCallCount++;
}

ADUC_LOG_SEVERITY ADUC_Logging_GetLevel(void)
{
    return static_cast<ADUC_LOG_SEVERITY>(g_mockExportsDeps.lastLogLevel);
}

ADUC_Result MicrosoftDeltaDownloadHandler_ProcessUpdate(
    const ADUC_WorkflowHandle /* workflowHandle */,
    const ADUC_FileEntity* /* fileEntity */,
    const char* /* payloadFilePath */,
    const char* /* updateCacheBasePath */)
{
    g_mockExportsDeps.processUpdateCallCount++;
    return g_mockExportsDeps.processUpdateResult;
}

ADUC_Result MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted(
    const ADUC_WorkflowHandle /* workflowHandle */,
    const char* /* updateCacheBasePath */)
{
    g_mockExportsDeps.onUpdateWorkflowCompletedCallCount++;
    return g_mockExportsDeps.onUpdateWorkflowCompletedResult;
}

} /* extern "C" */
