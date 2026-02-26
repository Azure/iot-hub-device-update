/**
 * @file mock_extension_manager_helper_deps.cpp
 * @brief Mock implementations for extension_manager_helper.cpp dependencies.
 *
 * Provides mock implementations of DownloadHandlerFactory, DownloadHandlerPlugin,
 * SharedLib, ADUC_ConfigInfo, workflow_utils, and other dependencies.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "mock_extension_manager_helper_deps.h"

#include <aduc/config_utils.h>
#include <aduc/contract_utils.h>
#include <aduc/download_handler_factory.hpp>
#include <aduc/download_handler_plugin.hpp>
#include <aduc/logging.h>
#include <aduc/result.h>
#include <aduc/shared_lib.hpp>
#include <aduc/string_c_utils.h>

#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <stdexcept>

// =====================================================================
// Mock state variables
// =====================================================================

extern "C"
{
    bool mock_factory_get_instance_throws = false;
    bool mock_factory_load_returns_null = false;
    ADUC_ExtensionContractInfo mock_plugin_contract_info = { 1, 0 };
    ADUC_Result mock_plugin_get_contract_info_result = { 1, 0 };
    ADUC_Result mock_plugin_process_update_result = { 1, 0 };

    bool mock_helper_config_return_null = false;
    unsigned int mock_helper_config_download_timeout = 0;

    int mock_helper_workflow_add_erc_count = 0;
    int mock_helper_workflow_set_result_details_count = 0;

    void mock_extension_manager_helper_reset(void)
    {
        mock_factory_get_instance_throws = false;
        mock_factory_load_returns_null = false;
        mock_plugin_contract_info = { 1, 0 };
        mock_plugin_get_contract_info_result = { 1, 0 };
        mock_plugin_process_update_result = { 1, 0 };

        mock_helper_config_return_null = false;
        mock_helper_config_download_timeout = 0;

        mock_helper_workflow_add_erc_count = 0;
        mock_helper_workflow_set_result_details_count = 0;
    }
}

// =====================================================================
// Mock SharedLib implementation
// =====================================================================

namespace aduc
{
SharedLib::SharedLib(const std::string& /*libPath*/) : libHandle(nullptr)
{
}

SharedLib::~SharedLib()
{
}

void SharedLib::EnsureSymbols(std::vector<std::string> /*symbols*/) const
{
}

void* SharedLib::GetSymbol(const std::string& /*symbol*/) const
{
    return nullptr;
}
} // namespace aduc

// =====================================================================
// Mock DownloadHandlerPlugin implementation
// =====================================================================

DownloadHandlerPlugin::DownloadHandlerPlugin(const std::string& libPath, ADUC_LOG_SEVERITY /*logLevel*/) :
    lib(libPath)
{
}

DownloadHandlerPlugin::~DownloadHandlerPlugin() noexcept
{
}

ADUC_Result DownloadHandlerPlugin::ProcessUpdate(
    const ADUC_WorkflowHandle /*workflowHandle*/,
    const ADUC_FileEntity* /*fileEntity*/,
    const char* /*payloadFilePath*/) const noexcept
{
    return mock_plugin_process_update_result;
}

ADUC_Result DownloadHandlerPlugin::OnUpdateWorkflowCompleted(
    const ADUC_WorkflowHandle /*workflowHandle*/) const noexcept
{
    return ADUC_Result{ 1, 0 };
}

ADUC_Result DownloadHandlerPlugin::GetContractInfo(ADUC_ExtensionContractInfo* contractInfo) const noexcept
{
    if (contractInfo != nullptr)
    {
        *contractInfo = mock_plugin_contract_info;
    }
    return mock_plugin_get_contract_info_result;
}

// =====================================================================
// Mock DownloadHandlerFactory implementation
// =====================================================================

// Use a zero-initialized buffer as a stand-in for the private-constructed factory.
// LoadDownloadHandler is also mocked, so the object's internals are never accessed.
static char s_factory_storage[sizeof(DownloadHandlerFactory)] = {};
static std::unique_ptr<DownloadHandlerPlugin> s_mock_plugin;

DownloadHandlerFactory* DownloadHandlerFactory::GetInstance()
{
    if (mock_factory_get_instance_throws)
    {
        throw std::runtime_error("mock factory exception");
    }
    return reinterpret_cast<DownloadHandlerFactory*>(s_factory_storage);
}

DownloadHandlerPlugin* DownloadHandlerFactory::LoadDownloadHandler(const std::string& /*downloadHandlerId*/) noexcept
{
    if (mock_factory_load_returns_null)
    {
        return nullptr;
    }
    // Create a mock plugin if not already created
    if (!s_mock_plugin)
    {
        s_mock_plugin = std::make_unique<DownloadHandlerPlugin>("mock_plugin.so", ADUC_LOG_INFO);
    }
    return s_mock_plugin.get();
}

// =====================================================================
// Mock ADUC_ConfigInfo
// =====================================================================

static ADUC_ConfigInfo s_mock_helper_config = {};

extern "C"
{
    const ADUC_ConfigInfo* ADUC_ConfigInfo_GetInstance(void)
    {
        if (mock_helper_config_return_null)
        {
            return nullptr;
        }
        s_mock_helper_config.downloadTimeoutInMinutes = mock_helper_config_download_timeout;
        return &s_mock_helper_config;
    }

    int ADUC_ConfigInfo_ReleaseInstance(const ADUC_ConfigInfo* /*configInfo*/)
    {
        return 0;
    }

    // --- workflow utils ---
    void workflow_add_erc(void* /*handle*/, ADUC_Result_t /*erc*/)
    {
        mock_helper_workflow_add_erc_count++;
    }

    void workflow_set_result_details(void* /*handle*/, const char* /*format*/, ...)
    {
        mock_helper_workflow_set_result_details_count++;
    }

    // --- String utils ---
    bool IsNullOrEmpty(const char* str)
    {
        return (str == nullptr || *str == '\0');
    }

    // --- Logging ---
    void zlog_log(enum ZLOG_SEVERITY /*msg_level*/, const char* /*func*/, unsigned int /*line*/, const char* /*fmt*/, ...)
    {
        // no-op mock
    }

    ADUC_LOG_SEVERITY ADUC_Logging_GetLevel(void)
    {
        return ADUC_LOG_INFO;
    }

} // extern "C"
