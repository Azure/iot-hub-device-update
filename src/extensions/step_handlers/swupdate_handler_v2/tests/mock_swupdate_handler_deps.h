/**
 * @file mock_swupdate_handler_deps.h
 * @brief Configurable mock state for swupdate_handler_v2.cpp and handler_create.cpp dependencies.
 */
#ifndef MOCK_SWUPDATE_HANDLER_DEPS_H
#define MOCK_SWUPDATE_HANDLER_DEPS_H

#include <aduc/result.h>
#include <aduc/types/update_content.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* --- workflow_* mocks --- */
    extern bool mock_is_cancel_requested;
    extern size_t mock_update_files_count;
    extern bool mock_get_update_file_return;
    extern bool mock_get_update_file_by_name_return;
    extern const char* mock_workfolder;
    extern const char* mock_installed_criteria;
    extern const char* mock_workflow_id;
    extern int mock_workflow_level;
    extern int mock_workflow_step;
    extern bool mock_request_cancel_return;
    extern const char* mock_target_filename;
    extern const char* mock_selected_components;
    extern const char* mock_handler_property_scriptfilename;
    extern const char* mock_handler_property_swufilename;
    extern const char* mock_handler_property_apiversion;
    extern const char* mock_handler_property_arguments;

    /* --- ADUC_ConfigInfo mock --- */
    extern bool mock_config_available;
    extern const char* mock_adu_shell_path;
    extern const char* mock_config_folder;

    /* --- ExtensionManager::Download mock --- */
    extern ADUC_Result mock_extension_download_result;

    /* --- ADUC_LaunchChildProcess mock --- */
    extern int mock_launch_child_exit_code;
    extern int mock_launch_child_call_count;

    /* --- ADUC_SystemUtils_MkSandboxDirRecursive mock --- */
    extern int mock_mkdir_result;

    /* --- ADUC_WorkflowData_GetWorkFolder mock --- */
    extern const char* mock_workdata_workfolder;

    /* --- Reset all mocks --- */
    void mock_swupdate_handler_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_SWUPDATE_HANDLER_DEPS_H */
