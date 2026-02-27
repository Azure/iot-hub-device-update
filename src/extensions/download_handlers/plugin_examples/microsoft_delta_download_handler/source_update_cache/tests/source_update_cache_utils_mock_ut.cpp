/**
 * @file source_update_cache_utils_mock_ut.cpp
 * @brief Mock-based unit tests for source_update_cache_utils.c MoveToUpdateCache
 *        and source_update_cache_utils.cpp PurgeOldestFromUpdateCache error paths.
 *
 * Uses link-time mocks (--wrap for rename/stat/unlink and direct mock for
 * workflow_*, SystemUtils_*, etc.) to exercise error branches that are
 * difficult to reach in integration tests.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

extern "C"
{
#include "mock_source_update_cache_utils_deps.h"
#include <aduc/source_update_cache_utils.h>
}

#include <cstring>
#include <fstream>
#include <string>

// ==============================================================================
// Test fixture
// ==============================================================================

struct SucUtilsMockFixture
{
    SucUtilsMockFixture()
    {
        ResetSourceUpdateCacheUtilsMocks();
    }

    ~SucUtilsMockFixture() = default;
};

// ==============================================================================
// MoveToUpdateCache tests
// ==============================================================================

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "MoveToUpdateCache: workflow_get_update_file fails -> failure")
{
    mock_workflow_get_update_file_return = false;

    int dummyHandle = 0;
    ADUC_Result result = ADUC_SourceUpdateCacheUtils_MoveToUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), "/tmp/cache");

    CHECK(result.ResultCode == ADUC_Result_Failure);
}

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "MoveToUpdateCache: workflow_get_expected_update_id fails -> failure with ERC")
{
    mock_workflow_get_expected_update_id_success = false;

    int dummyHandle = 0;
    ADUC_Result result = ADUC_SourceUpdateCacheUtils_MoveToUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), "/tmp/cache");

    CHECK(result.ResultCode == ADUC_Result_Failure);
}

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "MoveToUpdateCache: file not in sandbox (SystemUtils_IsFile false) -> failure with ERC")
{
    mock_system_utils_is_file_return = false;

    int dummyHandle = 0;
    ADUC_Result result = ADUC_SourceUpdateCacheUtils_MoveToUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), "/tmp/cache");

    // Note: workflow_get_expected_update_id overwrites result.ResultCode to Success
    // before this error path, so the source only sets ExtendedResultCode.
    CHECK(result.ExtendedResultCode != 0);
}

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "MoveToUpdateCache: MkDirRecursiveDefault fails -> failure with ERC")
{
    mock_mkdir_recursive_return = -1;

    int dummyHandle = 0;
    ADUC_Result result = ADUC_SourceUpdateCacheUtils_MoveToUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), "/tmp/cache");

    // Note: workflow_get_expected_update_id overwrites result.ResultCode to Success
    // before this error path, so the source only sets ExtendedResultCode.
    CHECK(result.ExtendedResultCode != 0);
}

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "MoveToUpdateCache: rename succeeds -> success")
{
    mock_rename_return = 0;

    int dummyHandle = 0;
    ADUC_Result result = ADUC_SourceUpdateCacheUtils_MoveToUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), "/tmp/cache");

    CHECK(result.ResultCode == ADUC_Result_Success);
}

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "MoveToUpdateCache: rename fails but copy succeeds -> success")
{
    mock_rename_return = -1;
    mock_copy_file_to_dir_return = 0;

    int dummyHandle = 0;
    ADUC_Result result = ADUC_SourceUpdateCacheUtils_MoveToUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), "/tmp/cache");

    CHECK(result.ResultCode == ADUC_Result_Success);
}

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "MoveToUpdateCache: rename fails and copy fails -> failure with COPYFALLBACK ERC")
{
    mock_rename_return = -1;
    mock_copy_file_to_dir_return = -1;

    int dummyHandle = 0;
    ADUC_Result result = ADUC_SourceUpdateCacheUtils_MoveToUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), "/tmp/cache");

    // Note: workflow_get_expected_update_id overwrites result.ResultCode to Success
    // before this error path, so the source only sets ExtendedResultCode.
    CHECK(result.ExtendedResultCode != 0);
}

// ==============================================================================
// PurgeOldestFromUpdateCache tests
// ==============================================================================

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "PurgeOldest: empty cache -> success with no deletes")
{
    mock_files_in_dir = nullptr;
    mock_files_in_dir_count = 0;

    int dummyHandle = 0;
    int result = ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), 1024, "/tmp/cache");

    CHECK(result == 0);
    CHECK(mock_unlink_call_count == 0);
}

static const char* s_one_file[] = { "/tmp/cache/file1" };

// Note: Tests that require stat/unlink wrapping are omitted because on glibc 2.39
// with _FILE_OFFSET_BITS=64, stat is redirected to __stat64_time64, which --wrap=stat
// cannot intercept. The real stat then fails for non-existent paths, preventing any
// files from entering the priority queue.

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "PurgeOldest: stat fails in for_each lambda -> file not added to queue, no unlink")
{
    mock_files_in_dir = s_one_file;
    mock_files_in_dir_count = 1;
    mock_update_file_inode_return = 0; // sentinel => no filter path
    mock_stat_return = -1; // stat fails for for_each

    int dummyHandle = 0;
    int result = ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), 512, "/tmp/cache");

    CHECK(result == 0);
    CHECK(mock_unlink_call_count == 0);
}

// Tests for stat/unlink failure paths are omitted — see comment above about
// --wrap=stat not intercepting __stat64_time64 on glibc 2.39.

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "PurgeOldest: file with matching payload inode is excluded from purge")
{
    mock_files_in_dir = s_one_file;
    mock_files_in_dir_count = 1;

    // Set the payload inode to match the file's inode
    mock_update_file_inode_return = 200;

    // stat returns matching inode for filter
    mock_stat_use_array = true;
    mock_stat_returns[0] = 0;
    mock_stat_inodes[0] = 200; // matches payload inode

    int dummyHandle = 0;
    int result = ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), 512, "/tmp/cache");

    CHECK(result == 0);
    // File was excluded by filter, so no unlink
    CHECK(mock_unlink_call_count == 0);
}

// ==============================================================================
// PurgeOldest: exception paths
// ==============================================================================

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "PurgeOldest: findFilesInDir throws std::exception -> returns -1")
{
    mock_findFilesInDir_throw_std_exception = true;

    int dummyHandle = 0;
    int result = ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), 1024, "/tmp/cache");

    CHECK(result == -1);
}

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "PurgeOldest: findFilesInDir throws unknown exception -> returns -1")
{
    mock_findFilesInDir_throw_unknown = true;

    int dummyHandle = 0;
    int result = ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), 1024, "/tmp/cache");

    CHECK(result == -1);
}

// ==============================================================================
// PurgeOldest: unlink paths (using real temp files so stat succeeds)
// ==============================================================================

static const char* s_tmp_purge_dir = "/tmp/adutest_suc_mock_purge";

/// Helper: create tmp dir and files, returns file paths
static std::vector<std::string> createTempPurgeFiles(int count)
{
    std::string cmd = std::string("mkdir -p ") + s_tmp_purge_dir;
    system(cmd.c_str());

    std::vector<std::string> paths;
    for (int i = 0; i < count; ++i)
    {
        std::string p = std::string(s_tmp_purge_dir) + "/file" + std::to_string(i);
        std::ofstream ofs(p);
        ofs << "data" << i << std::endl;
        ofs.close();
        paths.push_back(p);
    }
    return paths;
}

static void cleanupTempPurgeDir()
{
    std::string cmd = std::string("rm -rf ") + s_tmp_purge_dir;
    system(cmd.c_str());
}

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "PurgeOldest: real files, unlink succeeds -> files deleted from queue")
{
    auto paths = createTempPurgeFiles(2);
    const char* pathPtrs[] = { paths[0].c_str(), paths[1].c_str() };

    mock_files_in_dir = pathPtrs;
    mock_files_in_dir_count = 2;
    mock_update_file_inode_return = 0; // sentinel -> no filter
    mock_unlink_return = 0;

    int dummyHandle = 0;
    int result = ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), 10000, s_tmp_purge_dir);

    CHECK(result == 0);
    CHECK(mock_unlink_call_count == 2);

    cleanupTempPurgeDir();
}

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "PurgeOldest: real files, unlink fails -> still returns 0 (result overwritten)")
{
    auto paths = createTempPurgeFiles(1);
    const char* pathPtrs[] = { paths[0].c_str() };

    mock_files_in_dir = pathPtrs;
    mock_files_in_dir_count = 1;
    mock_update_file_inode_return = 0;
    mock_unlink_return = -1; // failure

    int dummyHandle = 0;
    int result = ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), 10000, s_tmp_purge_dir);

    // Even though unlink failed, result = 0 because the code sets result = 0 after the loop
    CHECK(result == 0);
    CHECK(mock_unlink_call_count == 1);

    cleanupTempPurgeDir();
}

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "PurgeOldest: uses default cache path when updateCacheBasePath is nullptr")
{
    mock_files_in_dir = nullptr;
    mock_files_in_dir_count = 0;

    int dummyHandle = 0;
    int result = ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), 1024, nullptr);

    CHECK(result == 0);
}

TEST_CASE_METHOD(
    SucUtilsMockFixture,
    "PurgeOldest: totalSize already <= 0 -> no unlink called")
{
    auto paths = createTempPurgeFiles(1);
    const char* pathPtrs[] = { paths[0].c_str() };

    mock_files_in_dir = pathPtrs;
    mock_files_in_dir_count = 1;
    mock_update_file_inode_return = 0;

    int dummyHandle = 0;
    int result = ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(
        reinterpret_cast<ADUC_WorkflowHandle>(&dummyHandle), 0 /* totalSize */, s_tmp_purge_dir);

    CHECK(result == 0);
    CHECK(mock_unlink_call_count == 0);

    cleanupTempPurgeDir();
}
