/**
 * @file download_logstrings.h
 * @brief Log format strings for download service and downloader events.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_DOWNLOAD_LOGSTRINGS_H
#define ADUC_DOWNLOAD_LOGSTRINGS_H

#define ADUC_LOG_DL_SERVICE_INIT        "Download service initialized with %d downloaders"
#define ADUC_LOG_DL_SELECT              "Selected downloader '%s' for URI scheme '%s'"
#define ADUC_LOG_DL_START               "Downloading: uri=%s, dest=%s, offset=%ld"
#define ADUC_LOG_DL_PROGRESS            "Download progress: %ld/%ld bytes (%.1f%%)"
#define ADUC_LOG_DL_RETRY               "Download retry %d/%d after %dms: uri=%s"
#define ADUC_LOG_DL_HASH_OK             "Hash validation passed: algorithm=%s"
#define ADUC_LOG_DL_HASH_FAIL           "Hash validation FAILED: expected=%s, actual=%s"
#define ADUC_LOG_DL_RESUME              "Resuming download from offset %ld"
#define ADUC_LOG_DL_CANCEL              "Download cancelled: uri=%s"
#define ADUC_LOG_DL_COMPLETE            "Download complete: %ld bytes, %lums elapsed"
#define ADUC_LOG_DL_SIDELOAD            "Side-loading from local path: %s"
#define ADUC_LOG_DL_CURL_ERROR          "cURL error: code=%d, msg=%s"

#endif // ADUC_DOWNLOAD_LOGSTRINGS_H
