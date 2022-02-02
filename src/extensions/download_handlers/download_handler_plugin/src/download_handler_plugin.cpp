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
#include <aduc/plugin_call_helper.hpp> // for CallExport

using InitializeFn = void (*)(ADUC_LOG_SEVERITY logLevel);

using CleanupFn = void (*)();

using ProcessUpdateFn = ADUC_Result (*)(
    const ADUC_WorkflowHandle workflowHandle, const ADUC_FileEntity* fileEntity, const char* targetFilePath);

using OnUpdateWorkflowCompletedFn = ADUC_Result (*)(const ADUC_WorkflowHandle workflowHandle);
using GetContractInfoFn = ADUC_Result (*)(ADUC_ExtensionContractInfo* contractInfo);

/**
 * @brief Construct a new Download Handler Plugin object
 *
 * @param libPath The path to the dynamic library.
 * @param logLevel The log severity.
 */
DownloadHandlerPlugin::DownloadHandlerPlugin(const std::string& libPath, ADUC_LOG_SEVERITY logLevel) : lib(libPath)
{
    const char* const symbol = DOWNLOAD_HANDLER__Initialize__EXPORT_SYMBOL;
    CallExport<InitializeFn, false /* ExportReturnsAducResult */, ADUC_LOG_SEVERITY>(
        symbol, lib, nullptr /* outResult */, logLevel);
}

/**
 * @brief Destroy the Download Handler Plugin object
 *
 */
DownloadHandlerPlugin::~DownloadHandlerPlugin()
{
    const char* const symbol = DOWNLOAD_HANDLER__Cleanup__EXPORT_SYMBOL;
    CallExport<CleanupFn, false /* ExportReturnsAducResult */>(symbol, lib, nullptr /* outResult */);
}

/**
 * @brief Processes the update to either produce the target file path so that the core agent can skip downloading the
 * update payload or do some pre-download processing and then tell the agent to continue on downloading the payload.
 *
 * @param workflowHandle The workflow handle, containing the update deployment info.
 * @param fileEntity The file entity with the update payload metadata, where RelatedFiles is most applicable to
 * download handlers.
 * @param targetFilePath The file path of the file that the plugin should create if wanting to return a ResultCode of
 * ADUC_Result_Download_Handler_SuccessSkipDownload.
 * @return ADUC_Result The result. When able to produce the target file path using workflowHandle and fileEntity inputs,
 * it returns a result with ResultCode of ADUC_Result_Download_Handler_SuccessSkipDownload to tell the agent to skip
 * downloading the update content. When it wants the agent to go ahead and download the update payload as usual, it
 * returns ADUC_Result_Download_Handler_RequiredFullDownload success ResultCode.
 */
ADUC_Result DownloadHandlerPlugin::ProcessUpdate(
    const ADUC_WorkflowHandle workflowHandle, const ADUC_FileEntity* fileEntity, const char* targetFilePath) const
{
    ADUC_Result result{ ADUC_GeneralResult_Failure, 0 };

    const char* const symbol = DOWNLOAD_HANDLER__ProcessUpdate__EXPORT_SYMBOL;
    CallExport<ProcessUpdateFn, true /* ExportReturnsAducResult */>(
        symbol, lib, &result /* outResult */, workflowHandle, fileEntity, targetFilePath);

    Log_Info(
        "DownloadHandlerPlugin ProcessUpdate result - rc: %d, erc: %08x",
        result.ResultCode,
        result.ExtendedResultCode);

    return result;
}

/**
 * @brief Calls the download handler plugin's export function to handle workflow completion.
 * This is called on by the core agent for update payloads associated with this download handler when the update
 * deployment workflow was successfully applied to the device. The plugin can do cleanup of resources it needed for
 * payloads associated with this download handler plugin as well as any post-processing needed such as copying files
 * from the sandbox to caches, etc.
 *
 * @param workflowHandle The workflow handle.
 * @return ADUC_Result The result.
 */
ADUC_Result DownloadHandlerPlugin::OnUpdateWorkflowCompleted(const ADUC_WorkflowHandle workflowHandle) const
{
    ADUC_Result result{ ADUC_GeneralResult_Failure, 0 };

    CallExport<OnUpdateWorkflowCompletedFn, true /* ExportReturnsAducResult */>(
        DOWNLOAD_HANDLER__OnUpdateWorkflowCompleted__EXPORT_SYMBOL, lib, &result /* outResult */, workflowHandle);

    Log_Info(
        "DownloadHandlerPlugin OnUpdateWorkflowCompleted result - rc: %d, erc: %08x",
        result.ResultCode,
        result.ExtendedResultCode);

    return result;
}

/**
 * @brief Gets the contract info for the download handler plugin.
 *
 * @param[out] contractInfo The contract info.
 * @return ADUC_Result The result.
 */
ADUC_Result DownloadHandlerPlugin::GetContractInfo(ADUC_ExtensionContractInfo* contractInfo) const
{
    ADUC_Result result{ ADUC_GeneralResult_Failure, 0 };

    CallExport<GetContractInfoFn, true /* ExportReturnsAducResult */>(
        DOWNLOAD_HANDLER__GetContractInfo__EXPORT_SYMBOL, lib, &result /* outResult */, contractInfo);

    Log_Info(
        "DownloadHandlerPlugin GetContractInfo result - rc: %d, erc: %08x",
        result.ResultCode,
        result.ExtendedResultCode);

    return result;
}

EXTERN_C_BEGIN

/**
 * @brief The C API for invoking the post-processing after the update deployment has completed hook for an update with
 * a download handler id .
 *
 * @param handle  The download handler handle opaque object.
 * @param workflowHandle  The workflow handle.
 * @return ADUC_Result The result.
 */
ADUC_Result ADUC_DownloadHandlerPlugin_OnUpdateWorkflowCompleted(
    const DownloadHandlerHandle handle, const ADUC_WorkflowHandle workflowHandle)
{
    ADUC_Result result = { ADUC_Result_Failure,
                           ADUC_ERC_DOWNLOAD_HANDLER_PLUGIN_ON_UPDATE_WORKFLOW_COMPLETED_FAILURE };
    try
    {
        // Do not free the DownloadHandlerHandle handle that is owned by DownloadHandlerFactory.
        if (handle != nullptr)
        {
            auto plugin = reinterpret_cast<DownloadHandlerPlugin*>(handle);
            result = plugin->OnUpdateWorkflowCompleted(workflowHandle);
        }
    }
    catch (...)
    {
    }

    return result;
}

EXTERN_C_END
