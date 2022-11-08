/**
 * @file microsoft_delta_download_handler.h
 * @brief Function prototypes for the delta download handler library functions used
 * by the sample libmicrosoft_delta_download_handler.so plugin.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef __DELTA_DOWNLOAD_HANDLER_H__
#define __DELTA_DOWNLOAD_HANDLER_H__

#include <aduc/result.h> /* ADUC_Result */
#include <aduc/types/update_content.h> /* ADUC_FileEntity */
#include <aduc/types/workflow.h> /* ADUC_WorkflowHandle */

/**
 * @brief Processes the target update from FileEntity metadata at the given output filepath.
 * For this download handler, each relatedFile in the FileEntity metadata represents a delta update,
 * which is much smaller than the target update content. It attempts to download the delta update and
 * produce the target update using the delta processor. If successful, it tells the agent to skip download;
 * otherwise, it tells the agent that a full download is required.
 *
 * @param[in] workflowHandle The workflow handle.
 * @param[in] fileEntity The FileEntity metadata of the update content and its related files.
 * @param[in] payloadFilePath The sandbox output filepath where the update content would normally be written.
 * @param[in] updateCacheBasePath The update cache base path. Use NULL for default.
 * @return ADUC_Result The result.
 * On success, returns ADUC_Result_Download_Handler_SuccessSkipDownload to tell the
 * agent to skip downloading the update content (since it was able to produce it at the payloadFilePath).
 * On failure, returns ADUC_Result_Download_Handler_RequiredFullDownload success ResultCode
 * to tell the agent to download the update content as a fallback measure.
 */
ADUC_Result MicrosoftDeltaDownloadHandler_ProcessUpdate(
    const ADUC_WorkflowHandle workflowHandle,
    const ADUC_FileEntity* fileEntity,
    const char* payloadFilePath,
    const char* updateCacheBasePath);

/**
 * @brief Called when the update workflow successfully completes.
 * In the case of Delta download handler plugin, it moves all the payload files from download sandbox to the cache.
 *
 * @param[in] workflowHandle The workflow handle.
 * @param[in] updateCacheBasePath The update cache base path. Use NULL for default.
 * @return ADUC_Result The result.
 */
ADUC_Result MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted(
    const ADUC_WorkflowHandle workflowHandle, const char* updateCacheBasePath);

#endif /* __DELTA_DOWNLOAD_HANDLER_H__ */
