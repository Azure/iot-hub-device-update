/**
 * @file download_handler_plugin.cpp
 * @brief Implements the download handler plugin.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/download_handler_plugin.h"
#include "aduc/download_handler_plugin.hpp"

#include <aduc/contract_utils.h>
#include <aduc/exports/extension_export_symbols.h>
#include <aduc/logging.h>
#include <memory>

using InitializeFn = void (*)(ADUC_LOG_SEVERITY logLevel);

using CleanupFn = void (*)();

using ProcessUpdateFn = ADUC_Result (*)(
    const ADUC_WorkflowHandle workflowHandle, const ADUC_FileEntity* fileEntity, const char* targetFilePath);

using OnUpdateWorkflowCompletedFn = ADUC_Result (*)(const ADUC_WorkflowHandle workflowHandle);
using GetContractInfoFn = ADUC_Result (*)(ADUC_ExtensionContractInfo* contractInfo);

DownloadHandlerPlugin::DownloadHandlerPlugin(const std::string& libPath, ADUC_LOG_SEVERITY logLevel)
{
    lib = std::unique_ptr<aduc::SharedLib>(new aduc::SharedLib(libPath));
    lib->EnsureSymbols({ DOWNLOAD_HANDLER__Initialize__EXPORT_SYMBOL,
                         DOWNLOAD_HANDLER__Cleanup__EXPORT_SYMBOL,
                         DOWNLOAD_HANDLER__ProcessUpdate__EXPORT_SYMBOL,
                         DOWNLOAD_HANDLER__OnUpdateWorkflowCompleted__EXPORT_SYMBOL });

    Log_Debug("Calling '" DOWNLOAD_HANDLER__Initialize__EXPORT_SYMBOL "' export on download handler.");

    void* sym = lib->GetSymbol(DOWNLOAD_HANDLER__Initialize__EXPORT_SYMBOL);
    if (sym == nullptr)
    {
        Log_Error("Missing '%s' export", DOWNLOAD_HANDLER__Initialize__EXPORT_SYMBOL);
    }
    else
    {
        auto initializeFn = reinterpret_cast<InitializeFn>(sym);
        initializeFn(logLevel);
    }
}

DownloadHandlerPlugin::~DownloadHandlerPlugin()
{
    Log_Debug("Calling '" DOWNLOAD_HANDLER__Cleanup__EXPORT_SYMBOL "' export on download handler.");
    void* sym = lib->GetSymbol(DOWNLOAD_HANDLER__Cleanup__EXPORT_SYMBOL);
    if (sym == nullptr)
    {
        Log_Error("Missing '%s' export", DOWNLOAD_HANDLER__Cleanup__EXPORT_SYMBOL);
    }
    else
    {
        auto cleanupFn = reinterpret_cast<CleanupFn>(sym);
        cleanupFn();
    }
}

ADUC_Result DownloadHandlerPlugin::ProcessUpdate(
    const ADUC_WorkflowHandle workflowHandle, const ADUC_FileEntity* fileEntity, const char* targetFilePath) const
{
    Log_Debug(
        "Calling '" DOWNLOAD_HANDLER__ProcessUpdate__EXPORT_SYMBOL
        "' export on download handler with targetFilePath '%s'.",
        targetFilePath);
    void* sym = lib->GetSymbol(DOWNLOAD_HANDLER__ProcessUpdate__EXPORT_SYMBOL);
    if (sym == nullptr)
    {
        Log_Error("Missing '%s' export", DOWNLOAD_HANDLER__ProcessUpdate__EXPORT_SYMBOL);
        return ADUC_Result{ ADUC_GeneralResult_Failure, ADUC_ERC_DOWNLOAD_HANDLER_PLUGIN_MISSING_EXPORT_SYMBOL };
    }

    auto produceUpdateFn = reinterpret_cast<ProcessUpdateFn>(sym);
    return produceUpdateFn(workflowHandle, fileEntity, targetFilePath);
}

ADUC_Result DownloadHandlerPlugin::OnUpdateWorkflowCompleted(const ADUC_WorkflowHandle workflowHandle) const
{
    Log_Debug("Calling '" DOWNLOAD_HANDLER__OnUpdateWorkflowCompleted__EXPORT_SYMBOL "' export on download handler.");
    void* sym = lib->GetSymbol(DOWNLOAD_HANDLER__OnUpdateWorkflowCompleted__EXPORT_SYMBOL);
    if (sym == nullptr)
    {
        Log_Error("Missing '%s' export", DOWNLOAD_HANDLER__OnUpdateWorkflowCompleted__EXPORT_SYMBOL);
        return ADUC_Result{ ADUC_GeneralResult_Failure, ADUC_ERC_DOWNLOAD_HANDLER_PLUGIN_MISSING_EXPORT_SYMBOL };
    }

    auto onUpdateWorkflowCompletedFn = reinterpret_cast<OnUpdateWorkflowCompletedFn>(sym);
    return onUpdateWorkflowCompletedFn(workflowHandle);
}

ADUC_Result DownloadHandlerPlugin::GetContractInfo(ADUC_ExtensionContractInfo* contractInfo) const
{
    ADUC_Result result = { ADUC_GeneralResult_Success, 0 };

    Log_Debug("Calling '" DOWNLOAD_HANDLER__GetContractInfo__EXPORT_SYMBOL "' export on download handler.");

    void* sym = lib->GetSymbol(DOWNLOAD_HANDLER__GetContractInfo__EXPORT_SYMBOL);
    if (sym == nullptr)
    {
        contractInfo->majorVer = ADUC_V1_CONTRACT_MAJOR_VER;
        contractInfo->minorVer = ADUC_V1_CONTRACT_MINOR_VER;
    }
    else
    {
        auto getContractInfoFn = reinterpret_cast<GetContractInfoFn>(sym);
        result = getContractInfoFn(contractInfo);
    }

    return result;
}

EXTERN_C_BEGIN

ADUC_Result ADUC_DownloadHandlerPlugin_OnUpdateWorkflowCompleted(
    const DownloadHandlerHandle handle, const ADUC_WorkflowHandle workflowHandle)
{
    ADUC_Result result = { ADUC_Result_Failure,
                           ADUC_ERC_DOWNLOAD_HANDLER_PLUGIN_ON_UPDATE_WORKFLOW_COMPLETED_FAILURE };
    try
    {
        std::unique_ptr<DownloadHandlerPlugin> plugin{ reinterpret_cast<DownloadHandlerPlugin*>(handle) };
        result = plugin->OnUpdateWorkflowCompleted(workflowHandle);
    }
    catch (...)
    {
    }

    Log_Info("OnUpdateWorkflowCompleted result: %d, erc: %08x.", result.ResultCode, result.ExtendedResultCode);

    return result;
}

EXTERN_C_END
