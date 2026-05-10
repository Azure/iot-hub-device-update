/**
 * @file download_manager.h
 * @brief Download manager for ADU Gen2 agent content delivery.
 *
 * Handles downloading files referenced in deployment manifests with:
 * - HTTP/HTTPS transport via libcurl
 * - Resume support (range requests)
 * - SHA-256 hash validation
 * - Progress reporting
 * - Cancellation
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_DOWNLOAD_MANAGER_H
#define ADUC_DOWNLOAD_MANAGER_H

#include "aduc/extension_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct ADUC_DownloadRequest
{
    const char* url;             ///< Source URL
    const char* destPath;        ///< Destination file path
    const char* expectedSha256;  ///< Expected SHA-256 hash (NULL to skip verification)
    uint64_t expectedSize;       ///< Expected file size (0 to skip check)
    uint32_t timeoutSec;         ///< Per-download timeout (0 = no limit)
    uint32_t retryCount;         ///< Number of retries (default 3)
    uint32_t retryDelayMs;       ///< Delay between retries (default 1000)
} ADUC_DownloadRequest;

/**
 * @brief Progress callback invoked periodically during download.
 */
typedef void (*ADUC_DownloadProgressFn)(
    const char* fileId,
    uint64_t bytesDownloaded,
    uint64_t totalBytes,
    void* ctx);

typedef void* ADUC_DownloadHandle;

/**
 * @brief Start downloading a single file (non-blocking setup).
 */
ADUC_Result2 ADUC_Download_Start(
    const ADUC_DownloadRequest* request,
    ADUC_DownloadProgressFn progressFn,
    void* progressCtx,
    ADUC_DownloadHandle* outHandle);

/**
 * @brief Wait for download to complete (blocking).
 */
ADUC_Result2 ADUC_Download_Wait(ADUC_DownloadHandle handle);

/**
 * @brief Cancel an in-progress download.
 */
void ADUC_Download_Cancel(ADUC_DownloadHandle handle);

/**
 * @brief Check if download is complete.
 */
bool ADUC_Download_IsComplete(ADUC_DownloadHandle handle);

/**
 * @brief Get bytes downloaded so far.
 */
uint64_t ADUC_Download_GetBytesDownloaded(ADUC_DownloadHandle handle);

/**
 * @brief Free download handle and associated resources.
 */
void ADUC_Download_Destroy(ADUC_DownloadHandle handle);

/**
 * @brief Convenience: synchronous download (blocks until complete).
 */
ADUC_Result2 ADUC_Download_File(
    const ADUC_DownloadRequest* request,
    ADUC_DownloadProgressFn progressFn,
    void* progressCtx);

/**
 * @brief Compute SHA-256 hash of a file.
 * @param filePath  Path to the file.
 * @param hashBuf   Output buffer for hex-encoded hash string.
 * @param hashBufLen Size of hashBuf (must be >= 65 for SHA-256 hex + NUL).
 */
ADUC_Result2 ADUC_Download_ComputeSha256(
    const char* filePath,
    char* hashBuf,
    size_t hashBufLen);

#ifdef __cplusplus
}
#endif

#endif // ADUC_DOWNLOAD_MANAGER_H
