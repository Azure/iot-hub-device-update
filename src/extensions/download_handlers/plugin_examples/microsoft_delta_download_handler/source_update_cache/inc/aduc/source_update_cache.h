/**
 * @file source_update_cache.c
 * @brief The source update cache for delta download handler update payloads.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef __SOURCE_UPDATE_CACHE_H__
#define __SOURCE_UPDATE_CACHE_H__

#include <aduc/result.h> // ADUC_RESULT
#include <aduc/types/workflow.h> // ADUC_WorkflowHandle
#include <azure_c_shared_utility/strings.h> // STRING_HANDLE

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
    STRING_HANDLE* outSourceUpdatePath);

/**
 * @brief Moves a payload from the sandbox into the update cache.
 *
 * @param workflowHandle The workflow handle.
 * @param updateCacheBasePath The update cache base path. Use NULL for default.
 * @return ADUC_Result The result.
 */
ADUC_Result ADUC_SourceUpdateCache_Move(const ADUC_WorkflowHandle workflowHandle, const char* updateCacheBasePath);

#endif // __SOURCE_UPDATE_CACHE_H__
