/**
 * @file script_handler.cpp
 * @brief Implementation of ContentHandler API for 'microsoft/script:1' update type.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/script_handler.hpp"
#include "aduc/extension_manager.hpp"
#include "aduc/logging.h"
#include "aduc/process_utils.hpp" // ADUC_LaunchChildProcess
#include "aduc/string_c_utils.h" // IsNullOrEmpty
#include "aduc/string_utils.hpp" // ADUC::StringUtils::Split
#include "aduc/system_utils.h" // ADUC_SystemUtils_MkSandboxDirRecursive
#include "aduc/types/adu_core.h" // ADUC_Result_*
#include "aduc/workflow_data_utils.h" // ADUC_WorkflowData_GetWorkFolder
#include "aduc/workflow_utils.h" // workflow_*
#include "adushell_const.hpp"
#include <sstream>
#include <string>
#include <vector>

#define HANDLER_PROPERTIES_SCRIPT_FILENAME "scriptFileName"

namespace adushconst = Adu::Shell::Const;

EXTERN_C_BEGIN

/////////////////////////////////////////////////////////////////////////////
// BEGIN Shared Library Export Functions
//
// These are the function symbols that the device update agent will
// lookup and call.
//

/**
 * @brief Instantiates an Update Content Handler for 'microsoft/script:1' update type.
 * @return ContentHandler* The created instance.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel)
{
    ADUC_Logging_Init(logLevel, "script-handler");
    Log_Info("Instantiating an Update Content Handler for 'microsoft/script:1' update type.");
    try
    {
        return ScriptHandlerImpl::CreateContentHandler();
    }
    catch (const std::exception& e)
    {
        Log_Error("Unhandled std exception: %s", e.what());
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

// Forward declarations.
static ADUC_Result CancelApply(const char* logFolder);

/**
 * @brief Check whether to show additional debug logs.
 *
 * @return true if DU_AGENT_ENABLE_SCRIPT_HANDLER_EXTRA_DEBUG_LOGS is set
 */
static bool IsExtraDebugLogEnabled()
{
    return (!IsNullOrEmpty(getenv("DU_AGENT_ENABLE_SCRIPT_HANDLER_EXTRA_DEBUG_LOGS")));
}

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

static ADUC_Result Script_Handler_DownloadPrimaryScriptFile(ADUC_WorkflowHandle handle)
{
    ADUC_Result result = { ADUC_Result_Failure };
    const char* workflowId = nullptr;
    char* workFolder = nullptr;
    ADUC_FileEntity* entity = nullptr;
    int fileCount = workflow_get_update_files_count(handle);
    int createResult = 0;

    // Download the main script file.
    const char* scriptFileName =
        workflow_peek_update_manifest_handler_properties_string(handle, HANDLER_PROPERTIES_SCRIPT_FILENAME);
    if (IsNullOrEmpty(scriptFileName))
    {
        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_MISSING_SCRIPTFILENAME_PROPERTY;
        goto done;
    }

    if (fileCount <= 0)
    {
        result.ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_FAILURE_INVALID_FILE_COUNT;
        goto done;
    }

    if (!workflow_get_update_file_by_name(handle, scriptFileName, &entity))
    {
        result.ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_FAILURE_GET_PRIMARY_FILE_ENTITY;
        goto done;
    }

    workflowId = workflow_peek_id(handle);
    workFolder = workflow_get_workfolder(handle);

    createResult = ADUC_SystemUtils_MkSandboxDirRecursive(workFolder);
    if (createResult != 0)
    {
        Log_Error("Unable to create folder %s, error %d", workFolder, createResult);
        result = { ADUC_Result_Failure, ADUC_ERC_SCRIPT_HANDLER_CREATE_SANDBOX_FAILURE };
        goto done;
    }

    try
    {
        ExtensionManager_Download_Options downloadOptions = {
            .retryTimeout = DO_RETRY_TIMEOUT_DEFAULT,
        };

        result = ExtensionManager::Download(entity, handle, &downloadOptions, nullptr);
    }
    catch (...)
    {
        result.ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_DOWNLOAD_PRIMARY_FILE_FAILURE_UNKNOWNEXCEPTION;
    }

    workflow_free_file_entity(entity);
    entity = nullptr;

done:
    workflow_free_string(workFolder);
    return result;
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
ADUC_Result ScriptHandlerImpl::Download(const tagADUC_WorkflowData* workflowData)
{
    Log_Info("Script_Handler download task begin.");

    ADUC_WorkflowHandle workflowHandle = workflowData->WorkflowHandle;
    char* installedCriteria = nullptr;
    const char* workflowId = workflow_peek_id(workflowHandle);
    char* workFolder = workflow_get_workfolder(workflowData->WorkflowHandle);
    ADUC_FileEntity* entity = nullptr;
    int fileCount = workflow_get_update_files_count(workflowHandle);
    ADUC_Result result = Script_Handler_DownloadPrimaryScriptFile(workflowHandle);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
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

    for (int i = 0; i < fileCount; i++)
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
            ExtensionManager_Download_Options downloadOptions = {
                .retryTimeout = DO_RETRY_TIMEOUT_DEFAULT,
            };

            result = ExtensionManager::Download(entity, workflowHandle, &downloadOptions, nullptr);
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

    // Invoke primary script to download additional files, if required.
    result = PerformAction("--action-download", workflowData);

done:
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
 * @param resultFilePath A full path of the file containing serialized ADUC_Result value returned by the script.
 * @param[out] scriptFilePath A output script file path.
 * @param[out] args An output script arguments list.
 * @return ADUC_Result
 */
ADUC_Result ScriptHandlerImpl::PrepareScriptArguments(
    ADUC_WorkflowHandle workflowHandle,
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
    const char* scriptFileName = nullptr;

    char* installedCriteria = nullptr;
    const char* arguments = nullptr;
    const char* propNA = "n/a";

    bool success = false;
    if (workflowHandle == nullptr)
    {
        result.ExtendedResultCode = ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_NULL_WORKFLOW;
        goto done;
    }

    installedCriteria = workflow_get_installed_criteria(workflowHandle);

    // Parse components list. If the list is empty, nothing to download.
    selectedComponentsJson = workflow_peek_selected_components(workflowHandle);

    if (!IsNullOrEmpty(selectedComponentsJson))
    {
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
            result.ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_INVALID_COMPONENTS_DATA;
            goto done;
        }
    }

    // Prepare script file info.
    scriptFileName = workflow_peek_update_manifest_handler_properties_string(workflowHandle, "scriptFileName");
    if (IsNullOrEmpty(scriptFileName))
    {
        result.ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_MISSING_SCRIPTFILENAME_PROPERTY;
        workflow_set_result_details(workflowHandle, "Missing 'handlerProperties.scriptFileName' property");
        goto done;
    }

    filePath << workFolder.c_str() << "/" << scriptFileName;
    scriptFilePath = filePath.str();

    //
    // Prepare script arguments.
    //

    // Add customer specified arguments first.
    arguments = workflow_peek_update_manifest_handler_properties_string(workflowHandle, "arguments");
    if (arguments == nullptr)
    {
        Log_Info("Script workflow doesn't contain 'arguments' property. This is unusual, but not an error... ");
        arguments = "";
    }
    fileArgs = arguments;

    Log_Info("Parsing script arguments: %s", arguments);
    argumentList = ADUC::StringUtils::Split(fileArgs, ' ');
    for (int i = 0; i < argumentList.size(); i++)
    {
        const std::string argument = argumentList[i];
        if (!argument.empty())
        {
            if (argument == "--component-id-val")
            {
                const char* val = json_object_get_string(component, "id");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
                else
                {
                    args.emplace_back(propNA);
                }
            }
            else if (argument == "--component-name-val")
            {
                const char* val = json_object_get_string(component, "name");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
                else
                {
                    args.emplace_back(propNA);
                }
            }
            else if (argument == "--component-manufacturer-val")
            {
                const char* val = json_object_get_string(component, "manufacturer");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
                else
                {
                    args.emplace_back(propNA);
                }
            }
            else if (argument == "--component-model-val")
            {
                const char* val = json_object_get_string(component, "model");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
                else
                {
                    args.emplace_back(propNA);
                }
            }
            else if (argument == "--component-version-val")
            {
                const char* val = json_object_get_string(component, "version");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
                else
                {
                    args.emplace_back(propNA);
                }
            }
            else if (argument == "--component-group-val")
            {
                const char* val = json_object_get_string(component, "group");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
                else
                {
                    args.emplace_back(propNA);
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
                    else
                    {
                        args.emplace_back(propNA);
                    }
                    i++;
                }
                else
                {
                    args.emplace_back(propNA);
                }
            }
            else
            {
                args.emplace_back(argument);
            }
        }
    }

    // Default options.
    args.emplace_back("--work-folder");
    args.emplace_back(workFolder);

    args.emplace_back("--result-file");
    args.emplace_back(resultFilePath);

    args.emplace_back("--installed-criteria");

    if (installedCriteria != nullptr)
    {
        args.emplace_back(installedCriteria);
    }
    else
    {
        Log_Info("Installed criteria is null.");
        args.emplace_back("");
    }

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
ADUC_Result ScriptHandlerImpl::Install(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result = PerformAction("--action-install", workflowData);
    return result;
}

static ADUC_Result ScriptHandler_PerformAction(const std::string& action, const tagADUC_WorkflowData* workflowData)
{
    Log_Info("Action (%s) begin", action.c_str());
    ADUC_Result result = { ADUC_GeneralResult_Failure };

    std::string scriptFilePath;
    std::vector<std::string> args;
    std::string scriptOutput;
    int exitCode = 0;

    if (workflowData == nullptr || workflowData->WorkflowHandle == nullptr)
    {
        Log_Error("Workflow data or handler is null. This is unexpected!");
        result.ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_INSTALL_ERROR_NULL_WORKFLOW;
        return result;
    }

    char* workFolder = ADUC_WorkflowData_GetWorkFolder(workflowData);
    std::string scriptWorkfolder = workFolder;
    std::string scriptResultFile = scriptWorkfolder + "/action" + action + "_aduc_result.json";
    JSON_Value* actionResultValue = nullptr;
    JSON_Object* actionResultObject = nullptr;

    std::vector<std::string> aduShellArgs = { adushconst::update_type_opt,
                                              adushconst::update_type_microsoft_script,
                                              adushconst::update_action_opt,
                                              adushconst::update_action_execute };

    result = ScriptHandlerImpl::PrepareScriptArguments(
        workflowData->WorkflowHandle, scriptResultFile, scriptWorkfolder, scriptFilePath, args);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    // If any install-item reported that the update is already installed on the
    // selected component, we will skip the 'apply' phase, and then skip the
    // remaining install-item(s).
    // Also, don't continue if WorkflowHandle is NULL in the ADUInterface_Connected->HandleStartupWorkflowData flow.
    if (result.ResultCode == ADUC_Result_Install_Skipped_UpdateAlreadyInstalled
        || workflowData->WorkflowHandle == nullptr)
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

    if (IsExtraDebugLogEnabled())
    {
        std::stringstream ss;
        for (const auto& a : aduShellArgs)
        {
            ss << " " << a;
        }
        Log_Debug("##########\n# ADU-SHELL ARGS:\n##########\n %s", ss.str().c_str());
    }

    exitCode = ADUC_LaunchChildProcess(adushconst::adu_shell, aduShellArgs, scriptOutput);

    if (!scriptOutput.empty())
    {
        Log_Info(scriptOutput.c_str());
    }

    if (exitCode != 0)
    {
        int extendedCode = ADUC_ERC_SCRIPT_HANDLER_CHILD_PROCESS_FAILURE_EXITCODE(exitCode);
        Log_Error("Script failed (%s), extendedResultCode:0x%X (exitCode:%d)", action.c_str(), extendedCode, exitCode);
        result = { .ResultCode = ADUC_Result_Failure, .ExtendedResultCode = extendedCode };
        goto done;
    }

    // Parse result file.
    actionResultValue = json_parse_file(scriptResultFile.c_str());
    if (actionResultValue == nullptr)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_INSTALL_FAILURE_PARSE_RESULT_FILE };
        workflow_set_result_details(
            workflowData->WorkflowHandle, "Cannot parse the script result file '%s'.", scriptResultFile.c_str());
        goto done;
    }

    actionResultObject = json_object(actionResultValue);
    result.ResultCode = json_object_get_number(actionResultObject, "resultCode");
    result.ExtendedResultCode = json_object_get_number(actionResultObject, "extendedResultCode");
    workflow_set_result_details(
        workflowData->WorkflowHandle, json_object_get_string(actionResultObject, "resultDetails"));

    if (IsAducResultCodeFailure(result.ResultCode) && result.ExtendedResultCode == 0)
    {
        Log_Warn("Script result had non-actionable ExtendedResultCode of 0.");
        result.ExtendedResultCode = ADUC_ERC_SCRIPT_HANDLER_INSTALL_FAILURE_SCRIPT_RESULT_EXTENDEDRESULTCODE_ZERO;
    }

    Log_Info(
        "Action (%s) done - returning rc:%d, erc:0x%X, rd:%s",
        action.c_str(),
        result.ResultCode,
        result.ExtendedResultCode,
        workflow_peek_result_details(workflowData->WorkflowHandle));

done:
    workflow_set_result(workflowData->WorkflowHandle, result);

    // Note: the handler must request a system reboot or agent restart if required.
    switch (result.ResultCode)
    {
    case ADUC_Result_Install_RequiredImmediateReboot:
    case ADUC_Result_Apply_RequiredImmediateReboot:
        workflow_request_immediate_reboot(workflowData->WorkflowHandle);
        break;

    case ADUC_Result_Install_RequiredReboot:
    case ADUC_Result_Apply_RequiredReboot:
        workflow_request_reboot(workflowData->WorkflowHandle);
        break;

    case ADUC_Result_Install_RequiredImmediateAgentRestart:
    case ADUC_Result_Apply_RequiredImmediateAgentRestart:
        workflow_request_immediate_agent_restart(workflowData->WorkflowHandle);
        break;

    case ADUC_Result_Install_RequiredAgentRestart:
    case ADUC_Result_Apply_RequiredAgentRestart:
        workflow_request_agent_restart(workflowData->WorkflowHandle);
        break;
    }

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        workflow_set_state(workflowData->WorkflowHandle, ADUCITF_State_Failed);
    }

    json_value_free(actionResultValue);
    workflow_free_string(workFolder);
    return result;
}

ADUC_Result ScriptHandlerImpl::PerformAction(const std::string& action, const tagADUC_WorkflowData* workflowData)
{
    return ScriptHandler_PerformAction(action, workflowData);
}

static ADUC_Result DoCancel(const tagADUC_WorkflowData* workflowData)
{
    return ScriptHandler_PerformAction("--action-cancel", workflowData);
}

/**
 * @brief Performs 'Apply' task.
 * @return ADUC_Result The result return from script execution.
 */
ADUC_Result ScriptHandlerImpl::Apply(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result = PerformAction("--action-apply", workflowData);
    return result;
}

/**
 * @brief Performs 'Cancel' task.
 * @return ADUC_Result The result (always success)
 */
ADUC_Result ScriptHandlerImpl::Cancel(const tagADUC_WorkflowData* workflowData)
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
 * @brief Check whether the current device state satisfies specified workflow data.
 * @return ADUC_Result The result based on evaluating the workflow data.
 */
ADUC_Result ScriptHandlerImpl::IsInstalled(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result = Script_Handler_DownloadPrimaryScriptFile(workflowData->WorkflowHandle);
    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        result = PerformAction("--action-is-installed", workflowData);
    }
    return result;
}

/**
 * @brief Performs 'Backup' task.
 * @return ADUC_Result The result (always success)
 */
ADUC_Result ScriptHandlerImpl::Backup(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result = { ADUC_Result_Backup_Success_Unsupported };
    Log_Info("Script handler backup & restore is not supported. (no-op)");
    return result;
}

/**
 * @brief Performs 'Restore' task.
 * @return ADUC_Result The result (always success)
 */
ADUC_Result ScriptHandlerImpl::Restore(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result = { ADUC_Result_Restore_Success_Unsupported };
    Log_Info("Script handler backup & restore is not supported. (no-op)");
    return result;
}
