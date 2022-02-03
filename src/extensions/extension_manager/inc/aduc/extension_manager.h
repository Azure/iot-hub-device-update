/**
 * @file extension_manager.h
 * @brief Definition of the ExtensionManager (for C)
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_EXTENSION_MANAGER_H
#define ADUC_EXTENSION_MANAGER_H

EXTERN_C_BEGIN

/**
 * @brief Initializes the content downloader.
 *
 * @param initializeData The initialization data.
 * @return ADUC_Result The result of initialization.
 */
ADUC_Result ExtensionManager_InitializeContentDownloader(const char* initializeData);

/**
 * @brief Downloads using the content downloader extension.
 *
 * @param entity The file entity to download.
 * @param workflowId The workflow id.
 * @param workFolder The work folder sandbox.
 * @param retryTimeout The retry timeout.
 * @param downloadProgressCallback The download progress callback.
 * @return ADUC_Result The result of downloading.
 */
ADUC_Result ExtensionManager_Download(const ADUC_FileEntity* entity, const char* workflowId, const char* workFolder, unsigned int retryTimeout, ADUC_DownloadProgressCallback downloadProgressCallback);

/**
 * @brief Uninitializes the extension manager.
 */
void ExtensionManager_Uninit();

EXTERN_C_END

#endif // ADUC_EXTENSION_MANAGER_H
