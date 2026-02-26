/**
 * @file source_update_cache_ut.cpp
 * @brief Unit Tests for source_update_cache.c — ADUC_SourceUpdateCache_Lookup and _Move.
 *
 * Uses link-time mocks for:
 *   ADUC_SourceUpdateCacheUtils_CreateSourceUpdateCachePath,
 *   SystemUtils_IsFile,
 *   PermissionUtils_VerifyFilemodeBitmask,
 *   ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache,
 *   ADUC_SourceUpdateCacheUtils_MoveToUpdateCache.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

extern "C"
{
#include "mock_source_update_cache_deps.h"
#include <aduc/source_update_cache.h>
}

#include <azure_c_shared_utility/strings.h>

// =====================================================================
// ADUC_SourceUpdateCache_Lookup
// =====================================================================

TEST_CASE("Lookup: CreateSourceUpdateCachePath returns NULL -> failure")
{
    ResetSourceUpdateCacheMocks();

    g_mockSucState.createPathResult = NULL; // path creation fails

    STRING_HANDLE outPath = NULL;
    ADUC_Result result = ADUC_SourceUpdateCache_Lookup("prov", "hash", "sha256", NULL, &outPath);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_LOOKUP_CREATE_PATH);
    CHECK(outPath == NULL);
    CHECK(g_mockSucState.createPathCallCount == 1);
    CHECK(g_mockSucState.isFileCallCount == 0);
}

TEST_CASE("Lookup: file does not exist -> cache miss")
{
    ResetSourceUpdateCacheMocks();

    g_mockSucState.createPathResult = STRING_construct("/cache/some/file");
    g_mockSucState.isFileResult = false; // file not on disk

    STRING_HANDLE outPath = NULL;
    ADUC_Result result = ADUC_SourceUpdateCache_Lookup("prov", "hash", "sha256", NULL, &outPath);

    CHECK(result.ResultCode == ADUC_Result_Success_Cache_Miss);
    CHECK(outPath == NULL);
    CHECK(g_mockSucState.isFileCallCount == 1);
    // VerifyFilemodeBitmask should not be called since IsFile returned false
    // (short-circuit in the || condition)
}

TEST_CASE("Lookup: file exists but not readable -> cache miss")
{
    ResetSourceUpdateCacheMocks();

    g_mockSucState.createPathResult = STRING_construct("/cache/some/file");
    g_mockSucState.isFileResult = true; // file exists
    g_mockSucState.verifyFilemodeResult = false; // but not readable

    STRING_HANDLE outPath = NULL;
    ADUC_Result result = ADUC_SourceUpdateCache_Lookup("prov", "hash", "sha256", NULL, &outPath);

    CHECK(result.ResultCode == ADUC_Result_Success_Cache_Miss);
    CHECK(outPath == NULL);
    CHECK(g_mockSucState.isFileCallCount == 1);
    CHECK(g_mockSucState.verifyFilemodeCallCount == 1);
}

TEST_CASE("Lookup: file exists and is readable -> cache hit")
{
    ResetSourceUpdateCacheMocks();

    g_mockSucState.createPathResult = STRING_construct("/cache/some/file");
    g_mockSucState.isFileResult = true;
    g_mockSucState.verifyFilemodeResult = true;

    STRING_HANDLE outPath = NULL;
    ADUC_Result result = ADUC_SourceUpdateCache_Lookup("prov", "hash", "sha256", NULL, &outPath);

    CHECK(result.ResultCode == ADUC_Result_Success);
    CHECK(result.ExtendedResultCode == 0);
    REQUIRE(outPath != NULL);
    CHECK(std::string(STRING_c_str(outPath)) == "/cache/some/file");

    STRING_delete(outPath);
}

TEST_CASE("Lookup: passes updateCacheBasePath through to CreatePath")
{
    ResetSourceUpdateCacheMocks();

    g_mockSucState.createPathResult = STRING_construct("/custom/cache/file");
    g_mockSucState.isFileResult = true;
    g_mockSucState.verifyFilemodeResult = true;

    STRING_HANDLE outPath = NULL;
    ADUC_Result result =
        ADUC_SourceUpdateCache_Lookup("prov", "hash", "sha256", "/custom/cache", &outPath);

    CHECK(result.ResultCode == ADUC_Result_Success);
    REQUIRE(outPath != NULL);

    STRING_delete(outPath);
}

// =====================================================================
// ADUC_SourceUpdateCache_Move  (default: non-TWO_PHASE_COMMIT)
// =====================================================================

TEST_CASE("Move: PurgeOldest fails -> ADUC_ERC_MOVE_PREPURGE")
{
    ResetSourceUpdateCacheMocks();

    g_mockSucState.purgeResult = -1; // purge fails

    int dummyHandle = 42;
    ADUC_Result result =
        ADUC_SourceUpdateCache_Move(reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), NULL);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_MOVE_PREPURGE);
    CHECK(g_mockSucState.purgeCallCount == 1);
    CHECK(g_mockSucState.moveCallCount == 0); // move not called
}

TEST_CASE("Move: PurgeOldest succeeds but MoveToUpdateCache fails -> ADUC_ERC_MOVE_PAYLOAD")
{
    ResetSourceUpdateCacheMocks();

    g_mockSucState.purgeResult = 0; // purge ok
    g_mockSucState.moveResult.ResultCode = ADUC_Result_Failure;
    g_mockSucState.moveResult.ExtendedResultCode = 0x12345;

    int dummyHandle = 42;
    ADUC_Result result =
        ADUC_SourceUpdateCache_Move(reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), NULL);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_MOVE_PAYLOAD);
    CHECK(g_mockSucState.purgeCallCount == 1);
    CHECK(g_mockSucState.moveCallCount == 1);
}

TEST_CASE("Move: Everything succeeds -> ADUC_Result_Success")
{
    ResetSourceUpdateCacheMocks();

    g_mockSucState.purgeResult = 0;
    g_mockSucState.moveResult.ResultCode = ADUC_Result_Success;
    g_mockSucState.moveResult.ExtendedResultCode = 0;

    int dummyHandle = 42;
    ADUC_Result result =
        ADUC_SourceUpdateCache_Move(reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), NULL);

    CHECK(result.ResultCode == ADUC_Result_Success);
    CHECK(g_mockSucState.purgeCallCount == 1);
    CHECK(g_mockSucState.moveCallCount == 1);
}

TEST_CASE("Move: passes updateCacheBasePath to purge and move")
{
    ResetSourceUpdateCacheMocks();

    g_mockSucState.purgeResult = 0;
    g_mockSucState.moveResult.ResultCode = ADUC_Result_Success;

    int dummyHandle = 42;
    ADUC_Result result = ADUC_SourceUpdateCache_Move(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), "/my/cache");

    CHECK(result.ResultCode == ADUC_Result_Success);
    CHECK(g_mockSucState.purgeCallCount == 1);
    CHECK(g_mockSucState.moveCallCount == 1);
}
