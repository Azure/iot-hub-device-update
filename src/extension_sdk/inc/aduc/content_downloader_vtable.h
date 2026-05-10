/**
 * @file content_downloader_vtable.h
 * @brief Content downloader extension vtable — transport layer for fetching bytes.
 */
#ifndef ADUC_CONTENT_DOWNLOADER_VTABLE_H
#define ADUC_CONTENT_DOWNLOADER_VTABLE_H

#include "aduc/extension_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ADUC_DownloadCallbacks
{
    void (*OnProgress)(uint64_t bytesDownloaded, uint64_t bytesTotal, void* userData);
    bool (*IsCancelled)(void* userData);
    void* userData;
} ADUC_DownloadCallbacks;

typedef struct ADUC_DownloadStats
{
    uint64_t bytesDownloaded;
    uint64_t bytesTotal;
    double   elapsedSeconds;
    double   bytesPerSecond;
    int      retryCount;
} ADUC_DownloadStats;

/**
 * @brief Content downloader vtable — transport mechanism for byte transfer.
 * Extension descriptor's `vtable` field points to this when type == ADUC_EXT_TYPE_DOWNLOADER.
 */
typedef struct ADUC_DownloaderVtable
{
    uint32_t structVersion;  // 1

    /** Check if this downloader can handle the given URI scheme. */
    bool (*CanHandle)(const char* uri);

    /** Download (or resume) content from URI to local file.
     *  @param uri       Source URI
     *  @param destPath  Local destination file path
     *  @param offset    Byte offset to resume from (0 = start fresh)
     *  @param callbacks Progress and cancellation callbacks
     */
    ADUC_Result2 (*Download)(
        const char* uri,
        const char* destPath,
        uint64_t offset,
        const ADUC_DownloadCallbacks* callbacks);

    /** Suspend the current download. State must be resumable via Download(offset). */
    ADUC_Result2 (*Suspend)(void);

    /** Cancel and clean up. */
    ADUC_Result2 (*Cancel)(void);

    /** Get download statistics. */
    ADUC_Result2 (*GetStats)(ADUC_DownloadStats* outStats);

} ADUC_DownloaderVtable;

#ifdef __cplusplus
}
#endif

#endif // ADUC_CONTENT_DOWNLOADER_VTABLE_H
