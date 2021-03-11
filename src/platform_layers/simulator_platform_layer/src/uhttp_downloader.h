/**
 * @file uhttp_downloader.h
 * @brief Simple HTTP downloader based on uHTTP
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * Note that uHTTP is a rudimentary HTTP implementation and may not support production-level requirements.
 */
#ifndef UHTTP_DOWNLOADER_H
#define UHTTP_DOWNLOADER_H

typedef enum tagUHttpDownloaderResult
{
    DR_OK,
    DR_INVALID_ARG,
    DR_TIMEOUT,
    DR_FILE_ERROR,

    DR_ERROR,
    DR_OPEN_FAILED,
    DR_SEND_FAILED,
    DR_ALREADY_INIT,
    DR_HTTP_HEADERS_FAILED,
    DR_INVALID_STATE,

    DR_CALLBACK_OPEN_FAILED,
    DR_CALLBACK_SEND_FAILED,
    DR_CALLBACK_ERROR,
    DR_CALLBACK_PARSING_ERROR,
    DR_CALLBACK_DESTROY,
    DR_CALLBACK_DISCONNECTED,
} UHttpDownloaderResult;

UHttpDownloaderResult
DownloadFile(const char* url, const char* base64Sha256Hash, const char* outputFile, unsigned int timeoutSecs = 60);

#endif // UHTTP_DOWNLOADER_H
