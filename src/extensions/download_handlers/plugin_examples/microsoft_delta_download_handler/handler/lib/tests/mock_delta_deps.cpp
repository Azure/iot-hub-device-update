/**
 * @file mock_delta_deps.cpp
 * @brief Link-time mock implementations for microsoft_delta_download_handler dependencies.
 *
 * Provides mock implementations for:
 *   - MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile
 *   - MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate
 *   - MicrosoftDeltaDownloadHandlerUtils_DownloadDeltaUpdate
 *   - ADUC_SourceUpdateCache_Move
 *   - workflow_add_erc
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "mock_delta_deps.h"

#include <aduc/result.h>
#include <aduc/types/update_content.h>
#include <cstring>

typedef void* ADUC_WorkflowHandle;

MockProcessRelatedFileState g_mockProcessRelatedFile{};
MockSourceUpdateCacheMoveState g_mockSourceUpdateCacheMove{};
MockWorkflowAddErcState g_mockWorkflowAddErc{};

void ResetAllDeltaMocks(void)
{
    memset(&g_mockProcessRelatedFile, 0, sizeof(g_mockProcessRelatedFile));
    memset(&g_mockSourceUpdateCacheMove, 0, sizeof(g_mockSourceUpdateCacheMove));
    memset(&g_mockWorkflowAddErc, 0, sizeof(g_mockWorkflowAddErc));
}

/*
 * Type aliases matching the real function pointer types.
 */
typedef ADUC_Result (*ProcessDeltaUpdateFn)(
    const char* sourceUpdateFilePath, const char* deltaUpdateFilePath, const char* targetUpdateFilePath);
typedef ADUC_Result (*DownloadDeltaUpdateFn)(
    const ADUC_WorkflowHandle workflowHandle, const ADUC_RelatedFile* relatedFile);

extern "C" {

ADUC_Result MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile(
    const ADUC_WorkflowHandle /* workflowHandle */,
    const ADUC_RelatedFile* /* relatedFile */,
    const char* /* payloadFilePath */,
    const char* /* updateCacheBasePath */,
    ProcessDeltaUpdateFn /* processDeltaUpdateFn */,
    DownloadDeltaUpdateFn /* downloadDeltaUpdateFn */)
{
    int idx = g_mockProcessRelatedFile.callCount;
    g_mockProcessRelatedFile.callCount++;

    if (idx < g_mockProcessRelatedFile.numEntries)
    {
        return g_mockProcessRelatedFile.results[idx];
    }

    /* Default: failure */
    ADUC_Result r = { 0, 0 };
    return r;
}

ADUC_Result MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate(
    const char* /* sourceUpdateFilePath */,
    const char* /* deltaUpdateFilePath */,
    const char* /* targetUpdateFilePath */)
{
    /* Stub — only passed as function pointer, never called by the mock ProcessRelatedFile. */
    ADUC_Result r = { 0, 0 };
    return r;
}

ADUC_Result MicrosoftDeltaDownloadHandlerUtils_DownloadDeltaUpdate(
    const ADUC_WorkflowHandle /* workflowHandle */,
    const ADUC_RelatedFile* /* relatedFile */)
{
    /* Stub — only passed as function pointer, never called by the mock ProcessRelatedFile. */
    ADUC_Result r = { 0, 0 };
    return r;
}

ADUC_Result ADUC_SourceUpdateCache_Move(
    const ADUC_WorkflowHandle /* workflowHandle */,
    const char* /* updateCacheBasePath */)
{
    g_mockSourceUpdateCacheMove.callCount++;
    return g_mockSourceUpdateCacheMove.result;
}

void workflow_add_erc(ADUC_WorkflowHandle /* handle */, ADUC_Result_t erc)
{
    g_mockWorkflowAddErc.callCount++;
    g_mockWorkflowAddErc.lastErc = erc;
}

} /* extern "C" */
