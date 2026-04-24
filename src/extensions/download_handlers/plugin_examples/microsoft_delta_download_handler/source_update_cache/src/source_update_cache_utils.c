
/**
 * @file source_update_cache_utils.c
 * @brief utils for source_update_cache
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/source_update_cache_utils.h"
#include <aduc/hash_utils.h> // ADUC_HashUtils_VerifyWithStrongestHash
#include <aduc/logging.h> // Log_*
#include <aduc/parser_utils.h> // ADUC_FileEntity_Uninit
#include <aduc/path_utils.h> // PathUtils_SanitizePathSegment
#include <aduc/result.h> // ADUC_ERC_*
#include <aduc/string_c_utils.h> // IsNullOrEmpty
#include <aduc/system_utils.h> // ADUC_SystemUtils_*
#include <aduc/types/update_content.h> // ADUC_FileEntity
#include <aduc/workflow_utils.h> // workflow_*
#include <azure_c_shared_utility/crt_abstractions.h> // mallocAndStrcpy_s, strcat_s
#include <azure_c_shared_utility/strings.h> // STRING_*
#include <stdlib.h> // free, malloc
#include <time.h> // clock_gettime, CLOCK_MONOTONIC

#include <aducpal/stdio.h> //rename

#include <libgen.h> // dirname
#include <sys/statvfs.h> // statvfs

EXTERN_C_BEGIN

/**
 * @brief Converts a base64 encoded string to a safer file name / path segment.
 * @param unencoded The base64 encoded string that has not been encoded to the file path encoding scheme.
 * @return STRING_HANDLE The file path string, or NULL on error.
 * @details Caller owns it and must call STRING_delete() when done with it.
 */
static STRING_HANDLE encodeBase64ForFilePath(const char* unencoded)
{
    char encoded[1024] = { 0 };
    char str[2] = { 0, 0 };
    STRING_HANDLE encodedHandle = NULL;
    size_t lengthUnencoded = 0;

    if (IsNullOrEmpty(unencoded))
    {
        return NULL;
    }

    // base64 alphabet has a-z, A-Z, 0-9, '+', '/', and '='.
    // For '+', '/', and '=', encode using '_' followed by 2-digit hex ascii code.

    lengthUnencoded = strlen(unencoded);
    for (size_t index = 0; index < lengthUnencoded; ++index)
    {
        switch (unencoded[index])
        {
        case '+':
            if (strcat_s(encoded, ARRAY_SIZE(encoded), "_2B") != 0)
            {
                goto done;
            }
            break;
        case '/':
            if (strcat_s(encoded, ARRAY_SIZE(encoded), "_2F") != 0)
            {
                goto done;
            }
            break;
        case '=':
            if (strcat_s(encoded, ARRAY_SIZE(encoded), "_3D") != 0)
            {
                goto done;
            }
            break;
        default:
            str[0] = unencoded[index];
            if (strcat_s(encoded, ARRAY_SIZE(encoded), str) != 0)
            {
                goto done;
            }
            break;
        }
    }

    encodedHandle = STRING_construct(encoded);
    if (encodedHandle == NULL)
    {
        goto done;
    }

done:

    return encodedHandle;
}

/**
 * @brief Creates the string file path for the cache file.
 * @param provider The updateId provider from the update metadata.
 * @param hash The related file source hash from the update metadata.
 * @param alg The related file source hash algorithm from the update metadata.
 * @param updateCacheBasePath The path to the base of update cache. NULL for default.
 * @return STRING_HANDLE The file path string. Caller owns it and must call free() when done with it.
 */
STRING_HANDLE ADUC_SourceUpdateCacheUtils_CreateSourceUpdateCachePath(
    const char* provider, const char* hash, const char* alg, const char* updateCacheBasePath)
{
    STRING_HANDLE resultPath = NULL;

    STRING_HANDLE sanitizedProvider = NULL;
    STRING_HANDLE sanitizedHashAlgorithm = NULL;
    STRING_HANDLE encodedHash = NULL;

    // file path format:
    //     {base_dir}/{provider}/{hashAlg}-{hash}

    sanitizedProvider = PathUtils_SanitizePathSegment(provider);
    if (sanitizedProvider == NULL)
    {
        goto done;
    }

    sanitizedHashAlgorithm = PathUtils_SanitizePathSegment(alg);
    if (sanitizedHashAlgorithm == NULL)
    {
        goto done;
    }

    encodedHash = encodeBase64ForFilePath(hash);
    if (encodedHash == NULL)
    {
        goto done;
    }

    resultPath = STRING_construct_sprintf(
        "%s/%s/%s-%s",
        IsNullOrEmpty(updateCacheBasePath) ? ADUC_DELTA_DOWNLOAD_HANDLER_SOURCE_UPDATE_CACHE_DIR : updateCacheBasePath,
        STRING_c_str(sanitizedProvider),
        STRING_c_str(sanitizedHashAlgorithm),
        STRING_c_str(encodedHash));

done:
    STRING_delete(sanitizedProvider);
    STRING_delete(sanitizedHashAlgorithm);
    STRING_delete(encodedHash);

    return resultPath;
}

/**
 * @brief Moves all payloads of the current update from the download sandbox work folder to the update cache.
 * @param workflowHandle The workflow handle.
 * @param updateCacheBasePath The path to the base of update cache. NULL for default.
 * @return ADUC_Result The result.
 */
ADUC_Result ADUC_SourceUpdateCacheUtils_MoveToUpdateCache(
    const ADUC_WorkflowHandle workflowHandle, const char* updateCacheBasePath)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure };
    int res = -1;
    ADUC_FileEntity fileEntity;
    memset(&fileEntity, 0, sizeof(fileEntity));
    STRING_HANDLE sandboxUpdatePayloadFile = NULL;
    ADUC_UpdateId* updateId = NULL;
    STRING_HANDLE updateCacheFilePath = NULL;
    char dirPath[1024] = "";
    struct timespec funcStartTime, funcEndTime;
    struct timespec fileStartTime, fileEndTime;
    long totalCopyMs = 0;

    clock_gettime(CLOCK_MONOTONIC, &funcStartTime);

    size_t countPayloads = workflow_get_update_files_count(workflowHandle);
    Log_Info("[TIMING] MoveToUpdateCache: Processing %zu payload(s)", countPayloads);

    for (size_t index = 0; index < countPayloads; ++index)
    {
        clock_gettime(CLOCK_MONOTONIC, &fileStartTime);

        if (!workflow_get_update_file(workflowHandle, index, &fileEntity))
        {
            Log_Error("[DELTA] Failed to get update file %d", index);
            goto done;
        }

        // Validate fileEntity has hash information to prevent segfaults
        if (fileEntity.Hash == NULL || fileEntity.HashCount == 0)
        {
            Log_Error("[DELTA] FileEntity %d has no hash information (Hash=%p, HashCount=%d)",
                      index, fileEntity.Hash, fileEntity.HashCount);
            result.ExtendedResultCode = ADUC_ERC_NOTRECOVERABLE;
            goto done;
        }

        if (fileEntity.Hash[0].value == NULL || fileEntity.Hash[0].type == NULL)
        {
            Log_Error("[DELTA] FileEntity %d has invalid hash data (value=%p, type=%p)",
                      index, fileEntity.Hash[0].value, fileEntity.Hash[0].type);
            result.ExtendedResultCode = ADUC_ERC_NOTRECOVERABLE;
            goto done;
        }

        workflow_get_entity_workfolder_filepath(workflowHandle, &fileEntity, &sandboxUpdatePayloadFile);

        if (sandboxUpdatePayloadFile == NULL)
        {
            Log_Error("[DELTA] Failed to get workfolder filepath for file %d", index);
            result.ExtendedResultCode = ADUC_ERC_NOTRECOVERABLE;
            goto done;
        }

        result = workflow_get_expected_update_id(workflowHandle, &updateId);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            Log_Error("[DELTA] Failed to get updateId, erc 0x%08x", result.ExtendedResultCode);
            goto done;
        }

        if (updateId == NULL || updateId->Provider == NULL)
        {
            Log_Error("[DELTA] Invalid updateId (updateId=%p, Provider=%p)",
                      updateId, updateId ? updateId->Provider : NULL);
            result.ExtendedResultCode = ADUC_ERC_NOTRECOVERABLE;
            goto done;
        }

        const char* provider = updateId->Provider;
        const char* hash = (fileEntity.Hash[0]).value;
        const char* alg = (fileEntity.Hash[0]).type;

        updateCacheFilePath =
            ADUC_SourceUpdateCacheUtils_CreateSourceUpdateCachePath(provider, hash, alg, updateCacheBasePath);
        if (updateCacheFilePath == NULL)
        {
            result.ExtendedResultCode = ADUC_ERC_MOVE_CREATE_CACHE_PATH;
            goto done;
        }

        // Check if file already exists in cache with valid hash
        // This handles the case where CacheSourceUpdate was called before reboot
        // and OnUpdateWorkflowCompleted is now being called after reboot
        if (SystemUtils_IsFile(STRING_c_str(updateCacheFilePath), NULL))
        {
            // Verify the cached file hash matches the expected hash from FileEntity
            // Note: fileEntity.Hash was already validated at the start of the loop
            if (ADUC_HashUtils_VerifyWithStrongestHash(
                    STRING_c_str(updateCacheFilePath), fileEntity.Hash, fileEntity.HashCount))
            {
                // Cache file exists and hash is valid - already cached correctly
                Log_Info(
                    "[DELTA] File already cached at '%s' with valid hash - skipping",
                    STRING_c_str(updateCacheFilePath));

                ADUC_FileEntity_Uninit(&fileEntity);
                ADUC_UpdateId_UninitAndFree(updateId);
                updateId = NULL;
                STRING_delete(updateCacheFilePath);
                updateCacheFilePath = NULL;
                STRING_delete(sandboxUpdatePayloadFile);
                sandboxUpdatePayloadFile = NULL;
                continue; // Skip to next file
            }
            else
            {
                // Cache file exists but hash verification failed - will overwrite
                Log_Warn(
                    "[DELTA] Cache file exists at '%s' but hash verification failed - will overwrite",
                    STRING_c_str(updateCacheFilePath));
            }
        }

        if (strcpy_s(dirPath, ARRAY_SIZE(dirPath), STRING_c_str(updateCacheFilePath)) != 0)
        {
            result.ExtendedResultCode = ADUC_ERC_NOTRECOVERABLE;
            goto done;
        }

        const char* dirPathCache = dirname(dirPath); // free() not needed
        if (dirPathCache == NULL)
        {
            result.ExtendedResultCode = ADUC_ERC_NOTRECOVERABLE;
            goto done;
        }

        if (ADUC_SystemUtils_MkDirRecursiveDefault(dirPathCache) != 0)
        {
            result.ExtendedResultCode = ADUC_ERC_MOVE_CREATE_CACHE_PATH;
            goto done;
        }

        // Check available disk space at destination
        struct statvfs vfs;
        if (statvfs(dirPathCache, &vfs) == 0)
        {
            unsigned long long availableBytes = (unsigned long long)vfs.f_bavail * vfs.f_frsize;
            double availableMB = availableBytes / (1024.0 * 1024.0);
            Log_Info(
                "[DELTA] Destination '%s' has %.2f MB available (%.2f GB)",
                dirPathCache,
                availableMB,
                availableMB / 1024.0);
        }
        else
        {
            Log_Warn("[DELTA] Failed to get disk space for destination '%s', errno: %d", dirPathCache, errno);
        }

        // First try to move the file.
        // errno EXDEV would be common if copying across different mount points.
        // For any failure, it falls back to copy.

        Log_Debug("moving '%s' -> '%s'", STRING_c_str(sandboxUpdatePayloadFile), STRING_c_str(updateCacheFilePath));

        res = ADUCPAL_rename(STRING_c_str(sandboxUpdatePayloadFile), STRING_c_str(updateCacheFilePath));
        if (res != 0)
        {
            Log_Warn("[DELTA] Rename failed with errno %d (EXDEV=%d) - falling back to copy", errno, EXDEV);

            // fallback to file-to-file copy with explicit target path
            // Note: Cannot use CopyFileToDir as it uses basename from source which would not match our hash-based filename
            FILE* sourceFile = fopen(STRING_c_str(sandboxUpdatePayloadFile), "rb");
            if (sourceFile == NULL)
            {
                Log_Error("[DELTA] Failed to open source file '%s' for reading: errno %d", STRING_c_str(sandboxUpdatePayloadFile), errno);
                result.ExtendedResultCode = ADUC_ERC_MOVE_COPYFALLBACK;
                goto done;
            }

            FILE* destFile = fopen(STRING_c_str(updateCacheFilePath), "wb");
            if (destFile == NULL)
            {
                Log_Error("[DELTA] Failed to open destination file '%s' for writing: errno %d", STRING_c_str(updateCacheFilePath), errno);
                fclose(sourceFile);
                result.ExtendedResultCode = ADUC_ERC_MOVE_COPYFALLBACK;
                goto done;
            }

            // Copy file contents
            unsigned char buffer[8192];
            size_t bytesRead;
            bool copyFailed = false;
            while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0)
            {
                if (fwrite(buffer, 1, bytesRead, destFile) != bytesRead)
                {
                    Log_Error("[DELTA] Write failed during copy to '%s': errno %d", STRING_c_str(updateCacheFilePath), errno);
                    copyFailed = true;
                    break;
                }
            }

            // Check for read errors after loop
            if (ferror(sourceFile))
            {
                Log_Error("[DELTA] Read error during copy from '%s': errno %d", STRING_c_str(sandboxUpdatePayloadFile), errno);
                copyFailed = true;
            }

            fclose(sourceFile);
            fclose(destFile);

            if (copyFailed)
            {
                // Clean up the partially written destination file
                Log_Warn("[DELTA] Removing incomplete destination file: '%s'", STRING_c_str(updateCacheFilePath));
                unlink(STRING_c_str(updateCacheFilePath));
                result.ExtendedResultCode = ADUC_ERC_MOVE_COPYFALLBACK;
                goto done;
            }

            Log_Info("[DELTA] File copy completed from '%s' to '%s'", STRING_c_str(sandboxUpdatePayloadFile), STRING_c_str(updateCacheFilePath));

            // Verify the copied file has the expected hash to ensure copy succeeded
            // Note: fileEntity.Hash was already validated at the start of the loop
            Log_Debug("Verifying copied file integrity: '%s'", STRING_c_str(updateCacheFilePath));
            if (!ADUC_HashUtils_VerifyWithStrongestHash(
                    STRING_c_str(updateCacheFilePath),
                    fileEntity.Hash,
                    fileEntity.HashCount))
            {
                Log_Error("[DELTA] Hash verification failed after copy - file may be corrupted or zero-length");
                // Clean up the invalid file
                Log_Warn("[DELTA] Removing file with invalid hash: '%s'", STRING_c_str(updateCacheFilePath));
                unlink(STRING_c_str(updateCacheFilePath));
                result.ExtendedResultCode = ADUC_ERC_MOVE_HASH_VERIFICATION_FAILED;
                goto done;
            }
            Log_Info("[DELTA] Copy and hash verification succeeded for '%s'", STRING_c_str(updateCacheFilePath));
        }

        // Create metadata .info file
        STRING_HANDLE infoFilePath = STRING_construct(STRING_c_str(updateCacheFilePath));
        if (infoFilePath != NULL)
        {
            if (STRING_concat(infoFilePath, ".info") == 0)
            {
                // Get current timestamp
                time_t now = time(NULL);
                struct tm* tm_info = gmtime(&now);
                char timestamp[64];
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", tm_info);

                // Build JSON metadata
                char metadata[2048];
                snprintf(
                    metadata,
                    sizeof(metadata),
                    "{\n"
                    "  \"originalFilename\": \"%s\",\n"
                    "  \"cachedTimestamp\": \"%s\",\n"
                    "  \"provider\": \"%s\",\n"
                    "  \"sourceHash\": \"%s\",\n"
                    "  \"hashAlgorithm\": \"%s\",\n"
                    "  \"fileSize\": %zu,\n"
                    "  \"sourceUpdateId\": {\n"
                    "    \"provider\": \"%s\",\n"
                    "    \"name\": \"%s\",\n"
                    "    \"version\": \"%s\"\n"
                    "  }\n"
                    "}\n",
                    fileEntity.TargetFilename,
                    timestamp,
                    provider,
                    hash,
                    alg,
                    fileEntity.SizeInBytes,
                    updateId->Provider,
                    updateId->Name,
                    updateId->Version);

                if (ADUC_SystemUtils_WriteStringToFile(STRING_c_str(infoFilePath), metadata) == 0)
                {
                    Log_Info("[DELTA] Created metadata file: %s", STRING_c_str(infoFilePath));
                }
                else
                {
                    Log_Warn("[DELTA] Failed to create metadata file: %s", STRING_c_str(infoFilePath));
                }
            }
            STRING_delete(infoFilePath);
        }

        clock_gettime(CLOCK_MONOTONIC, &fileEndTime);
        long fileMs = (fileEndTime.tv_sec - fileStartTime.tv_sec) * 1000 +
                      (fileEndTime.tv_nsec - fileStartTime.tv_nsec) / 1000000;
        totalCopyMs += fileMs;
        Log_Info("[TIMING] MoveToUpdateCache: File %zu cached in %ld ms", index, fileMs);

        ADUC_FileEntity_Uninit(&fileEntity);

        ADUC_UpdateId_UninitAndFree(updateId);
        updateId = NULL;

        STRING_delete(updateCacheFilePath);
        updateCacheFilePath = NULL;

        STRING_delete(sandboxUpdatePayloadFile);
        sandboxUpdatePayloadFile = NULL;
    }

    result.ResultCode = ADUC_Result_Success;

    clock_gettime(CLOCK_MONOTONIC, &funcEndTime);
    long totalMs = (funcEndTime.tv_sec - funcStartTime.tv_sec) * 1000 +
                   (funcEndTime.tv_nsec - funcStartTime.tv_nsec) / 1000000;
    Log_Info("[TIMING] MoveToUpdateCache: Completed %zu file(s) in %ld ms total (copy time: %ld ms)",
             countPayloads, totalMs, totalCopyMs);

done:
    ADUC_FileEntity_Uninit(&fileEntity);
    ADUC_UpdateId_UninitAndFree(updateId);
    STRING_delete(sandboxUpdatePayloadFile);
    STRING_delete(updateCacheFilePath);

    return result;
}

EXTERN_C_END
