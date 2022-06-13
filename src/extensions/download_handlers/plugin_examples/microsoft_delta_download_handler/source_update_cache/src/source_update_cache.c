
/**
 * @file source_update_cache.cpp
 * @brief File cache for updates used as a source for delta update processing.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/source_update_cache.h"
#include "aduc/source_update_cache_utils.h" // ADUC_SourceUpdateCacheUtils_CreateSourceUpdateCachePath
#include <aduc/permission_utils.h> // PermissionUtils_VerifyFilemodeBitmask
#include <aduc/system_utils.h> // SystemUtils_IsFile, ADUC_SystemUtils_MkDirRecursiveAduUser
#include <aduc/types/adu_core.h> // ADUC_Result_Success_Cache_Miss
#include <azure_c_shared_utility/crt_abstractions.h> // for mallocAndStrcpy_s
#include <azure_c_shared_utility/strings.h>
#include <libgen.h> // dirname
#include <stdio.h> // rename
#include <stdlib.h> // free
#include <sys/stat.h> // S_IRUSR

/**
 * @brief Looks up a source update from the source update cache.
 *
 * @param updateIdProvider The provider of the UpdateId.
 * @param sourceUpdateHash The hash of the source update that acts as the key for the cache entry.
 * @param sourceUpdateAlgorithm The hash algorithm of the source update that makes up the other half of the composite key.
 * @param updateCacheBasePath The update cache base path. Use NULL for default.
 * @param outSourceUpdatePath The output filepath of the source update in the cache.
 * @return ADUC_Result The result. On success that is not ADUC_Result_Success_Cache_Miss, outSourceUpdatePath will have a path to the source update file.
 */
ADUC_Result ADUC_SourceUpdateCache_Lookup(
    const char* updateIdProvider,
    const char* sourceUpdateHash,
    const char* sourceUpdateAlgorithm,
    const char* updateCacheBasePath,
    STRING_HANDLE* outSourceUpdatePath)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure };

    STRING_HANDLE filePath = ADUC_SourceUpdateCacheUtils_CreateSourceUpdateCachePath(
        updateIdProvider, sourceUpdateHash, sourceUpdateAlgorithm, updateCacheBasePath);
    if (filePath == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_LOOKUP_CREATE_PATH;
        goto done;
    }

    // file exists and is readable
    if (!SystemUtils_IsFile(STRING_c_str(filePath), NULL)
        || !PermissionUtils_VerifyFilemodeBitmask(STRING_c_str(filePath), S_IRUSR))
    {
        result.ResultCode = ADUC_Result_Success_Cache_Miss;
        goto done;
    }

    *outSourceUpdatePath = filePath;
    filePath = NULL;

    result.ResultCode = ADUC_Result_Success;

done:
    STRING_delete(filePath);

    return result;
}

static ADUC_Result getPayloadTotalSize(const ADUC_WorkflowHandle workflowHandle, off_t* outSize)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure };
    *outSize = 0;
    return result;
}

/**
 * @brief Moves all payloads from the download sandbox work folder to the cache.
 *
 * @param workflowHandle The workflow handle.
 * @param updateCacheBasePath The update cache base path. Use NULL for default.
 * @return ADUC_Result The result.
 */
ADUC_Result ADUC_SourceUpdateCache_Move(const ADUC_WorkflowHandle workflowHandle, const char* updateCacheBasePath)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure };

    off_t spaceRequired = 0;
    result = getPayloadTotalSize(workflowHandle, &spaceRequired);

    int res = -1;

#ifndef TWO_PHASE_COMMIT
    // When NOT two-phase commit, proactively make space by pre-purging cache dir of oldest files upto size of sandboxFilePath.
    res = ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(workflowHandle, spaceRequired, updateCacheBasePath);
    if (res != 0)
    {
        Log_Error("pre-purge failed, res %d", res);
        result.ExtendedResultCode = ADUC_ERC_MOVE_PREPURGE;
        goto done;
    }
#endif

    result = ADUC_SourceUpdateCacheUtils_MoveToUpdateCache(workflowHandle, updateCacheBasePath);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("Failed to move sandbox payloads to update cache. erc: %d", result.ExtendedResultCode);
        result.ExtendedResultCode = ADUC_ERC_MOVE_PAYLOAD;
        goto done;
    }

#ifdef TWO_PHASE_COMMIT
    // In the case of two-phase commit, purge cache dir of oldest files upto size of sandboxFilePath AFTER move/copy.
    res = ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(workflowHandle, spaceRequired, updateCacheBasePath);
    if (res != 0)
    {
        Log_Error("post-purge failed, res %d", res);
        result.ExtendedResultCode = ADUC_ERC_MOVE_POSTPURGE;
        goto done;
    }
#endif

    result.ResultCode = ADUC_Result_Success;

done:

    return result;
}
