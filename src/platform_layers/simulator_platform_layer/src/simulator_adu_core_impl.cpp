/**
 * @file simulator_adu_core_impl.cpp
 * @brief Implements an ADUC "simulator" mode.
 *
 * Define DISABLE_REAL_DOWNLOADING to disable support for real downloads, e.g. to ease porting.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#include "simulator_adu_core_impl.hpp"
#include "simulator_device_info.h"

#include <chrono>
#include <future> // this_thread

#include <cstring>
#include <vector>

#ifndef DISABLE_REAL_DOWNLOADING
#    include "uhttp_downloader.h"
#    include <sys/stat.h> // stat
#    include <unistd.h> // rmdir
#endif // DISABLE_REAL_DOWNLOADING

#include <aduc/content_handler_factory.hpp>
#include <aduc/logging.h>
#include <aduc/string_utils.hpp>

using ADUC::SimulatorPlatformLayer;

std::unique_ptr<SimulatorPlatformLayer> SimulatorPlatformLayer::Create(
    SimulationType type /*= SimulationType::AllSuccessful*/, bool performDownload /*= false*/)
{
    return std::unique_ptr<SimulatorPlatformLayer>{ new SimulatorPlatformLayer(type, performDownload) };
}

/**
 * @brief Construct a new Simulator Impl object
 *
 * @param type Simulation type to run.
 * @param performDownload Whether an actual download should occur.
 */
SimulatorPlatformLayer::SimulatorPlatformLayer(SimulationType type, bool performDownload) :
    _simulationType(type), _shouldPerformDownload(performDownload), _cancellationRequested(false)
{
#ifdef DISABLE_REAL_DOWNLOADING
    _shouldPerformDownload = false;
#endif
}

ADUC_Result SimulatorPlatformLayer::SetRegisterData(ADUC_RegisterData* data)
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

void SimulatorPlatformLayer::Idle(const char* workflowId)
{
    Log_Info("{%s} Now idle", workflowId);

    _cancellationRequested = false;
}

ADUC_Result SimulatorPlatformLayer::Prepare(const char* workflowId, const ADUC_PrepareInfo* prepareInfo)
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
        Log_Error("Failed to create content handler for simulator");
        return ADUC_Result{ ADUC_PrepareResult_Failure, ADUC_ERC_NOTRECOVERABLE };
        ;
    }

    result = _contentHandler->Prepare(prepareInfo);
    if (result.ResultCode == ADUC_PrepareResult_Failure)
    {
        if (result.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_VERSION
            || result.ExtendedResultCode == ADUC_ERC_APT_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_VERSION)
        {
            Log_Error(
                "Metadata validation failed due to wrong version, Update Type %s, Version %u",
                prepareInfo->updateType,
                prepareInfo->updateTypeVersion);
            return result;
        }

        if (result.ExtendedResultCode == ADUC_ERC_SWUPDATE_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_FILECOUNT
            || result.ExtendedResultCode == ADUC_ERC_APT_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_FILECOUNT)
        {
            Log_Error(
                "Metadata validation failed due to wrong file count, Update Type %s, File Count %u",
                prepareInfo->updateType,
                prepareInfo->fileCount);
            return result;
        }
    }

    return result;
}

ADUC_Result
SimulatorPlatformLayer::Download(const char* workflowId, const char* updateType, const ADUC_DownloadInfo* info)
{
    Log_Info(
        "{%s} (UpdateType: %s) Downloading %d files to %s", workflowId, updateType, info->FileCount, info->WorkFolder);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const ADUC_FileEntity& entity = info->Files[0];

    // We create the content handler as part of the Download phase since this is the start of the rollout workflow
    // and we need to call into the content handler for any additional downloads it may need.
    _contentHandler = ContentHandlerFactory::Create(
        updateType, { info->WorkFolder, ADUC_LOG_FOLDER, entity.TargetFilename, entity.FileId });

    if (CancellationRequested())
    {
        Log_Warn("Cancellation requested. Cancelling download");

        info->NotifyDownloadProgress(workflowId, entity.FileId, ADUC_DownloadProgressState_Cancelled, 0, 0);

        return ADUC_Result{ ADUC_DownloadResult_Cancelled };
    }

    Log_Info("File Info\n\tHash: %s\n\tUri: %s\n\tFile: %s", entity.FileId, entity.DownloadUri, entity.TargetFilename);

    if (GetSimulationType() == SimulationType::DownloadFailed)
    {
        Log_Warn("Simulating a download failure");

        info->NotifyDownloadProgress(workflowId, entity.FileId, ADUC_DownloadProgressState_Error, 0, 0);

        return ADUC_Result{ ADUC_DownloadResult_Failure, ADUC_ERC_NOTRECOVERABLE };
    }

#ifndef DISABLE_REAL_DOWNLOADING
    if (ShouldPerformDownload())
    {
        // Actual download.
        std::string outputFile{ info->WorkFolder };
        outputFile += '/';
        outputFile += entity.TargetFilename;

        const UHttpDownloaderResult downloadResult =
            DownloadFile(entity.DownloadUri, entity.FileId, outputFile.c_str());
        if (downloadResult != DR_OK)
        {
            Log_Error("Download failed, error %u", downloadResult);

            info->NotifyDownloadProgress(workflowId, entity.FileId, ADUC_DownloadProgressState_Error, 0, 0);

            return ADUC_Result{ ADUC_DownloadResult_Failure, downloadResult };
        }

        struct stat st = {};
        const off_t fileSize{ (stat(outputFile.c_str(), &st) == 0) ? st.st_size : 0 };

        info->NotifyDownloadProgress(
            workflowId, entity.FileId, ADUC_DownloadProgressState_Completed, fileSize, fileSize);
    }
    else
#endif
    {
        // Simulation mode.

        info->NotifyDownloadProgress(workflowId, entity.FileId, ADUC_DownloadProgressState_Completed, 424242, 424242);

        Log_Info("Simulator sleeping...");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    ADUC_Result result{ _contentHandler->Download() };
    Log_Info("Download resultCode: %d, extendedCode: %d", result.ResultCode, result.ExtendedResultCode);

    // Success!
    return result;
}

ADUC_Result SimulatorPlatformLayer::Install(const char* workflowId, const ADUC_InstallInfo* info)
{
    Log_Info("{%s} Installing from %s", workflowId, info->WorkFolder);
    ADUC_Result result{ _contentHandler->Install() };
    if (CancellationRequested())
    {
        Log_Warn("\tCancellation requested. Cancelling install");
        result = _contentHandler->Cancel();
        if (IsAducResultCodeSuccess(result.ResultCode))
        {
            return ADUC_Result{ ADUC_InstallResult_Cancelled };
        }

        return ADUC_Result{ ADUC_InstallResult_Failure };
    }

    Log_Info("Simulator sleeping...");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (GetSimulationType() == SimulationType::InstallationFailed)
    {
        Log_Warn("Simulating an install failure");
        return ADUC_Result{ ADUC_InstallResult_Failure, ADUC_ERC_NOTRECOVERABLE };
    }

    return ADUC_Result{ ADUC_InstallResult_Success };
}

ADUC_Result SimulatorPlatformLayer::Apply(const char* workflowId, const ADUC_ApplyInfo* info)
{
    Log_Info("{%s} Applying data from %s", workflowId, info->WorkFolder);
    ADUC_Result result{ _contentHandler->Apply() };
    if (CancellationRequested())
    {
        Log_Warn("\tCancellation requested. Cancelling apply");
        result = _contentHandler->Cancel();
        if (IsAducResultCodeSuccess(result.ResultCode))
        {
            return ADUC_Result{ ADUC_InstallResult_Cancelled };
        }

        return ADUC_Result{ ADUC_InstallResult_Failure };
    }

    Log_Info("Simulator sleeping...");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (GetSimulationType() == SimulationType::ApplyFailed)
    {
        Log_Warn("Simulating an apply failure");
        return ADUC_Result{ ADUC_InstallResult_Failure, ADUC_ERC_NOTRECOVERABLE };
    }

    Log_Info("Apply succeeded.");

    // Can alternately return ADUC_ApplyResult_SuccessRebootRequired to indicate reboot required.
    // Success is returned here to force a new swVersion to be sent back to the server.
    return ADUC_Result{ ADUC_ApplyResult_Success };
}

void SimulatorPlatformLayer::Cancel(const char* workflowId)
{
    Log_Info("{%s} Cancel requested", workflowId);
    _cancellationRequested = true;
}

ADUC_Result
SimulatorPlatformLayer::IsInstalled(const char* workflowId, const char* updateType, const char* installedCriteria)
{
    Log_Info(
        "IsInstalled called workflowId: %s, UpdateType: %s, installed criteria: %s",
        workflowId,
        updateType,
        installedCriteria);

    if (updateType == NULL)
    {
        return ADUC_Result{ ADUC_IsInstalledResult_Failure };
    }

    if (installedCriteria == NULL)
    {
        return ADUC_Result{ ADUC_IsInstalledResult_NotInstalled };
    }

    // If we don't currently have a content handler, create one that will get replaced once
    // we are in a deployment.
    if (!_contentHandler)
    {
        _contentHandler = ContentHandlerFactory::Create(updateType, ContentHandlerCreateData{});
    }

    ADUC_Result result{ _contentHandler->IsInstalled(installedCriteria) };
    if (GetSimulationType() == SimulationType::IsInstalledFailed)
    {
        Log_Warn("Simulating IsInstalled failure");
        return ADUC_Result{ ADUC_IsInstalledResult_Failure, 42 };
    }

    return result;
}

ADUC_Result SimulatorPlatformLayer::SandboxCreate(const char* workflowId, char** workFolder)
{
    const char* sandboxFolder = "/tmp/sandbox";

    Log_Info("{%s} Creating sandbox %s", workflowId, sandboxFolder);

    *workFolder = strdup(sandboxFolder);
    if (*workFolder == nullptr)
    {
        return ADUC_Result{ ADUC_SandboxCreateResult_Failure, ADUC_ERC_NOMEM };
    }

#ifndef DISABLE_REAL_DOWNLOADING
    if (ShouldPerformDownload())
    {
        // Real download, create sandbox.
        errno = 0;

        if (mkdir(*workFolder, S_IRWXU) != 0 && errno != EEXIST)
        {
            Log_Warn("Unable to create sandbox, error %u", errno);
            return ADUC_Result{ ADUC_SandboxCreateResult_Failure, MAKE_ADUC_ERRNO_EXTENDEDRESULTCODE(errno) };
        }
    }
    else
#endif
    {
        // Simulation.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return ADUC_Result{ ADUC_SandboxCreateResult_Success };
}

void SimulatorPlatformLayer::SandboxDestroy(const char* workflowId, const char* workFolder)
{
    // If SandboxCreate failed or didn't specify a workfolder, we'll get nullptr here.
    if (workFolder == nullptr)
    {
        return;
    }

    Log_Info("{%s} Deleting sandbox: %s", workflowId, workFolder);

#ifndef DISABLE_REAL_DOWNLOADING
    if (ShouldPerformDownload())
    {
        // Real download, remove sandbox.
        (void)rmdir(workFolder);
    }
    else
#endif
    {
        // Simulation.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}