
/**
 * @file source_update_cache_utils.c
 * @brief utils for source_update_cache
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/source_update_cache_utils.h"
#include <aduc/path_utils.h> // SanitizePathSegment
#include <aduc/string_c_utils.h> // IsNullOrEmpty
#include <aduc/system_utils.h> // ADUC_SystemUtils_*
#include <aduc/types/update_content.h> // ADUC_FileEntity
#include <aduc/workflow_utils.h> // workflow_*
#include <azure_c_shared_utility/crt_abstractions.h> // mallocAndStrcpy_s, strcat_s
#include <azure_c_shared_utility/strings.h> // STRING_*
#include <libgen.h> // dirname
#include <stdlib.h> // free, malloc

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
    for (int index = 0; index < lengthUnencoded; ++index)
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

    sanitizedProvider = SanitizePathSegment(provider);
    if (sanitizedProvider == NULL)
    {
        goto done;
    }

    sanitizedHashAlgorithm = SanitizePathSegment(alg);
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
    ADUC_FileEntity* fileEntity = NULL;
    STRING_HANDLE sandboxUpdatePayloadFile = NULL;
    ADUC_UpdateId* updateId = NULL;
    STRING_HANDLE updateCacheFilePath = NULL;
    char dirPath[1024] = "";

    size_t countPayloads = workflow_get_update_files_count(workflowHandle);
    for (size_t index = 0; index < countPayloads; ++index)
    {
        workflow_free_file_entity(fileEntity);
        fileEntity = NULL;

        if (!workflow_get_update_file(workflowHandle, index, &fileEntity))
        {
            Log_Error("get update file %d", index);
            goto done;
        }

        workflow_get_entity_workfolder_filepath(workflowHandle, fileEntity, &sandboxUpdatePayloadFile);

        result = workflow_get_expected_update_id(workflowHandle, &updateId);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            Log_Error("get updateId, erc 0x%08x", result.ExtendedResultCode);
            goto done;
        }

        // When update is already installed, payloads would not be downloaded but it would still
        // attempt to move to cache with OnUpdateWorkflowCompleted contract call because overall
        // is it Apply Success result, so guard against non-existent sandbox file.
        if (!SystemUtils_IsFile(STRING_c_str(sandboxUpdatePayloadFile), NULL))
        {
            result.ExtendedResultCode = ADUC_ERC_MISSING_SOURCE_SANDBOX_FILE;
            goto done;
        }

        const char* provider = updateId->Provider;
        const char* hash = (fileEntity->Hash[0]).value;
        const char* alg = (fileEntity->Hash[0]).type;

        updateCacheFilePath =
            ADUC_SourceUpdateCacheUtils_CreateSourceUpdateCachePath(provider, hash, alg, updateCacheBasePath);
        if (updateCacheFilePath == NULL)
        {
            result.ExtendedResultCode = ADUC_ERC_MOVE_CREATE_CACHE_PATH;
            goto done;
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

        // First try to move the file.
        // errno EXDEV would be common if copying across different mount points.
        // For any failure, it falls back to copy.

        Log_Debug(
            "moving '%s' -> '%s'", STRING_c_str(sandboxUpdatePayloadFile), STRING_c_str(updateCacheFilePath));

        res = rename(STRING_c_str(sandboxUpdatePayloadFile), STRING_c_str(updateCacheFilePath));
        if (res != 0)
        {
            Log_Warn("rename, errno %d", errno);

            // fallback to copy
            //
            if (ADUC_SystemUtils_CopyFileToDir(
                    STRING_c_str(sandboxUpdatePayloadFile), dirPathCache, false /* overwriteExistingFile */)
                != 0)
            {
                Log_Error("Copy Failed");
                result.ExtendedResultCode = ADUC_ERC_MOVE_COPYFALLBACK;
                goto done;
            }
        }

        STRING_delete(updateCacheFilePath);
        updateCacheFilePath = NULL;
    }

    result.ResultCode = ADUC_Result_Success;

done:
    STRING_delete(sandboxUpdatePayloadFile);
    STRING_delete(updateCacheFilePath);
    workflow_free_file_entity(fileEntity);

    return result;
}

EXTERN_C_END
