/**
 * @file content_downloader_helper.h
 * @brief Helper functions for content download operations
 *
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#ifndef ADUC_CONTENT_DOWNLOADER_HELPER_H
#define ADUC_CONTENT_DOWNLOADER_HELPER_H

#include "aduc/result.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Progress callback function type
 */
typedef void (*ADUC_DownloadProgressCallback)(void* context, size_t downloadedBytes, size_t totalBytes);

/**
 * @brief Download context structure
 */
typedef struct
{
    char* downloadUri;                          ///< Download URI
    char* targetPath;                          ///< Target file path
    ADUC_DownloadProgressCallback progressCallback; ///< Progress callback
    void* progressContext;                     ///< Progress callback context
    size_t totalBytes;                         ///< Total bytes to download
    size_t downloadedBytes;                    ///< Bytes downloaded so far
} ADUC_DownloadContext;

/**
 * @brief Create a download context
 * @return Pointer to new download context, or NULL on failure
 */
ADUC_DownloadContext* ADUC_DownloadContext_Create(void);

/**
 * @brief Free a download context
 * @param context The download context to free
 */
void ADUC_DownloadContext_Free(ADUC_DownloadContext* context);

/**
 * @brief Set download URI
 * @param context The download context
 * @param uri The download URI
 * @return ADUC_Result_t indicating success or failure
 */
ADUC_Result_t ADUC_DownloadContext_SetUri(ADUC_DownloadContext* context, const char* uri);

/**
 * @brief Set target path
 * @param context The download context
 * @param path The target file path
 * @return ADUC_Result_t indicating success or failure
 */
ADUC_Result_t ADUC_DownloadContext_SetTargetPath(ADUC_DownloadContext* context, const char* path);

#ifdef __cplusplus
}
#endif

#endif // ADUC_CONTENT_DOWNLOADER_HELPER_H
