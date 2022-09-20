/**
 * @file swupdate_handler.cpp
 * @brief Implementation of ContentHandler API for swupdate.
 *
 * Will call into wrapper script for swupdate to install image files.
 *
 * microsoft/swupdate
 * v1:
 *   Description:
 *   Initial revision.
 *
 *   Expected files:
 *   .swu - contains swupdate image.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/swupdate_handler.hpp"

#include "aduc/adu_core_exports.h"
#include "aduc/extension_manager.hpp"
#include "aduc/logging.h"
#include "aduc/process_utils.hpp"
#include "aduc/string_c_utils.h"
#include "aduc/string_utils.hpp"
#include "aduc/system_utils.h"
#include "aduc/types/update_content.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_utils.h"
#include "adushell_const.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string>

#include <dirent.h>

namespace adushconst = Adu::Shell::Const;

EXTERN_C_BEGIN

/////////////////////////////////////////////////////////////////////////////
// BEGIN Shared Library Export Functions
//
// These are the function symbols that the device update agent will
// lookup and call.
//

/**
 * @brief Instantiates an Update Content Handler for 'microsoft/swupdate:1' update type.
 * @return ContentHandler* The created instance.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel)
{
    ADUC_Logging_Init(logLevel, "swupdate-handler");
    Log_Info("Instantiating an Update Content Handler for 'microsoft/swupdate:1'");
    try
    {
        return SWUpdateHandlerImpl::CreateContentHandler();
    }
    catch (const std::exception& e)
    {
        const char* what = e.what();
        Log_Error("Unhandled std exception: %s", what);
    }
    catch (...)
    {
        Log_Error("Unhandled exception");
    }

    return nullptr;
}

/**
 * @brief Gets the extension contract info.
 *
 * @param[out] contractInfo The extension contract info.
 * @return ADUC_Result The result.
 */
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo)
{
    contractInfo->majorVer = ADUC_V1_CONTRACT_MAJOR_VER;
    contractInfo->minorVer = ADUC_V1_CONTRACT_MINOR_VER;
    return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
}

//
// END Shared Library Export Functions
/////////////////////////////////////////////////////////////////////////////

EXTERN_C_END

/**
 * @brief Destructor for the SWUpdate Handler Impl class.
 */
SWUpdateHandlerImpl::~SWUpdateHandlerImpl() // override
{
    ADUC_Logging_Uninit();
}

// Forward declarations.
static ADUC_Result CancelApply(const char* logFolder);

/**
 * @brief Creates a new SWUpdateHandlerImpl object and casts to a ContentHandler.
 * Note that there is no way to create a SWUpdateHandlerImpl directly.
 *
 * @return ContentHandler* SimulatorHandlerImpl object as a ContentHandler.
 */
ContentHandler* SWUpdateHandlerImpl::CreateContentHandler()
{
    return new SWUpdateHandlerImpl();
}

/**
 * @brief Performs 'Download' task.
 *
 * @return ADUC_Result The result of the download (always success)
 */
ADUC_Result SWUpdateHandlerImpl::Download(const tagADUC_WorkflowData* workflowData)
{
    std::stringstream updateFilename;
    ADUC_Result result = { ADUC_Result_Failure };
    ADUC_FileEntity* entity = nullptr;
    ADUC_WorkflowHandle workflowHandle = workflowData->WorkflowHandle;
    char* workFolder = workflow_get_workfolder(workflowHandle);
    int fileCount = 0;

    char* updateType = workflow_get_update_type(workflowHandle);
    char* updateName = nullptr;
    unsigned int updateTypeVersion = 0;
    bool updateTypeOk = false;

    if (workflow_is_cancel_requested(workflowHandle))
    {
        result = this->Cancel(workflowData);
        goto done;
    }

    updateTypeOk = ADUC_ParseUpdateType(updateType, &updateName, &updateTypeVersion);
    if (!updateTypeOk)
    {
        Log_Error("SWUpdate packages download failed. Unknown Handler Version (UpdateDateType:%s)", updateType);
        result.ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_UNKNOWN_UPDATE_VERSION;
        goto done;
    }

    if (updateTypeVersion != 1)
    {
        Log_Error("SWUpdate packages download failed. Wrong Handler Version %d", updateTypeVersion);
        result.ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_WRONG_UPDATE_VERSION;
        goto done;
    }

    // For 'microsoft/swupdate:1', we're expecting 1 payload file.
    fileCount = workflow_get_update_files_count(workflowHandle);
    if (fileCount != 1)
    {
        Log_Error("SWUpdate expecting one file. (%d)", fileCount);
        result.ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_WRONG_FILECOUNT;
        goto done;
    }

    if (!workflow_get_update_file(workflowHandle, 0, &entity))
    {
        result.ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_BAD_FILE_ENTITY;
        goto done;
    }

    updateFilename << workFolder << "/" << entity->TargetFilename;

    {
        ExtensionManager_Download_Options downloadOptions = {
            .retryTimeout = DO_RETRY_TIMEOUT_DEFAULT,
        };

        result = ExtensionManager::Download(entity, workflowHandle, &downloadOptions, nullptr);
    }

done:
    workflow_free_string(workFolder);
    workflow_free_file_entity(entity);

    return result;
}

/**
 * @brief Install implementation for swupdate.
 * Calls into the swupdate wrapper script to install an image file.
 *
 * @return ADUC_Result The result of the install.
 */
ADUC_Result SWUpdateHandlerImpl::Install(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result = { ADUC_Result_Failure };
    ADUC_FileEntity* entity = nullptr;
    ADUC_WorkflowHandle workflowHandle = workflowData->WorkflowHandle;
    char* workFolder = workflow_get_workfolder(workflowHandle);

    Log_Info("Installing from %s", workFolder);
    std::unique_ptr<DIR, std::function<int(DIR*)>> directory(
        opendir(workFolder), [](DIR* dirp) -> int { return closedir(dirp); });
    if (directory == nullptr)
    {
        Log_Error("opendir failed, errno = %d", errno);

        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_INSTALL_FAILURE_CANNOT_OPEN_WORKFOLDER };
        goto done;
    }

    if (workflow_is_cancel_requested(workflowHandle))
    {
        result = this->Cancel(workflowData);
        goto done;
    }

    if (!workflow_get_update_file(workflowHandle, 0, &entity))
    {
        result.ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_INSTALL_FAILURE_BAD_FILE_ENTITY;
        goto done;
    }

    // For 'microsoft/swupdate:1', we are only support 1 image file.

    // Execute the install command with  "-i <image_file>"  to install the update image file.
    // For swupdate the image file is typically a .swu file

    // This is equivalent to: command << c_installScript << " -l " << _logFolder << " -i '" << _workFolder << "/" << filename << "'"

    {
        std::string command = adushconst::adu_shell;
        std::vector<std::string> args{ adushconst::update_type_opt,
                                       adushconst::update_type_microsoft_swupdate,
                                       adushconst::update_action_opt,
                                       adushconst::update_action_install };

        std::stringstream data;
        data << workFolder << "/" << entity->TargetFilename;
        args.emplace_back(adushconst::target_data_opt);
        args.emplace_back(data.str().c_str());

        args.emplace_back(adushconst::target_log_folder_opt);

        args.emplace_back(ADUC_LOG_FOLDER);

        std::string output;
        const int exitCode = ADUC_LaunchChildProcess(command, args, output);

        if (exitCode != 0)
        {
            Log_Error("Install failed, extendedResultCode = %d", exitCode);
            result = { .ResultCode = ADUC_Result_Failure, .ExtendedResultCode = exitCode };
            goto done;
        }
    }

    Log_Info("Install succeeded");
    result.ResultCode = ADUC_Result_Install_Success;

done:
    workflow_free_string(workFolder);
    workflow_free_file_entity(entity);
    return result;
}

/**
 * @brief Apply implementation for swupdate.
 * Calls into the swupdate wrapper script to perform apply.
 * Will flip bootloader flag to boot into update partition for A/B update.
 *
 * @return ADUC_Result The result of the apply.
 */
ADUC_Result SWUpdateHandlerImpl::Apply(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result = { ADUC_Result_Failure };
    char* workFolder = NULL;

    if (workflow_is_cancel_requested(workflowData->WorkflowHandle))
    {
        return this->Cancel(workflowData);
    }

    workFolder = workflow_get_workfolder(workflowData->WorkflowHandle);
    Log_Info("Applying data from %s", workFolder);

    // Execute the install command with  "-a" to apply the install by telling
    // the bootloader to boot to the updated partition.

    // This is equivalent to : command << c_installScript << " -l " << _logFolder << " -a"

    std::string command = adushconst::adu_shell;
    std::vector<std::string> args{ adushconst::update_type_opt,
                                   adushconst::update_type_microsoft_swupdate,
                                   adushconst::update_action_opt,
                                   adushconst::update_action_apply };

    args.emplace_back(adushconst::target_log_folder_opt);
    args.emplace_back(ADUC_LOG_FOLDER);

    std::string output;

    const int exitCode = ADUC_LaunchChildProcess(command, args, output);

    if (exitCode != 0)
    {
        Log_Error("Apply failed, extendedResultCode = %d", exitCode);
        result = { ADUC_Result_Failure, exitCode };
        goto done;
    }

    // Cancel requested?
    if (workflow_is_cancel_requested(workflowData->WorkflowHandle))
    {
        result = this->Cancel(workflowData);
        goto done;
    }

    if (workflow_get_operation_cancel_requested(workflowData->WorkflowHandle))
    {
        CancelApply(ADUC_LOG_FOLDER);
    }

done:
    workflow_free_string(workFolder);

    // Always require a reboot after successful apply
    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        workflow_request_immediate_reboot(workflowData->WorkflowHandle);
        result = { ADUC_Result_Apply_RequiredImmediateReboot };
    }

    return result;
}

/**
 * @brief Cancel implementation for swupdate.
 * We don't have many hooks into swupdate to cancel an ongoing install.
 * We can cancel apply by reverting the bootloader flag to boot into the original partition.
 * Calls into the swupdate wrapper script to cancel apply.
 * Cancel after or during any other operation is a no-op.
 *
 * @return ADUC_Result The result of the cancel.
 */
ADUC_Result SWUpdateHandlerImpl::Cancel(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Cancel_Success, .ExtendedResultCode = 0 };
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    ADUC_WorkflowHandle stepWorkflowHandle = nullptr;

    const char* workflowId = workflow_peek_id(handle);
    int workflowLevel = workflow_get_level(handle);
    int workflowStep = workflow_get_step_index(handle);

    Log_Info(
        "Requesting cancel operation (workflow id '%s', level %d, step %d).", workflowId, workflowLevel, workflowStep);
    if (!workflow_request_cancel(handle))
    {
        Log_Error(
            "Cancellation request failed. (workflow id '%s', level %d, step %d)",
            workflowId,
            workflowLevel,
            workflowStep);
        result.ResultCode = ADUC_Result_Cancel_UnableToCancel;
    }

    return result;
}

/**
 * @brief Reads a first line of a file, trims trailing whitespace, and returns as string.
 *
 * @param filePath Path to the file to read value from.
 * @return std::string Returns the value from the file. Returns empty string if there was an error.
 */
/*static*/
std::string SWUpdateHandlerImpl::ReadValueFromFile(const std::string& filePath)
{
    if (filePath.empty())
    {
        Log_Error("Empty file path.");
        return std::string{};
    }

    if ((filePath.length()) + 1 > PATH_MAX)
    {
        Log_Error("Path is too long.");
        return std::string{};
    }

    std::ifstream file(filePath);
    if (!file.is_open())
    {
        Log_Error("File %s failed to open, error: %d", filePath.c_str(), errno);
        return std::string{};
    }

    std::string result;
    std::getline(file, result);
    if (file.bad())
    {
        Log_Error("Unable to read from file %s, error: %d", filePath.c_str(), errno);
        return std::string{};
    }

    // Trim whitespace
    ADUC::StringUtils::Trim(result);
    return result;
}

/**
 * @brief Checks if the installed content matches the installed criteria.
 *
 * @param installedCriteria The installed criteria string. e.g. The firmware version.
 *  installedCriteria has already been checked to be non-empty before this call.
 *
 * @return ADUC_Result
 */
ADUC_Result SWUpdateHandlerImpl::IsInstalled(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result;

    char* installedCriteria = ADUC_WorkflowData_GetInstalledCriteria(workflowData);
    if (installedCriteria == nullptr)
    {
        Log_Error("Missing installedCriteria.");
        result = { ADUC_Result_Failure, ADUC_ERC_SWUPDATE_HANDLER_MISSING_INSTALLED_CRITERIA };
        return result;
    }

    std::string version{ ReadValueFromFile(ADUC_VERSION_FILE) };
    if (version.empty())
    {
        Log_Error("Version file %s did not contain a version or could not be read.", ADUC_VERSION_FILE);
        result = { ADUC_Result_Failure };
        goto done;
    }

    if (version == installedCriteria)
    {
        Log_Info("Installed criteria %s was installed.", installedCriteria);
        result = { ADUC_Result_IsInstalled_Installed };
        goto done;
    }

    Log_Info("Installed criteria %s was not installed, the current version is %s", installedCriteria, version.c_str());

    result = { ADUC_Result_IsInstalled_NotInstalled };

done:
    workflow_free_string(installedCriteria);
    return result;
}

/**
 * @brief Helper function to perform cancel when we are doing an apply.
 *
 * @return ADUC_Result The result of the cancel.
 */
static ADUC_Result CancelApply(const char* logFolder)
{
    // Execute the install command with  "-r" to reverts the apply by
    // telling the bootloader to boot into the current partition

    // This is equivalent to : command << c_installScript << " -l " << logFolder << " -r"

    std::string command = adushconst::adu_shell;
    std::vector<std::string> args{ adushconst::update_type_opt,       adushconst::update_type_microsoft_swupdate,
                                   adushconst::update_action_opt,     adushconst::update_action_apply,
                                   adushconst::target_log_folder_opt, logFolder };

    std::string output;

    const int exitCode = ADUC_LaunchChildProcess(command, args, output);
    if (exitCode != 0)
    {
        // If failed to cancel apply, apply should return SuccessRebootRequired.
        Log_Error("Failed to cancel Apply, extendedResultCode = %d", exitCode);
        return ADUC_Result{ ADUC_Result_Failure, exitCode };
    }

    Log_Info("Apply was cancelled");
    return ADUC_Result{ ADUC_Result_Failure_Cancelled };
}

/**
 * @brief Backup implementation for swupdate.
 * Calls into the swupdate wrapper script to perform backup.
 * For swupdate, no operation is required.
 *
 * @return ADUC_Result The result of the backup.
 * It will always return ADUC_Result_Backup_Success.
 */
ADUC_Result SWUpdateHandlerImpl::Backup(const tagADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);
    ADUC_Result result = { ADUC_Result_Backup_Success };
    Log_Info("SWUpdate doesn't require a specific operation to backup. (no-op) ");
    return result;
}

/**
 * @brief Restore implementation for swupdate.
 * Calls into the swupdate wrapper script to perform restore.
 * Will flip bootloader flag to boot into the previous partition for A/B update.
 *
 * @return ADUC_Result The result of the restore.
 */
ADUC_Result SWUpdateHandlerImpl::Restore(const tagADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);
    ADUC_Result result = { ADUC_Result_Restore_Success };
    ADUC_Result cancel_result = CancelApply(ADUC_LOG_FOLDER);
    if (cancel_result.ResultCode != ADUC_Result_Failure_Cancelled)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_UPPERLEVEL_WORKFLOW_FAILED_RESTORE_FAILED };
    }
    return result;
}
