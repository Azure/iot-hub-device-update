/**
 * @file mock_source_update_cache_utils_deps.h
 * @brief Configurable mock state for source_update_cache_utils.c / .cpp dependencies.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef MOCK_SOURCE_UPDATE_CACHE_UTILS_DEPS_H
#define MOCK_SOURCE_UPDATE_CACHE_UTILS_DEPS_H

#include <aduc/result.h>
#include <aduc/types/update_content.h>
#include <aduc/types/workflow.h>
#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // =====================================================================
    // MoveToUpdateCache mocks
    // =====================================================================

    /** workflow_get_update_files_count mock */
    extern size_t mock_update_files_count;

    /** workflow_get_update_file mock */
    extern bool mock_workflow_get_update_file_return;

    /** workflow_get_entity_workfolder_filepath mock */
    extern bool mock_workflow_get_entity_workfolder_filepath_return;
    extern const char* mock_sandbox_filepath;

    /** workflow_get_expected_update_id mock */
    extern bool mock_workflow_get_expected_update_id_success;
    extern const char* mock_update_id_provider;

    /** SystemUtils_IsFile mock */
    extern bool mock_system_utils_is_file_return;

    /** ADUC_SystemUtils_MkDirRecursiveDefault mock */
    extern int mock_mkdir_recursive_return;

    /** ADUCPAL_rename mock — intercepted via --wrap */
    extern int mock_rename_return;

    /** ADUC_SystemUtils_CopyFileToDir mock */
    extern int mock_copy_file_to_dir_return;

    // =====================================================================
    // PurgeOldestFromUpdateCache mocks
    // =====================================================================

    /** workflow_get_update_file_inode mock */
    extern ino_t mock_update_file_inode_return;

    /** findFilesInDir mock — fills vector with these paths */
    extern const char** mock_files_in_dir;
    extern size_t mock_files_in_dir_count;

    /** stat mock — intercepted via --wrap */
    extern int mock_stat_return;
    extern ino_t mock_stat_inode;
    extern time_t mock_stat_mtime;
    extern off_t mock_stat_size;

    /** Array-based stat mock for multiple files */
    extern int mock_stat_call_count;
    extern int mock_stat_returns[10];
    extern ino_t mock_stat_inodes[10];
    extern time_t mock_stat_mtimes[10];
    extern off_t mock_stat_sizes[10];
    extern bool mock_stat_use_array;    /**< when true, use per-call arrays */

    /** unlink mock — intercepted via --wrap */
    extern int mock_unlink_return;
    extern int mock_unlink_call_count;

    /** findFilesInDir throw control */
    extern bool mock_findFilesInDir_throw_std_exception;
    extern bool mock_findFilesInDir_throw_unknown;

    /** Mock reset */
    void ResetSourceUpdateCacheUtilsMocks(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_SOURCE_UPDATE_CACHE_UTILS_DEPS_H */
