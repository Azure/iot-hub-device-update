/**
 * @file deliveryoptimization_content_downloader.cpp
 * @brief Content Downloader Extension using Microsoft Delivery Optimization Agent.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/connection_string_utils.h"
#include "aduc/content_downloader_extension.hpp"
#include "aduc/contract_utils.h"
#include "aduc/hash_utils.h"
#include "aduc/logging.h"
#include "aduc/process_utils.hpp"

#include <atomic>
#include <sstream>
#include <stdio.h> // for FILE
#include <stdlib.h> // for calloc
#include <strings.h> // for strcasecmp
#include <sys/stat.h> // for stat
#include <vector>

#include <do_config.h>
#include <do_download.h>

namespace MSDO = microsoft::deliveryoptimization;

ADUC_Result do_download(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int retryTimeout,
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

    std::stringstream fullFilePath;
    fullFilePath << workFolder << "/" << entity->TargetFilename;

    Log_Info(
        "Downloading File '%s' from '%s' to '%s'",
        entity->TargetFilename,
        entity->DownloadUri,
        fullFilePath.str().c_str());

    const std::error_code doErrorCode = MSDO::download::download_url_to_path(
        entity->DownloadUri, fullFilePath.str(), false, std::chrono::seconds(retryTimeout));
    if (!doErrorCode)
    {
        resultCode = ADUC_Result_Download_Success;
        extendedResultCode = 0;
    }
    else
    {
        // Note: The call to download_url_to_path() does not make use of a cancellation token,
        // so the download can only timeout or hit a fatal error.
        Log_Error("DO error, msg: %s, code: %#08x, timeout? %d", doErrorCode.message().c_str(), doErrorCode.value(),
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
                fullFilePath.str().c_str(),
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
            fullFilePath.str().c_str(),
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
    const off_t fileSize{ (stat(fullFilePath.str().c_str(), &st) == 0) ? st.st_size : 0 };

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
