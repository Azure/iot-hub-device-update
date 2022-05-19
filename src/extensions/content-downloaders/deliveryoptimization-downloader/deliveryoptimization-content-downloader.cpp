/**
 * @file deliveryoptimization_content_downloader.cpp
 * @brief Content Downloader Extension using Microsoft Delivery Optimization Agent.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/connection_string_utils.h"
#include "aduc/content_downloader_extension.hpp"
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
#include <do_exceptions.h>

namespace MSDO = microsoft::deliveryoptimization;

EXTERN_C_BEGIN

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

    try
    {
        MSDO::download::download_url_to_path(
            entity->DownloadUri, fullFilePath.str(), false, std::chrono::seconds(retryTimeout));

        resultCode = ADUC_Result_Download_Success;
    }
    // Catch DO exception only to get extended result code. Other exceptions will be caught by CallResultMethodAndHandleExceptions
    catch (const MSDO::exception& e)
    {
        const int32_t doErrorCode = e.error_code();

        Log_Info("Caught DO exception, msg: %s, code: %d (%#08x)", e.what(), doErrorCode, doErrorCode);

        if (doErrorCode == static_cast<int32_t>(std::errc::operation_canceled))
        {
            Log_Info("Download was cancelled");
            if (downloadProgressCallback != nullptr)
            {
                downloadProgressCallback(workflowId, entity->FileId, ADUC_DownloadProgressState_Cancelled, 0, 0);
            }

            resultCode = ADUC_Result_Failure_Cancelled;
        }
        else
        {
            if (doErrorCode == static_cast<int32_t>(std::errc::timed_out))
            {
                Log_Error("Download failed due to DO timeout");
            }

            resultCode = ADUC_Result_Failure;
        }

        extendedResultCode = MAKE_ADUC_DELIVERY_OPTIMIZATION_EXTENDEDRESULTCODE(doErrorCode);
    }
    catch (const std::exception& e)
    {
        Log_Error("DO download failed with an unhandled std exception: %s", e.what());

        resultCode = ADUC_Result_Failure;
        if (errno != 0)
        {
            extendedResultCode = MAKE_ADUC_ERRNO_EXTENDEDRESULTCODE(errno);
        }
        else
        {
            extendedResultCode = ADUC_ERC_NOTRECOVERABLE;
        }
    }
    catch (...)
    {
        Log_Error("DO download failed due to an unknown exception");

        resultCode = ADUC_Result_Failure;

        if (errno != 0)
        {
            extendedResultCode = MAKE_ADUC_ERRNO_EXTENDEDRESULTCODE(errno);
        }
        else
        {
            extendedResultCode = ADUC_ERC_NOTRECOVERABLE;
        }
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

ADUC_Result Download(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int retryTimeout,
    ADUC_DownloadProgressCallback downloadProgressCallback)
{
    return do_download(entity, workflowId, workFolder, retryTimeout, downloadProgressCallback);
}

ADUC_Result Initialize(const char* initializeData)
{
    ADUC_Result result{ ADUC_GeneralResult_Success };

    if (initializeData == nullptr)
    {
        Log_Info("Skipping downloader initialization. NULL input.");
        goto done;
    }

    // The connection string is valid (IoT hub connection successful) and we are ready for further processing.
    // Send connection string to DO SDK for it to discover the Edge gateway if present.
    if (ConnectionStringUtils_IsNestedEdge(initializeData))
    {
        const int ret = deliveryoptimization_set_iot_connection_string(initializeData);
        if (ret != 0)
        {
            // Since it is nested edge and if DO fails to accept the connection string, then we go ahead and
            // fail the startup.
            Log_Error("Failed to set DO connection string in Nested Edge scenario, result: %d", ret);
            result.ResultCode = ADUC_Result_Failure;
            result.ExtendedResultCode = ADUC_ERROR_DELIVERY_OPTIMIZATION_DOWNLOADER_EXTERNAL_FAILURE(ret);
            goto done;
        }
    }

done:
    return result;
}

EXTERN_C_END
