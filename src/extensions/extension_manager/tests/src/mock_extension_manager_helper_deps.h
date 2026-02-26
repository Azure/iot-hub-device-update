/**
 * @file mock_extension_manager_helper_deps.h
 * @brief Mock declarations for extension_manager_helper.cpp dependencies.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef MOCK_EXTENSION_MANAGER_HELPER_DEPS_H
#define MOCK_EXTENSION_MANAGER_HELPER_DEPS_H

#include <aduc/contract_utils.h>
#include <aduc/result.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // =====================================================================
    // Mock control state
    // =====================================================================

    // --- DownloadHandlerFactory ---
    extern bool mock_factory_get_instance_throws;
    extern bool mock_factory_load_returns_null;
    extern ADUC_ExtensionContractInfo mock_plugin_contract_info;
    extern ADUC_Result mock_plugin_get_contract_info_result;
    extern ADUC_Result mock_plugin_process_update_result;

    // --- ADUC_ConfigInfo ---
    extern bool mock_helper_config_return_null;
    extern unsigned int mock_helper_config_download_timeout;

    // --- workflow_add_erc ---
    extern int mock_helper_workflow_add_erc_count;
    extern int mock_helper_workflow_set_result_details_count;

    // --- IsNullOrEmpty ---
    // Uses real implementation: returns true if str is null or empty.

    // --- Reset ---
    void mock_extension_manager_helper_reset(void);

#ifdef __cplusplus
}
#endif

#endif // MOCK_EXTENSION_MANAGER_HELPER_DEPS_H
