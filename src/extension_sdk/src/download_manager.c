/**
 * @file download_manager.c
 * @brief Implementation of the ADU Gen2 download manager.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/download_manager.h"

#include <curl/curl.h>
#include <errno.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#define STRCASECMP _stricmp
#else
#include <unistd.h>
#include <strings.h>
#define SLEEP_MS(ms) usleep((ms) * 1000)
#define STRCASECMP strcasecmp
#endif

#define SHA256_HEX_LEN 64
#define SHA256_BUF_MIN (SHA256_HEX_LEN + 1)
#define READ_BUF_SIZE 8192
#define PART_SUFFIX ".part"

#define DEFAULT_RETRY_COUNT 3
#define DEFAULT_RETRY_DELAY_MS 1000

// Error codes within ADUC_FACILITY_DOWNLOAD
#define DL_ERR_INVALID_ARG   ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 0x0001)
#define DL_ERR_ALLOC         ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_RESOURCE, 0x0001)
#define DL_ERR_FILE_OPEN     ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 0x0002)
#define DL_ERR_CURL_INIT     ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_PROTOCOL, 0x0001)
#define DL_ERR_CURL_PERFORM  ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_PROTOCOL, 0x0002)
#define DL_ERR_HASH_MISMATCH ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 0x0003)
#define DL_ERR_SIZE_MISMATCH ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 0x0004)
#define DL_ERR_RENAME        ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 0x0005)
#define DL_ERR_HASH_COMPUTE  ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_IO, 0x0006)
#define DL_ERR_CANCELLED     ADUC_RESULT2_MAKE(ADUC_FACILITY_DOWNLOAD, ADUC_CATEGORY_STATE, 0x0001)

typedef struct ADUC_DownloadState
{
    CURL* curl;
    FILE* fp;
    char* url;
    char* destPath;
    char* tempPath;
    char* expectedSha256;
    uint64_t expectedSize;
    uint32_t timeoutSec;
    uint32_t retryCount;
    uint32_t retryDelayMs;

    ADUC_DownloadProgressFn progressFn;
    void* progressCtx;

    volatile int cancelFlag;
    volatile int complete;
    uint64_t bytesDownloaded;
    uint64_t totalBytes;
    ADUC_Result2 result;
} ADUC_DownloadState;

static char* str_dup(const char* s)
{
    if (s == NULL)
    {
        return NULL;
    }
    size_t len = strlen(s);
    char* d = (char*)malloc(len + 1);
    if (d != NULL)
    {
        memcpy(d, s, len + 1);
    }
    return d;
}

static size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    ADUC_DownloadState* state = (ADUC_DownloadState*)userdata;
    size_t bytes = size * nmemb;
    size_t written = fwrite(ptr, 1, bytes, state->fp);
    state->bytesDownloaded += written;
    return written;
}

static int progress_callback(
    void* clientp,
    curl_off_t dltotal,
    curl_off_t dlnow,
    curl_off_t ultotal,
    curl_off_t ulnow)
{
    (void)ultotal;
    (void)ulnow;

    ADUC_DownloadState* state = (ADUC_DownloadState*)clientp;

    if (state->cancelFlag)
    {
        return 1; // non-zero aborts transfer
    }

    if (dltotal > 0)
    {
        state->totalBytes = (uint64_t)dltotal;
    }

    if (state->progressFn != NULL)
    {
        state->progressFn(
            state->destPath,
            state->bytesDownloaded,
            state->totalBytes,
            state->progressCtx);
    }

    return 0;
}

static uint64_t get_file_size(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (f == NULL)
    {
        return 0;
    }
    if (fseek(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return 0;
    }
    long size = ftell(f);
    fclose(f);
    return (size > 0) ? (uint64_t)size : 0;
}

ADUC_Result2 ADUC_Download_ComputeSha256(
    const char* filePath,
    char* hashBuf,
    size_t hashBufLen)
{
    if (filePath == NULL || hashBuf == NULL || hashBufLen < SHA256_BUF_MIN)
    {
        return DL_ERR_INVALID_ARG;
    }

    FILE* f = fopen(filePath, "rb");
    if (f == NULL)
    {
        return DL_ERR_FILE_OPEN;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx == NULL)
    {
        fclose(f);
        return DL_ERR_ALLOC;
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1)
    {
        EVP_MD_CTX_free(ctx);
        fclose(f);
        return DL_ERR_HASH_COMPUTE;
    }

    unsigned char buf[READ_BUF_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buf, 1, sizeof(buf), f)) > 0)
    {
        if (EVP_DigestUpdate(ctx, buf, bytesRead) != 1)
        {
            EVP_MD_CTX_free(ctx);
            fclose(f);
            return DL_ERR_HASH_COMPUTE;
        }
    }

    fclose(f);

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;
    if (EVP_DigestFinal_ex(ctx, digest, &digestLen) != 1)
    {
        EVP_MD_CTX_free(ctx);
        return DL_ERR_HASH_COMPUTE;
    }
    EVP_MD_CTX_free(ctx);

    // Hex-encode
    static const char hexChars[] = "0123456789abcdef";
    for (unsigned int i = 0; i < digestLen; i++)
    {
        hashBuf[i * 2] = hexChars[(digest[i] >> 4) & 0x0F];
        hashBuf[i * 2 + 1] = hexChars[digest[i] & 0x0F];
    }
    hashBuf[digestLen * 2] = '\0';

    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_Download_Start(
    const ADUC_DownloadRequest* request,
    ADUC_DownloadProgressFn progressFn,
    void* progressCtx,
    ADUC_DownloadHandle* outHandle)
{
    if (request == NULL || request->url == NULL || request->destPath == NULL || outHandle == NULL)
    {
        return DL_ERR_INVALID_ARG;
    }

    ADUC_DownloadState* state = (ADUC_DownloadState*)calloc(1, sizeof(ADUC_DownloadState));
    if (state == NULL)
    {
        return DL_ERR_ALLOC;
    }

    state->url = str_dup(request->url);
    state->destPath = str_dup(request->destPath);
    state->expectedSha256 = str_dup(request->expectedSha256);
    state->expectedSize = request->expectedSize;
    state->timeoutSec = request->timeoutSec;
    state->retryCount = (request->retryCount > 0) ? request->retryCount : DEFAULT_RETRY_COUNT;
    state->retryDelayMs = (request->retryDelayMs > 0) ? request->retryDelayMs : DEFAULT_RETRY_DELAY_MS;
    state->progressFn = progressFn;
    state->progressCtx = progressCtx;

    if (state->url == NULL || state->destPath == NULL)
    {
        free(state->url);
        free(state->destPath);
        free(state->expectedSha256);
        free(state);
        return DL_ERR_ALLOC;
    }

    // Build temp path
    size_t destLen = strlen(state->destPath);
    state->tempPath = (char*)malloc(destLen + sizeof(PART_SUFFIX));
    if (state->tempPath == NULL)
    {
        free(state->url);
        free(state->destPath);
        free(state->expectedSha256);
        free(state);
        return DL_ERR_ALLOC;
    }
    memcpy(state->tempPath, state->destPath, destLen);
    memcpy(state->tempPath + destLen, PART_SUFFIX, sizeof(PART_SUFFIX));

    // Check for existing partial download (resume support)
    uint64_t existingBytes = get_file_size(state->tempPath);
    state->bytesDownloaded = existingBytes;

    // Open temp file for append (resume) or write
    state->fp = fopen(state->tempPath, existingBytes > 0 ? "ab" : "wb");
    if (state->fp == NULL)
    {
        free(state->url);
        free(state->destPath);
        free(state->tempPath);
        free(state->expectedSha256);
        free(state);
        return DL_ERR_FILE_OPEN;
    }

    // Initialize curl
    state->curl = curl_easy_init();
    if (state->curl == NULL)
    {
        fclose(state->fp);
        free(state->url);
        free(state->destPath);
        free(state->tempPath);
        free(state->expectedSha256);
        free(state);
        return DL_ERR_CURL_INIT;
    }

    curl_easy_setopt(state->curl, CURLOPT_URL, state->url);
    curl_easy_setopt(state->curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(state->curl, CURLOPT_WRITEDATA, state);
    curl_easy_setopt(state->curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(state->curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(state->curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(state->curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(state->curl, CURLOPT_XFERINFODATA, state);

    if (existingBytes > 0)
    {
        curl_easy_setopt(state->curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)existingBytes);
    }

    if (state->timeoutSec > 0)
    {
        curl_easy_setopt(state->curl, CURLOPT_TIMEOUT, (long)state->timeoutSec);
    }

    *outHandle = (ADUC_DownloadHandle)state;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_Download_Wait(ADUC_DownloadHandle handle)
{
    if (handle == NULL)
    {
        return DL_ERR_INVALID_ARG;
    }

    ADUC_DownloadState* state = (ADUC_DownloadState*)handle;
    CURLcode res = CURLE_OK;
    uint32_t attempts = 0;

    while (attempts <= state->retryCount)
    {
        if (state->cancelFlag)
        {
            state->result = DL_ERR_CANCELLED;
            state->complete = 1;
            if (state->fp != NULL)
            {
                fclose(state->fp);
                state->fp = NULL;
            }
            return state->result;
        }

        res = curl_easy_perform(state->curl);

        if (res == CURLE_OK)
        {
            break;
        }

        // Check if cancelled via progress callback
        if (state->cancelFlag)
        {
            state->result = DL_ERR_CANCELLED;
            state->complete = 1;
            if (state->fp != NULL)
            {
                fclose(state->fp);
                state->fp = NULL;
            }
            return state->result;
        }

        attempts++;
        if (attempts <= state->retryCount)
        {
            SLEEP_MS(state->retryDelayMs);

            // Reopen file for resume on next attempt
            if (state->fp != NULL)
            {
                fclose(state->fp);
                state->fp = NULL;
            }
            uint64_t currentBytes = get_file_size(state->tempPath);
            state->bytesDownloaded = currentBytes;
            state->fp = fopen(state->tempPath, "ab");
            if (state->fp == NULL)
            {
                state->result = DL_ERR_FILE_OPEN;
                state->complete = 1;
                return state->result;
            }
            curl_easy_setopt(state->curl, CURLOPT_WRITEDATA, state);
            curl_easy_setopt(state->curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)currentBytes);
        }
    }

    // Close file
    if (state->fp != NULL)
    {
        fclose(state->fp);
        state->fp = NULL;
    }

    if (res != CURLE_OK)
    {
        state->result = DL_ERR_CURL_PERFORM;
        state->complete = 1;
        return state->result;
    }

    // Validate file size if expected
    if (state->expectedSize > 0)
    {
        uint64_t actualSize = get_file_size(state->tempPath);
        if (actualSize != state->expectedSize)
        {
            remove(state->tempPath);
            state->result = DL_ERR_SIZE_MISMATCH;
            state->complete = 1;
            return state->result;
        }
    }

    // Validate SHA-256 hash if expected
    if (state->expectedSha256 != NULL)
    {
        char actualHash[SHA256_BUF_MIN];
        ADUC_Result2 hashResult = ADUC_Download_ComputeSha256(state->tempPath, actualHash, sizeof(actualHash));
        if (ADUC_RESULT2_IS_FAILURE(hashResult))
        {
            remove(state->tempPath);
            state->result = hashResult;
            state->complete = 1;
            return state->result;
        }

        if (STRCASECMP(actualHash, state->expectedSha256) != 0)
        {
            remove(state->tempPath);
            state->result = DL_ERR_HASH_MISMATCH;
            state->complete = 1;
            return state->result;
        }
    }

    // Rename .part file to final destination
    // Remove destination first in case it exists
    remove(state->destPath);
    if (rename(state->tempPath, state->destPath) != 0)
    {
        state->result = DL_ERR_RENAME;
        state->complete = 1;
        return state->result;
    }

    state->result = ADUC_RESULT2_SUCCESS;
    state->complete = 1;
    return state->result;
}

void ADUC_Download_Cancel(ADUC_DownloadHandle handle)
{
    if (handle == NULL)
    {
        return;
    }
    ADUC_DownloadState* state = (ADUC_DownloadState*)handle;
    state->cancelFlag = 1;
}

bool ADUC_Download_IsComplete(ADUC_DownloadHandle handle)
{
    if (handle == NULL)
    {
        return true;
    }
    ADUC_DownloadState* state = (ADUC_DownloadState*)handle;
    return state->complete != 0;
}

uint64_t ADUC_Download_GetBytesDownloaded(ADUC_DownloadHandle handle)
{
    if (handle == NULL)
    {
        return 0;
    }
    ADUC_DownloadState* state = (ADUC_DownloadState*)handle;
    return state->bytesDownloaded;
}

void ADUC_Download_Destroy(ADUC_DownloadHandle handle)
{
    if (handle == NULL)
    {
        return;
    }
    ADUC_DownloadState* state = (ADUC_DownloadState*)handle;

    if (state->curl != NULL)
    {
        curl_easy_cleanup(state->curl);
    }
    if (state->fp != NULL)
    {
        fclose(state->fp);
    }
    free(state->url);
    free(state->destPath);
    free(state->tempPath);
    free(state->expectedSha256);
    free(state);
}

ADUC_Result2 ADUC_Download_File(
    const ADUC_DownloadRequest* request,
    ADUC_DownloadProgressFn progressFn,
    void* progressCtx)
{
    ADUC_DownloadHandle handle = NULL;

    ADUC_Result2 result = ADUC_Download_Start(request, progressFn, progressCtx, &handle);
    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        return result;
    }

    result = ADUC_Download_Wait(handle);
    ADUC_Download_Destroy(handle);
    return result;
}
