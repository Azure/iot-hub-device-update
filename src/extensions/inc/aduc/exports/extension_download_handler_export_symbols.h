/**
 * @file extension_download_handler_export_symbols.h
 * @brief The function export symbols for download handler extensions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef EXTENSION_DOWNLOAD_HANDLER_EXPORT_SYMBOLS_H
#define EXTENSION_DOWNLOAD_HANDLER_EXPORT_SYMBOLS_H

#include <aduc/exports/extension_common_export_symbols.h> // for GetContractInfo__EXPORT_SYMBOL

//
// The Download Handler Extension export V1 symbols.
// These are the V1 symbols that must be implemented by Download Handler Extensions.
//
#define DOWNLOAD_HANDLER__GetContractInfo__EXPORT_SYMBOL GetContractInfo__EXPORT_SYMBOL

/**
 * @brief One-time initialization for the download handler.
 *
 * @param logLevel The desired loglevel if logging is used.
 * @details void Initialize(ADUC_LOG_SEVERITY logLevel)
 */
#define DOWNLOAD_HANDLER__Initialize__EXPORT_SYMBOL "Initialize"

/**
 * @brief Cleanup logic before library is unloaded.
 * @details void Cleanup()
 */
#define DOWNLOAD_HANDLER__Cleanup__EXPORT_SYMBOL "Cleanup"

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
 * @details ADUC_Result ProcessUpdate(const ADUC_WorkflowHandle workflowHandle, const ADUC_FileEntity* fileEntity, const char* targetUpdateFilePath)
 */
#define DOWNLOAD_HANDLER__ProcessUpdate__EXPORT_SYMBOL "ProcessUpdate"

/**
 * @brief Called when the update workflow successfully completes.
 * In the case of Delta download handler plugin, it moves all the payloads from sandbox to cache
 * so that they will available as source updates for future delta updates.
 *
 * @param[in] workflowHandle The workflow handle.
 * @return ADUC_Result The result.
 * @details ADUC_Result OnUpdateWorkflowCompleted(const ADUC_WorkflowHandle workflowHandle)
 */
#define DOWNLOAD_HANDLER__OnUpdateWorkflowCompleted__EXPORT_SYMBOL "OnUpdateWorkflowCompleted"

#endif // EXTENSION_DOWNLOAD_HANDLER_EXPORT_SYMBOLS_H
