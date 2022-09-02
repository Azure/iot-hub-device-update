/**
 * @file microsoft_delta_download_handler.c
 * @brief Implementation for the delta download handler library functions used
 * by the sample libmicrosoft_delta_download_handler.so plugin.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/microsoft_delta_download_handler.h"
#include "aduc/microsoft_delta_download_handler_utils.h"
#include <aduc/logging.h> // ADUC_Logging_*, Log_*
#include <aduc/source_update_cache.h> // ADUC_SourceUpdateCache_Move
#include <aduc/string_c_utils.h> // IsNullOrEmpty
#include <aduc/types/adu_core.h> // ADUC_Result_Success, etc
#include <aduc/workflow_utils.h> // workflow_get_workfolder

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
    const char* updateCacheBasePath)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure, .ExtendedResultCode = 0 };

    // These represent hard failures for this download handler.
    // Most notably, this download handler requires related files.
    // In general, related files may be optional for a download handler.
    if (workflowHandle == NULL || fileEntity == NULL || payloadFilePath == NULL || fileEntity->RelatedFiles == NULL
        || fileEntity->RelatedFileCount <= 0)
    {
        result.ExtendedResultCode = ADUC_ERC_DDH_BAD_ARGS;
        goto done;
    }

    // Each relatedFile represents a delta update associated with a different
    // source update in the source update update cache.
    //
    // To save bandwidth (delta updates are much smaller than a full update),
    // try processing each delta update until one succeeds.
    //
    // If processing of all relatedFile fails, then return
    // ADUC_Result_Download_RequiredFullDownload success result code, which
    // will cause the agent to not fail and download the original, full update.
    for (int index = 0; index < fileEntity->RelatedFileCount; ++index)
    {
        ADUC_Result relatedFileResult = {};
        ADUC_RelatedFile* relatedFile = &fileEntity->RelatedFiles[index];

        if (relatedFile->Properties == NULL || relatedFile->PropertiesCount < 1)
        {
            result.ExtendedResultCode = ADUC_ERC_DDH_RELATEDFILE_NO_PROPERTIES;
            goto done;
        }

        relatedFileResult = MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile(
            workflowHandle,
            relatedFile,
            payloadFilePath,
            updateCacheBasePath,
            MicrosoftDeltaDownloadHandlerUtils_ProcessDeltaUpdate,
            MicrosoftDeltaDownloadHandlerUtils_DownloadDeltaUpdate);

        if (relatedFileResult.ResultCode == ADUC_Result_Success_Cache_Miss)
        {
            Log_Warn("src update cache miss for Delta %d", index);
            workflow_set_success_erc(workflowHandle, ADUC_ERC_DDH_SOURCE_UPDATE_CACHE_MISS);
            continue;
        }

        if (IsAducResultCodeSuccess(relatedFileResult.ResultCode))
        {
            Log_Info("Processing Delta %d succeeded", index);
            result.ResultCode = ADUC_Result_Success;
            break;
        }

        Log_Warn("Delta %d failed, ERC: 0x%08x.", index, relatedFileResult.ExtendedResultCode);
        workflow_set_success_erc(workflowHandle, relatedFileResult.ExtendedResultCode);
        // continue processing the next relatedFile
    }

    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        result.ResultCode = ADUC_Result_Download_Handler_SuccessSkipDownload;
    }
    else
    {
        result.ResultCode = ADUC_Result_Download_Handler_RequiredFullDownload;
    };

done:

    return result;
}

/**
 * @brief Called when the update workflow successfully completes.
 * In the case of Delta download handler plugin, it moves all the payloads from sandbox to cache
 * so that they will available as source updates for future delta updates.
 *
 * @param[in] workflowHandle The workflow handle.
 * @param[in] updateCacheBasePath The update cache base path. Use NULL for default.
 * @return ADUC_Result The result.
 */
ADUC_Result MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted(
    const ADUC_WorkflowHandle workflowHandle, const char* updateCacheBasePath)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure };

    if (workflowHandle == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_DDH_BAD_ARGS;
        goto done;
    }

    result = ADUC_SourceUpdateCache_Move(workflowHandle, updateCacheBasePath);

done:

    return result;
}
