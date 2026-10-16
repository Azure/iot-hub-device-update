/**
 * @file curl_content_downloader.cpp
 * @brief Content Downloader Extension using curl command.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "curl_content_downloader.h"
#include "aduc/config_utils.h" // ADUC_ConfigInfo*
#include "aduc/content_downloader_extension.hpp"
#include "aduc/contract_utils.h"
#include "aduc/hash_utils.h"
#include "aduc/logging.h"
#include "aduc/process_utils.hpp" // for ADUC_LaunchChildProcess

#include <algorithm> // for std::min
#include <chrono>
#include <sstream>
#include <string>
#include <stdint.h> // for UINT32_MAX
#include <sys/stat.h> // for stat
#include <thread> // for std::this_thread::sleep_for
#include <vector>

// keep this last to minimize chance to interfere with system header includes.
#include "aduc/aduc_banned.h"

//
// Retry tuning for downloadNoProgressTimeoutInMinutes. An attempt that receives no bytes for
// MaxStallSecondsPerAttempt is aborted, so a stalled connection is retried.
//
static const unsigned int MaxStallSecondsPerAttempt = 60;
static const unsigned int MaxConnectSecondsPerAttempt = 30;
static const std::chrono::seconds MaxRetryDelay{ 30 };

bool IsTransientCurlExitCode(int exitCode)
{
    // See https://curl.se/libcurl/c/libcurl-errors.html
    switch (exitCode)
    {
    case 5: // Couldn't resolve proxy
    case 6: // Couldn't resolve host
    case 7: // Failed to connect to host
    case 16: // HTTP/2 framing layer error
    case 18: // Partial file
    case 28: // Operation timed out, including --speed-time
    case 35: // SSL connect error
    case 52: // Empty reply from server
    case 55: // Failed sending network data
    case 56: // Failure in receiving network data
    case 92: // HTTP/2 stream error
        return true;
    default:
        return false;
    }
}

static off_t GetFileSize(const std::string& filePath)
{
    struct stat st;
    return (stat(filePath.c_str(), &st) == 0) ? st.st_size : 0;
}

/**
 * @brief Gets the no-progress timeout from the downloadNoProgressTimeoutInMinutes config.
 * @return The timeout in seconds, or 0 when it is not configured.
 */
static unsigned int GetNoProgressTimeoutInSeconds()
{
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    if (config == nullptr)
    {
        Log_Warn("Cannot get config info. Downloading without retries.");
        return 0;
    }

    const unsigned int maxMinutes = UINT32_MAX / 60;
    const unsigned int seconds = 60 * std::min(config->downloadNoProgressTimeoutInMinutes, maxMinutes);
    ADUC_ConfigInfo_ReleaseInstance(config);
    return seconds;
}

/**
 * @brief Runs curl, retrying transient failures while the download makes progress.
 * @return The exit code of the last curl attempt.
 */
static int RunCurl(
    const char* downloadUri,
    const std::string& filePath,
    unsigned int timeoutInSeconds,
    unsigned int noProgressTimeoutInSeconds,
    std::string& output)
{
    std::vector<std::string> args;

    // -L (or --location). Handle 3xx redirects. See https://curl.se/docs/manpage.html#-L
    args.emplace_back("-L");

    // -C (or --continue-at). The hyphen after, i.e. '-C -', allows auto-resuming the transfer. See https://curl.se/docs/manpage.html#-C
    args.emplace_back("-C");
    args.emplace_back("-");

    if (noProgressTimeoutInSeconds > 0)
    {
        // Abort an attempt that receives no bytes so that it is retried. See https://curl.se/docs/manpage.html#-y
        const unsigned int stallSeconds = std::min(noProgressTimeoutInSeconds, MaxStallSecondsPerAttempt);
        args.emplace_back("--speed-limit");
        args.emplace_back("1");
        args.emplace_back("--speed-time");
        args.emplace_back(std::to_string(stallSeconds));
        args.emplace_back("--connect-timeout");
        args.emplace_back(std::to_string(std::min(stallSeconds, MaxConnectSecondsPerAttempt)));
    }

    // -o (or --output). Output to file instead of stdout. See https://curl.se/docs/manpage.html#-o
    // NOTE: -O (or --remote-name) is not needed as we already have an /absolute/ file path.
    args.emplace_back("-o");
    args.emplace_back(filePath);

    // Finally, tack the url onto the end
    args.emplace_back(downloadUri);

    const auto start = std::chrono::steady_clock::now();
    auto lastProgress = start;
    off_t lastSize = GetFileSize(filePath);
    std::chrono::seconds retryDelay{ 1 };

    for (;;)
    {
        output.clear();
        const int exitCode = ADUC_LaunchChildProcess("/usr/bin/curl", args, output);
        if (exitCode == 0 || noProgressTimeoutInSeconds == 0 || !IsTransientCurlExitCode(exitCode))
        {
            return exitCode;
        }

        const auto now = std::chrono::steady_clock::now();
        const off_t size = GetFileSize(filePath);
        if (size > lastSize)
        {
            lastSize = size;
            lastProgress = now;
            retryDelay = std::chrono::seconds{ 1 };
        }

        const auto secondsWithoutProgress =
            std::chrono::duration_cast<std::chrono::seconds>(now - lastProgress).count();
        const auto secondsElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if (secondsWithoutProgress >= noProgressTimeoutInSeconds || secondsElapsed >= timeoutInSeconds)
        {
            Log_Error(
                "curl failed with exit code %d. Giving up after %lld seconds without progress, %lld seconds in total.",
                exitCode,
                static_cast<long long>(secondsWithoutProgress),
                static_cast<long long>(secondsElapsed));
            return exitCode;
        }

        Log_Warn(
            "curl failed with exit code %d. Retrying in %lld seconds, %lld bytes downloaded so far.",
            exitCode,
            static_cast<long long>(retryDelay.count()),
            static_cast<long long>(size));
        std::this_thread::sleep_for(retryDelay);
        retryDelay = std::min(retryDelay * 2, MaxRetryDelay);
    }
}

ADUC_Result Download_curl(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int timeoutInSeconds,
    ADUC_DownloadProgressCallback downloadProgressCallback)
{
    return Download_curl_WithNoProgressTimeout(
        entity, workflowId, workFolder, timeoutInSeconds, GetNoProgressTimeoutInSeconds(), downloadProgressCallback);
}

ADUC_Result Download_curl_WithNoProgressTimeout(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int timeoutInSeconds,
    unsigned int noProgressTimeoutInSeconds,
    ADUC_DownloadProgressCallback downloadProgressCallback)
{
    ADUC_Result result = { ADUC_Result_Failure };
    SHAversion algVersion;
    std::string output;
    int exitCode = 1;
    std::stringstream fullFilePath;
    bool isValidHash;
    bool reportProgress = false;

    if (entity == nullptr)
    {
        result.ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_INVALID_FILE_ENTITY;
        goto done;
    }

    if (entity->DownloadUri == nullptr || *entity->DownloadUri == 0)
    {
        result.ExtendedResultCode = ADUC_ERC_CONTENT_DOWNLOADER_INVALID_DOWNLOAD_URI;
        goto done;
    }

    if (entity->HashCount == 0)
    {
        Log_Error("File entity does not contain a file hash! Cannot validate cancelling download.");
        result.ExtendedResultCode = ADUC_ERC_VALIDATION_FILE_HASH_IS_EMPTY;
        if (downloadProgressCallback != nullptr)
        {
            downloadProgressCallback(
                workflowId,
                entity->FileId,
                ADUC_DownloadProgressState_Error,
                result.ResultCode,
                result.ExtendedResultCode);
        }
        goto done;
    }

    fullFilePath << workFolder << "/" << entity->TargetFilename;

    if (!ADUC_HashUtils_GetShaVersionForTypeString(
            ADUC_HashUtils_GetHashType(entity->Hash, entity->HashCount, 0), &algVersion))
    {
        Log_Error(
            "FileEntity for %s has unsupported hash type %s",
            fullFilePath.str().c_str(),
            ADUC_HashUtils_GetHashType(entity->Hash, entity->HashCount, 0));
        result.ExtendedResultCode = ADUC_ERC_VALIDATION_FILE_HASH_TYPE_NOT_SUPPORTED;

        if (downloadProgressCallback != nullptr)
        {
            downloadProgressCallback(
                workflowId,
                entity->FileId,
                ADUC_DownloadProgressState_Error,
                result.ResultCode,
                result.ExtendedResultCode);
        }
        goto done;
    }

    // If target file exists, validate file hash.
    // If file is valid, then skip the download.
    isValidHash = ADUC_HashUtils_IsValidFileHash(
        fullFilePath.str().c_str(),
        ADUC_HashUtils_GetHashValue(entity->Hash, entity->HashCount, 0),
        algVersion,
        false /* suppressErrorLog */);

    if (isValidHash)
    {
        result = { ADUC_Result_Download_Skipped_FileExists };
        reportProgress = true;
        goto done;
    }

    Log_Info(
        "Downloading File '%s' from '%s' to '%s'",
        entity->TargetFilename,
        entity->DownloadUri,
        fullFilePath.str().c_str());

    exitCode = RunCurl(
        entity->DownloadUri, fullFilePath.str(), timeoutInSeconds, noProgressTimeoutInSeconds, output);

    if (exitCode == 0)
    {
        result = { ADUC_Result_Download_Success };
    }
    else
    {
        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = ADUC_ERROR_CURL_DOWNLOADER_EXTERNAL_FAILURE(exitCode);
        reportProgress = true;
        goto done;
    }

    Log_Info("Download output:: \n%s", output.c_str());

    // If we downloaded successfully, validate the file hash.
    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        // Note: Currently we expect there to be only one hash, but
        // support for multiple hashes is already built in.
        Log_Info("Validating file hash");

        const bool isValid = ADUC_HashUtils_IsValidFileHash(
            fullFilePath.str().c_str(),
            ADUC_HashUtils_GetHashValue(entity->Hash, entity->HashCount, 0),
            algVersion,
            true /* suppressErrorLog */);

        if (!isValid)
        {
            Log_Error("Hash for %s is not valid", entity->TargetFilename);

            result.ResultCode = ADUC_Result_Failure;
            result.ExtendedResultCode = ADUC_ERC_VALIDATION_FILE_HASH_INVALID_HASH;
            reportProgress = true;
            goto done;
        }
    }

done:

    if (reportProgress && (downloadProgressCallback != nullptr))
    {
        if (IsAducResultCodeSuccess(result.ResultCode))
        {
            struct stat st;
            const off_t fileSize{ (stat(fullFilePath.str().c_str(), &st) == 0) ? st.st_size : 0 };
            downloadProgressCallback(
                workflowId, entity->FileId, ADUC_DownloadProgressState_Completed, fileSize, entity->SizeInBytes);
        }
        else
        {
            downloadProgressCallback(
                workflowId,
                entity->FileId,
                (result.ResultCode == ADUC_Result_Failure_Cancelled) ? ADUC_DownloadProgressState_Cancelled
                                                                     : ADUC_DownloadProgressState_Error,
                0,
                entity->SizeInBytes);
        }
    }

    Log_Info(
        "Download task end. resultCode: %d, extendedCode: %d (0x%X)",
        result.ResultCode,
        result.ExtendedResultCode,
        result.ExtendedResultCode);
    return result;
}
