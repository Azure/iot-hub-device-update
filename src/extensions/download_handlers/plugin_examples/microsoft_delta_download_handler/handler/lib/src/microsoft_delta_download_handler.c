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
#include <time.h> // clock_gettime, CLOCK_MONOTONIC

/**
 * @brief Helper to get elapsed time in milliseconds between two timespecs.
 */
static long get_elapsed_ms(struct timespec* start, struct timespec* end)
{
    return (end->tv_sec - start->tv_sec) * 1000 + (end->tv_nsec - start->tv_nsec) / 1000000;
}

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

    // Validate required parameters
    if (workflowHandle == NULL || fileEntity == NULL || payloadFilePath == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_DDH_BAD_ARGS;
        goto done;
    }

    // If no relatedFiles, this is a source-only update meant to be cached for future delta updates.
    // This is typical for the first update in a delta update sequence (e.g., V1 that will be
    // used as source for V2 delta). Agent will download the full update and cache it via either
    // CacheSourceUpdate (before reboot) or OnUpdateWorkflowCompleted (after successful install).
    if (fileEntity->RelatedFiles == NULL || fileEntity->RelatedFileCount <= 0)
    {
        Log_Info("[DELTA] No relatedFiles specified - treating as source update for future delta updates");
        Log_Info("[DELTA] Agent will download full update and cache it via CacheSourceUpdate or OnUpdateWorkflowCompleted");
        result.ResultCode = ADUC_Result_Download_Handler_RequiredFullDownload;
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
        ADUC_Result relatedFileResult;
        memset(&relatedFileResult, 0, sizeof(relatedFileResult));
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
            Log_Warn("[DELTA] Source update cache miss for delta %d", index);
            workflow_add_erc(workflowHandle, ADUC_ERC_DDH_SOURCE_UPDATE_CACHE_MISS);
            continue;
        }

        if (IsAducResultCodeSuccess(relatedFileResult.ResultCode))
        {
            Log_Info("[DELTA] Processing delta %d succeeded", index);

            // Log bandwidth savings achieved through delta update
            size_t deltaSize = relatedFile->SizeInBytes;
            size_t fullUpdateSize = fileEntity->SizeInBytes;
            if (deltaSize > 0 && fullUpdateSize > 0 && fullUpdateSize > deltaSize)
            {
                size_t bytesSaved = fullUpdateSize - deltaSize;
                unsigned int percentSaved = (unsigned int)((bytesSaved * 100) / fullUpdateSize);
                Log_Info(
                    "[DELTA] Reconstruction SUCCESS - Downloaded %zu bytes (delta) instead of %zu bytes (full), saved %zu bytes (%u%%)",
                    deltaSize,
                    fullUpdateSize,
                    bytesSaved,
                    percentSaved);
            }
            else
            {
                Log_Info("[DELTA] Reconstruction SUCCESS - Delta processing completed");
            }

            result.ResultCode = ADUC_Result_Success;
            break;
        }

        Log_Warn("[DELTA] Delta %d failed, ERC: 0x%08x", index, relatedFileResult.ExtendedResultCode);
        workflow_add_erc(workflowHandle, relatedFileResult.ExtendedResultCode);
        // continue processing the next relatedFile
    }

    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        result.ResultCode = ADUC_Result_Download_Handler_SuccessSkipDownload;
    }
    else
    {
        Log_Warn(
            "[DELTA] Reconstruction FAILED for all %d delta(s) - falling back to full download (%zu bytes)",
            fileEntity->RelatedFileCount,
            fileEntity->SizeInBytes);
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
    struct timespec startTime, endTime;

    if (workflowHandle == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_DDH_BAD_ARGS;
        goto done;
    }

    Log_Info("[TIMING] OnUpdateWorkflowCompleted: Starting cache operation");
    clock_gettime(CLOCK_MONOTONIC, &startTime);

    result = ADUC_SourceUpdateCache_Move(workflowHandle, updateCacheBasePath);

    clock_gettime(CLOCK_MONOTONIC, &endTime);
    Log_Info("[TIMING] OnUpdateWorkflowCompleted: Cache operation completed in %ld ms (rc: %d, erc: 0x%08x)",
             get_elapsed_ms(&startTime, &endTime), result.ResultCode, result.ExtendedResultCode);

done:

    return result;
}

/**
 * @brief Called to immediately cache source updates before system reboot/restart.
 * This is invoked by the workflow before initiating a reboot to ensure source files
 * are cached while the sandbox directory still exists. Unlike OnUpdateWorkflowCompleted,
 * this function performs unconditional caching without checking if update is already installed.
 *
 * @param[in] workflowHandle The workflow handle.
 * @param[in] updateCacheBasePath The update cache base path. Use NULL for default.
 * @return ADUC_Result The result.
 */
ADUC_Result MicrosoftDeltaDownloadHandler_CacheSourceUpdate(
    const ADUC_WorkflowHandle workflowHandle, const char* updateCacheBasePath)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure };
    struct timespec startTime, endTime;

    if (workflowHandle == NULL)
    {
        Log_Error("[DELTA] CacheSourceUpdate called with NULL workflowHandle");
        result.ExtendedResultCode = ADUC_ERC_DDH_BAD_ARGS;
        goto done;
    }

    Log_Info("[TIMING] CacheSourceUpdate: Starting pre-reboot cache operation");
    Log_Info("[DELTA] updateCacheBasePath = %s", IsNullOrEmpty(updateCacheBasePath) ? "NULL (will use default)" : updateCacheBasePath);

    clock_gettime(CLOCK_MONOTONIC, &startTime);

    // Immediately cache source updates - same as OnUpdateWorkflowCompleted but called at different time
    Log_Info("[DELTA] Calling ADUC_SourceUpdateCache_Move...");
    result = ADUC_SourceUpdateCache_Move(workflowHandle, updateCacheBasePath);

    clock_gettime(CLOCK_MONOTONIC, &endTime);
    long elapsed_ms = get_elapsed_ms(&startTime, &endTime);

    Log_Info("[TIMING] CacheSourceUpdate: Cache operation completed in %ld ms", elapsed_ms);
    Log_Info("[DELTA] ADUC_SourceUpdateCache_Move returned - rc: %d, erc: 0x%08x", result.ResultCode, result.ExtendedResultCode);

    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        Log_Info("[TIMING] CacheSourceUpdate: SUCCESS - Source update cached in %ld ms before reboot", elapsed_ms);
    }
    else
    {
        Log_Error("[TIMING] CacheSourceUpdate: FAILED after %ld ms - rc: %d, erc: 0x%08x",
                  elapsed_ms, result.ResultCode, result.ExtendedResultCode);
    }

done:
    Log_Info("[DELTA] CacheSourceUpdate returning with rc: %d, erc: 0x%08x", result.ResultCode, result.ExtendedResultCode);
    return result;
}
