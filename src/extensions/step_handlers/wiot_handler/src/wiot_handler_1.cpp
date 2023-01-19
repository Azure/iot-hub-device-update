
/**
 * @file wiot_handler_1.cpp
 * @brief Implements WiotHandler1
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "wiot_handler_1.hpp"

#include "aducresult.hpp"
#include "step_handler.hpp"
#include "workflow_ptr.hpp"

#include <functional> // std::function
#include <string>
#include <vector>

#include "aduc/extension_manager.hpp" // ExtensionManager_Download_Options
#include "aduc/string_c_utils.h" // ADUC_ParseUpdateType
#include "aduc/workflow_data_utils.h" // ADUC_WorkflowData
#include "aduc/workflow_utils.h" // workflow_get_*

// Prefix sent to logging for identification
#define HANDLER_LOG_ID "[microsoft/wiot:1] "

// TODO(JeffMill): Currently using SWUPDATE error codes.
// Bug 43012743: Error codes are too content handler specific.

EXTERN_C_BEGIN
extern ExtensionManager_Download_Options Default_ExtensionManager_Download_Options;
EXTERN_C_END

static std::vector<std::string> workflow_get_update_file_list(const ADUC_WorkflowHandle& workflowHandle)
{
    std::vector<std::string> fileList;
    const size_t fileCount = workflow_get_update_files_count(workflowHandle);

    for (size_t fileIndex = 0; fileIndex < fileCount; fileIndex++)
    {
        ADUC_FileEntity* entity_temp;
        if (!workflow_get_update_file(workflowHandle, fileIndex, &entity_temp))
        {
            // Return empty file list on error.
            fileList.clear();
            break;
        }

        fileList.push_back(entity_temp->TargetFilename);
        workflow_free_file_entity(entity_temp);
    }

    return fileList;
}

/**
 * @brief Destructor
 */
WiotHandler1::~WiotHandler1()
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
ADUC_Result WiotHandler1::IsInstalled(const ADUC_WorkflowData* workflowData)
{
    Log_Info(HANDLER_LOG_ID "IsInstalled");

    workflow_string_ptr installedCriteria(ADUC_WorkflowData_GetInstalledCriteria(workflowData));
    if (installedCriteria == nullptr)
    {
        Log_Error("Unable to get installed criteria.");
        return ADUCResult(ADUC_Result_Failure, ADUC_ERC_SWUPDATE_HANDLER_MISSING_INSTALLED_CRITERIA);
    }

    if (!StepHandler::IsInstalled(installedCriteria.get()))
    {
        Log_Info("Not installed");
        return ADUCResult(ADUC_Result_IsInstalled_NotInstalled);
    }

    Log_Info("Installed");
    return ADUCResult(ADUC_Result_IsInstalled_Installed);
}

/**
 * @brief Download implementation
 *
 * @return ADUC_Result The result of the download
 * ADUC_Result_Download_Success - success
*/
ADUC_Result WiotHandler1::Download(const ADUC_WorkflowData* workflowData)
{
    Log_Info(HANDLER_LOG_ID "Download");

    const ADUC_WorkflowHandle workflowHandle{ workflowData->WorkflowHandle };

    if (workflow_is_cancel_requested(workflowHandle))
    {
        return Cancel(workflowData);
    }

    if (!IsValidUpdateTypeInfo(workflowHandle))
    {
        return ADUCResult(ADUC_Result_Failure, ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_WRONG_UPDATE_VERSION);
    }

    const size_t fileCount = workflow_get_update_files_count(workflowHandle);
    if (fileCount != 1)
    {
        // For v1, only 1 file is expected.
        Log_Error("Incorrect file count: %u", fileCount);
        return ADUCResult(ADUC_Result_Failure, ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_WRONG_FILECOUNT);
    }

    //
    // Check to see if the content is already installed.
    //

    if (IsInstalled(workflowData).ResultCode == ADUC_Result_IsInstalled_Installed)
    {
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

        Log_Info("Downloading file #%u", fileIndex);

        ADUC_FileEntity* entity_temp;
        if (!workflow_get_update_file(workflowHandle, fileIndex, &entity_temp))
        {
            return ADUCResult(ADUC_Result_Failure, ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_GET_PAYLOAD_FILE_ENTITY);
        }
        workflow_file_entity_ptr entity(entity_temp);

        try
        {
            ADUCResult result = ExtensionManager::Download(
                entity.get(),
                workflowHandle,
                &Default_ExtensionManager_Download_Options,
                nullptr /*downloadProgressCallback*/);
            if (result.IsResultCodeFailure())
            {
                Log_Error("Cannot download payload file#%u, error 0x%X", fileIndex, result.ExtendedResultCode());
                return result;
            }
        }
        catch (...)
        {
            return ADUCResult(
                ADUC_Result_Failure, ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_PAYLOAD_FILE_FAILURE_UNKNOWNEXCEPTION);
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
ADUC_Result WiotHandler1::Install(const ADUC_WorkflowData* workflowData)
{
    Log_Info(HANDLER_LOG_ID "Install");

    const ADUC_WorkflowHandle workflowHandle{ workflowData->WorkflowHandle };

    if (workflow_is_cancel_requested(workflowHandle))
    {
        return Cancel(workflowData);
    }

    if (!IsValidUpdateTypeInfo(workflowHandle))
    {
        return ADUCResult(ADUC_Result_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_UNKNOWNEXCEPTION);
    }

    //
    // Check to see if the content is already installed.
    //

    if (IsInstalled(workflowData).ResultCode == ADUC_Result_IsInstalled_Installed)
    {
        return ADUCResult(ADUC_Result_Install_Success);
    }

    workflow_string_ptr workFolder(workflow_get_workfolder(workflowHandle));
    if (workFolder.get() == nullptr)
    {
        Log_Error("Unable to get work folder");

        return ADUCResult(ADUC_Result_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_UNKNOWNEXCEPTION);
    }

    Log_Info("Installing from %s", workFolder.get());

    std::vector<std::string> fileList(workflow_get_update_file_list(workflowHandle));
    if (fileList.size() != 1)
    {
        // For v1, only 1 file is expected.
        Log_Error("Incorrect file count: %u", fileList.size());
        return ADUCResult(ADUC_Result_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_UNKNOWNEXCEPTION);
    }

    if (!StepHandler::Install(workFolder.get(), fileList))
    {
        Log_Error("Install failed");
        return ADUCResult(ADUC_Result_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_UNKNOWNEXCEPTION);
    }

    return ADUCResult(ADUC_Result_Install_Success);
}

/**
 * @brief Apply implementation
 *
 * @return ADUC_Result The result of the apply.
 * ADUC_Result_Success - success
 * ADUC_Result_Apply_RequiredImmediateReboot - reboot required
 */
ADUC_Result WiotHandler1::Apply(const ADUC_WorkflowData* workflowData)
{
    Log_Info(HANDLER_LOG_ID "Apply");

    const ADUC_WorkflowHandle workflowHandle{ workflowData->WorkflowHandle };

    if (workflow_get_operation_cancel_requested(workflowHandle))
    {
        return Cancel(workflowData);
    }

    workflow_string_ptr workFolder(workflow_get_workfolder(workflowHandle));
    if (workFolder.get() == nullptr)
    {
        Log_Error("Unable to get work folder");

        return ADUCResult(ADUC_Result_Failure, ADUC_ERC_SCRIPT_HANDLER_APPLY_FAILURE_UNKNOWNEXCEPTION);
    }

    Log_Info("Applying from %s", workFolder.get());

    std::vector<std::string> fileList(workflow_get_update_file_list(workflowHandle));
    if (fileList.size() != 1)
    {
        // For v1, only 1 file is expected.
        Log_Error("Incorrect file count: %u", fileList.size());
        return ADUCResult(ADUC_Result_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_UNKNOWNEXCEPTION);
    }

    if (!StepHandler::Apply(workFolder.get(), fileList))
    {
        Log_Error("Apply failed");
        return ADUCResult(ADUC_Result_Failure, ADUC_ERC_SCRIPT_HANDLER_APPLY_FAILURE_UNKNOWNEXCEPTION);
    }

    return ADUCResult(ADUC_Result_Success);
}

/**
 * @brief Cancel implementation
 *
 * @return ADUC_Result The result of the cancel.
 * ADUC_Result_Cancel_Success - success
 * ADUC_Result_Cancel_UnableToCancel - unable to cancel
 */
ADUC_Result WiotHandler1::Cancel(const ADUC_WorkflowData* workflowData)
{
    Log_Info(HANDLER_LOG_ID "Cancel");

    const ADUC_WorkflowHandle workflowHandle{ workflowData->WorkflowHandle };
    const char* workflowId = workflow_peek_id(workflowHandle);
    const int workflowLevel = workflow_get_level(workflowHandle);
    const int workflowStep = workflow_get_step_index(workflowHandle);

    Log_Info(
        "Requesting cancel operation (workflow id '%s', level %d, step %d).", workflowId, workflowLevel, workflowStep);
    if (!workflow_request_cancel(workflowHandle))
    {
        Log_Error(
            "Cancellation request failed. (workflow id '%s', level %d, step %d)",
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
 * ADUC_Result_Backup_Success_Unsupported - np-op
 */
ADUC_Result WiotHandler1::Backup(const ADUC_WorkflowData* workflowData)
{
    Log_Info(HANDLER_LOG_ID "Backup");

    return ADUCResult(ADUC_Result_Backup_Success_Unsupported);
}

/**
 * @brief Restore implementation.
 *
 * @return ADUC_Result The result of the restore.
 * ADUC_Result_Restore_Success - success
 * ADUC_Result_Restore_Success_Unsupported - no-op
 */
ADUC_Result WiotHandler1::Restore(const ADUC_WorkflowData* workflowData)
{
    Log_Info(HANDLER_LOG_ID "Restore");

    return ADUCResult(ADUC_Result_Restore_Success_Unsupported);
}

/*static*/
bool WiotHandler1::IsValidUpdateTypeInfo(const ADUC_WorkflowHandle& workflowHandle)
{
    //
    // Verify update type info
    //

    workflow_string_ptr updateType(workflow_get_update_type(workflowHandle));
    if (updateType.get() == nullptr)
    {
        Log_Error("Unable to get update type");
        return false;
    }

    unsigned int updateTypeVersion;
    if (!ADUC_ParseUpdateType(updateType.get(), NULL /*updateName*/, &updateTypeVersion))
    {
        Log_Error("Unable to parse update type");
        return false;
    }

    if (updateTypeVersion != 1)
    {
        Log_Error("Wrong Handler Version %d", updateTypeVersion);
        return false;
    }

    return true;
}
