/**
 * @file mock_steps_handler_deps.h
 * @brief Configurable mock state for steps_handler.cpp and handler_create.cpp dependencies.
 */
#ifndef MOCK_STEPS_HANDLER_DEPS_H
#define MOCK_STEPS_HANDLER_DEPS_H

#include <aduc/result.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* --- workflow cancel/state --- */
    extern bool mock_is_cancel_requested;
    extern int mock_workflow_level;
    extern int mock_workflow_step;
    extern const char* mock_workflow_id;
    extern bool mock_request_cancel_return;
    extern const char* mock_workfolder;

    /* --- workflow child/step management --- */
    extern int mock_instructions_steps_count;
    extern size_t mock_children_count;
    extern bool mock_is_inline_step;
    extern const char* mock_selected_components;
    extern bool mock_set_selected_components_return;
    extern ADUC_Result mock_create_from_inline_step_result;
    extern bool mock_get_step_detached_manifest_return;
    extern ADUC_Result mock_init_from_file_result;
    extern bool mock_insert_child_return;
    extern const char* mock_step_handler_update_type;
    extern const char* mock_compat_string;

    /* --- MkSandboxDirRecursive --- */
    extern int mock_mksandbox_return;

    /* --- ExtensionManager mocks --- */
    extern ADUC_Result mock_ext_download_result;
    extern ADUC_Result mock_load_handler_result;
    extern bool mock_is_components_enumerator_registered;
    extern ADUC_Result mock_select_components_result;
    extern const char* mock_select_components_output;

    /* --- Content handler behavior --- */
    extern ADUC_Result mock_handler_download_result;
    extern ADUC_Result mock_handler_install_result;
    extern ADUC_Result mock_handler_apply_result;
    extern ADUC_Result mock_handler_backup_result;
    extern ADUC_Result mock_handler_restore_result;
    extern ADUC_Result mock_handler_is_installed_result;
    extern bool mock_handler_is_installed_throws;

    /* --- Reboot/restart flags --- */
    extern bool mock_immediate_reboot_requested;
    extern bool mock_immediate_agent_restart_requested;
    extern bool mock_reboot_requested;
    extern bool mock_agent_restart_requested;

    /* --- contract info --- */
    extern int mock_contract_major;
    extern int mock_contract_minor;

    void mock_steps_handler_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_STEPS_HANDLER_DEPS_H */
