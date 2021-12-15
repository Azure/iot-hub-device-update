/**
 * @file download.h
 * @brief Defines types related to file download functionality.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_TYPES_DOWNLOAD_H
#define ADUC_TYPES_DOWNLOAD_H

#include <stdint.h> // uint64_t

/**
 * @brief Defines download progress state for download progress callback.
 */
typedef enum tagADUC_DownloadProgressState
{
    ADUC_DownloadProgressState_NotStarted = 0,
    ADUC_DownloadProgressState_InProgress,
    ADUC_DownloadProgressState_Completed,
    ADUC_DownloadProgressState_Cancelled,
    ADUC_DownloadProgressState_Error
} ADUC_DownloadProgressState;

/**
 * @brief Function signature for callback to send download progress to.
 */
typedef void (*ADUC_DownloadProgressCallback)(
    const char* workflowId,
    const char* fileId,
    ADUC_DownloadProgressState state,
    uint64_t bytesTransferred,
    uint64_t bytesTotal);

#endif // ADUC_TYPES_DOWNLOAD_H
