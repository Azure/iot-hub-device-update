/**
 * @file download_service.h
 * @brief Download Service — core orchestrator for content delivery.
 *
 * The Download Service selects content downloaders, validates integrity,
 * manages resume state, and applies file properties. It is NOT an extension;
 * it is part of ADU Core.
 */
#ifndef ADUC_DOWNLOAD_SERVICE_H
#define ADUC_DOWNLOAD_SERVICE_H

#include "aduc/content_downloader_vtable.h"
#include "aduc/extension_types.h"
#include "aduc/file_info.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ADUC_DownloadService ADUC_DownloadService;

// Forward declaration
typedef struct ADUC_ExtensionRegistry ADUC_ExtensionRegistry;

typedef struct ADUC_DownloadServiceConfig
{
    uint32_t maxRetries;
    uint32_t retryBackoffSec;
    uint32_t connectTimeoutSec;
    uint32_t transferTimeoutSec;
    uint32_t maxConcurrent;
    const char* tempDir;
    const char* stateDir;
    const char* preferredDownloader;  ///< Capability string, empty = auto
} ADUC_DownloadServiceConfig;

/**
 * @brief Create a download service.
 */
ADUC_Result2 ADUC_DownloadService_Create(
    ADUC_DownloadService** outService,
    ADUC_ExtensionRegistry* registry,
    const ADUC_DownloadServiceConfig* config);

/**
 * @brief Download a single file described by ADUC_FileInfo.
 * Selects downloader, validates hash, sets file properties.
 */
ADUC_Result2 ADUC_DownloadService_DownloadFile(
    ADUC_DownloadService* service,
    ADUC_FileInfo* fileInfo,
    const char* destDir,
    const ADUC_DownloadCallbacks* callbacks);

/**
 * @brief Download multiple files.
 */
ADUC_Result2 ADUC_DownloadService_DownloadFiles(
    ADUC_DownloadService* service,
    ADUC_FileInfo* fileInfos,
    size_t count,
    const char* destDir,
    const ADUC_DownloadCallbacks* callbacks);

/**
 * @brief Resume incomplete downloads from previous session.
 */
ADUC_Result2 ADUC_DownloadService_ResumeAll(ADUC_DownloadService* service);

/**
 * @brief Cancel all in-progress downloads.
 */
ADUC_Result2 ADUC_DownloadService_CancelAll(ADUC_DownloadService* service);

/**
 * @brief Destroy the download service.
 */
void ADUC_DownloadService_Destroy(ADUC_DownloadService* service);

#ifdef __cplusplus
}
#endif

#endif // ADUC_DOWNLOAD_SERVICE_H
