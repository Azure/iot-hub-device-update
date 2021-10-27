/**
 * @file script_handler.cpp
 * @brief Implementation of ContentHandler API for 'microsoft/script:1' update type.
 *
 * @copyright Copyright (c), Microsoft Corporation.
 */
#include "aduc/script_handler.hpp"

#include "aduc/adu_core_exports.h"
#include "aduc/extension_manager.hpp"
#include "aduc/logging.h"
#include "aduc/process_utils.hpp"
#include "aduc/string_utils.hpp"
#include "aduc/system_utils.h"
#include "aduc/types/workflow.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_utils.h"
#include "adushell_const.hpp"

#include <algorithm>
#include <azure_c_shared_utility/crt_abstractions.h> // for mallocAndStrcpy_s
#include <azure_c_shared_utility/strings.h> // STRING_*
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <dirent.h>

namespace adushconst = Adu::Shell::Const;

EXTERN_C_BEGIN

/**
 * @brief Instantiates an Update Content Handler for 'microsoft/bundle:1' update type.
 */
ContentHandler* CreateUpdateContentHandlerExtension()
{
    Log_Info("Instantiating an Update Content Handler for 'microsoft/script:1'");
    return ScriptHandlerImpl::CreateContentHandler();
}

EXTERN_C_END

// Forward declarations.
static ADUC_Result CancelApply(const char* logFolder);

/**
 * @brief Creates a new ScriptHandlerImpl object and casts to a ContentHandler.
 * Note that there is no way to create a ScriptHandlerImpl directly.
 *
 * @return ContentHandler* ScriptHandlerImpl object as a ContentHandler.
 */
ContentHandler* ScriptHandlerImpl::CreateContentHandler()
{
    return new ScriptHandlerImpl();
}

/**
 * @brief Performs a download task. The first file in FileEntity list must be the main script file,
 * which will be downloaded into the 'Working Folder' for the current 'Workflow' context.
 *
 * This handler will then execute the main script with '--is-installed' argument to determine whether
 * to continue downloading the remaining file(s) in FileEntity list, if any.
 *
 * @return ADUC_Result The result of this download task.
 *
 *     Following are the potential extended result codes:
 *
 *     ADUC_ERC_UPDATE_CONTENT_HANDLER_DOWNLOAD_FAILURE_BADFILECOUNT(201)
 *     ADUC_ERC_UPDATE_CONTENT_HANDLER_DOWNLOAD_FAILURE_UNKNOWNEXCEPTION(202)
 *
 *     ADUC_ERC_CONTENT_DOWNLOADER_*
 */
ADUC_Result ScriptHandlerImpl::Download(const ADUC_WorkflowData* workflowData)
{
    Log_Info("Script_Handler download task begin.");

    ADUC_Result result = { ADUC_Result_Failure };
    char* installedCriteria = nullptr;
    char* workflowId = nullptr;
    char* workFolder = nullptr;
    ADUC_FileEntity* entity = nullptr;
    ADUC_WorkflowHandle workflowHandle = workflowData->WorkflowHandle;
    int fileCount = workflow_get_update_files_count(workflowHandle);

    if (fileCount <= 0)
    {
        result.ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_FAILURE_INVALID_FILE_COUNT;
        goto done;
    }

    // Download the main script file.
    if (!workflow_get_update_file(workflowHandle, 0, &entity))
    {
        result.ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_FAILURE_GET_PRIMARY_FILE_ENTITY;
        goto done;
    }

    workflowId = workflow_get_id(workflowHandle);
    workFolder = workflow_get_workfolder(workflowHandle);

    try
    {
        result = ExtensionManager::Download(entity, workflowId, workFolder, DO_RETRY_TIMEOUT_DEFAULT, nullptr);
    }
    catch (...)
    {
        result.ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_PRIMARY_FILE_FAILURE_UNKNOWNEXCEPTION;
    }

    workflow_free_file_entity(entity);
    entity = nullptr;

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("Cannot download main script file. (0x%X)", result.ExtendedResultCode);
        goto done;
    }

    // Determine whether to continue downloading the rest.
    installedCriteria = workflow_get_installed_criteria(workflowData->WorkflowHandle);
    result = IsInstalled(workflowData);

    if (result.ResultCode == ADUC_Result_IsInstalled_Installed)
    {
        result = { ADUC_Result_Download_Skipped_UpdateAlreadyInstalled };
        goto done;
    }

    result = { ADUC_Result_Download_Success };

    // Continue downloading the remaining files, if any.
    for (int i = 1; i < fileCount; i++)
    {
        Log_Info("Downloading file #%d", i);

        if (!workflow_get_update_file(workflowHandle, i, &entity))
        {
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_FAILURE_GET_PAYLOAD_FILE_ENTITY };
            goto done;
        }

        try
        {
            result = ExtensionManager::Download(entity, workflowId, workFolder, DO_RETRY_TIMEOUT_DEFAULT, nullptr);
        }
        catch (...)
        {
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_PAYLOAD_FILE_FAILURE_UNKNOWNEXCEPTION };
        }

        workflow_free_file_entity(entity);
        entity = nullptr;

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            Log_Error("Cannot download script payload file#%d. (0x%X)", i, result.ExtendedResultCode);
            goto done;
        }
    }

done:
    workflow_free_string(workflowId);
    workflow_free_string(workFolder);
    workflow_free_file_entity(entity);
    workflow_free_string(installedCriteria);
    Log_Info("Script_Handler download task end.");
    return result;
}

/**
 * @brief A helper function that return a script file path, and arguments list.
 *
 * @param workflowHandle An 'Install' phase workflow data containing script information and selected component.
 * @param resultFilePath A full path of the file containing seiralized ADUC_Result value returned by the script.
 * @param[out] scriptFilePath A output script file path.
 * @param[out] args An output script arguments list.
 * @return ADUC_Result
 */
ADUC_Result ScriptHandlerImpl::PrepareScriptArguments(
    const ADUC_WorkflowHandle workflowHandle,
    std::string resultFilePath,
    std::string workFolder,
    std::string& scriptFilePath,
    std::vector<std::string>& args)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure };

    ADUC_FileEntity* scriptFileEntity = nullptr;

    const char* selectedComponentsJson = nullptr;
    JSON_Value* selectedComponentsValue = nullptr;
    JSON_Object* selectedComponentsObject = nullptr;
    JSON_Array* componentsArray = nullptr;
    int componentCount = 0;
    JSON_Object* component = nullptr;

    std::string fileArgs;
    std::vector<std::string> argumentList;
    std::stringstream filePath;

    char* installedCriteria = nullptr;

    bool success = false;
    if (workflowHandle == nullptr)
    {
        result.ExtendedResultCode = ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_NULL_WORKFLOW;
        goto done;
    }

    installedCriteria = workflow_get_installed_criteria(workflowHandle);

    // Parse componenets list. If the list is empty, nothing to download.
    selectedComponentsJson = workflow_peek_selected_components(workflowHandle);

    if (selectedComponentsJson == nullptr || *selectedComponentsJson == '\0')
    {
        result = { ADUC_Result_Download_Skipped_NoMatchingComponents };
        goto done;
    }

    selectedComponentsValue = json_parse_string(selectedComponentsJson);
    if (selectedComponentsValue == nullptr)
    {
        result.ExtendedResultCode = ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_MISSING_PRIMARY_COMPONENT;
        goto done;
    }

    selectedComponentsObject = json_value_get_object(selectedComponentsValue);
    componentsArray = json_object_get_array(selectedComponentsObject, "components");
    if (componentsArray == nullptr)
    {
        result.ExtendedResultCode = ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_MISSING_PRIMARY_COMPONENT;
        goto done;
    }

    // Prepare target component info.
    componentCount = json_array_get_count(componentsArray);

    if (componentCount <= 0)
    {
        result.ResultCode = ADUC_Result_Download_Skipped_NoMatchingComponents;
        goto done;
    }

    if (componentCount > 1)
    {
        Log_Error("Expecting only 1 component, but got %d.", componentCount);
        result.ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_TOO_MANY_COMPONENTS;
    }

    component = json_array_get_object(componentsArray, 0);
    if (component == nullptr)
    {
        result.ExtendedResultCode = ADUC_ERC_COMPONENTS_HANDLER_INVALID_COMPONENTS_DATA;
        goto done;
    }

    //
    // Prepare script file info.
    // Note: for MCU v1, the primary script file has 'fileType' property equals to 'script'.
    //
    success = workflow_get_first_update_file_of_type(workflowHandle, "script", &scriptFileEntity);
    if (!success)
    {
        result.ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_INSTALL_FAILURE_MISSING_PRIMARY_SCRIPT_FILE;
        goto done;
    }

    filePath << workFolder.c_str() << "/" << scriptFileEntity->TargetFilename;
    scriptFilePath = filePath.str();

    //
    // Prepare script arguments.
    //

    // Add customer specified arguments first.
    fileArgs = scriptFileEntity->Arguments;
    argumentList = ADUC::StringUtils::Split(fileArgs, ' ');
    for (int i = 0; i < argumentList.size(); i++)
    {
        const std::string argument = argumentList[i];
        if (!argument.empty())
        {
            // TODO(Nox): 34317934: [Code Clean Up] Create map for simplify processing of microsoft/script argument placeholders.
            if (argument == "--component-id-val")
            {
                const char* val = json_object_get_string(component, "id");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
            }
            else if (argument == "--component-name-val")
            {
                const char* val = json_object_get_string(component, "name");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
            }
            else if (argument == "--component-manufacturer-val")
            {
                const char* val = json_object_get_string(component, "manufacturer");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
            }
            else if (argument == "--component-model-val")
            {
                const char* val = json_object_get_string(component, "model");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
            }
            else if (argument == "--component-version-val")
            {
                const char* val = json_object_get_string(component, "version");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
            }
            else if (argument == "--component-group-val")
            {
                const char* val = json_object_get_string(component, "group");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
            }
            else if (argument == "--component-prop-val")
            {
                if (i < argumentList.size() - 1)
                {
                    std::string propertyPath = "properties.";
                    propertyPath += argumentList[i + 1];
                    const char* val = json_object_dotget_string(component, propertyPath.c_str());
                    if (val != nullptr)
                    {
                        args.emplace_back(val);
                    }
                    i++;
                }
            }
            else
            {
                args.emplace_back(argument);
            }
        }
    }

    // Default options.
    args.emplace_back("--workfolder");
    args.emplace_back(workFolder);

    args.emplace_back("--result-file");
    args.emplace_back(resultFilePath);

    args.emplace_back("--installed-criteria");
    args.emplace_back(installedCriteria);

    result = { ADUC_Result_Success };

done:
    if (selectedComponentsValue != nullptr)
    {
        json_value_free(selectedComponentsValue);
    }

    workflow_free_string(installedCriteria);
    return result;
}

/**
 * @brief Performs 'Install' task.
 * @return ADUC_Result The result
 */
ADUC_Result ScriptHandlerImpl::Install(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result = PerformAction("--action-install", workflowData);
    return result;
}

ADUC_Result ScriptHandlerImpl::PerformAction(const std::string& action, const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure };
    STRING_HANDLE resultDetails;

    std::string scriptFilePath;
    std::vector<std::string> args;
    std::string scriptOutput;
    int exitCode = 0;

    if (workflowData == nullptr || workflowData->WorkflowHandle == nullptr)
    {
        result.ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_INSTALL_ERROR_NULL_WORKFLOW;
        return result;
    }

    char* workFolder = ADUC_WorkflowData_GetWorkFolder(workflowData);
    std::string scriptWorkfolder = workFolder;
    std::string scriptResultFile = scriptWorkfolder + "/" + "aduc_result.json";
    JSON_Value* installResultValue = nullptr;

    std::vector<std::string> aduShellArgs = { adushconst::update_type_opt,
                                              adushconst::update_type_microsoft_script,
                                              adushconst::update_action_opt,
                                              adushconst::update_action_execute };

    std::stringstream ss;

    result =
        PrepareScriptArguments(workflowData->WorkflowHandle, scriptResultFile, scriptWorkfolder, scriptFilePath, args);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    // If any install-item reported that the update is already installed on the
    // selected component, we will skip the 'apply' phase, and then skip the
    // remaining install-item(s).
    // Also, don't continue if WorkflowHandle is NULL in the ADUInterface_Connected->HandleStartupWorkflowData flow.
    if (result.ResultCode == ADUC_Result_Install_Skipped_UpdateAlreadyInstalled ||
        workflowData->WorkflowHandle == nullptr)
    {
        goto done;
    }

    aduShellArgs.emplace_back(adushconst::target_data_opt);
    aduShellArgs.emplace_back(scriptFilePath);

    aduShellArgs.emplace_back(adushconst::target_options_opt);
    aduShellArgs.emplace_back(action.c_str());

    for (auto a : args)
    {
        aduShellArgs.emplace_back(adushconst::target_options_opt);
        aduShellArgs.emplace_back(a);
    }

#if _ADU_DEBUG
    for (const auto a : aduShellArgs)
    {
        ss << " " << a;
    }
    Log_Debug("##########\n# ADU-SHELL ARGS:\n##########\n %s", ss.str().c_str());
#endif

    exitCode = ADUC_LaunchChildProcess(adushconst::adu_shell, aduShellArgs, scriptOutput);
    if (exitCode != 0)
    {
        int extendedCode = ADUC_ERC_SCRIPT_HANDLER_CHILD_PROCESS_FAILURE_EXITCODE(exitCode);
        Log_Error("Install failed, extendedResultCode:0x%X (exitCode:%d)", extendedCode, exitCode);
        result = { .ResultCode = ADUC_Result_Failure, .ExtendedResultCode = extendedCode };
    }

    if (!scriptOutput.empty())
    {
        Log_Info(scriptOutput.c_str());
    }

    // Parse result file.
    installResultValue = json_parse_file(scriptResultFile.c_str());
    if (installResultValue == nullptr)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_INSTALL_FAILURE_PARSE_RESULT_FILE };
        workflow_set_result_details(
            workflowData->WorkflowHandle,
            "The install script doesn't create a result file '%s'.",
            scriptResultFile.c_str());
        goto done;
    }
    else
    {
        JSON_Object* installResultObject = json_object(installResultValue);
        result.ResultCode = json_object_get_number(installResultObject, "resultCode");
        result.ExtendedResultCode = json_object_get_number(installResultObject, "extendedResultCode");
        const char* details = json_object_get_string(installResultObject, "resultDetails");
        workflow_set_result_details(workflowData->WorkflowHandle, details);
    }

    Log_Info(
        "Install done - returning rc:%d, erc:0x%X, rd:%s",
        result.ResultCode,
        result.ExtendedResultCode,
        workflow_peek_result_details(workflowData->WorkflowHandle));

done:
    if (IsAducResultCodeFailure(result.ResultCode) && workflowData->WorkflowHandle != nullptr)
    {
        workflow_set_result(workflowData->WorkflowHandle, result);
        workflow_set_state(workflowData->WorkflowHandle, ADUCITF_State_Failed);
    }

    json_value_free(installResultValue);
    workflow_free_string(workFolder);
    return result;
}

/**
 * @brief Performs 'Apply' task.
 * @return ADUC_Result The result return from script execution.
 */
ADUC_Result ScriptHandlerImpl::Apply(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result = PerformAction("--action-apply", workflowData);
    return result;
}

/**
 * @brief Performs 'Cancel' task.
 * @return ADUC_Result The result (always success)
 */
ADUC_Result ScriptHandlerImpl::Cancel(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result = PerformAction("--action-cancel", workflowData);
    return result;
}

/**
 * @brief Check whether the current device state satisfies specified 'installed-critieria'.
 * @param isntalledCriteria A string used to determine whether an update is installed on the device.
 * @return ADUC_Result The result based on evaluating the installed criteria.
 */
ADUC_Result ScriptHandlerImpl::IsInstalled(const ADUC_WorkflowData* workflowData)
{
    ADUC_Result result = PerformAction("--action-is-installed", workflowData);
    return result;
}