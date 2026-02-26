/**
 * @file mock_source_update_cache_deps.h
 * @brief Configurable mock state for source_update_cache.c dependencies.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef MOCK_SOURCE_UPDATE_CACHE_DEPS_H
#define MOCK_SOURCE_UPDATE_CACHE_DEPS_H

#include <aduc/result.h>
#include <aduc/types/workflow.h>
#include <azure_c_shared_utility/strings.h>
#include <sys/types.h>

/** Configurable mock state for source_update_cache.c dependencies. */
struct MockSourceUpdateCacheDepsState
{
    /* ADUC_SourceUpdateCacheUtils_CreateSourceUpdateCachePath mock */
    STRING_HANDLE createPathResult;
    int createPathCallCount;

    /* SystemUtils_IsFile mock */
    bool isFileResult;
    int isFileCallCount;

    /* PermissionUtils_VerifyFilemodeBitmask mock */
    bool verifyFilemodeResult;
    int verifyFilemodeCallCount;

    /* ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache mock */
    int purgeResult;
    int purgeCallCount;

    /* ADUC_SourceUpdateCacheUtils_MoveToUpdateCache mock */
    ADUC_Result moveResult;
    int moveCallCount;
};

#ifdef __cplusplus
extern "C"
{
#endif

    extern struct MockSourceUpdateCacheDepsState g_mockSucState;
    void ResetSourceUpdateCacheMocks(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_SOURCE_UPDATE_CACHE_DEPS_H */
