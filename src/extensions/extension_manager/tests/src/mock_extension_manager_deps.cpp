/**
 * @file mock_extension_manager_deps.cpp
 * @brief Mock implementations for extension_manager.cpp dependencies.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "mock_extension_manager_deps.h"

#include <aduc/config_utils.h>
#include <aduc/contract_utils.h>
#include <aduc/extension_manager_download_options.h>
#include <aduc/extension_manager_helper.hpp>
#include <aduc/hash_utils.h>
#include <aduc/logging.h>
#include <aduc/result.h>
#include <aduc/string_c_utils.h>
#include <aduc/types/update_content.h>

#include <cstdarg>
#include <cstdlib>
#include <cstring>

// =====================================================================
// Mock state variables
// =====================================================================

extern "C"
{
    // dlopen/dlsym/dlclose/dlerror
    void* mock_dlopen_return = nullptr;
    void* mock_dlsym_return = nullptr;
    int mock_dlclose_return = 0;
    const char* mock_dlerror_return = "mock dlerror";
    int mock_dlsym_call_count = 0;
    void* mock_dlsym_returns[10] = {};

    // config info
    MockConfigInfo mock_config_info = {};
    bool mock_config_info_return_null = false;

    // Extension file entity
    bool mock_get_extension_file_entity_return = false;
    ADUC_FileEntity mock_extension_file_entity = {};

    // Hash utils
    bool mock_get_sha_version_return = true;
    bool mock_is_valid_file_hash_return = true;
    const char* mock_hash_type_return = "sha256";
    const char* mock_hash_value_return = "abc123";

    // access
    int mock_access_return = -1; // default: file does not exist

    // workflow utils
    bool mock_workflow_get_entity_workfolder_filepath_return = true;
    const char* mock_workflow_peek_id_return = "test-workflow-id";
    char* mock_workflow_get_workfolder_return = nullptr;
    int mock_workflow_add_erc_call_count = 0;
    ADUC_Result_t mock_workflow_add_erc_last_erc = 0;

    // contract utils
    bool mock_is_v1_contract_return = true;

    // PathUtils_SanitizePathSegment
    bool mock_sanitize_path_return_null = false;

    // ProcessDownloadHandlerExtensibility
    ADUC_Result mock_process_download_handler_result = { 0, 0 };
    int mock_process_download_handler_call_count = 0;

    // GetDownloadTimeoutInMinutes
    unsigned int mock_get_download_timeout_return = 480;

    static ADUC_ConfigInfo s_mock_config = {};

    void mock_extension_manager_reset(void)
    {
        mock_dlopen_return = nullptr;
        mock_dlsym_return = nullptr;
        mock_dlclose_return = 0;
        mock_dlerror_return = "mock dlerror";
        mock_dlsym_call_count = 0;
        memset(mock_dlsym_returns, 0, sizeof(mock_dlsym_returns));

        memset(&mock_config_info, 0, sizeof(mock_config_info));
        mock_config_info_return_null = false;

        mock_get_extension_file_entity_return = false;
        memset(&mock_extension_file_entity, 0, sizeof(mock_extension_file_entity));

        mock_get_sha_version_return = true;
        mock_is_valid_file_hash_return = true;
        mock_hash_type_return = "sha256";
        mock_hash_value_return = "abc123";

        mock_access_return = -1;

        mock_workflow_get_entity_workfolder_filepath_return = true;
        mock_workflow_peek_id_return = "test-workflow-id";
        mock_workflow_get_workfolder_return = nullptr;
        mock_workflow_add_erc_call_count = 0;
        mock_workflow_add_erc_last_erc = 0;

        mock_is_v1_contract_return = true;

        mock_sanitize_path_return_null = false;

        mock_process_download_handler_result = { 0, 0 };
        mock_process_download_handler_call_count = 0;

        mock_get_download_timeout_return = 480;

        memset(&s_mock_config, 0, sizeof(s_mock_config));
    }

    // =====================================================================
    // Mock implementations of external functions
    // =====================================================================

    // --- aducpal/dlfcn.h (intercepted via --wrap linker flags) ---
    void* __wrap_dlopen(const char* /*filename*/, int /*flag*/)
    {
        return mock_dlopen_return;
    }

    char* __wrap_dlerror(void)
    {
        return const_cast<char*>(mock_dlerror_return);
    }

    void* __wrap_dlsym(void* /*handle*/, const char* /*symbol*/)
    {
        int idx = mock_dlsym_call_count++;
        if (idx < 10 && mock_dlsym_returns[idx] != nullptr)
        {
            return mock_dlsym_returns[idx];
        }
        return mock_dlsym_return;
    }

    int __wrap_dlclose(void* /*handle*/)
    {
        return mock_dlclose_return;
    }

    // --- aducpal/unistd.h (intercepted via --wrap linker flag) ---
    int __wrap_access(const char* /*pathname*/, int /*mode*/)
    {
        return mock_access_return;
    }

    // --- ADUC_ConfigInfo ---
    const ADUC_ConfigInfo* ADUC_ConfigInfo_GetInstance(void)
    {
        if (mock_config_info_return_null)
        {
            return nullptr;
        }
        s_mock_config.extensionsStepHandlerFolder =
            const_cast<char*>(mock_config_info.extensionsStepHandlerFolder);
        s_mock_config.extensionsDownloadHandlerFolder =
            const_cast<char*>(mock_config_info.extensionsDownloadHandlerFolder);
        s_mock_config.downloadTimeoutInMinutes = mock_config_info.downloadTimeoutInMinutes;
        return &s_mock_config;
    }

    int ADUC_ConfigInfo_ReleaseInstance(const ADUC_ConfigInfo* /*configInfo*/)
    {
        return 0;
    }

    // --- Extension utils ---
    bool GetExtensionFileEntity(const char* /*extensionRegFile*/, ADUC_FileEntity* fileEntity)
    {
        if (mock_get_extension_file_entity_return && fileEntity != nullptr)
        {
            *fileEntity = mock_extension_file_entity;
        }
        return mock_get_extension_file_entity_return;
    }

    // --- Hash utils ---
    bool ADUC_HashUtils_GetShaVersionForTypeString(const char* /*hashTypeStr*/, SHAversion* algorithm)
    {
        if (algorithm != nullptr)
        {
            *algorithm = SHA256;
        }
        return mock_get_sha_version_return;
    }

    char* ADUC_HashUtils_GetHashType(
        const ADUC_Hash* /*hashArray*/, size_t /*arraySize*/, size_t /*index*/)
    {
        return const_cast<char*>(mock_hash_type_return);
    }

    char* ADUC_HashUtils_GetHashValue(
        const ADUC_Hash* /*hashArray*/, size_t /*arraySize*/, size_t /*index*/)
    {
        return const_cast<char*>(mock_hash_value_return);
    }

    bool ADUC_HashUtils_IsValidFileHash(
        const char* /*path*/,
        const char* /*hashBase64*/,
        SHAversion /*algorithm*/,
        bool /*suppressErrorLog*/)
    {
        return mock_is_valid_file_hash_return;
    }

    // --- Parser utils ---
    void ADUC_FileEntity_Uninit(ADUC_FileEntity* /*entity*/)
    {
        // no-op in mock
    }

    // --- Contract utils ---
    bool ADUC_ContractUtils_IsV1Contract(ADUC_ExtensionContractInfo* /*contractInfo*/)
    {
        return mock_is_v1_contract_return;
    }

    // --- Workflow utils ---
    bool workflow_get_entity_workfolder_filepath(
        void* /*workflowHandle*/, const ADUC_FileEntity* /*entity*/, STRING_HANDLE* outFilePath)
    {
        if (mock_workflow_get_entity_workfolder_filepath_return && outFilePath != nullptr)
        {
            *outFilePath = STRING_construct("/tmp/test/target_file");
        }
        return mock_workflow_get_entity_workfolder_filepath_return;
    }

    const char* workflow_peek_id(void* /*handle*/)
    {
        return mock_workflow_peek_id_return;
    }

    char* workflow_get_workfolder(const void* /*handle*/)
    {
        if (mock_workflow_get_workfolder_return != nullptr)
        {
            return strdup(mock_workflow_get_workfolder_return);
        }
        return strdup("/tmp/test/workfolder");
    }

    void workflow_add_erc(void* /*handle*/, ADUC_Result_t erc)
    {
        mock_workflow_add_erc_call_count++;
        mock_workflow_add_erc_last_erc = erc;
    }

    void workflow_set_result_details(void* /*handle*/, const char* /*format*/, ...)
    {
        // no-op
    }

    // --- String utils ---
    bool IsNullOrEmpty(const char* str)
    {
        return (str == nullptr || *str == '\0');
    }

    // --- Path utils ---
    STRING_HANDLE PathUtils_SanitizePathSegment(const char* unsanitized)
    {
        if (mock_sanitize_path_return_null || unsanitized == nullptr)
        {
            return nullptr;
        }
        return STRING_construct(unsanitized);
    }

    // --- Logging ---
    ADUC_LOG_SEVERITY ADUC_Logging_GetLevel(void)
    {
        return ADUC_LOG_INFO;
    }

    void zlog_log(enum ZLOG_SEVERITY /*msg_level*/, const char* /*func*/, unsigned int /*line*/, const char* /*fmt*/, ...)
    {
        // no-op mock
    }

    // --- Extension Manager Helper functions ---
    ADUC_Result ProcessDownloadHandlerExtensibility(
        void* /*workflowHandle*/, const ADUC_FileEntity* /*entity*/, const char* /*targetUpdateFilePath*/) noexcept
    {
        mock_process_download_handler_call_count++;
        return mock_process_download_handler_result;
    }

    unsigned int GetDownloadTimeoutInMinutes(const ExtensionManager_Download_Options* /*downloadOptions*/) noexcept
    {
        return mock_get_download_timeout_return;
    }

} // extern "C"
