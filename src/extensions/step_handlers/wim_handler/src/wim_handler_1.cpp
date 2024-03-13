
/**
 * @file wim_handler_1.cpp
 * @brief Implements WimHandler1
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "wim_handler_1.hpp"

#include "aducresult.hpp"
#include "wim_step_handler.hpp"
#include "workflow_ptr.hpp"

#include <inttypes.h> // PRIu64
#include <vector>

#include "aduc/extension_manager.hpp" // ExtensionManager_Download_Options
#include "aduc/parser_utils.h" // ADUC_FileEntity_Uninit
#include "aduc/string_c_utils.h" // ADUC_ParseUpdateType
#include "aduc/types/download.h" // ADUCDownloadProgressState
#include "aduc/workflow_data_utils.h" // ADUC_WorkflowData
#include "aduc/workflow_utils.h" // workflow_get_*


// keep this last to avoid interfering with system headers
#include "aduc/aduc_banned.h"

EXTERN_C_BEGIN

extern ExtensionManager_Download_Options Default_ExtensionManager_Download_Options;

EXTERN_C_END

class ADUCFileEntity : public ADUC_FileEntity
{
public:
    ADUCFileEntity()
    {
        memset(this, 0, sizeof(*this));
    }
    ~ADUCFileEntity()
    {
        ADUC_FileEntity_Uninit(this);
    }
};

static void DownloadProgressCallback(
    const char* workflowId,
    const char* fileId,
    ADUC_DownloadProgressState state,
    uint64_t bytesTransferred,
    uint64_t bytesTotal)
{
    Log_Info(
        "[%s] DownloadProgress: %s; %s; %u (%" PRIu64 "/%" PRIu64 ")",
        WimHandler1::ID(),
        workflowId,
        fileId,
        state,
        bytesTransferred,
        bytesTotal);
}

/**
 * @brief Destructor
 */
WimHandler1::~WimHandler1()
{
    ADUC_Logging_Uninit();
}

/**
 * @brief IsInstalled implementation
 *
 * @return ADUC_Result The result based on evaluating the workflow data.
 * ADUC_Result_IsInstalled_Installed - already installed.
 * ADUC_Result_IsInstalled_NotInstalled - not installed.
 */
ADUC_Result WimHandler1::IsInstalled(const ADUC_WorkflowData* workflowData)
{
    Log_Info("[%s] IsInstalled", ID());

    workflow_string_ptr installedCriteria(ADUC_WorkflowData_GetInstalledCriteria(workflowData));
    if (installedCriteria == nullptr)
    {
        Log_Error("[%s] Unable to get installed criteria.", ID());
        return ADUCResult(ADUC_Result_IsInstalled_NotInstalled);
    }

    const bool result = WimStepHandler::IsInstalled(installedCriteria.get());

    Log_Info("[%s] IsInstalled '%s'? %s", ID(), installedCriteria.get(), result ? "Yes" : "No");

    return result ? ADUCResult(ADUC_Result_IsInstalled_Installed) : ADUCResult(ADUC_Result_IsInstalled_NotInstalled);
}

/**
 * @brief Download implementation
 *
 * @return ADUC_Result The result of the download
 * ADUC_Result_Download_Success - success
*/
ADUC_Result WimHandler1::Download(const ADUC_WorkflowData* workflowData)
{
    Log_Info("[%s] Download", ID());

    const ADUC_WorkflowHandle workflowHandle{ workflowData->WorkflowHandle };

    if (workflow_is_cancel_requested(workflowHandle))
    {
        return Cancel(workflowData);
    }

    if (!IsValidUpdateTypeInfo(workflowHandle))
    {
        return ADUCResult(ADUC_Result_Failure, WimStepHandler::RC::Manifest_UnsupportedUpdateVersion);
    }

    const size_t fileCount = workflow_get_update_files_count(workflowHandle);
    if (fileCount != 1)
    {
        // Only supporting a single .WIM file in payload.
        Log_Error("[%s] Incorrect file count: %u", ID(), fileCount);
        return ADUCResult(ADUC_Result_Failure, WimStepHandler::RC::Manifest_WrongFileCount);
    }

    //
    // Check to see if the content is already installed.
    //

    if (IsInstalled(workflowData).ResultCode == ADUC_Result_IsInstalled_Installed)
    {
        Log_Info("[%s] Download: Already installed. no-op", ID());
        return ADUCResult(ADUC_Result_Download_Skipped_UpdateAlreadyInstalled);
    }

    //
    // Download each of the files in the manifest
    //

    for (size_t fileIndex = 0; fileIndex < fileCount; fileIndex++)
    {
        if (workflow_is_cancel_requested(workflowHandle))
        {
            return Cancel(workflowData);
        }

        Log_Info("[%s] Downloading file #%u", ID(), fileIndex);

        ADUCFileEntity entity;
        if (!workflow_get_update_file(workflowHandle, fileIndex, &entity))
        {
            return ADUCResult(ADUC_Result_Failure, WimStepHandler::RC::Manifest_CantGetFileEntity);
        }

        try
        {
            ADUCResult result = ExtensionManager::Download(
                &entity, workflowHandle, &Default_ExtensionManager_Download_Options, DownloadProgressCallback);
            Log_Info("[%s] Download result: %u,%u", ID(), result.ResultCode(), result.ExtendedResultCode());
            if (result.IsResultCodeFailure())
            {
                return result;
            }
        }
        catch (...)
        {
            return ADUCResult(ADUC_Result_Failure, WimStepHandler::RC::Download_UnknownException);
        }
    }

    return ADUCResult(ADUC_Result_Download_Success);
}

/**
 * @brief Install implementation
 *
 * @return ADUC_Result The result of the install.
 * ADUC_Result_Install_Success - success
 */
ADUC_Result WimHandler1::Install(const ADUC_WorkflowData* workflowData)
{
    Log_Info("[%s] Install", ID());

    const ADUC_WorkflowHandle workflowHandle{ workflowData->WorkflowHandle };

    if (workflow_is_cancel_requested(workflowHandle))
    {
        return Cancel(workflowData);
    }

    if (!IsValidUpdateTypeInfo(workflowHandle))
    {
        return ADUCResult(ADUC_Result_Failure, WimStepHandler::RC::Manifest_UnsupportedUpdateVersion);
    }

    //
    // Check to see if the content is already installed.
    //

    if (IsInstalled(workflowData).ResultCode == ADUC_Result_IsInstalled_Installed)
    {
        Log_Info("[%s] Install: Already installed. no-op", ID());
        return ADUCResult(ADUC_Result_Install_Success);
    }

    workflow_string_ptr workFolder(workflow_get_workfolder(workflowHandle));
    if (workFolder.get() == nullptr)
    {
        Log_Error("[%s] Unable to get work folder", ID());
        return ADUCResult(ADUC_Result_Failure, WimStepHandler::RC::General_CantGetWorkFolder);
    }

    const size_t fileCount = workflow_get_update_files_count(workflowHandle);
    if (fileCount != 1)
    {
        // Only supporting a single .WIM file in payload.
        Log_Error("[%s] Invalid file count: %u", ID(), fileCount);
        return ADUCResult(ADUC_Result_Failure, WimStepHandler::RC::Manifest_WrongFileCount);
    }

    ADUCFileEntity entity;
    if (!workflow_get_update_file(workflowHandle, 0, &entity))
    {
        Log_Error("[%s] Unable to get filename", ID());
        return ADUCResult(ADUC_Result_Failure, WimStepHandler::RC::Manifest_CantGetFileEntity);
    }

    Log_Info("Installing %s from %s", entity.TargetFilename, workFolder.get());

    ADUC_Result_t erc = WimStepHandler::Install(workFolder.get(), entity.TargetFilename);
    ADUCResult result(ADUC_Result_Failure, erc);
    Log_Info("[%s] Install result: %u,%u", ID(), result.ResultCode(), result.ExtendedResultCode());
    if (erc == WimStepHandler::RC::Success)
    {
        result.Set(ADUC_Result_Install_Success, 0);
    }

    return result;
}

/**
 * @brief Apply implementation
 *
 * @return ADUC_Result The result of the apply.
 * ADUC_Result_Success - success
 * ADUC_Result_Apply_RequiredImmediateReboot - reboot required
 */
ADUC_Result WimHandler1::Apply(const ADUC_WorkflowData* workflowData)
{
    Log_Info("[%s] Apply", ID());

    const ADUC_WorkflowHandle workflowHandle{ workflowData->WorkflowHandle };

    if (workflow_get_operation_cancel_requested(workflowHandle))
    {
        return Cancel(workflowData);
    }

    workflow_string_ptr workFolder(workflow_get_workfolder(workflowHandle));
    if (workFolder.get() == nullptr)
    {
        Log_Error("[%s] Unable to get work folder", ID());

        return ADUCResult(ADUC_Result_Failure, WimStepHandler::RC::General_CantGetWorkFolder);
    }

    const size_t fileCount = workflow_get_update_files_count(workflowHandle);
    if (fileCount != 1)
    {
        // Only supporting a single .WIM file in payload.
        Log_Error("[%s] Invalid file count: %u", ID(), fileCount);
        return ADUCResult(ADUC_Result_Failure, WimStepHandler::RC::Manifest_WrongFileCount);
    }

    ADUCFileEntity entity;
    if (!workflow_get_update_file(workflowHandle, 0, &entity))
    {
        Log_Error("[%s] Unable to get filename", ID());
        return ADUCResult(ADUC_Result_Failure, WimStepHandler::RC::Manifest_CantGetFileEntity);
    }

    Log_Info("[%s] Applying %s from %s", ID(), entity.TargetFilename, workFolder.get());

    ADUC_Result_t erc = WimStepHandler::Apply(workFolder.get(), entity.TargetFilename);
    ADUCResult result(ADUC_Result_Failure, erc);
    Log_Info("[%s] Apply result: %u,%u", ID(), result.ResultCode(), result.ExtendedResultCode());
    if (erc == WimStepHandler::RC::Success)
    {
        result.Set(ADUC_Result_Success, 0);
    }
    else if (erc == WimStepHandler::RC::Success_Reboot_Required)
    {
        result.Set(ADUC_Result_Apply_RequiredReboot, 0);
        workflow_request_reboot(workflowData->WorkflowHandle);
    }

    return result;
}

/**
 * @brief Cancel implementation
 *
 * @return ADUC_Result The result of the cancel.
 * ADUC_Result_Cancel_Success - success
 * ADUC_Result_Cancel_UnableToCancel - unable to cancel
 */
ADUC_Result WimHandler1::Cancel(const ADUC_WorkflowData* workflowData)
{
    Log_Info("[%s] Cancel", ID());

    const ADUC_WorkflowHandle workflowHandle{ workflowData->WorkflowHandle };
    const char* workflowId = workflow_peek_id(workflowHandle);
    const int workflowLevel = workflow_get_level(workflowHandle);
    const int workflowStep = workflow_get_step_index(workflowHandle);

    Log_Info(
        "[%s] Requesting cancel operation (workflow id '%s', level %d, step %d).",
        ID(),
        workflowId,
        workflowLevel,
        workflowStep);
    if (!workflow_request_cancel(workflowHandle))
    {
        Log_Error(
            "[%s] Cancellation request failed. (workflow id '%s', level %d, step %d)",
            ID(),
            workflowId,
            workflowLevel,
            workflowStep);
        return ADUCResult(ADUC_Result_Cancel_UnableToCancel);
    }

    return ADUCResult(ADUC_Result_Cancel_Success);
}

/**
 * @brief Backup implementation
 *
 * @return ADUC_Result The result of the backup.
 * ADUC_Result_Backup_Success - success
 * ADUC_Result_Backup_Success_Unsupported - no-op
 */
ADUC_Result WimHandler1::Backup(const ADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);

    Log_Info("[%s] Backup", ID());

    return ADUCResult(ADUC_Result_Backup_Success_Unsupported);
}

/**
 * @brief Restore implementation.
 *
 * @return ADUC_Result The result of the restore.
 * ADUC_Result_Restore_Success - success
 * ADUC_Result_Restore_Success_Unsupported - no-op
 */
ADUC_Result WimHandler1::Restore(const ADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);

    Log_Info("[%s] Restore", ID());

    return ADUCResult(ADUC_Result_Restore_Success_Unsupported);
}

/*static*/
bool WimHandler1::IsValidUpdateTypeInfo(const ADUC_WorkflowHandle& workflowHandle)
{
    //
    // Verify update type info
    //

    workflow_string_ptr updateType(workflow_get_update_type(workflowHandle));
    if (updateType.get() == nullptr)
    {
        Log_Error("[%s] Unable to get update type", ID());
        return false;
    }

    unsigned int updateTypeVersion;
    if (!ADUC_ParseUpdateType(updateType.get(), NULL /*updateName*/, &updateTypeVersion))
    {
        Log_Error("[%s] Unable to parse update type", ID());
        return false;
    }

    if (updateTypeVersion != 1)
    {
        Log_Error("[%s] Wrong Handler Version %d", ID(), updateTypeVersion);
        return false;
    }

    return true;
}
