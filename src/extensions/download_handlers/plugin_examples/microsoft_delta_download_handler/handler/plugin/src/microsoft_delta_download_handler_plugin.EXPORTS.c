
/**
 * @file microsoft_delta_download_handler_plugin.c
 * @brief An example implementation of a DownloadHandler plugin module
 * that produces full target updates using a source update update cache and
 * can cache updates once they've been verified to be good upon workflow success.
 *
 * This plugin module provides the following exported function symbols to satisfy the DownloadHandler agent interface:
 * Initialize                 - Do one-time initialization (e.g. initialize logging),
 * Cleanup                    - Free resources and cleanup right before unloading,
 * ProcessUpdate              - Do processing using data provided by ADUC_WorkflowHandle and update file metadata (ADUC_FileEntity),
 * OnUpdateWorkflowCompleted  - Callback for post-processing when the current update has been installed and applied successfully.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/microsoft_delta_download_handler.h"

#include <aduc/contract_utils.h> // ADUC_ExtensionContractInfo
#include <aduc/logging.h> // ADUC_Logging_*, Log_*
#include <aduc/types/adu_core.h> // ADUC_Result_*

/////////////////////////////////////////////////////////////////////////////
// BEGIN Shared Library Export Functions
//
// These are the function symbols that the device update agent will
// lookup and call.
//

/**
 * @brief One-time initialization for the download handler.
 *
 * @param logLevel The desired loglevel if logging is used.
 */
void Initialize(ADUC_LOG_SEVERITY logLevel)
{
    ADUC_Logging_Init(logLevel, "delta-download-handler");
}

/**
 * @brief Cleanup logic before library is unloaded.
 */
void Cleanup()
{
    ADUC_Logging_Uninit();
}

/**
 * @brief Processes the target update from FileEntity metadata at the given output filepath.
 * For this download handler, each relatedFile in the FileEntity metadata represents a delta update,
 * which is much smaller than the target update content. It attempts to download the delta update and
 * produce the target update using the delta processor. If successful, it tells the agent to skip download;
 * otherwise, it tells the agent that a full download is required.
 *
 * @param[in] workflowHandle The workflow handle.
 * @param[in] fileEntity The FileEntity metadata of the update content and its related files.
 * @param[in] targetUpdateFilePath The target update path to write the update content when returning ADUC_Result_Download_Handler_SuccessSkipDownload.
 * @return ADUC_Result The result.
 * Returning ADUC_Result_Download_Handler_SuccessSkipDownload ResultCode tells the agent to skip downloading the update content (since this download handler able to produce it at the targetUpdateFilePath).
 * Returning ADUC_Result_Download_Handler_RequiredFullDownload ResultCode tells the agent to download the update content (this download handler did not produce it by other means).
 */
ADUC_Result ProcessUpdate(
    const ADUC_WorkflowHandle workflowHandle, const ADUC_FileEntity* fileEntity, const char* targetUpdateFilePath)
{
    return MicrosoftDeltaDownloadHandler_ProcessUpdate(
        workflowHandle, fileEntity, targetUpdateFilePath, NULL /* updateCacheBasePath */);
}

/**
 * @brief Called when the update workflow successfully completes.
 * In the case of Delta download handler plugin, it moves all the payloads from sandbox to cache
 * so that they will available as source updates for future delta updates.
 *
 * @param[in] workflowHandle The workflow handle.
 * @return ADUC_Result The result.
 */
ADUC_Result OnUpdateWorkflowCompleted(const ADUC_WorkflowHandle workflowHandle)
{
    return MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted(workflowHandle, NULL /* updateCacheBasePath */);
}

/**
 * @brief Gets the extension contract info.
 *
 * @param[out] contractInfo The extension contract info.
 * @return ADUC_Result The result.
 */
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo)
{
    ADUC_Result result = { ADUC_GeneralResult_Success, 0 };
    contractInfo->majorVer = ADUC_V1_CONTRACT_MAJOR_VER;
    contractInfo->minorVer = ADUC_V1_CONTRACT_MINOR_VER;
    return result;
}

//
// END Shared Library Export Functions
/////////////////////////////////////////////////////////////////////////////
