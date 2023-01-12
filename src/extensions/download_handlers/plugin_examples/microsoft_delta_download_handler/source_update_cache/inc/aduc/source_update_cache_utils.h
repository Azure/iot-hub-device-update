/**
 * @file source_update_cache.c
 * @brief The source update cache for delta download handler update payloads.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef SOURCE_UPDATE_CACHE_UTILS_H
#define SOURCE_UPDATE_CACHE_UTILS_H

#include <aduc/c_utils.h> // EXTERN_C_*
#include <aduc/result.h> // ADUC_Result
#include <aduc/types/workflow.h> // ADUC_WorkflowHandle
#include <azure_c_shared_utility/strings.h> // STRING_HANDLE
#include <sys/types.h> // off_t

EXTERN_C_BEGIN

STRING_HANDLE ADUC_SourceUpdateCacheUtils_CreateSourceUpdateCachePath(
    const char* provider, const char* hash, const char* alg, const char* updateCacheBasePath);

ADUC_Result ADUC_SourceUpdateCacheUtils_MoveToUpdateCache(
    const ADUC_WorkflowHandle workflowHandle, const char* updateCacheBasePath);

int ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(
    const ADUC_WorkflowHandle workflowHandle, off_t totalSize, const char* updateCacheBasePath);

EXTERN_C_END

#endif // SOURCE_UPDATE_CACHE_UTILS_H
