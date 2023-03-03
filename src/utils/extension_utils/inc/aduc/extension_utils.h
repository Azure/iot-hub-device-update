/**
 * @file extension_utils.h
 * @brief Utilities for the Device Update Agent extensibility.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_EXTENSION_UTILS_H
#define ADUC_EXTENSION_UTILS_H

#include "aduc/types/update_content.h" // ADUC_FileEntity
#include "aduc/types/workflow.h" // ADUC_WorkflowHandle
#include <stdbool.h> // bool

EXTERN_C_BEGIN

bool GetDownloadHandlerFileEntity(const char* downloadHandlerId, ADUC_FileEntity* fileEntity);

bool GetExtensionFileEntity(const char* extensionRegFile, ADUC_FileEntity* fileEntity);

bool RegisterUpdateContentHandler(const char* updateType, const char* handlerFilePath);

bool RegisterDownloadHandler(const char* downloadHandlerId, const char* handlerFilePath);

bool RegisterComponentEnumeratorExtension(const char* extensionFilePath);

bool RegisterContentDownloaderExtension(const char* extensionFilePath);

bool RegisterExtension(const char* extensionDir, const char* extensionFilePath);

EXTERN_C_END

#endif // ADUC_EXTENSION_UTILS_H
