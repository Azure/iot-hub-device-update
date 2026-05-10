/**
 * @file file_sideloader.c
 * @brief EDK-based file side-loader content downloader extension.
 *
 * Handles file:// URI scheme — loads content from local filesystem.
 * Use cases: testing, air-gapped devices, factory provisioning.
 * Capability: "adu/downloader:file"
 */
#include "aduc/content_downloader_vtable.h"
#include "aduc/extension_descriptor.h"
#include "aduc/extension_types.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define COPY_BUFFER_SIZE 65536

static struct
{
    bool cancelled;
    uint64_t bytesDownloaded;
    uint64_t bytesTotal;
} s_state = {0};

// ─── Vtable Implementation ──────────────────────────────────────────────────

static bool FileDL_CanHandle(const char* uri)
{
    if (!uri) return false;
    return (strncmp(uri, "file://", 7) == 0);
}

static ADUC_Result2 FileDL_Download(
    const char* uri,
    const char* destPath,
    uint64_t offset,
    const ADUC_DownloadCallbacks* callbacks)
{
    if (!uri || !destPath)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 1);
    }

    // Strip file:// prefix
    const char* srcPath = uri + 7;
    // Handle file:///path (3 slashes) — common on Linux
    if (srcPath[0] == '/' && srcPath[1] == '/')
    {
        srcPath += 2;  // file:////path -> /path  (unusual, handle gracefully)
    }

    s_state.cancelled = false;
    s_state.bytesDownloaded = 0;

    // Get source file size
    struct stat srcStat;
    if (stat(srcPath, &srcStat) != 0)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 2);
    }
    s_state.bytesTotal = (uint64_t)srcStat.st_size;

    if (offset > s_state.bytesTotal)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 3);
    }

    // Open source
    FILE* src = fopen(srcPath, "rb");
    if (!src)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 3);
    }

    // Seek to offset for resume
    if (offset > 0)
    {
        if (fseek(src, (long)offset, SEEK_SET) != 0)
        {
            fclose(src);
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 4);
        }
    }

    // Open destination (append for resume, write for fresh)
    const char* mode = (offset > 0) ? "ab" : "wb";
    FILE* dst = fopen(destPath, mode);
    if (!dst)
    {
        fclose(src);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 5);
    }

    // Copy with progress
    unsigned char buf[COPY_BUFFER_SIZE];
    size_t bytesRead;
    uint64_t totalCopied = offset;

    while ((bytesRead = fread(buf, 1, sizeof(buf), src)) > 0)
    {
        if (s_state.cancelled)
        {
            fclose(src);
            fclose(dst);
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 60);
        }

        if (callbacks && callbacks->IsCancelled && callbacks->IsCancelled(callbacks->userData))
        {
            s_state.cancelled = true;
            fclose(src);
            fclose(dst);
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 60);
        }

        size_t written = fwrite(buf, 1, bytesRead, dst);
        if (written != bytesRead)
        {
            fclose(src);
            fclose(dst);
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 6);
        }

        totalCopied += written;
        s_state.bytesDownloaded = totalCopied - offset;

        if (callbacks && callbacks->OnProgress)
        {
            callbacks->OnProgress(totalCopied, s_state.bytesTotal, callbacks->userData);
        }
    }

    fclose(src);
    fclose(dst);

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 FileDL_Suspend(void)
{
    s_state.cancelled = true;
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 FileDL_Cancel(void)
{
    s_state.cancelled = true;
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 FileDL_GetStats(ADUC_DownloadStats* outStats)
{
    if (!outStats)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 1);
    }

    outStats->bytesDownloaded = s_state.bytesDownloaded;
    outStats->bytesTotal = s_state.bytesTotal;
    outStats->elapsedSeconds = 0;
    outStats->bytesPerSecond = 0;
    outStats->retryCount = 0;

    return ADUC_RESULT2_SUCCESS;
}

// ─── Extension Descriptor ───────────────────────────────────────────────────

static ADUC_Result2 FileDL_Initialize(const ADUC_ExtensionContext* ctx)
{
    (void)ctx;
    return ADUC_RESULT2_SUCCESS;
}

static void FileDL_Uninitialize(void)
{
    // Nothing to clean up
}

static const ADUC_DownloaderVtable s_fileVtable = {
    .structVersion = 1,
    .CanHandle = FileDL_CanHandle,
    .Download = FileDL_Download,
    .Suspend = FileDL_Suspend,
    .Cancel = FileDL_Cancel,
    .GetStats = FileDL_GetStats,
};

static const char* s_capabilities[] = { "adu/downloader:file", NULL };

static const ADUC_ExtensionDescriptor s_descriptor = {
    .structVersion = ADUC_EXTENSION_DESCRIPTOR_VERSION,
    .id = "microsoft.adu.downloader.file",
    .name = "ADU File Side-Loader",
    .version = "2.0.0",
    .type = ADUC_EXT_TYPE_DOWNLOADER,
    .minHostApiVersion = ADUC_HOST_API_VERSION,
    .Initialize = FileDL_Initialize,
    .Uninitialize = FileDL_Uninitialize,
    .vtable = &s_fileVtable,
    .capabilities = s_capabilities,
};

ADUC_DECLARE_EXTENSION(s_descriptor)
