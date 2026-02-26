/**
 * @file mock_source_update_cache_utils_deps.c
 * @brief Mock implementations for source_update_cache_utils.c / .cpp dependencies.
 *
 * Mocked functions:
 *   - workflow_get_update_files_count
 *   - workflow_get_update_file
 *   - workflow_get_entity_workfolder_filepath
 *   - workflow_get_expected_update_id
 *   - SystemUtils_IsFile
 *   - ADUC_SystemUtils_MkDirRecursiveDefault
 *   - ADUC_SystemUtils_CopyFileToDir
 *   - ADUC_FileEntity_Uninit
 *   - ADUC_UpdateId_UninitAndFree
 *   - workflow_get_update_file_inode
 *   - PathUtils_SanitizePathSegment
 *   - IsNullOrEmpty
 *   - Logging stubs
 *
 * Wrapped via linker:
 *   - __wrap_rename (for ADUCPAL_rename -> rename)
 *   - __wrap_stat (for stat calls in .cpp)
 *   - __wrap_unlink (for unlink calls in .cpp)
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "mock_source_update_cache_utils_deps.h"

#include <aduc/result.h>
#include <aduc/types/update_content.h>
#include <azure_c_shared_utility/strings.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

/* =====================================================================
 * Mock state variables
 * ===================================================================== */

size_t mock_update_files_count = 1;
bool mock_workflow_get_update_file_return = true;
bool mock_workflow_get_entity_workfolder_filepath_return = true;
const char* mock_sandbox_filepath = "/tmp/sandbox/update.swu";
bool mock_workflow_get_expected_update_id_success = true;
const char* mock_update_id_provider = "TestProvider";
bool mock_system_utils_is_file_return = true;
int mock_mkdir_recursive_return = 0;
int mock_rename_return = 0;
int mock_copy_file_to_dir_return = 0;

ino_t mock_update_file_inode_return = 0;
const char** mock_files_in_dir = NULL;
size_t mock_files_in_dir_count = 0;
int mock_stat_return = 0;
ino_t mock_stat_inode = 100;
time_t mock_stat_mtime = 1000;
off_t mock_stat_size = 1024;
int mock_stat_call_count = 0;
int mock_stat_returns[10] = { 0 };
ino_t mock_stat_inodes[10] = { 0 };
time_t mock_stat_mtimes[10] = { 0 };
off_t mock_stat_sizes[10] = { 0 };
bool mock_stat_use_array = false;
int mock_unlink_return = 0;
int mock_unlink_call_count = 0;

static ADUC_UpdateId s_mock_update_id;
static ADUC_Hash s_mock_hash;
static ADUC_FileEntity s_mock_file_entity;

void ResetSourceUpdateCacheUtilsMocks(void)
{
    mock_update_files_count = 1;
    mock_workflow_get_update_file_return = true;
    mock_workflow_get_entity_workfolder_filepath_return = true;
    mock_sandbox_filepath = "/tmp/sandbox/update.swu";
    mock_workflow_get_expected_update_id_success = true;
    mock_update_id_provider = "TestProvider";
    mock_system_utils_is_file_return = true;
    mock_mkdir_recursive_return = 0;
    mock_rename_return = 0;
    mock_copy_file_to_dir_return = 0;

    mock_update_file_inode_return = 0;
    mock_files_in_dir = NULL;
    mock_files_in_dir_count = 0;
    mock_stat_return = 0;
    mock_stat_inode = 100;
    mock_stat_mtime = 1000;
    mock_stat_size = 1024;
    mock_stat_call_count = 0;
    memset(mock_stat_returns, 0, sizeof(mock_stat_returns));
    memset(mock_stat_inodes, 0, sizeof(mock_stat_inodes));
    memset(mock_stat_mtimes, 0, sizeof(mock_stat_mtimes));
    memset(mock_stat_sizes, 0, sizeof(mock_stat_sizes));
    mock_stat_use_array = false;
    mock_unlink_return = 0;
    mock_unlink_call_count = 0;

    memset(&s_mock_update_id, 0, sizeof(s_mock_update_id));
    memset(&s_mock_hash, 0, sizeof(s_mock_hash));
    memset(&s_mock_file_entity, 0, sizeof(s_mock_file_entity));
}

/* =====================================================================
 * Workflow utils mocks
 * ===================================================================== */

size_t workflow_get_update_files_count(ADUC_WorkflowHandle handle)
{
    (void)handle;
    return mock_update_files_count;
}

bool workflow_get_update_file(ADUC_WorkflowHandle handle, size_t index, ADUC_FileEntity* entity)
{
    (void)handle;
    (void)index;
    if (!mock_workflow_get_update_file_return || entity == NULL)
    {
        return false;
    }
    memset(entity, 0, sizeof(*entity));
    s_mock_hash.type = "sha256";
    s_mock_hash.value = "testhash+/==";
    entity->Hash = &s_mock_hash;
    entity->HashCount = 1;
    entity->TargetFilename = "update.swu";
    return true;
}

bool workflow_get_entity_workfolder_filepath(
    ADUC_WorkflowHandle handle, const ADUC_FileEntity* entity, STRING_HANDLE* outFilePath)
{
    (void)handle;
    (void)entity;
    if (!mock_workflow_get_entity_workfolder_filepath_return || outFilePath == NULL)
    {
        return false;
    }
    *outFilePath = STRING_construct(mock_sandbox_filepath);
    return true;
}

ADUC_Result workflow_get_expected_update_id(ADUC_WorkflowHandle handle, ADUC_UpdateId** updateId)
{
    (void)handle;
    ADUC_Result result;
    if (!mock_workflow_get_expected_update_id_success || updateId == NULL)
    {
        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = 0x12345;
        return result;
    }
    s_mock_update_id.Provider = (char*)mock_update_id_provider;
    s_mock_update_id.Name = "testName";
    s_mock_update_id.Version = "1.0";
    *updateId = &s_mock_update_id;
    result.ResultCode = ADUC_Result_Success;
    result.ExtendedResultCode = 0;
    return result;
}

ino_t workflow_get_update_file_inode(ADUC_WorkflowHandle handle, size_t index)
{
    (void)handle;
    (void)index;
    return mock_update_file_inode_return;
}

/* =====================================================================
 * System utils mocks
 * ===================================================================== */

bool SystemUtils_IsFile(const char* path, int* err)
{
    (void)path;
    if (err != NULL)
    {
        *err = 0;
    }
    return mock_system_utils_is_file_return;
}

int ADUC_SystemUtils_MkDirRecursiveDefault(const char* path)
{
    (void)path;
    return mock_mkdir_recursive_return;
}

int ADUC_SystemUtils_CopyFileToDir(const char* filePath, const char* dirPath, bool overwriteExistingFile)
{
    (void)filePath;
    (void)dirPath;
    (void)overwriteExistingFile;
    return mock_copy_file_to_dir_return;
}

/* =====================================================================
 * Parser utils mock
 * ===================================================================== */

void ADUC_FileEntity_Uninit(ADUC_FileEntity* entity)
{
    (void)entity;
    /* no-op in mock */
}

void ADUC_UpdateId_UninitAndFree(ADUC_UpdateId* updateId)
{
    (void)updateId;
    /* no-op in mock — we use static test data */
}

/* =====================================================================
 * Path utils mock
 * ===================================================================== */

STRING_HANDLE PathUtils_SanitizePathSegment(const char* unsanitized)
{
    if (unsanitized == NULL)
    {
        return NULL;
    }
    /* Replace '/' with '_' to mimic real behavior */
    char buffer[512];
    size_t len = strlen(unsanitized);
    if (len >= sizeof(buffer))
    {
        len = sizeof(buffer) - 1;
    }
    for (size_t i = 0; i < len; ++i)
    {
        buffer[i] = (unsanitized[i] == '/') ? '_' : unsanitized[i];
    }
    buffer[len] = '\0';
    return STRING_construct(buffer);
}

/* =====================================================================
 * String utils mock
 * ===================================================================== */

bool IsNullOrEmpty(const char* str)
{
    return (str == NULL || *str == '\0');
}

/* =====================================================================
 * Linker --wrap intercepted functions
 * ===================================================================== */

int __wrap_rename(const char* oldpath, const char* newpath)
{
    (void)oldpath;
    (void)newpath;
    return mock_rename_return;
}

// Note: __wrap_stat and __wrap_unlink are intentionally omitted.
// On glibc 2.39 with _FILE_OFFSET_BITS=64, stat is redirected to __stat64_time64,
// so --wrap=stat cannot intercept it. The stat/unlink mock state variables are kept
// for potential future use with the correct symbol wrapping.

/* =====================================================================
 * Logging stubs
 * ===================================================================== */

ADUC_LOG_SEVERITY ADUC_Logging_GetLevel(void)
{
    return ADUC_LOG_INFO;
}

void zlog_log(
    enum ZLOG_SEVERITY msg_level, const char* func, unsigned int line, const char* fmt, ...)
{
    (void)msg_level;
    (void)func;
    (void)line;
    (void)fmt;
    /* no-op mock */
}
