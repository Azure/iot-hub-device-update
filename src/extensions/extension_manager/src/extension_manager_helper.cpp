/**
 * @file extension_manager_helper.cpp
 * @brief Implementation of ExtensionManager helpers.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/extension_manager_helper.hpp"

#include <aduc/download_handler_factory.hpp>
#include <aduc/download_handler_plugin.hpp>
#include <aduc/download_utils.hpp>
#include <aduc/result.h>
#include <aduc/string_c_utils.h>
#include <aduc/workflow_utils.h>
#include <content_protection_utils.h>
#include <string>

/**
 * @brief Processes Download Handler extensibility for the downloadHandlerId in the file entity.
 *
 * @param workflowHandle The workflow handle.
 * @param entity The file entity with the downloader handler id.
 * @param targetUpdateFilePath The target file path to which to write the resultant update.
 * @return ADUC_Result The result.
 */
ADUC_Result ProcessDownloadHandlerExtensibility(
    ADUC_WorkflowHandle workflowHandle, const ADUC_FileEntity* entity, const char* targetUpdateFilePath) noexcept
{
    ADUC_Result result = { ADUC_GeneralResult_Failure, 0 };

    if (workflowHandle == nullptr || entity == nullptr || IsNullOrEmpty(entity->DownloadHandlerId)
        || IsNullOrEmpty(targetUpdateFilePath))
    {
        result.ExtendedResultCode = ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_HELPER_BAD_ARG;
        return result;
    }

    ADUC_ExtensionContractInfo contractInfo{};
    DownloadHandlerFactory* factory = nullptr;
    DownloadHandlerPlugin* plugin = nullptr;
    const char* downloadHandlerId = entity->DownloadHandlerId;

    try
    {
        factory = DownloadHandlerFactory::GetInstance();
    }
    catch (...)
    {
        result.ExtendedResultCode = ADUC_ERC_DOWNLOAD_HANDLER_CREATE_FACTORY_INSTANCE;
        goto done;
    }

    plugin = factory->LoadDownloadHandler(downloadHandlerId);
    if (plugin == nullptr)
    {
        Log_Warn("Load Download Handler %s failed", downloadHandlerId);

        result.ExtendedResultCode = ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_CREATE_FAILURE_CREATE;

        workflow_set_success_erc(workflowHandle, ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_CREATE_FAILURE_CREATE);

        goto done;
    }

    Log_Debug("Getting contract info for download handler '%s'.", downloadHandlerId);

    result = plugin->GetContractInfo(&contractInfo);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error(
            "GetContractInfo failed for download handler '%s': result 0x%08x, erc 0x%08x",
            downloadHandlerId,
            result.ResultCode,
            result.ExtendedResultCode);

        workflow_set_success_erc(workflowHandle, ADUC_ERC_DOWNLOAD_HANDLER_EXTENSIBILITY_GET_CONTRACT);

        goto done;
    }

    Log_Debug(
        "Downloadhandler '%s' Contract Version: %d.%d",
        downloadHandlerId,
        contractInfo.majorVer,
        contractInfo.minorVer);

    if (!(contractInfo.majorVer == ADUC_V1_CONTRACT_MAJOR_VER && contractInfo.minorVer == ADUC_V1_CONTRACT_MINOR_VER))
    {
        Log_Error("Unsupported contract %d.%d", contractInfo.majorVer, contractInfo.minorVer);

        result.ResultCode = ADUC_GeneralResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_UNSUPPORTED_CONTRACT_VERSION;

        workflow_set_success_erc(
            workflowHandle, ADUC_ERC_DOWNLOAD_HANDLER_EXTENSION_MANAGER_UNSUPPORTED_CONTRACT_VERSION);

        goto done;
    }

    Log_Info("Invoking DownloadHandler plugin ProcessUpdate for '%s'", targetUpdateFilePath);

    result = plugin->ProcessUpdate(workflowHandle, entity, targetUpdateFilePath);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        workflow_set_success_erc(workflowHandle, result.ExtendedResultCode);
        workflow_set_result_details(workflowHandle, "plugin err %d for ProcessUpdate", result.ExtendedResultCode);
        goto done;
    }

done:

    Log_Info("DownloadHandler Extensibility ret %d, erc 0x%08x", result.ResultCode, result.ExtendedResultCode);

    return result;
}

/**
 * @brief Handles decryption of the update content paylaod if the update is encrypted.
 *
 * @param workflowHandle The workflow handle.
 * @param workFolder The sandbox work folder.
 * @param entity The file entity.
 * @return ADUC_Result The result.
 */
ADUC_Result
HandleContentDecryption(ADUC_WorkflowHandle workflowHandle, const char* workFolder, const ADUC_FileEntity* entity)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure, 0 };
    ADUC_Decryption_Info decryptionInfo{};

    if (workflow_are_payloads_encrypted(workflowHandle))
    {
        decryptionInfo.decryptedDEK = workflow_get_dek(workflowHandle);
        //TODO: decrypt the dek using kek. phase 0 it would be in decrypted form.

        std::string downloadedSandboxFilePath = aduc::download_utils::build_download_filepath(workFolder, entity);

        result = ContentProtectionUtils_DecryptFile(&decryptionInfo, downloadedSandboxFilePath.c_str());
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            Log_Error(
                "DecryptFile '%s' failed: ERC %08x", downloadedSandboxFilePath.c_str(), result.ExtendedResultCode);
            goto done;
        }
    }

    result.ResultCode = ADUC_Result_Success;

done:
    DecryptionInfo_Uninit(&decryptionInfo);

    return result;
}
