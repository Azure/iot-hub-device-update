/**
 * @file extension_manager.h
 * @brief Definition of the ExtensionManager (for C)
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_EXTENSION_MANAGER_H
#define ADUC_EXTENSION_MANAGER_H

#include <aduc/extension_manager_download_options.h> /* ExtensionManager_DownloadOptions */
#include <aduc/result.h> /* ADUC_RESULT */
#include <aduc/types/download.h> /* ADUC_DownloadProgressCallback */
#include <aduc/types/update_content.h> /* ADUC_FileEntity */
#include <aduc/types/workflow.h> /* ADUC_WorkflowHandle */

EXTERN_C_BEGIN

/**
 * @brief Initializes the content downloader.
 *
 * @param initializeData The initialization data.
 * @return ADUC_Result The result of initialization.
 */
ADUC_Result ExtensionManager_InitializeContentDownloader(const char* initializeData);

/**
 * @brief Downloads by using metadata download handler id and falls back to download with content downloader extension.
 *
 * @param entity The file entity to download.
 * @param workflowHandle The workflow handle opaque object for per-workflow workflow data.
 * @param options The options for the download.
 * @param downloadProgressCallback The download progress callback.
 * @return ADUC_Result The result of downloading.
 */
ADUC_Result ExtensionManager_Download(
    const ADUC_FileEntity* entity,
    ADUC_WorkflowHandle workflowHandle,
    ExtensionManager_Download_Options* options,
    ADUC_DownloadProgressCallback downloadProgressCallback);

/**
 * @brief Uninitializes the extension manager.
 */
void ExtensionManager_Uninit();

/**
 * @brief Gets the file path of the entity target update under the download work folder sandbox.
 *
 * @param workflowHandle The workflow handle.
 * @param entity The file entity.
 * @return char* the workflow folder, or NULL on error.
 */
char* ExtensionManager_GetEntityWorkFolderFilePath(ADUC_WorkflowHandle workflowHandle, const ADUC_FileEntity* entity);

EXTERN_C_END

#endif // ADUC_EXTENSION_MANAGER_H
