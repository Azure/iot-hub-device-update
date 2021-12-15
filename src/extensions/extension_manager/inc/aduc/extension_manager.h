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

ADUC_Result ExtensionManager_InitializeContentDownloader(const char* initializeData);
ADUC_Result ExtensionManager_Download(const ADUC_FileEntity* entity, const char* workflowId, const char* workFolder, unsigned int retryTimeout, ADUC_DownloadProgressCallback downloadProgressCallback);

EXTERN_C_END

#endif // ADUC_EXTENSION_MANAGER_H
