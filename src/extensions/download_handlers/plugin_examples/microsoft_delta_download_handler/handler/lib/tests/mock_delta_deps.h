/**
 * @file mock_delta_deps.h
 * @brief Configurable link-time mock state for microsoft_delta_download_handler dependencies.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef MOCK_DELTA_DEPS_H
#define MOCK_DELTA_DEPS_H

#include <aduc/result.h>

/**
 * @brief State for MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile mock.
 *
 * Supports per-call configuration for up to 4 related files.
 */
struct MockProcessRelatedFileState
{
    /** Result codes to return for each successive call (up to 4). */
    ADUC_Result results[4];
    /** Number of entries populated in results[]. */
    int numEntries;
    /** Tracks how many times the mock has been called. */
    int callCount;
};

/**
 * @brief State for ADUC_SourceUpdateCache_Move mock.
 */
struct MockSourceUpdateCacheMoveState
{
    ADUC_Result result;
    int callCount;
};

/**
 * @brief State for workflow_add_erc mock.
 */
struct MockWorkflowAddErcState
{
    int callCount;
    ADUC_Result_t lastErc;
};

extern MockProcessRelatedFileState g_mockProcessRelatedFile;
extern MockSourceUpdateCacheMoveState g_mockSourceUpdateCacheMove;
extern MockWorkflowAddErcState g_mockWorkflowAddErc;

void ResetAllDeltaMocks(void);

#endif /* MOCK_DELTA_DEPS_H */
