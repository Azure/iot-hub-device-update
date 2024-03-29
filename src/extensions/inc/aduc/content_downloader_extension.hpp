/**
 * @file content_downloader_extension.hpp
 * @brief Defines APIs for Device Update Content Downloader.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_CONTENT_DOWNLOADER_EXTENSION_HPP
#define ADUC_CONTENT_DOWNLOADER_EXTENSION_HPP

#include "aduc/adu_core_exports.h"

EXTERN_C_BEGIN

typedef ADUC_Result (*InitializeProc)(const char* initializeData);

typedef ADUC_Result (*DownloadProc)(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int timeoutInSeconds,
    ADUC_DownloadProgressCallback downloadProgressCallback);

EXTERN_C_END

#endif // ADUC_CONTENT_DOWNLOADER_EXTENSION_HPP
