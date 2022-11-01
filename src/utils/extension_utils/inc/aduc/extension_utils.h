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
#include <stdbool.h> // _Bool

EXTERN_C_BEGIN

_Bool GetDownloadHandlerFileEntity(const char* downloadHandlerId, ADUC_FileEntity* fileEntity);

_Bool GetExtensionFileEntity(const char* extensionRegFile, ADUC_FileEntity* fileEntity);

_Bool RegisterUpdateContentHandler(const char* updateType, const char* handlerFilePath);

_Bool RegisterDownloadHandler(const char* downloadHandlerId, const char* handlerFilePath);

_Bool RegisterComponentEnumeratorExtension(const char* extensionFilePath);

_Bool RegisterContentDownloaderExtension(const char* extensionFilePath);

_Bool RegisterExtension(const char* extensionDir, const char* extensionFilePath);

EXTERN_C_END

#endif // ADUC_EXTENSION_UTILS_H
