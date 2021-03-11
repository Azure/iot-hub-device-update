/**
 * @file simulator_adu_core_impl.cpp
 * @brief Implements an ADUC "simulator" mode.
 *
 * Define DISABLE_REAL_DOWNLOADING to disable support for real downloads, e.g. to ease porting.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "simulator_adu_core_impl.hpp"
#include "simulator_device_info.h"

#include <chrono>
#include <future> // this_thread

#include <cstring>
#include <vector>

#include "aduc/logging.h"
#include "aduc/string_utils.hpp"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_utils.h"
#include "uhttp_downloader.h"

using ADUC::SimulatorPlatformLayer;

std::unique_ptr<SimulatorPlatformLayer> SimulatorPlatformLayer::Create(
    SimulationType type /*= SimulationType::AllSuccessful*/)
{
    return std::unique_ptr<SimulatorPlatformLayer>{ new SimulatorPlatformLayer(type) };
}

/**
 * @brief Construct a new Simulator Impl object
 *
 * @param type Simulation type to run.
 */
SimulatorPlatformLayer::SimulatorPlatformLayer(SimulationType type) :
    _simulationType(type), _cancellationRequested(false)
{
}

ADUC_Result SimulatorPlatformLayer::SetUpdateActionCallbacks(ADUC_UpdateActionCallbacks* data)
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

    data->DoWorkCallback = DoWorkCallback;

    // Opaque token, passed to callbacks.

    data->PlatformLayerHandle = this;

    return ADUC_Result{ ADUC_Result_Register_Success };
}

void SimulatorPlatformLayer::Idle(const char* workflowId)
{
    Log_Info("{%s} Now idle", workflowId);

    _cancellationRequested = false;
}

ADUC_Result SimulatorPlatformLayer::Download(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result = { ADUC_Result_Failure };
    ADUC_FileEntity* entity = nullptr;
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    char* workflowId = workflow_get_id(handle);
    char* updateType = workflow_get_update_type(handle);
    char* workFolder = workflow_get_workfolder(handle);

    Log_Info(
        "{%s} (UpdateType: %s) Downloading %d files to %s",
        workflowId,
        updateType,
        workflow_get_update_files_count(handle),
        workFolder);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (!workflow_get_update_file(handle, 0, &entity))
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_COMPONENTS_HANDLER_GET_FILE_ENTITY_FAILURE };

        goto done;
    }

    // Note: for simulator, we don't load Update Content Handler.

    if (CancellationRequested())
    {
        Log_Warn("Cancellation requested. Cancelling download");

        workflowData->DownloadProgressCallback(workflowId, entity->FileId, ADUC_DownloadProgressState_Cancelled, 0, 0);

        result = { ADUC_Result_Failure_Cancelled };
        goto done;
    }

    Log_Info(
        "File Info\n\tHash: %s\n\tUri: %s\n\tFile: %s", entity->FileId, entity->DownloadUri, entity->TargetFilename);

    if (GetSimulationType() == SimulationType::DownloadFailed)
    {
        Log_Warn("Simulating a download failure");

        workflowData->DownloadProgressCallback(workflowId, entity->FileId, ADUC_DownloadProgressState_Error, 0, 0);

        result = { ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
        goto done;
    }

    // Simulation mode.

    workflowData->DownloadProgressCallback(
        workflowId, entity->FileId, ADUC_DownloadProgressState_Completed, 424242, 424242);

    Log_Info("Simulator sleeping...");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Download in progress.
    result = { ADUC_Result_Download_InProgress };
    Log_Info("Download resultCode: %d, extendedCode: %d", result.ResultCode, result.ExtendedResultCode);

done:
    workflow_free_string(workflowId);
    workflow_free_string(updateType);
    workflow_free_string(workFolder);
    workflow_free_file_entity(entity);

    // Success!
    return result;
}

ADUC_Result SimulatorPlatformLayer::Install(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result;
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    char* workflowId = workflow_get_id(handle);
    char* updateType = workflow_get_update_type(handle);
    char* workFolder = workflow_get_workfolder(handle);

    Log_Info("{%s} Installing from %s", workflowId, workFolder);

    if (CancellationRequested())
    {
        Log_Warn("\tCancellation requested. Cancelling install");
        result = { ADUC_Result_Failure_Cancelled };
        goto done;
    }

    Log_Info("Simulator sleeping...");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (GetSimulationType() == SimulationType::InstallationFailed)
    {
        Log_Warn("Simulating an install failure");
        result = { ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
        goto done;
    }

    result = { ADUC_Result_Install_Success };

done:
    workflow_free_string(workflowId);
    workflow_free_string(updateType);
    workflow_free_string(workFolder);

    return result;
}

ADUC_Result SimulatorPlatformLayer::Apply(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result;
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    char* updateType = workflow_get_update_type(handle);
    char* workFolder = workflow_get_workfolder(handle);
    char* workflowId = workflow_get_id(handle);

    Log_Info("{%s} Applying data from %s", workflowId, workFolder);

    if (CancellationRequested())
    {
        Log_Warn("\tCancellation requested. Cancelling apply");
        result = { ADUC_Result_Failure_Cancelled };
        goto done;
    }

    Log_Info("Simulator sleeping...");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (GetSimulationType() == SimulationType::ApplyFailed)
    {
        Log_Warn("Simulating an apply failure");
        result = { ADUC_Result_Failure, ADUC_ERC_NOTRECOVERABLE };
        goto done;
    }

    Log_Info("Apply succeeded.");
    result = { ADUC_Result_Apply_Success };

done:
    workflow_free_string(updateType);
    workflow_free_string(workFolder);
    workflow_free_string(workflowId);

    // Can alternately return ADUC_Result_Apply_RequiredReboot to indicate reboot required.
    // Success is returned here to force a new swVersion to be sent back to the server.
    return result;
}

void SimulatorPlatformLayer::Cancel(const ADUC_WorkflowData* workflowData)
{
    char* workflowId = workflow_get_id(workflowData->WorkflowHandle);
    Log_Info("{%s} Cancel requested", workflowId);
    workflow_free_string(workflowId);
    _cancellationRequested = true;
}

ADUC_Result SimulatorPlatformLayer::IsInstalled(const ADUC_WorkflowData* workflowData)
{
    Log_Info("IsInstalled called");

    ContentHandler* contentHandler = nullptr;
    char* installedCriteria = ADUC_WorkflowData_GetInstalledCriteria(workflowData);
    char* updateType = ADUC_WorkflowData_GetUpdateType(workflowData);
    ADUC_Result result = { ADUC_Result_Failure };

    if (updateType == nullptr)
    {
        result = { ADUC_Result_Failure };
        goto done;
    }

    if (installedCriteria == nullptr)
    {
        result = { ADUC_Result_IsInstalled_NotInstalled };
        goto done;
    }

    if (GetSimulationType() == SimulationType::IsInstalledFailed)
    {
        Log_Warn("Simulating IsInstalled failure");
        result = { ADUC_Result_Failure, 42 };
        goto done;
    }

    result = ExtensionManager::LoadUpdateContentHandlerExtension(updateType, &contentHandler);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = { contentHandler->IsInstalled(workflowData) };

done:
    workflow_free_string(installedCriteria);
    workflow_free_string(updateType);
    return result;
}

ADUC_Result SimulatorPlatformLayer::SandboxCreate(const char* workflowId, char* workFolder)
{
    Log_Info("{%s} Creating sandbox %s", workflowId, workFolder);

    // Simulation.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    return ADUC_Result{ ADUC_Result_SandboxCreate_Success };
}

void SimulatorPlatformLayer::SandboxDestroy(const char* workflowId, const char* workFolder)
{
    // If SandboxCreate failed or didn't specify a workfolder, we'll get nullptr here.
    if (workFolder == nullptr)
    {
        return;
    }

    Log_Info("{%s} Deleting sandbox: %s", workflowId, workFolder);

    // Simulation.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}
