/**
 * @file linux_adu_core_impl.cpp
 * @brief Implements an ADUC reference implementation library.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#include "linux_adu_core_impl.hpp"
#include <aduc/content_handler_factory.hpp>
#include <aduc/hash_utils.h>
#include <aduc/string_utils.hpp>
#include <aduc/system_utils.h>

#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

#include <memory>
#include <sstream>
#include <system_error>
#include <vector>

#include <do_download.h>
#include <do_exceptions.h>

namespace MSDO = microsoft::deliveryoptimization;

using ADUC::LinuxPlatformLayer;

/**
 * @brief Factory method for LinuxPlatformLayer
 * @return std::unique_ptr<LinuxPlatformLayer> The newly created LinuxPlatformLayer object.
 */
std::unique_ptr<LinuxPlatformLayer> LinuxPlatformLayer::Create()
{
    return std::unique_ptr<LinuxPlatformLayer>{ new LinuxPlatformLayer() };
}

/**
 * @brief Set the ADUC_RegisterData object
 *
 * @param data Object to set.
 * @return ADUC_Result ResultCode.
 */
ADUC_Result LinuxPlatformLayer::SetRegisterData(ADUC_RegisterData* data)
{
    // Message handlers.
    data->IdleCallback = IdleCallback;
    data->DownloadCallback = DownloadCallback;
    data->InstallCallback = InstallCallback;
    data->ApplyCallback = ApplyCallback;
    data->CancelCallback = CancelCallback;

    data->IsInstalledCallback = IsInstalledCallback;

    data->SandboxCreateCallback = SandboxCreateCallback;
    data->SandboxDestroyCallback = SandboxDestroyCallback;

    data->PrepareCallback = PrepareCallback;

    data->DoWorkCallback = DoWorkCallback;

    // Opaque token, passed to callbacks.

    data->Token = this;

    return ADUC_Result{ ADUC_RegisterResult_Success };
}

/**
 * @brief Class implementation of Idle method.
 */
void LinuxPlatformLayer::Idle(const char* workflowId)
{
    Log_Info("Now idle. workflowId: %s", workflowId);
    _IsCancellationRequested = false;
}

/**
 * @brief Class implementation of Prepare method.
 * @param workflowId The workflow ID.
 * @param prepareInfo
 * @return ADUC_Result
 */
ADUC_Result LinuxPlatformLayer::Prepare(const char* workflowId, const ADUC_PrepareInfo* prepareInfo)
{
    Log_Info(
        "{%s} Received Metadata, UpdateType: %s, UpdateTypeName: %s, UpdateTypeVersion: %u, FileCount: %u",
        workflowId,
        prepareInfo->updateType,
        prepareInfo->updateTypeName,
        prepareInfo->updateTypeVersion,
        prepareInfo->fileCount);

    ADUC_Result result{ ADUC_PrepareResult_Failure, ADUC_ERC_NOTRECOVERABLE };

    _contentHandler = ContentHandlerFactory::Create(prepareInfo->updateType, ContentHandlerCreateData{});

    if (!_contentHandler)
    {
        Log_Error("Failed to create content handler for update type %s.", prepareInfo->updateType);
        return ADUC_Result{ ADUC_PrepareResult_Failure, ADUC_ERC_NOTRECOVERABLE };
    }

    result = _contentHandler->Prepare(prepareInfo);
    if (result.ResultCode == ADUC_PrepareResult_Failure)
    {
        Log_Error(
            "Metadata validation failed, Version %u, File Count %u",
            prepareInfo->updateTypeVersion,
            prepareInfo->fileCount);
    }

    return result;
}

/**
 * @brief Class implementation of Download method.
 * @return ADUC_Result
 */
ADUC_Result LinuxPlatformLayer::Download(const char* workflowId, const char* updateType, const ADUC_DownloadInfo* info)
{
    Log_Debug("Downloading %d files to %s", info->FileCount, info->WorkFolder);

    ADUC_Result_t resultCode = ADUC_DownloadResult_Failure;
    ADUC_Result_t extendedResultCode = ADUC_ERC_NOTRECOVERABLE;

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const ADUC_FileEntity& entity = info->Files[0];

    if (entity.HashCount == 0)
    {
        Log_Error("File entity does not contain a file hash! Cannot validate cancelling download.");
        resultCode = ADUC_DownloadResult_Failure;
        extendedResultCode = ADUC_ERC_VALIDATION_FILE_HASH_IS_EMPTY;
        info->NotifyDownloadProgress(workflowId, entity.FileId, ADUC_DownloadProgressState_Error, 0, 0);
        return ADUC_Result{ resultCode, extendedResultCode };
    }

    if (_IsCancellationRequested)
    {
        Log_Info("Cancellation requested.");
        resultCode = ADUC_DownloadResult_Cancelled;
        info->NotifyDownloadProgress(workflowId, entity.FileId, ADUC_DownloadProgressState_Cancelled, 0, 0);
        return ADUC_Result{ resultCode, extendedResultCode };
    }

    std::stringstream fullFilePath;
    fullFilePath << info->WorkFolder << "/" << entity.TargetFilename;

    Log_Info(
        "Downloading File '%s' from '%s' to '%s'",
        entity.TargetFilename,
        entity.DownloadUri,
        fullFilePath.str().c_str());

    try
    {
        MSDO::download::download_url_to_path(
            entity.DownloadUri, fullFilePath.str(), std::ref(_IsCancellationRequested));

        resultCode = ADUC_DownloadResult_Success;
    }
    // Catch DO exception only to get extended result code. Other exceptions will be caught by CallResultMethodAndHandleExceptions
    catch (const MSDO::exception& e)
    {
        const int32_t doErrorCode = e.error_code();

        Log_Info("Caught DO exception, msg: %s, code: %d", e.what(), doErrorCode);

        if (doErrorCode == static_cast<int32_t>(std::errc::operation_canceled))
        {
            Log_Info("Download was cancelled");
            info->NotifyDownloadProgress(workflowId, entity.FileId, ADUC_DownloadProgressState_Cancelled, 0, 0);

            resultCode = ADUC_DownloadResult_Cancelled;
        }
        else
        {
            if (doErrorCode == static_cast<int32_t>(std::errc::timed_out))
            {
                Log_Error("Download failed due to DO timeout");
            }

            resultCode = ADUC_DownloadResult_Failure;
        }

        extendedResultCode = MAKE_ADUC_DELIVERY_OPTIMIZATION_EXTENDEDRESULTCODE(doErrorCode);
    }
    catch (const std::exception& e)
    {
        Log_Error("DO download failed with an unhandled std exception: %s", e.what());

        resultCode = ADUC_DownloadResult_Failure;
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

        resultCode = ADUC_DownloadResult_Failure;

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
    if (resultCode == ADUC_DownloadResult_Success)
    {
        // TODO(Nic): 28241106 - Add crypto agility for hash validation
        // Note: Currently we expect there to be only one hash, but
        // support for multiple hashes is already built in.
        Log_Info("Validating file hash");

        SHAversion algVersion;
        if (!ADUC_HashUtils_GetShaVersionForTypeString(
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                entity.Hash[0].type,
                &algVersion))
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            Log_Error(
                "FileEntity for %s has unsupported hash type %s", fullFilePath.str().c_str(), entity.Hash[0].type);
            resultCode = ADUC_DownloadResult_Failure;
            extendedResultCode = ADUC_ERC_VALIDATION_FILE_HASH_TYPE_NOT_SUPPORTED;

            info->NotifyDownloadProgress(
                workflowId, entity.FileId, ADUC_DownloadProgressState_Error, resultCode, extendedResultCode);
            return ADUC_Result{ resultCode, extendedResultCode };
        }

        const bool isValid = ADUC_HashUtils_IsValidFileHash(
            fullFilePath.str().c_str(),
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            entity.Hash[0].value,
            algVersion);
        if (!isValid)
        {
            Log_Error("Hash for %s is not valid", entity.TargetFilename);

            resultCode = ADUC_DownloadResult_Failure;
            extendedResultCode = ADUC_ERC_VALIDATION_FILE_HASH_INVALID_HASH;

            info->NotifyDownloadProgress(
                workflowId, entity.FileId, ADUC_DownloadProgressState_Error, resultCode, extendedResultCode);
            return ADUC_Result{ resultCode, extendedResultCode };
        }
    }

    // Call into content handler
    if (resultCode == ADUC_DownloadResult_Success)
    {
        // We create the content handler as part of the Download phase since this is the start of the rollout workflow
        // and we need to call into the content handler for any additional downloads it may need.
        _contentHandler = ContentHandlerFactory::Create(
            updateType, { info->WorkFolder, ADUC_LOG_FOLDER, entity.TargetFilename, entity.FileId });

        const ADUC_Result contentHandlerResult{ _contentHandler->Download() };
        resultCode = contentHandlerResult.ResultCode;
        extendedResultCode = contentHandlerResult.ExtendedResultCode;

        Log_Info("Content Handler Download resultCode: %d, extendedCode: %d", resultCode, extendedResultCode);
    }

    // Report progress.
    struct stat st
    {
    };
    const off_t fileSize{ (stat(fullFilePath.str().c_str(), &st) == 0) ? st.st_size : 0 };

    if (resultCode == ADUC_DownloadResult_Success)
    {
        info->NotifyDownloadProgress(
            workflowId, entity.FileId, ADUC_DownloadProgressState_Completed, fileSize, fileSize);
    }
    else
    {
        info->NotifyDownloadProgress(
            workflowId,
            entity.FileId,
            (resultCode == ADUC_DownloadResult_Cancelled) ? ADUC_DownloadProgressState_Cancelled
                                                          : ADUC_DownloadProgressState_Error,
            fileSize,
            fileSize);
    }

    Log_Info("Download resultCode: %d, extendedCode: %d", resultCode, extendedResultCode);
    return ADUC_Result{ resultCode, extendedResultCode };
}

/**
 * @brief Class implementation of Install method.
 * @return ADUC_Result
 */
ADUC_Result LinuxPlatformLayer::Install(const char* workflowId, const ADUC_InstallInfo* /*info*/)
{
    ADUC_Result result{ _contentHandler->Install() };
    if (_IsCancellationRequested)
    {
        Log_Info("Cancellation requested. Cancelling install. workflowId: %s", workflowId);
        result = _contentHandler->Cancel();
        if (IsAducResultCodeSuccess(result.ResultCode))
        {
            result = ADUC_Result{ ADUC_InstallResult_Cancelled };
        }
        else
        {
            result = ADUC_Result{ ADUC_InstallResult_Failure };
        }
    }

    return result;
}

/**
 * @brief Class implementation of Apply method.
 * @return ADUC_Result
 */
ADUC_Result LinuxPlatformLayer::Apply(const char* workflowId, const ADUC_ApplyInfo* /*info*/)
{
    ADUC_Result result{ _contentHandler->Apply() };
    if (_IsCancellationRequested)
    {
        Log_Info("Cancellation requested. Cancelling apply. workflowId: %s", workflowId);
        result = _contentHandler->Cancel();
        if (IsAducResultCodeSuccess(result.ResultCode))
        {
            result = ADUC_Result{ ADUC_ApplyResult_Cancelled };
        }
        else
        {
            result = ADUC_Result{ ADUC_ApplyResult_Failure };
        }
    }

    return result;
}

/**
 * @brief Class implementation of Cancel method.
 */
void LinuxPlatformLayer::Cancel(const char* workflowId)
{
    Log_Info("Cancelling. workflowId: %s", workflowId);
    _IsCancellationRequested = true;
}

/**
 * @brief Class implementation of the IsInstalled callback.
 * Calls into the content handler to determine if the given installed criteria is installed.
 *
 * @param workflowId The workflow ID.
 * @param installedCriteria The installed criteria for the content.
 * @return ADUC_Result The result of the IsInstalled call.
 */
ADUC_Result
LinuxPlatformLayer::IsInstalled(const char* workflowId, const char* updateType, const char* installedCriteria)
{
    Log_Info("IsInstalled called workflowId: %s, installed criteria: %s", workflowId, installedCriteria);

    // If we don't currently have a content handler, create one that will get replaced once
    // we are in a deployment.
    if (!_contentHandler)
    {
        try
        {
            _contentHandler = ContentHandlerFactory::Create(updateType, ContentHandlerCreateData{});
        }
        catch (const ADUC::Exception& e)
        {
            Log_Error(
                "Failed to create content handler, updateType:%s code: %d, message: %s",
                updateType,
                e.Code(),
                e.Message().c_str());
            return ADUC_Result{ ADUC_IsInstalledResult_Failure, ADUC_ERC_NOTRECOVERABLE };
        }
        catch (...)
        {
            Log_Error("Failed to create content handler, updateType:%s", updateType);
            return ADUC_Result{ ADUC_IsInstalledResult_Failure, ADUC_ERC_NOTRECOVERABLE };
        }
    }

    return _contentHandler->IsInstalled(installedCriteria);
}

ADUC_Result LinuxPlatformLayer::SandboxCreate(const char* workflowId, char** workFolder)
{
    *workFolder = nullptr;
    if (workflowId == nullptr || *workflowId == '\0')
    {
        Log_Error("Invalid workflowId passed to SandboxCreate! Uninitialized workflow?");
        return ADUC_Result{ ADUC_SandboxCreateResult_Failure, ADUC_ERC_NOTRECOVERABLE };
    }

    // Determine the work folder name, which will be used for download, install and apply.
    std::string tempPath{ ADUC_SystemUtils_GetTemporaryPathName() };

    std::stringstream folderName;
    folderName << tempPath << "/aduc-dl-" << workflowId;

    // Try to delete existing directory.
    int dir_result;
    struct stat sb;
    if (stat(folderName.str().c_str(), &sb) == 0 && S_ISDIR(sb.st_mode))
    {
        dir_result = ADUC_SystemUtils_RmDirRecursive(folderName.str().c_str());
        if (dir_result != 0)
        {
            // Not critical if failed.
            Log_Info("Unable to remove folder %s, error %d", folderName.str().c_str(), dir_result);
        }
    }

    // Create the sandbox folder with ownership as the same as this process.
    // Permissions are set to u=rwx,g=rwx. We grant read/write/execute to group owner so that partner
    // processes like the DO daemon can download files to our sandbox.
    dir_result = ADUC_SystemUtils_MkDirRecursive(
        folderName.str().c_str(), getuid(), getgid(), S_IRWXU | S_IRGRP | S_IWGRP | S_IXGRP);
    if (dir_result != 0)
    {
        Log_Error("Unable to create folder %s, error %d", folderName.str().c_str(), dir_result);
        return ADUC_Result{ ADUC_SandboxCreateResult_Failure, dir_result };
    }

    Log_Info("Setting sandbox %s", folderName.str().c_str());

    *workFolder = strdup(folderName.str().c_str());
    if (*workFolder == nullptr)
    {
        return ADUC_Result{ ADUC_SandboxCreateResult_Failure, ADUC_ERC_NOMEM };
    }

    return ADUC_Result{ ADUC_SandboxCreateResult_Success };
}

void LinuxPlatformLayer::SandboxDestroy(const char* workflowId, const char* workFolder)
{
    // If SandboxCreate failed or didn't specify a workfolder, we'll get nullptr here.
    if (workFolder == nullptr)
    {
        return;
    }

    Log_Info("Destroying sandbox %s. workflowId: %s", workFolder, workflowId);

    int ret = ADUC_SystemUtils_RmDirRecursive(workFolder);
    if (ret != 0)
    {
        // Not a fatal error.
        Log_Error("Unable to remove sandbox, error %d", ret);
    }
}
