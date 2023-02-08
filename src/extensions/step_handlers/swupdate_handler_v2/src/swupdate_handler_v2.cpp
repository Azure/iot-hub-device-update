/**
 * @file swupdate_handler_v2.cpp
 * @brief Implementation of ContentHandler API for swupdate wrapper script.
 *
 *     The wrapper script must be delivered to a device as part of the update payloads.
 *     Script options and arguments can be specified in:
 *         - swupdate-handler-config.json
 *         - Update manifest's instructions-step's handlerProperties['arguments']
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/swupdate_handler_v2.hpp"

#include "aduc/adu_core_exports.h"
#include "aduc/extension_manager.hpp"
#include "aduc/logging.h"
#include "aduc/parser_utils.h" // ADUC_FileEntity_Uninit
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
#include <memory>
#include <string>

#include <parson.h>

#define HANDLER_PROPERTIES_SCRIPT_FILENAME "scriptFileName"
#define HANDLER_PROPERTIES_SWU_FILENAME "swuFileName"

namespace adushconst = Adu::Shell::Const;

/* external linkage */
extern ExtensionManager_Download_Options Default_ExtensionManager_Download_Options;

struct JSONValueDeleter
{
    void operator()(JSON_Value* value)
    {
        json_value_free(value);
    }
};

using AutoFreeJsonValue_t = std::unique_ptr<JSON_Value, JSONValueDeleter>;

/**
 * @brief Destructor for the SWUpdate Handler Impl class.
 */
SWUpdateHandlerImpl::~SWUpdateHandlerImpl() // override
{
    ADUC_Logging_Uninit();
}

/**
 * @brief Downloads a main script file into a sandbox folder.
 *        The 'handlerProperties["scriptFileName"]' contains the main script file name.
 *
 * @param handle A workflow object.
 * @return ADUC_Result
 */
static ADUC_Result SWUpdate_Handler_DownloadScriptFile(ADUC_WorkflowHandle handle)
{
    ADUC_Result result = { ADUC_Result_Failure };
    char* workFolder = nullptr;
    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    int fileCount = workflow_get_update_files_count(handle);
    int createResult = 0;
    // Download the main script file.
    const char* scriptFileName =
        workflow_peek_update_manifest_handler_properties_string(handle, HANDLER_PROPERTIES_SCRIPT_FILENAME);
    if (scriptFileName == nullptr)
    {
        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_MISSING_SCRIPT_FILE_NAME;
        goto done;
    }

    if (fileCount <= 1)
    {
        result.ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_WRONG_FILECOUNT;
        goto done;
    }

    if (!workflow_get_update_file_by_name(handle, scriptFileName, &entity))
    {
        result.ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_GET_SCRIPT_FILE_ENTITY;
        goto done;
    }

    workFolder = workflow_get_workfolder(handle);

    createResult = ADUC_SystemUtils_MkSandboxDirRecursive(workFolder);
    if (createResult != 0)
    {
        Log_Error("Unable to create folder %s, error %d", workFolder, createResult);
        result = { ADUC_Result_Failure, ADUC_ERC_SWUPDATE_HANDLER_CREATE_SANDBOX_FAILURE };
        goto done;
    }

    try
    {
        result = ExtensionManager::Download(&entity, handle, &Default_ExtensionManager_Download_Options, nullptr);
    }
    catch (...)
    {
        result.ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_PRIMARY_FILE_FAILURE_UNKNOWNEXCEPTION;
    }

done:
    ADUC_FileEntity_Uninit(&entity);
    workflow_free_string(workFolder);
    return result;
}

/**
 * @brief Perform a workflow action. If @p prepareArgsOnly is true, only prepare data, but not actually
 *        perform any action.
 *
 * @param action Indicate an action to perform. This can be '--action-download', '--action-install',
 *               '--action-apply', "--action-cancel", and "--action-is-installed".
 * @param workflowData An object containing workflow data.
 * @param prepareArgsOnly  Boolean indicates whether to prepare action data only.
 * @param[out] scriptFilePath Output string contains a script to be run.
 * @param[in] args List of options and arguments.
 * @param[out] commandLineArgs An output command-line arguments.
 * @param[out] scriptOutput If @p prepareArgsOnly is false, this will contains the action output string.
 * @return ADUC_Result
 */
ADUC_Result SWUpdateHandler_PerformAction(
    const std::string& action,
    const tagADUC_WorkflowData* workflowData,
    bool prepareArgsOnly,
    std::string& scriptFilePath,
    std::vector<std::string>& args,
    std::vector<std::string>& commandLineArgs,
    std::string& scriptOutput)
{
    Log_Info("Action (%s) begin", action.c_str());
    ADUC_Result result = { ADUC_GeneralResult_Failure };

    int exitCode = 0;
    commandLineArgs.clear();

    if (workflowData == nullptr || workflowData->WorkflowHandle == nullptr)
    {
        result.ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_INSTALL_ERROR_NULL_WORKFLOW;
        return result;
    }

    char* workFolder = ADUC_WorkflowData_GetWorkFolder(workflowData);
    std::string scriptWorkfolder = workFolder;
    std::string scriptResultFile = scriptWorkfolder + "/" + "aduc_result.json";
    JSON_Value* actionResultValue = nullptr;

    std::vector<std::string> aduShellArgs = { adushconst::update_type_opt,
                                              adushconst::update_type_microsoft_script,
                                              adushconst::update_action_opt,
                                              adushconst::update_action_execute };

    std::stringstream ss;

    result = SWUpdateHandlerImpl::PrepareCommandArguments(
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
    commandLineArgs.emplace_back(scriptFilePath);

    aduShellArgs.emplace_back(adushconst::target_options_opt);
    aduShellArgs.emplace_back(action.c_str());
    commandLineArgs.emplace_back(action.c_str());

    for (const auto& a : args)
    {
        aduShellArgs.emplace_back(adushconst::target_options_opt);
        aduShellArgs.emplace_back(a);
        commandLineArgs.emplace_back(a);
    }

    if (prepareArgsOnly)
    {
        for (const auto& a : aduShellArgs)
        {
            if (a[0] != '-')
            {
                ss << " \"" << a << "\"";
            }
            else
            {
                ss << " " << a;
            }
        }
        scriptOutput = ss.str();

        Log_Debug("Prepare Only! adu-shell Command:\n\n %s", scriptOutput.c_str());
        result = { .ResultCode = ADUC_Result_Success, .ExtendedResultCode = 0 };
        goto done;
    }

    exitCode = ADUC_LaunchChildProcess(adushconst::adu_shell, aduShellArgs, scriptOutput);
    if (exitCode != 0)
    {
        int extendedCode = ADUC_ERC_SWUPDATE_HANDLER_CHILD_FAILURE_PROCESS_EXITCODE(exitCode);
        Log_Error("Install failed, extendedResultCode:0x%X (exitCode:%d)", extendedCode, exitCode);
        result = { .ResultCode = ADUC_Result_Failure, .ExtendedResultCode = extendedCode };
    }

    if (!scriptOutput.empty())
    {
        Log_Info(scriptOutput.c_str());
    }

    // Parse result file.
    actionResultValue = json_parse_file(scriptResultFile.c_str());

    if (actionResultValue == nullptr)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_INSTALL_FAILURE_PARSE_RESULT_FILE };
        workflow_set_result_details(
            workflowData->WorkflowHandle,
            "The install script doesn't create a result file '%s'.",
            scriptResultFile.c_str());
        goto done;
    }
    else
    {
        JSON_Object* actionResultObject = json_object(actionResultValue);
        result.ResultCode = json_object_get_number(actionResultObject, "resultCode");
        result.ExtendedResultCode = json_object_get_number(actionResultObject, "extendedResultCode");
        const char* details = json_object_get_string(actionResultObject, "resultDetails");
        workflow_set_result_details(workflowData->WorkflowHandle, details);
    }

    Log_Info(
        "Action (%s) done - returning rc:%d, erc:0x%X, rd:%s",
        action.c_str(),
        result.ResultCode,
        result.ExtendedResultCode,
        workflow_peek_result_details(workflowData->WorkflowHandle));

done:
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        workflow_set_result(workflowData->WorkflowHandle, result);
        workflow_set_state(workflowData->WorkflowHandle, ADUCITF_State_Failed);
    }

    json_value_free(actionResultValue);
    workflow_free_string(workFolder);
    return result;
}

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
    Log_Info("SWUpdate handler v2 download task begin.");

    ADUC_WorkflowHandle workflowHandle = workflowData->WorkflowHandle;
    char* installedCriteria = nullptr;
    char* workFolder = workflow_get_workfolder(workflowData->WorkflowHandle);
    ADUC_FileEntity fileEntity;
    memset(&fileEntity, 0, sizeof(fileEntity));
    int fileCount = workflow_get_update_files_count(workflowHandle);
    ADUC_Result result = SWUpdate_Handler_DownloadScriptFile(workflowHandle);

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

        if (!workflow_get_update_file(workflowHandle, i, &fileEntity))
        {
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_FAILURE_GET_PAYLOAD_FILE_ENTITY };
            goto done;
        }

        try
        {
            result = ExtensionManager::Download(
                &fileEntity, workflowHandle, &Default_ExtensionManager_Download_Options, nullptr);
        }
        catch (...)
        {
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode =
                           ADUC_ERC_SWUPDATE_HANDLER_DOWNLOAD_PAYLOAD_FILE_FAILURE_UNKNOWNEXCEPTION };
        }

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            Log_Error("Cannot download payload file#%d. (0x%X)", i, result.ExtendedResultCode);
            goto done;
        }
    }

    // Invoke primary script to download additional files, if required.
    result = PerformAction("--action-download", workflowData);

done:
    workflow_free_string(workFolder);
    ADUC_FileEntity_Uninit(&fileEntity);
    workflow_free_string(installedCriteria);
    Log_Info("SWUpdate_Handler download task end.");
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
    ADUC_Result result = PerformAction("--action-install", workflowData);

    // Note: the handler must request a system reboot or agent restart if required.
    switch (result.ResultCode)
    {
    case ADUC_Result_Install_RequiredImmediateReboot:
        workflow_request_immediate_reboot(workflowData->WorkflowHandle);
        break;

    case ADUC_Result_Install_RequiredReboot:
        workflow_request_reboot(workflowData->WorkflowHandle);
        break;

    case ADUC_Result_Install_RequiredImmediateAgentRestart:
        workflow_request_immediate_agent_restart(workflowData->WorkflowHandle);
        break;

    case ADUC_Result_Install_RequiredAgentRestart:
        workflow_request_agent_restart(workflowData->WorkflowHandle);
        break;
    }

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
    char* workFolder = workflow_get_workfolder(workflowData->WorkflowHandle);
    Log_Info("Applying data from %s", workFolder);

    result = PerformAction("--action-apply", workflowData);

    // Cancellation requested after applied?
    if (workflow_get_operation_cancel_requested(workflowData->WorkflowHandle))
    {
        result = Cancel(workflowData);
    }

    // Note: the handler must request a system reboot or agent restart if required.
    switch (result.ResultCode)
    {
    case ADUC_Result_Apply_RequiredImmediateReboot:
        workflow_request_immediate_reboot(workflowData->WorkflowHandle);
        break;

    case ADUC_Result_Apply_RequiredReboot:
        workflow_request_reboot(workflowData->WorkflowHandle);
        break;

    case ADUC_Result_Apply_RequiredImmediateAgentRestart:
        workflow_request_immediate_agent_restart(workflowData->WorkflowHandle);
        break;

    case ADUC_Result_Apply_RequiredAgentRestart:
        workflow_request_agent_restart(workflowData->WorkflowHandle);
        break;
    }

done:
    workflow_free_string(workFolder);
    return result;
}

/**
 * @brief Cancel implementation for swupdate.
 * We don't have many hooks into swupdate to cancel an ongoing install.
 * For A/B update pattern, we can cancel apply by reverting the bootloader flag to boot into the original partition.
 * We defer the cancellation decision to the device builder by call into the swupdate wrapper script to cancel apply.
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
 * @brief Check whether the current device state match all desired state in workflow data.
 *
 * Key concepts:
 *   - Decouple the business logics from device-specific configurations.
 *   - Device builder defines the how to evaluate wither the current step can be considered 'completed'.
 *     Note that the term 'IsInstalled' was carried over from the original design where the agent would
 *     ask the handler that "Is an 'update' is currently installed on the device.".
 *   -
 * @return ADUC_Result The result based on evaluating the workflow data.
 */
ADUC_Result SWUpdateHandlerImpl::IsInstalled(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result = SWUpdate_Handler_DownloadScriptFile(workflowData->WorkflowHandle);
    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        result = PerformAction("--action-is-installed", workflowData);
    }
    return result;
}

/**
 * @brief Reads handler configuration from @p configFile
 *
 * @param values An output dictionary containing handler configurations.
 * @return ADUC_Result
 */
ADUC_Result
SWUpdateHandlerImpl::ReadConfig(const std::string& configFile, std::unordered_map<std::string, std::string>& values)
{
    // Fill in the output map.
    AutoFreeJsonValue_t rootValue{ json_parse_file(configFile.c_str()) };
    if (!rootValue)
    {
        return ADUC_Result{ .ResultCode = ADUC_Result_Failure,
                            .ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_BAD_SWUPDATE_CONFIG_FILE };
    }

    JSON_Object* rootObject = json_value_get_object(rootValue.get());

    for (size_t i = 0; i < json_object_get_count(rootObject); i++)
    {
        const char* name = json_object_get_name(rootObject, i);
        const char* val = json_value_get_string(json_object_get_value_at(rootObject, i));
        values[name] = val;
    }

    return ADUC_Result{ .ResultCode = ADUC_Result_Success, .ExtendedResultCode = 0 };
}

/**
 * @brief A helper function that return a command file path, and arguments list.
 *
 * @param workflowHandle A workflow data containing update information and selected component.
 * @param resultFilePath A full path of the file containing serialized ADUC_Result value returned by the command.
 * @param[out] commandFilePath A output command file path.
 * @param[out] args An output command arguments list.
 * @return ADUC_Result
 */
ADUC_Result SWUpdateHandlerImpl::PrepareCommandArguments(
    ADUC_WorkflowHandle workflowHandle,
    std::string resultFilePath,
    std::string workFolder,
    std::string& commandFilePath,
    std::vector<std::string>& args)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure };
    bool isComponentsAware = false;
    ADUC_FileEntity* scriptFileEntity = nullptr;

    const char* selectedComponentsJson = nullptr;
    JSON_Value* selectedComponentsValue = nullptr;
    JSON_Object* selectedComponentsObject = nullptr;
    JSON_Array* componentsArray = nullptr;
    int componentCount = 0;
    JSON_Object* component = nullptr;

    std::string fileArgs;
    std::vector<std::string> argumentList;
    std::unordered_map<std::string, std::string> swupdateConfigs;

    std::stringstream filePath;
    std::stringstream swuFilePath;
    const char* scriptFileName = nullptr;
    const char* swuFileName = nullptr;

    char* installedCriteria = nullptr;
    const char* arguments = nullptr;

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
        isComponentsAware = true;
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
            result.ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_TOO_MANY_COMPONENTS;
        }

        component = json_array_get_object(componentsArray, 0);
        if (component == nullptr)
        {
            result.ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_INVALID_COMPONENTS_DATA;
            goto done;
        }
    }

    // Prepare main script file info.
    scriptFileName =
        workflow_peek_update_manifest_handler_properties_string(workflowHandle, HANDLER_PROPERTIES_SCRIPT_FILENAME);
    if (IsNullOrEmpty(scriptFileName))
    {
        result.ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_MISSING_SCRIPT_FILE_NAME;
        workflow_set_result_details(workflowHandle, "Missing 'handlerProperties.scriptFileName' property");
        goto done;
    }

    filePath << workFolder.c_str() << "/" << scriptFileName;
    commandFilePath = filePath.str();

    // swu file.
    swuFileName =
        workflow_peek_update_manifest_handler_properties_string(workflowHandle, HANDLER_PROPERTIES_SWU_FILENAME);
    if (IsNullOrEmpty(swuFileName))
    {
        result.ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_MISSING_SWU_FILE_NAME;
        workflow_set_result_details(workflowHandle, "Missing 'handlerProperties.swuFileName' property");
        goto done;
    }

    swuFilePath << workFolder.c_str() << "/" << swuFileName;

    args.emplace_back("--swu-file");
    args.emplace_back(swuFilePath.str());

    //
    // Prepare command-line arguments.
    //

    // Read arguments from swupdate_handler_config.json
    result = ReadConfig(ADUC_SWUPDATE_HANDLER_CONF_FILE_PATH, swupdateConfigs);
    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        for (const auto& a : swupdateConfigs)
        {
            args.emplace_back(a.first);
            args.emplace_back(a.second);
        }
    }

    // Add customer specified arguments first.
    arguments = workflow_peek_update_manifest_handler_properties_string(workflowHandle, "arguments");
    if (arguments == nullptr)
    {
        arguments = "";
    }
    fileArgs = arguments;

    Log_Info("Parsing handlerProperties.arguments: %s", arguments);
    argumentList = ADUC::StringUtils::Split(fileArgs, ' ');
    for (int i = 0; i < argumentList.size(); i++)
    {
        const std::string argument = argumentList[i];
        if (!argument.empty())
        {
            if (argument == "--component-id-val" || argument == "${du_component_id}")
            {
                const char* val = json_object_get_string(component, "id");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
                else
                {
                    args.emplace_back("n/a");
                }
            }
            else if (argument == "--component-name-val" || argument == "${du_component_name}")
            {
                const char* val = json_object_get_string(component, "name");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
                else
                {
                    args.emplace_back("n/a");
                }
            }
            else if (argument == "--component-manufacturer-val" || argument == "${du_component_manufacturer}")
            {
                const char* val = json_object_get_string(component, "manufacturer");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
                else
                {
                    args.emplace_back("n/a");
                }
            }
            else if (argument == "--component-model-val" || argument == "${du_component_model}")
            {
                const char* val = json_object_get_string(component, "model");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
                else
                {
                    args.emplace_back("n/a");
                }
            }
            else if (argument == "--component-version-val" || argument == "${du_component_version}")
            {
                const char* val = json_object_get_string(component, "version");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
                else
                {
                    args.emplace_back("n/a");
                }
            }
            else if (argument == "--component-group-val" || argument == "${du_component_group}")
            {
                const char* val = json_object_get_string(component, "group");
                if (val != nullptr)
                {
                    args.emplace_back(val);
                }
                else
                {
                    args.emplace_back("n/a");
                }
            }
            else if (argument == "--component-prop-val" || argument == "${du_component_prop}")
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
                else
                {
                    args.emplace_back("n/a");
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

    if (IsNullOrEmpty(installedCriteria))
    {
        Log_Info("--installed-criteria is not specified");
    }
    else
    {
        args.emplace_back("--installed-criteria");
        args.emplace_back(installedCriteria);
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

ADUC_Result SWUpdateHandlerImpl::PerformAction(const std::string& action, const tagADUC_WorkflowData* workflowData)
{
    std::string scriptFilePath;
    std::vector<std::string> args;
    std::vector<std::string> commandLineArgs;
    std::string scriptOutput;

    return SWUpdateHandler_PerformAction(
        action, workflowData, false, scriptFilePath, args, commandLineArgs, scriptOutput);
}

/**
 * @brief Helper function to perform cancel when we are doing an apply.
 *
 * @return ADUC_Result The result of the cancel.
 */
ADUC_Result SWUpdateHandlerImpl::CancelApply(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result;

    result = PerformAction("--action-cancel", workflowData);
    if (result.ResultCode != ADUC_Result_Cancel_Success)
    {
        Log_Error("Failed to cancel Apply, extendedResultCode = (0x%X)", result.ExtendedResultCode);
        return result;
    }
    else
    {
        Log_Info("Apply was cancelled");
        return ADUC_Result{ ADUC_Result_Failure_Cancelled };
    }
}

/**
 * @brief Backup implementation for swupdate V2.
 * Calls into the swupdate wrapper script to perform backup.
 * For swupdate, no operation is required.
 *
 * @return ADUC_Result The result of the backup.
 * It will always return ADUC_Result_Backup_Success.
 */
ADUC_Result SWUpdateHandlerImpl::Backup(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result = { ADUC_Result_Backup_Success };
    Log_Info("Swupdate doesn't require a specific operation to backup. (no-op) ");
    return result;
}

/**
 * @brief Restore implementation for swupdate V2.
 * Calls into the swupdate wrapper script to perform restore.
 * Will flip bootloader flag to boot into the previous partition for A/B update.
 *
 * @return ADUC_Result The result of the restore.
 */
ADUC_Result SWUpdateHandlerImpl::Restore(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result = { ADUC_Result_Restore_Success };
    ADUC_Result cancel_result = CancelApply(workflowData);
    if (cancel_result.ResultCode != ADUC_Result_Failure_Cancelled)
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_UPPERLEVEL_WORKFLOW_FAILED_RESTORE_FAILED };
    }
    return result;
}