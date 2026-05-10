/**
 * @file curl_downloader_v2.c
 * @brief EDK-based curl content downloader extension.
 *
 * Handles https:// and http:// URI schemes with resume, retry, and progress.
 * Capability: "adu/downloader:curl"
 */
#include "aduc/content_downloader_vtable.h"
#include "aduc/extension_descriptor.h"
#include "aduc/extension_types.h"

#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ─── State ──────────────────────────────────────────────────────────────────

static struct
{
    CURL* curl;
    bool suspended;
    bool cancelled;
    uint64_t bytesDownloaded;
    uint64_t bytesTotal;
    double startTime;
} s_state = {0};

// ─── Callbacks ──────────────────────────────────────────────────────────────

typedef struct WriteCtx
{
    FILE* file;
    const ADUC_DownloadCallbacks* callbacks;
    uint64_t offset;
} WriteCtx;

static size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    WriteCtx* ctx = (WriteCtx*)userdata;
    size_t written = fwrite(ptr, size, nmemb, ctx->file);
    s_state.bytesDownloaded += written;

    if (ctx->callbacks && ctx->callbacks->OnProgress)
    {
        ctx->callbacks->OnProgress(
            s_state.bytesDownloaded + ctx->offset,
            s_state.bytesTotal,
            ctx->callbacks->userData);
    }

    if (ctx->callbacks && ctx->callbacks->IsCancelled &&
        ctx->callbacks->IsCancelled(ctx->callbacks->userData))
    {
        s_state.cancelled = true;
        return 0; // Abort transfer
    }

    return written;
}

static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                             curl_off_t ultotal, curl_off_t ulnow)
{
    (void)ultotal;
    (void)ulnow;
    (void)clientp;
    (void)dlnow;

    if (dltotal > 0)
    {
        s_state.bytesTotal = (uint64_t)dltotal;
    }

    return s_state.cancelled ? 1 : 0;
}

// ─── Vtable Implementation ──────────────────────────────────────────────────

static bool CurlDL_CanHandle(const char* uri)
{
    if (!uri) return false;
    return (strncmp(uri, "https://", 8) == 0 || strncmp(uri, "http://", 7) == 0);
}

static ADUC_Result2 CurlDL_Download(
    const char* uri,
    const char* destPath,
    uint64_t offset,
    const ADUC_DownloadCallbacks* callbacks)
{
    if (!uri || !destPath)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 1);
    }

    s_state.cancelled = false;
    s_state.suspended = false;
    s_state.bytesDownloaded = 0;
    s_state.bytesTotal = 0;
    s_state.startTime = (double)time(NULL);

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_RESOURCE, 1);
    }
    s_state.curl = curl;

    // Open file for append (resume) or write
    const char* mode = (offset > 0) ? "ab" : "wb";
    FILE* fp = fopen(destPath, mode);
    if (!fp)
    {
        curl_easy_cleanup(curl);
        s_state.curl = NULL;
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 1);
    }

    WriteCtx wctx = { .file = fp, .callbacks = callbacks, .offset = offset };

    curl_easy_setopt(curl, CURLOPT_URL, uri);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &wctx);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    // Resume support
    if (offset > 0)
    {
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)offset);
    }

    // TLS settings
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    // Proxy support via env var
    const char* proxy = getenv("ADUC_HTTPS_PROXY");
    if (proxy && proxy[0])
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxy);
    }

    CURLcode res = curl_easy_perform(curl);

    fclose(fp);
    curl_easy_cleanup(curl);
    s_state.curl = NULL;

    if (res == CURLE_OK)
    {
        return ADUC_RESULT2_SUCCESS;
    }
    else if (s_state.cancelled)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 60);
    }
    else if (res == CURLE_RANGE_ERROR || res == CURLE_HTTP_RETURNED_ERROR)
    {
        // Non-retryable
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_PROTOCOL, (uint16_t)res);
    }
    else
    {
        // Transient / retryable
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, (uint16_t)res);
    }
}

static ADUC_Result2 CurlDL_Suspend(void)
{
    s_state.suspended = true;
    s_state.cancelled = true; // triggers abort in write callback
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 CurlDL_Cancel(void)
{
    s_state.cancelled = true;
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 CurlDL_GetStats(ADUC_DownloadStats* outStats)
{
    if (!outStats)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 1);
    }

    double now = (double)time(NULL);
    outStats->bytesDownloaded = s_state.bytesDownloaded;
    outStats->bytesTotal = s_state.bytesTotal;
    outStats->elapsedSeconds = now - s_state.startTime;
    outStats->bytesPerSecond = (outStats->elapsedSeconds > 0)
        ? (double)s_state.bytesDownloaded / outStats->elapsedSeconds
        : 0.0;
    outStats->retryCount = 0;

    return ADUC_RESULT2_SUCCESS;
}

// ─── Extension Descriptor ───────────────────────────────────────────────────

static ADUC_Result2 CurlDL_Initialize(const ADUC_ExtensionContext* ctx)
{
    (void)ctx;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    return ADUC_RESULT2_SUCCESS;
}

static void CurlDL_Uninitialize(void)
{
    curl_global_cleanup();
}

static const ADUC_DownloaderVtable s_curlVtable = {
    .structVersion = 1,
    .CanHandle = CurlDL_CanHandle,
    .Download = CurlDL_Download,
    .Suspend = CurlDL_Suspend,
    .Cancel = CurlDL_Cancel,
    .GetStats = CurlDL_GetStats,
};

static const char* s_capabilities[] = { "adu/downloader:curl", NULL };

static const ADUC_ExtensionDescriptor s_descriptor = {
    .structVersion = ADUC_EXTENSION_DESCRIPTOR_VERSION,
    .id = "microsoft.adu.downloader.curl",
    .name = "ADU Curl Content Downloader",
    .version = "2.0.0",
    .type = ADUC_EXT_TYPE_DOWNLOADER,
    .minHostApiVersion = ADUC_HOST_API_VERSION,
    .Initialize = CurlDL_Initialize,
    .Uninitialize = CurlDL_Uninitialize,
    .vtable = &s_curlVtable,
    .capabilities = s_capabilities,
};

ADUC_DECLARE_EXTENSION(s_descriptor)
