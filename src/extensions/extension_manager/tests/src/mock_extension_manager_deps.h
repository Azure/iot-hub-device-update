/**
 * @file mock_extension_manager_deps.h
 * @brief Mock declarations for extension_manager.cpp dependencies.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef MOCK_EXTENSION_MANAGER_DEPS_H
#define MOCK_EXTENSION_MANAGER_DEPS_H

#include <aduc/contract_utils.h>
#include <aduc/result.h>
#include <aduc/types/update_content.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // =====================================================================
    // Mock control — set these before calling the function under test
    // =====================================================================

    // --- dlopen/dlsym/dlclose/dlerror mocks ---
    extern void* mock_dlopen_return;
    extern void* mock_dlsym_return;
    extern int mock_dlclose_return;
    extern const char* mock_dlerror_return;
    extern int mock_dlsym_call_count;
    extern void* mock_dlsym_returns[10]; // array of return values for successive dlsym calls

    // --- ADUC_ConfigInfo mocks ---
    typedef struct tagMockConfigInfo
    {
        const char* extensionsStepHandlerFolder;
        const char* extensionsDownloadHandlerFolder;
        unsigned int downloadTimeoutInMinutes;
    } MockConfigInfo;

    extern MockConfigInfo mock_config_info;
    extern bool mock_config_info_return_null;

    // --- GetExtensionFileEntity mock ---
    extern bool mock_get_extension_file_entity_return;
    extern ADUC_FileEntity mock_extension_file_entity;

    // --- Hash utils mocks ---
    extern bool mock_get_sha_version_return;
    extern bool mock_is_valid_file_hash_return;
    extern const char* mock_hash_type_return;
    extern const char* mock_hash_value_return;

    // --- access mock ---
    extern int mock_access_return;

    // --- workflow utils mocks ---
    extern bool mock_workflow_get_entity_workfolder_filepath_return;
    extern const char* mock_workflow_peek_id_return;
    extern char* mock_workflow_get_workfolder_return;
    extern int mock_workflow_add_erc_call_count;
    extern ADUC_Result_t mock_workflow_add_erc_last_erc;

    // --- contract utils ---
    extern bool mock_is_v1_contract_return;

    // --- PathUtils_SanitizePathSegment ---
    extern bool mock_sanitize_path_return_null;

    // --- ProcessDownloadHandlerExtensibility ---
    extern ADUC_Result mock_process_download_handler_result;
    extern int mock_process_download_handler_call_count;

    // --- GetDownloadTimeoutInMinutes ---
    extern unsigned int mock_get_download_timeout_return;

    // --- Mock reset function ---
    void mock_extension_manager_reset(void);

#ifdef __cplusplus
}
#endif

#endif // MOCK_EXTENSION_MANAGER_DEPS_H
