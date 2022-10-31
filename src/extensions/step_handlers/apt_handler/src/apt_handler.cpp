/**
 * @file apt_handler.cpp
 * @brief Implementation of APT handler plug-in for APT (Advanced Package Tool)
 *
 * microsoft/apt
 * v1:
 *   Description:
 *   Initial revision.
 *
 *   Expected files:
 *   <manifest>.json - contains apt configuration and package list.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/apt_handler.hpp"
#include "aduc/adu_core_exports.h"
#include "aduc/extension_manager.hpp"
#include "aduc/installed_criteria_utils.hpp"
#include "aduc/logging.h"
#include "aduc/process_utils.hpp"
#include "aduc/string_c_utils.h"
#include "aduc/types/update_content.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_utils.h"
#include "adushell_const.hpp"

#include <chrono>
#include <fstream>
#include <parson.h>
#include <sstream>
#include <string>

namespace adushconst = Adu::Shell::Const;

/* external linkage */
extern ExtensionManager_Download_Options Default_ExtensionManager_Download_Options;

EXTERN_C_BEGIN

/////////////////////////////////////////////////////////////////////////////
// BEGIN Shared Library Export Functions
//
// These are the function symbols that the device update agent will
// lookup and call.
//

/**
 * @brief Instantiates an Update Content Handler for 'microsoft/apt:1' update type.
 * @return ContentHandler* The created instance.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel)
{
    ADUC_Logging_Init(logLevel, "apt-handler");
    Log_Info("Instantiating an Update Content Handler for 'microsoft/apt:1'");
    try
    {
        return AptHandlerImpl::CreateContentHandler();
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
 * @brief Destructor for the Apt Handler Impl class.
 */
AptHandlerImpl::~AptHandlerImpl() // override
{
    ADUC_Logging_Uninit();
}

/**
 * @brief Creates a new APTHandlerImpl object and casts to a ContentHandler.
 * @details Note that there is no way to create a APTHandlerImpl directly.
 *
 * @return ContentHandler* APTHandlerImpl object as a ContentHandler.
 */
ContentHandler* AptHandlerImpl::CreateContentHandler()
{
    return new AptHandlerImpl();
}

ADUC_Result AptHandlerImpl::ParseContent(const std::string& aptManifestFile, std::unique_ptr<AptContent>& aptContent)
{
    ADUC_Result result = { ADUC_GeneralResult_Success };

    // Parse the APT manifest file.
    try
    {
        aptContent = AptParser::ParseAptContentFromFile(aptManifestFile);
    }
    catch (const AptParser::ParserException& pe)
    {
        aptContent.reset();
        Log_Error("An error occurred while parsing APT manifest. %s", pe.what());
        result = { ADUC_GeneralResult_Failure, ADUC_ERC_UPDATE_CONTENT_HANDLER_PARSE_BAD_FORMAT };
        goto done;
    }

done:
    return result;
}

/**
 * @brief Download implementation for APT handler.
 *
 * @return ADUC_Result The result of the download.
 */
ADUC_Result AptHandlerImpl::Download(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure, .ExtendedResultCode = 0 };
    std::stringstream aptManifestFilename;
    std::unique_ptr<AptContent> aptContent{ nullptr };
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    int fileCount = 0;
    char* installedCriteria = nullptr;

    if (workflow_is_cancel_requested(handle))
    {
        return this->Cancel(workflowData);
    }

    // For 'microsoft/apt:1', we're expecting 1 payload file.
    fileCount = workflow_get_update_files_count(handle);
    if (fileCount != 1)
    {
        Log_Error("APT packages expecting one file. (%d)", fileCount);
        return ADUC_Result{ .ResultCode = ADUC_Result_Failure,
                            .ExtendedResultCode = ADUC_ERC_APT_HANDLER_PACKAGE_PREPARE_FAILURE_WRONG_FILECOUNT };
    }

    char* workFolder = workflow_get_workfolder(handle);
    ADUC_FileEntity* fileEntity = nullptr;

    if (!workflow_get_update_file(handle, 0, &fileEntity))
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_APT_HANDLER_GET_FILEENTITY_FAILURE };
        goto done;
    }

    installedCriteria = workflow_get_installed_criteria(handle);
    if (IsNullOrEmpty(installedCriteria))
    {
        workflow_set_result_details(handle, "Property 'installedCriteria' in handlerProperties is missing or empty.");
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_APT_HANDLER_MISSING_INSTALLED_CRITERIA };
        goto done;
    }

    aptManifestFilename << workFolder << "/" << fileEntity->TargetFilename;

    // Download the APT manifest file.
    result = ExtensionManager::Download(
        fileEntity, workflowData->WorkflowHandle, &Default_ExtensionManager_Download_Options, nullptr);

    workflow_free_file_entity(fileEntity);
    fileEntity = nullptr;

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = ParseContent(aptManifestFilename.str(), aptContent);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    {
        std::string aptOutput;
        int aptExitCode = -1;

        // Perform apt-get update to fetch latest packages catalog.
        // We'll log warning if failed, but will try to download specified packages.
        try
        {
            // Perform apt-get update.
            const std::vector<std::string> args = { adushconst::update_type_opt,
                                                    adushconst::update_type_microsoft_apt,
                                                    adushconst::update_action_opt,
                                                    adushconst::update_action_initialize };

            aptExitCode = ADUC_LaunchChildProcess(adushconst::adu_shell, args, aptOutput);

            if (!aptOutput.empty())
            {
                Log_Info(aptOutput.c_str());
            }
        }
        catch (const std::exception& de)
        {
            Log_Error("Exception occurred while processing apt-get update.\n%s", de.what());
            aptExitCode = -1;
        }

        if (aptExitCode != 0)
        {
            Log_Error("APT update failed. (Exit code: %d)", aptExitCode);
        }

        // Download packages.
        result = { ADUC_Result_Download_Success };
        try
        {
            std::vector<std::string> args = { adushconst::update_type_opt,
                                              adushconst::update_type_microsoft_apt,
                                              adushconst::update_action_opt,
                                              adushconst::update_action_download };

            // For microsoft/apt, target-data is a list of packages.
            std::stringstream data;
            data << "'";
            for (const std::string& package : aptContent->Packages)
            {
                data << package << " ";
            }
            data << "'";

            args.emplace_back(adushconst::target_data_opt);
            args.emplace_back(data.str());

            aptExitCode = ADUC_LaunchChildProcess(adushconst::adu_shell, args, aptOutput);

            if (!aptOutput.empty())
            {
                Log_Info("\n\nadu-shell logs\n================\n\n%s", aptOutput.c_str());
            }
        }
        catch (const std::exception& de)
        {
            Log_Error("Exception occurred during download. %s", de.what());
            aptExitCode = -1;
        }

        if (aptExitCode != 0)
        {
            result.ResultCode = ADUC_Result_Failure;
            result.ExtendedResultCode = ADUC_ERC_APT_HANDLER_PACKAGE_DOWNLOAD_FAILURE;
            Log_Error("APT packages download failed. (Exit code: %d)", aptExitCode);
            goto done;
        }
    }

    result = { .ResultCode = ADUC_Result_Download_Success, .ExtendedResultCode = 0 };

done:
    workflow_free_string(installedCriteria);
    workflow_free_string(workFolder);
    workflow_free_file_entity(fileEntity);

    return result;
}

/**
 * @brief Install implementation for APT Handler.
 *
 * @return ADUC_Result The result of the install.
 */
ADUC_Result AptHandlerImpl::Install(const ADUC_WorkflowData* workflowData)
{
    std::string aptOutput;
    int aptExitCode = -1;
    ADUC_Result result = { .ResultCode = ADUC_Result_Download_Success, .ExtendedResultCode = 0 };
    ADUC_FileEntity* fileEntity = nullptr;
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    char* workFolder = workflow_get_workfolder(handle);
    std::stringstream aptManifestFilename;
    std::unique_ptr<AptContent> aptContent;

    if (workflow_is_cancel_requested(handle))
    {
        result = this->Cancel(workflowData);
        goto done;
    }

    if (!workflow_get_update_file(handle, 0, &fileEntity))
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_APT_HANDLER_GET_FILEENTITY_FAILURE };
        goto done;
    }

    aptManifestFilename << workFolder << "/" << fileEntity->TargetFilename;

    result = ParseContent(aptManifestFilename.str(), aptContent);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    try
    {
        std::vector<std::string> args = { adushconst::update_type_opt,
                                          adushconst::update_type_microsoft_apt,
                                          adushconst::update_action_opt,
                                          adushconst::update_action_install };

        //
        // For public preview, we're passing following additional options to apt-get.
        //
        // '-y' (assumed yes)
        //  -o Dpkg::Options::=--force-confdef -o Dpkg::Options::=--force-confold" (preserve existing config.yaml file by default)
        //
        args.emplace_back(adushconst::target_options_opt);
        args.emplace_back("-o Dpkg::Options::=--force-confdef -o Dpkg::Options::=--force-confold");

        // For microsoft/apt, target-data is a list of packages.
        std::stringstream data;
        for (const std::string& package : aptContent->Packages)
        {
            data << package << " ";
        }
        args.emplace_back(adushconst::target_data_opt);
        args.emplace_back(data.str());

        aptExitCode = ADUC_LaunchChildProcess(adushconst::adu_shell, args, aptOutput);

        if (!aptOutput.empty())
        {
            Log_Info("\n\nadu-shell logs\n================\n\n%s", aptOutput.c_str());
        }
    }
    catch (const std::exception& de)
    {
        Log_Error("Exception occurred during install. %s", de.what());
        aptExitCode = -1;
    }

    if (aptExitCode != 0)
    {
        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = ADUC_ERC_APT_HANDLER_PACKAGE_INSTALL_FAILURE;
        Log_Error("APT packages install failed. (Exit code: %d)", aptExitCode);
        goto done;
    }

    result = { .ResultCode = ADUC_Result_Install_Success, .ExtendedResultCode = 0 };

done:
    workflow_free_string(workFolder);
    workflow_free_file_entity(fileEntity);
    return result;
}

/**
 * @brief Apply implementation for APT Handler.
 *
 * @return ADUC_Result The result of the apply.
 */
ADUC_Result AptHandlerImpl::Apply(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Apply_Success, .ExtendedResultCode = 0 };
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    char* installedCriteria = workflow_get_installed_criteria(handle);
    char* workFolder = workflow_get_workfolder(handle);
    std::unique_ptr<AptContent> aptContent{ nullptr };
    std::stringstream aptManifestFilename;
    ADUC_FileEntity* entity = nullptr;

    if (workflow_is_cancel_requested(handle))
    {
        result = this->Cancel(workflowData);
        goto done;
    }

    if (!PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria))
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_APT_HANDLER_INSTALLCRITERIA_PERSIST_FAILURE };
        goto done;
    }

    if (!workflow_get_update_file(handle, 0, &entity))
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_APT_HANDLER_GET_FILEENTITY_FAILURE };
        goto done;
    }

    aptManifestFilename << workFolder << "/" << entity->TargetFilename;

    result = ParseContent(aptManifestFilename.str(), aptContent);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        workflow_set_result_details(handle, "Invalid APT manifest file.");
        goto done;
    }

    if (aptContent->AgentRestartRequired)
    {
        Log_Debug("The install task completed successfully, DU Agent restart is required for this update.");
        workflow_request_immediate_agent_restart(handle);
        result = { .ResultCode = ADUC_Result_Apply_RequiredImmediateAgentRestart, .ExtendedResultCode = 0 };
        goto done;
    }
    else
    {
        result = { .ResultCode = ADUC_Result_Apply_Success, .ExtendedResultCode = 0 };
    }

    Log_Info("Apply succeeded");

done:
    workflow_free_string(workFolder);
    workflow_free_string(installedCriteria);
    workflow_free_file_entity(entity);
    return result;
}

/**
 * @brief Cancel implementation for APT Handler.
 *
 * @return ADUC_Result The result of the cancel.
 */
ADUC_Result AptHandlerImpl::Cancel(const ADUC_WorkflowData* workflowData)
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
 * @brief Checks if the installed content matches the installed criteria.
 *
 * @param installedCriteria The installed criteria string. e.g. The firmware version or APT id.
 *  installedCriteria has already been checked to be non-empty before this call.
 *
 * @return ADUC_Result
 */
ADUC_Result AptHandlerImpl::IsInstalled(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_IsInstalled_NotInstalled, .ExtendedResultCode = 0 };
    char* installedCriteria = ADUC_WorkflowData_GetInstalledCriteria(workflowData);
    if (installedCriteria == nullptr)
    {
        Log_Error("installedCriteria is null.");
        goto done;
    }

    result = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria);

done:
    workflow_free_string(installedCriteria);
    return result;
}

/**
 * @brief Backup implementation for APT Handler.
 *
 * @return ADUC_Result The result of the backup.
 */
ADUC_Result AptHandlerImpl::Backup(const tagADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);
    ADUC_Result result = { .ResultCode = ADUC_Result_Backup_Success_Unsupported, .ExtendedResultCode = 0 };
    Log_Info("Apt update backup & restore is not supported. (no-op)");
    return result;
}

/**
 * @brief Restore implementation for APT Handler.
 *
 * @return ADUC_Result The result of the restore.
 */
ADUC_Result AptHandlerImpl::Restore(const tagADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);
    ADUC_Result result = { .ResultCode = ADUC_Result_Restore_Success_Unsupported, .ExtendedResultCode = 0 };
    Log_Info("Apt update backup & restore is not supported. (no-op)");
    return result;
}
