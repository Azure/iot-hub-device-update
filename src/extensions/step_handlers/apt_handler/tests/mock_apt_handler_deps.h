/**
 * @file mock_apt_handler_deps.h
 * @brief Configurable mock state for apt_handler.cpp dependencies.
 */
#ifndef MOCK_APT_HANDLER_DEPS_H
#define MOCK_APT_HANDLER_DEPS_H

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
    extern const char* mock_workfolder;
    extern const char* mock_installed_criteria;
    extern const char* mock_workflow_id;
    extern int mock_workflow_level;
    extern int mock_workflow_step;
    extern bool mock_request_cancel_return;
    extern const char* mock_target_filename;

    /* --- ADUC_ConfigInfo mock --- */
    extern bool mock_config_available;
    extern const char* mock_adu_shell_path;
    extern const char* mock_config_folder;

    /* --- ExtensionManager::Download mock --- */
    extern ADUC_Result mock_extension_download_result;

    /* --- ADUC_LaunchChildProcess mock --- */
    extern int mock_launch_child_exit_code;
    extern int mock_launch_child_exit_code_2; /* second call (download phase) */
    extern int mock_launch_child_call_count;

    /* --- PersistInstalledCriteria mock --- */
    extern bool mock_persist_installed_criteria_return;

    /* --- GetIsInstalled mock --- */
    extern ADUC_Result mock_get_is_installed_result;

    /* --- ADUC_WorkflowData_GetInstalledCriteria mock --- */
    extern const char* mock_workflowdata_installed_criteria;

    /* --- Reset all mocks --- */
    void mock_apt_handler_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_APT_HANDLER_DEPS_H */
