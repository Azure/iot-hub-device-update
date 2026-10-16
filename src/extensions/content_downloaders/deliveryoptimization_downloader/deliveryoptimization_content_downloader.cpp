/**
 * @file deliveryoptimization_content_downloader.cpp
 * @brief Content Downloader Extension using Microsoft Delivery Optimization Agent.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/config_utils.h" // ADUC_ConfigInfo*
#include "aduc/connection_string_utils.h"
#include "aduc/content_downloader_extension.hpp"
#include "aduc/contract_utils.h"
#include "aduc/hash_utils.h"
#include "aduc/logging.h"
#include "aduc/process_utils.hpp"

#include <algorithm> // for std::min
#include <chrono>
#include <memory> // for std::unique_ptr
#include <stdint.h> // for uint32_t, UINT32_MAX
#include <stdio.h> // for FILE
#include <stdlib.h> // for calloc
#include <sys/stat.h> // for stat
#include <thread> // for std::this_thread::sleep_for
#include <vector>

#include <do_config.h>
#include <do_download.h>
#include <do_download_status.h>

#if defined(WIN32)
// DO currently doesn't call CoInitialize, so need to do that here.
#    include <algorithm> // std::replace
#endif

// keep this last to minimize chance to interfere with system header includes.
#include "aduc/aduc_banned.h"

namespace MSDO = microsoft::deliveryoptimization;

// DO_E_DOWNLOAD_NO_PROGRESS: Delivery Optimization paused a download that received no bytes for 5 minutes.
static const int32_t DoErrorDownloadNoProgress = static_cast<int32_t>(0x80D02002);

/**
 * @brief Gets how long a download that receives no bytes keeps being resumed.
 * @return The timeout in seconds from the downloadNoProgressTimeoutInMinutes config, or 0 when it is not configured.
 */
static uint32_t GetNoProgressTimeoutInSeconds()
{
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    if (config == nullptr)
    {
        Log_Warn("Cannot get config info. Not resuming downloads that make no progress.");
        return 0;
    }

    const unsigned int maxMinutes = UINT32_MAX / 60;
    const uint32_t seconds = 60 * std::min(config->downloadNoProgressTimeoutInMinutes, maxMinutes);
    ADUC_ConfigInfo_ReleaseInstance(config);
    return seconds;
}

/**
 * @brief Like download::start_and_wait_until_completion(), but resumes the download when Delivery Optimization
 * pauses it for making no progress, e.g. while the network interface is up but the payload host is unreachable.
 * @details Delivery Optimization only waits out a missing network interface. Without this, any other outage
 * longer than 5 minutes fails the download. Resuming keeps the bytes already downloaded.
 * @param download The download to start.
 * @param timeout How long to wait for the download in total.
 * @param noProgressTimeout How long to keep resuming the download while it receives no bytes.
 * @return The download error, or no error once the download is finalized.
 */
static std::error_code StartAndWaitResumingOnNoProgress(
    MSDO::download& download, std::chrono::seconds timeout, std::chrono::seconds noProgressTimeout)
{
    const std::chrono::milliseconds maxPollTime{ 5000 };
    std::chrono::milliseconds pollTime{ 500 };
    const auto start = std::chrono::steady_clock::now();
    auto lastProgress = start;
    uint64_t lastBytesTransferred = 0;

    std::error_code errorCode = download.start();
    if (errorCode)
    {
        return errorCode;
    }

    for (;;)
    {
        std::this_thread::sleep_for(pollTime);
        if (pollTime < maxPollTime)
        {
            pollTime += std::chrono::milliseconds{ 500 };
        }

        MSDO::download_status status;
        errorCode = download.get_status(status);
        if (errorCode)
        {
            download.abort();
            return errorCode;
        }

        const auto now = std::chrono::steady_clock::now();
        if (status.bytes_transferred() > lastBytesTransferred)
        {
            lastBytesTransferred = status.bytes_transferred();
            lastProgress = now;
        }

        if (status.state() == MSDO::download_state::transferred)
        {
            return download.finalize();
        }

        if (now - start >= timeout)
        {
            download.abort();
            return std::make_error_code(std::errc::timed_out);
        }

        if (status.state() == MSDO::download_state::paused && !status.is_transient_error())
        {
            if (status.error_code().value() != DoErrorDownloadNoProgress || now - lastProgress >= noProgressTimeout)
            {
                download.abort();
                return status.error_code();
            }

            Log_Warn(
                "DO download made no progress, resuming. %llu bytes downloaded so far.",
                static_cast<unsigned long long>(lastBytesTransferred));
            errorCode = download.resume();
            if (errorCode)
            {
                download.abort();
                return errorCode;
            }
        }
    }
}

ADUC_Result do_download(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int timeoutInSeconds,
    ADUC_DownloadProgressCallback downloadProgressCallback)
{
    ADUC_Result_t resultCode = ADUC_Result_Failure;
    ADUC_Result_t extendedResultCode = ADUC_ERC_NOTRECOVERABLE;

    if (entity->HashCount == 0)
    {
        Log_Error("File entity does not contain a file hash! Cannot validate cancelling download.");
        resultCode = ADUC_Result_Failure;
        extendedResultCode = ADUC_ERC_VALIDATION_FILE_HASH_IS_EMPTY;
        if (downloadProgressCallback != nullptr)
        {
            downloadProgressCallback(workflowId, entity->FileId, ADUC_DownloadProgressState_Error, 0, 0);
        }
        return ADUC_Result{ resultCode, extendedResultCode };
    }

    std::string fullFilePath;

#if defined(WIN32)
    // DO requires a full path. Once paths are fixed up, this can be removed.
    if (workFolder[0] == '/')
    {
        fullFilePath = "c:";
    }
#endif

    fullFilePath += workFolder;
    fullFilePath += "/";
    fullFilePath += entity->TargetFilename;

#if defined(WIN32)
    // DO requires backslashes, so convert.
    std::replace(fullFilePath.begin(), fullFilePath.end(), '/', '\\');
#endif

    Log_Info(
        "Downloading File '%s' from '%s' to '%s'", entity->TargetFilename, entity->DownloadUri, fullFilePath.c_str());

    std::unique_ptr<MSDO::download> download;
    std::error_code doErrorCode = MSDO::download::make(entity->DownloadUri, fullFilePath.c_str(), download);
    if (!doErrorCode)
    {
        const uint32_t noProgressTimeoutInSeconds = GetNoProgressTimeoutInSeconds();
        if (noProgressTimeoutInSeconds == 0)
        {
            doErrorCode = download->start_and_wait_until_completion(std::chrono::seconds(timeoutInSeconds));
        }
        else
        {
            Log_Info("Resuming the download while it makes no progress for up to %u seconds", noProgressTimeoutInSeconds);
            doErrorCode = StartAndWaitResumingOnNoProgress(
                *download,
                std::chrono::seconds(timeoutInSeconds),
                std::chrono::seconds(noProgressTimeoutInSeconds));
        }
    }

    if (!doErrorCode)
    {
        resultCode = ADUC_Result_Download_Success;
        extendedResultCode = 0;
    }
    else
    {
        // Note: The download does not make use of a cancellation token,
        // so the download can only timeout or hit a fatal error.
        Log_Error(
            "DO error, msg: %s, code: %#08x, timeout? %d",
            doErrorCode.message().c_str(),
            doErrorCode.value(),
            (doErrorCode == std::errc::timed_out));

        resultCode = ADUC_Result_Failure;
        extendedResultCode = MAKE_ADUC_DELIVERY_OPTIMIZATION_EXTENDEDRESULTCODE(doErrorCode.value());
    }

    // If we downloaded successfully, validate the file hash.
    if (resultCode == ADUC_Result_Download_Success)
    {
        Log_Info("Validating file hash");

        SHAversion algVersion;
        if (!ADUC_HashUtils_GetShaVersionForTypeString(
                ADUC_HashUtils_GetHashType(entity->Hash, entity->HashCount, 0), &algVersion))
        {
            Log_Error(
                "FileEntity for %s has unsupported hash type %s",
                fullFilePath.c_str(),
                ADUC_HashUtils_GetHashType(entity->Hash, entity->HashCount, 0));
            resultCode = ADUC_Result_Failure;
            extendedResultCode = ADUC_ERC_VALIDATION_FILE_HASH_TYPE_NOT_SUPPORTED;

            if (downloadProgressCallback != nullptr)
            {
                downloadProgressCallback(
                    workflowId, entity->FileId, ADUC_DownloadProgressState_Error, resultCode, extendedResultCode);
            }
            return ADUC_Result{ resultCode, extendedResultCode };
        }

        const bool isValid = ADUC_HashUtils_IsValidFileHash(
            fullFilePath.c_str(),
            ADUC_HashUtils_GetHashValue(entity->Hash, entity->HashCount, 0),
            algVersion,
            false /* suppressErrorLog */);

        if (!isValid)
        {
            Log_Error("Hash for %s is not valid", entity->TargetFilename);

            resultCode = ADUC_Result_Failure;
            extendedResultCode = ADUC_ERC_VALIDATION_FILE_HASH_INVALID_HASH;

            if (downloadProgressCallback != nullptr)
            {
                downloadProgressCallback(
                    workflowId, entity->FileId, ADUC_DownloadProgressState_Error, resultCode, extendedResultCode);
            }
            return ADUC_Result{ resultCode, extendedResultCode };
        }
    }

    // Report progress.
    struct stat st
    {
    };
    const off_t fileSize{ (stat(fullFilePath.c_str(), &st) == 0) ? st.st_size : 0 };

    if (downloadProgressCallback != nullptr)
    {
        if (resultCode == ADUC_Result_Download_Success)
        {
            downloadProgressCallback(
                workflowId, entity->FileId, ADUC_DownloadProgressState_Completed, fileSize, fileSize);
        }
        else
        {
            downloadProgressCallback(
                workflowId,
                entity->FileId,
                (resultCode == ADUC_Result_Failure_Cancelled) ? ADUC_DownloadProgressState_Cancelled
                                                              : ADUC_DownloadProgressState_Error,
                fileSize,
                fileSize);
        }
    }

    Log_Info("Download resultCode: %d, extendedCode: %d", resultCode, extendedResultCode);
    return ADUC_Result{ resultCode, extendedResultCode };
}
