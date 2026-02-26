/**
 * @file mock_source_update_cache_deps.c
 * @brief Link-time mock implementations for source_update_cache.c dependencies.
 *
 * Mocks:
 *   - ADUC_SourceUpdateCacheUtils_CreateSourceUpdateCachePath
 *   - SystemUtils_IsFile
 *   - PermissionUtils_VerifyFilemodeBitmask
 *   - ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache
 *   - ADUC_SourceUpdateCacheUtils_MoveToUpdateCache
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "mock_source_update_cache_deps.h"

#include <azure_c_shared_utility/strings.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

struct MockSourceUpdateCacheDepsState g_mockSucState;

void ResetSourceUpdateCacheMocks(void)
{
    memset(&g_mockSucState, 0, sizeof(g_mockSucState));
}

STRING_HANDLE ADUC_SourceUpdateCacheUtils_CreateSourceUpdateCachePath(
    const char* provider, const char* hash, const char* alg, const char* updateCacheBasePath)
{
    (void)provider;
    (void)hash;
    (void)alg;
    (void)updateCacheBasePath;

    g_mockSucState.createPathCallCount++;
    return g_mockSucState.createPathResult;
}

bool SystemUtils_IsFile(const char* path, int* err)
{
    (void)path;

    if (err != NULL)
    {
        *err = 0;
    }
    g_mockSucState.isFileCallCount++;
    return g_mockSucState.isFileResult;
}

bool PermissionUtils_VerifyFilemodeBitmask(const char* path, mode_t bitmask)
{
    (void)path;
    (void)bitmask;

    g_mockSucState.verifyFilemodeCallCount++;
    return g_mockSucState.verifyFilemodeResult;
}

int ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(
    const ADUC_WorkflowHandle workflowHandle, off_t totalSize, const char* updateCacheBasePath)
{
    (void)workflowHandle;
    (void)totalSize;
    (void)updateCacheBasePath;

    g_mockSucState.purgeCallCount++;
    return g_mockSucState.purgeResult;
}

ADUC_Result ADUC_SourceUpdateCacheUtils_MoveToUpdateCache(
    const ADUC_WorkflowHandle workflowHandle, const char* updateCacheBasePath)
{
    (void)workflowHandle;
    (void)updateCacheBasePath;

    g_mockSucState.moveCallCount++;
    return g_mockSucState.moveResult;
}
