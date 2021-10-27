/**
 * @file components_update_handler.cpp
 * @brief Implementation of ContentHandler API for 'microsoft/components:1' update type.
 *
 * @copyright Copyright (c), Microsoft Corporation.
 */
#include "aduc/components_update_handler.hpp"

#include "aduc/adu_core_exports.h"
#include "aduc/extension_manager.hpp"
#include "aduc/installed_criteria_utils.hpp"
#include "aduc/logging.h"
#include "aduc/string_utils.hpp"
#include "aduc/system_utils.h"
#include "aduc/types/workflow.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_utils.h"
#include "parson.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string>

#include <dirent.h>

EXTERN_C_BEGIN

/**
 * @brief Instantiates an Update Content Handler for 'microsoft/components:1' update type.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel)
{
    ADUC_Logging_Init(logLevel, "components-handler");
    Log_Info("Instantiating an Update Content Handler for 'microsoft/components:1'");
    try
    {
        return ComponentsUpdateHandlerImpl::CreateContentHandler();
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

EXTERN_C_END

// Forward declarations.
static ADUC_Result CancelApply(const char* logFolder);

/**
 * @brief Creates a new ComponentsUpdateHandlerImpl object and casts to a ContentHandler.
 * Note that there is no way to create a ComponentsUpdateHandlerImpl directly.
 *
 * @return ContentHandler* ComponentsUpdateHandlerImpl object as a ContentHandler.
 */
ContentHandler* ComponentsUpdateHandlerImpl::CreateContentHandler()
{
    return new ComponentsUpdateHandlerImpl();
}

/**
 * @brief Performs a download task.
 *
 * @return ADUC_Result The result of this download task.
 *
 *     Following are the potential extended result codes:
 *
 *     ADUC_ERC_COMPONENTS_HANDLER_INVALID_COMPONENTS_DATA
 *     ADUC_ERC_COMPONENTS_HANDLER_CREATE_SANDBOX_FAILURE
 *     ADUC_ERC_COMPONENTS_HANDLER_GET_FILE_ENTITY_FAILURE
 *     ADUC_ERC_COMPONENTS_HANDLER_DOWNLOAD_FAILURE_UNKNOWN_EXCEPTION
 */
ADUC_Result ComponentsUpdateHandlerImpl::Download(const ADUC_WorkflowData* workflowData)
{
    Log_Info("Download phase begin.");
    ADUC_Result result = { ADUC_Result_Failure };
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    auto fileCount = static_cast<unsigned int>(workflow_get_update_files_count(handle));
    char* workflowId = workflow_get_id(handle);
    char* workingFolder = workflow_get_workfolder(handle);
    JSON_Object* rootObject = nullptr;
    JSON_Array* componentsArray = nullptr;
    JSON_Value* rootValue = nullptr;
    std::stringstream resultDetails;

    // Parse componenets list. If the list is empty, nothing to download.
    const char* selectedComponents = workflow_peek_selected_components(handle);

    if (selectedComponents == nullptr)
    {
        result = { ADUC_Result_Download_Skipped_NoMatchingComponents };
        goto done;
    }

    rootValue = json_parse_string(selectedComponents);
    if (rootValue == nullptr)
    {
        result.ExtendedResultCode = ADUC_ERC_COMPONENTS_HANDLER_INVALID_COMPONENTS_DATA;
        resultDetails << "Invalid component data.";
        goto done;
    }

    rootObject = json_value_get_object(rootValue);
    componentsArray = json_object_get_array(rootObject, "components");
    if (componentsArray == nullptr)
    {
        result.ExtendedResultCode = ADUC_ERC_COMPONENTS_HANDLER_INVALID_COMPONENTS_DATA;
        resultDetails << "Invalid component data.";
        goto done;
    }

    if (json_array_get_count(componentsArray) <= 0)
    {
        result.ResultCode = ADUC_Result_Download_Skipped_NoMatchingComponents;
        goto done;
    }

    for (int i = 0; i < fileCount; i++)
    {
        ADUC_FileEntity* entity = nullptr;

        int createResult = ADUC_SystemUtils_MkSandboxDirRecursive(workingFolder);
        if (createResult != 0)
        {
            Log_Error("Unable to create folder %s, error %d", workingFolder, createResult);
            resultDetails << "Unable to create working folder, error: " << createResult << ".";
            result = { ADUC_Result_Failure, ADUC_ERC_COMPONENTS_HANDLER_CREATE_SANDBOX_FAILURE };
            goto done;
        }

        if (!workflow_get_update_file(handle, i, &entity))
        {
            resultDetails << "Failed to get file#" << i << " entity.";
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_COMPONENTS_HANDLER_GET_FILE_ENTITY_FAILURE };

            goto done;
        }

        try
        {
            result = ExtensionManager::Download(entity, workflowId, workingFolder, DO_RETRY_TIMEOUT_DEFAULT, nullptr);
        }
        catch (...)
        {
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_COMPONENTS_HANDLER_DOWNLOAD_FAILURE_UNKNOWN_EXCEPTION };
        }

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            resultDetails << "Cannot download file#" << i << ".";
            goto done;
        }
    }

    result = { ADUC_Result_Download_Success };

done:
    if (rootValue != nullptr)
    {
        json_value_free(rootValue);
    }

    char* updateId = workflow_get_expected_update_id_string(handle);

    workflow_set_result(handle, result);
    workflow_set_result_details(handle, "%s", resultDetails.str().c_str());
    workflow_set_installed_update_id(handle, updateId);

    workflow_set_state(
        handle, IsAducResultCodeSuccess(result.ResultCode) ? ADUCITF_State_DownloadSucceeded : ADUCITF_State_Failed);

    workflow_free_string(updateId);
    workflow_free_string(workflowId);
    workflow_free_string(workingFolder);

    Log_Info("Download phase end.");
    return result;
}

// Return a json string containing a 'components' array with one component.
char* _createComponentSerializedString(JSON_Array* components, size_t index)
{
    JSON_Value* root = json_value_init_object();
    JSON_Value* componentClone = json_value_deep_copy(json_array_get_value(components, index));
    JSON_Array* array = json_array(json_value_init_array());
    json_array_append_value(array, componentClone);
    json_object_set_value(json_object(root), "components", json_array_get_wrapping_value(array));
    return json_serialize_to_string_pretty(root);
}

/**
 * @brief Perform 'install' and 'apply' actions on all 'installItems' entries.
 *
 * @param workflowData A Components Update workflow data object.
 * @param instructionsRoot An JSON_Value object contain an instruction file content.
 * @param components A JSON_Value object contain an array with a single selected component.
 * @return ADUC_Result
 */
ADUC_Result
_ProcessInstallItems(const ADUC_WorkflowData* workflowData, JSON_Value* instructionsRoot, JSON_Array* components)
{
    ADUC_Result result = { ADUC_Result_Failure };

    JSON_Array* installItems = json_object_get_array(json_object(instructionsRoot), "installItems");
    int installItemsCount = json_array_get_count(installItems);
    int componentInstanceCount = json_array_get_count(components);
    char* componentJson = nullptr;
    ADUC_WorkflowHandle itemHandle = nullptr;
    char* itemUpdateType = nullptr;
    ADUC_FileEntity* instructionFile = nullptr;

    for (int c = 0; c < componentInstanceCount; c++)
    {
        componentJson = _createComponentSerializedString(components, c);
        bool skipAnotherInstallItems = false;
        Log_Debug(
            "Processing intruction for component #%d, installing #%d item(s).\nComponent Json Data:%s\n",
            c,
            installItemsCount,
            componentJson);

        for (int i = 0; (i < installItemsCount) && !skipAnotherInstallItems; i++)
        {
            // Container workflow for itemHandle.
            ADUC_WorkflowData itemWorkflow;

            Log_Info("Installing item#%d for component#%d.", i, c);

            JSON_Value* item = json_value_deep_copy(json_array_get_value(installItems, i));
            result = workflow_create_from_instruction_value(workflowData->WorkflowHandle, item, &itemHandle);
            json_value_free(item);
            item = nullptr;

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                JSON_Object* component = json_array_get_object(components, c);
                workflow_set_result_details(
                    workflowData->WorkflowHandle,
                    R"({)"
                    R"("componentName":"%s", )"
                    R"(  "resultCode":"%d", )"
                    R"(  "extendedResultCode":"%d", )"
                    R"(  "resultDetails":"Failed to prepare workflow data for install-item #%d. (component #%d")." )"
                    R"(})",

                    json_object_get_string(component, "name"),
                    result.ResultCode,
                    result.ExtendedResultCode,
                    i,
                    c);

                Log_Error("Failed to prepare workflow data for install-item #%d (component #%d)", i, c);
                result = { .ResultCode = ADUC_Result_Failure,
                           .ExtendedResultCode = ADUC_ERC_COMPONENTS_HANDLER_INSTALL_FAILURE_INSTALLITEM_BAD_DATA };
                goto done;
            }

            if (!workflow_set_selected_components(itemHandle, componentJson))
            {
                result.ResultCode = ADUC_Result_Failure;
                result.ExtendedResultCode = ADUC_ERC_COMPONENTS_HANDLER_SET_SELECTED_COMPONENTS_FAILURE;
                goto done;
            }

            itemWorkflow.WorkflowHandle = itemHandle;

            itemUpdateType = workflow_get_update_type(itemHandle);
            if (itemUpdateType == nullptr)
            {
                Log_Error("Failed to get an Update Type of install-item #%d (component #%d)", i, c);
                result = { .ResultCode = ADUC_Result_Failure,
                           .ExtendedResultCode = ADUC_ERC_COMPONENTS_HANDLER_INSTALL_FAILURE_NO_UPDATE_TYPE };
                goto done;
            }

            Log_Info("Loading handler for install-item #%d update ('%s')", i, itemUpdateType);

            ContentHandler* contentHandler = nullptr;
            result = ContentHandlerFactory::LoadUpdateContentHandlerExtension(itemUpdateType, &contentHandler);

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                Log_Error("Cannot load Update Content Handler for install-item #%d (component #%d)", i, c);
                goto done;
            }

            // If this item is already installed, skip to the next one.
            try
            {
                result = contentHandler->IsInstalled(&itemWorkflow);
            }
            catch (...)
            {
                result = { .ResultCode = ADUC_Result_Failure,
                           .ExtendedResultCode =
                               ADUC_ERC_COMPONENTS_HANDLER_INSTALL_INSTALLITEM_ISINSTALLED_UNKNOWN_EXCEPTION };
            }

            if (IsAducResultCodeSuccess(result.ResultCode) && result.ResultCode == ADUC_Result_IsInstalled_Installed)
            {
                result.ResultCode = ADUC_Result_Install_Skipped_UpdateAlreadyInstalled;
                result.ExtendedResultCode = 0;
                // Skipping 'install' and 'apply'.
                goto instanceDone;
            }

            //
            // Perform 'install' action.
            //
            try
            {
                result = contentHandler->Install(&itemWorkflow);
            }
            catch (...)
            {
                result = { .ResultCode = ADUC_Result_Failure,
                           .ExtendedResultCode =
                               ADUC_ERC_COMPONENTS_HANDLER_INSTALL_INSTALLITEM_INSTALL_UNKNOWN_EXCEPTION };
                goto done;
            }

            switch (result.ResultCode)
            {
            case ADUC_Result_Install_RequiredImmediateReboot:
                workflow_request_immediate_reboot(workflowData->WorkflowHandle);
                // We can skip another instances.
                goto done;

            case ADUC_Result_Install_RequiredReboot:
                workflow_request_reboot(workflowData->WorkflowHandle);
                break;

            case ADUC_Result_Install_RequiredImmediateAgentRestart:
                workflow_request_immediate_agent_restart(workflowData->WorkflowHandle);
                goto done;

            case ADUC_Result_Install_RequiredAgentRestart:
                workflow_request_agent_restart(workflowData->WorkflowHandle);
                break;

            // If any install-item reported that the update is already installed on the
            // selected component, we will skip the 'apply' phase, and then skip the
            // remaining install-item(s).
            case ADUC_Result_Install_Skipped_UpdateAlreadyInstalled:
            case ADUC_Result_Install_Skipped_NoMatchingComponents:
                goto instanceDone;
            }

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                // Propagate item's resultDails to leaf.
                workflow_set_result_details(
                    workflowData->WorkflowHandle, workflow_peek_result_details(itemWorkflow.WorkflowHandle));

                goto done;
            }

            //
            // Perform 'apply' action.
            //
            try
            {
                result = contentHandler->Apply(&itemWorkflow);
            }
            catch (...)
            {
                result = { .ResultCode = ADUC_Result_Failure,
                           .ExtendedResultCode =
                               ADUC_ERC_COMPONENTS_HANDLER_APPLY_INSTALLITEM_INSTALL_UNKNOWN_EXCEPTION };
                goto done;
            }

            switch (result.ResultCode)
            {
            case ADUC_Result_Apply_RequiredImmediateReboot:
                workflow_request_immediate_reboot(workflowData->WorkflowHandle);
                // Translate into components-level result.
                result.ResultCode = ADUC_Result_Install_RequiredImmediateReboot;
                // We can skip another instances.
                goto done;

            case ADUC_Result_Apply_RequiredReboot:
                workflow_request_reboot(workflowData->WorkflowHandle);
                // Translate into 'install' result.
                result.ResultCode = ADUC_Result_Install_RequiredReboot;
                break;

            case ADUC_Result_Apply_RequiredImmediateAgentRestart:
                workflow_request_immediate_agent_restart(workflowData->WorkflowHandle);
                // Translate into components-level result.
                result.ResultCode = ADUC_Result_Install_RequiredImmediateAgentRestart;
                goto done;

            case ADUC_Result_Apply_RequiredAgentRestart:
                workflow_request_agent_restart(workflowData->WorkflowHandle);
                // Translate into components-level result.
                result.ResultCode = ADUC_Result_Install_RequiredAgentRestart;
                break;
            }

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                // Propagate item's resultDails to leaf.
                workflow_set_result_details(
                    workflowData->WorkflowHandle, workflow_peek_result_details(itemWorkflow.WorkflowHandle));
            }

        instanceDone:
            workflow_free_string(itemUpdateType);
            itemUpdateType = nullptr;
            workflow_free(itemHandle);
            itemHandle = nullptr;

            // TODO(Nox) 34241019: [Bundle Update] Components update handler should support 'continueOnError' install policy.
            // If the result indicates a failure, determine whether to proceed with next item,
            // based on 'instance install policy'.
            if (IsAducResultCodeFailure(result.ResultCode))
            {
                goto componentDone;
            }
        } // installItems loop

    componentDone:
        // Optional: Persist Instance-Level install result.

        json_free_serialized_string(componentJson);
        componentJson = nullptr;

        // TODO(Nox) 34241019: [Bundle Update] Components update handler should support 'continueOnError' install policy.
        // If the component failed, determine whether to proceed with the next component
        // based on 'instance install policy'
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }
    }

done:
    // NOTE: Do not free child workflow here, so that it can be reused in the next phase.
    // Only free child handle when the workflow is done.
    //
    // Alternatively, we can persist child workflow state, to free up some memery, and
    // load the state when needed in the next phase
    workflow_set_result(workflowData->WorkflowHandle, result);

    workflow_set_state(
        workflowData->WorkflowHandle,
        IsAducResultCodeSuccess(result.ResultCode) ? ADUCITF_State_InstallSucceeded : ADUCITF_State_Failed);

    json_free_serialized_string(componentJson);
    workflow_free_string(itemUpdateType);
    workflow_free(itemHandle);

    return result;
}

//
// Determines whether the specified fileName is an 'Instruction' file.
// The 'Instruction' file name must ends with 'instructions.json' (case-sensitive).
//
bool _isInstructionFile(const char* fileName)
{
    if (fileName == nullptr)
    {
        return false;
    }
    std::string name = fileName;
    static std::string ext = "instructions.json";
    if (name.size() < ext.size())
    {
        return false;
    }
    return std::equal(ext.rbegin(), ext.rend(), name.rbegin());
}

ADUC_Result _ProcessInstruction(const ADUC_WorkflowData* workflowData, JSON_Array* componentsArray)
{
    ADUC_Result result = { ADUC_Result_Failure };
    ADUC_FileEntity* instructionFile = nullptr;
    JSON_Value* rootValue = nullptr;
    std::stringstream path;
    int fileCount = workflow_get_update_files_count(workflowData->WorkflowHandle);
    char* workFolder = workflow_get_workfolder(workflowData->WorkflowHandle);

    for (int i = 0; i < fileCount; i++)
    {
        if (!workflow_get_update_file(workflowData->WorkflowHandle, i, &instructionFile))
        {
            Log_Error("Failed to read file entity #%d", i);
            result.ExtendedResultCode = ADUC_ERC_SWUPDATE_HANDLER_PREPARE_FAILURE_BAD_FILE_ENTITY;
            goto done;
        }

        if (_isInstructionFile(instructionFile->TargetFilename))
        {
            break;
        }

        workflow_free_file_entity(instructionFile);
    }

    if (instructionFile == nullptr)
    {
        result.ExtendedResultCode = ADUC_ERC_COMPONENTS_HANDLER_INSTALL_FAILURE_NO_INSTRUCTION_FILE;
        goto done;
    }

    path << workFolder << "/" << instructionFile->TargetFilename;

    Log_Debug("Processing instruction file '%s'", path.str().c_str());

    rootValue = json_parse_file(path.str().c_str());

    if (rootValue == nullptr)
    {
        Log_Error("Can not parse an instruction file '%s'", path.str().c_str());
        result.ExtendedResultCode = ADUC_ERC_COMPONENTS_HANDLER_INSTALL_FAILURE_INSTRUCTION_PARSE_FAILURE;
        goto done;
    }

    // Iterate though 'installItems' array then perform Install & Apply actions using specified component context.
    result = _ProcessInstallItems(workflowData, rootValue, componentsArray);

done:
    workflow_free_file_entity(instructionFile);
    workflow_free_string(workFolder);
    json_value_free(rootValue);

    return result;
}

/**
 * @brief Performs 'Install' task.
 * @return ADUC_Result The result (alw
 */
ADUC_Result ComponentsUpdateHandlerImpl::Install(const ADUC_WorkflowData* workflowData)
{
    Log_Info("Install phase begin.");
    ADUC_Result result = { ADUC_Result_Failure };

    JSON_Value* rootValue = nullptr;
    JSON_Object* rootObject = nullptr;
    JSON_Array* componentsArray = nullptr;
    int componentCount = 0;
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    char* installedCriteria = workflow_get_installed_criteria(handle);

    // Parse componenets list. If the list is empty, nothing to install.
    const char* selectedComponent = workflow_peek_selected_components(handle);
    if (selectedComponent == nullptr)
    {
        result = { ADUC_Result_Install_Skipped_NoMatchingComponents };
        goto done;
    }

    rootValue = json_parse_string(selectedComponent);
    if (rootValue == nullptr)
    {
        result.ExtendedResultCode = ADUC_ERC_COMPONENTS_HANDLER_INVALID_COMPONENTS_DATA;
        goto done;
    }

    rootObject = json_object(rootValue);
    componentsArray = json_object_get_array(rootObject, "components");
    if (componentsArray == nullptr)
    {
        result.ExtendedResultCode = ADUC_ERC_COMPONENTS_HANDLER_INVALID_COMPONENTS_DATA;
        goto done;
    }

    componentCount = json_array_get_count(componentsArray);
    if (componentCount <= 0)
    {
        Log_Info("No components selected. Skipping install phase.");
        result.ResultCode = ADUC_Result_Download_Skipped_NoMatchingComponents;
        goto done;
    }

    result = _ProcessInstruction(workflowData, componentsArray);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        // TODO(Nox): 34317519: [Bundle Update] Support 'continue' on error Instance-level install policy
        // For v1, always abort the installation.
        goto done;
    }

    // All instances are up to date. Mark this Components Update as 'installed'.
    if (!PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria))
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_BUNDLE_HANDLER_INSTALLCRITERIA_PERSIST_FAILURE };
        goto done;
    }

done:
    if (rootValue != nullptr)
    {
        json_value_free(rootValue);
    }

    workflow_set_result(handle, result);
    workflow_set_state(
        handle, IsAducResultCodeSuccess(result.ResultCode) ? ADUCITF_State_InstallSucceeded : ADUCITF_State_Failed);

    workflow_free_string(installedCriteria);
    Log_Info("Install phase end.");
    return result;
}

/**
 * @brief Performs 'Apply' task.
 * @return ADUC_Result The result (always success)
 */
ADUC_Result ComponentsUpdateHandlerImpl::Apply(const ADUC_WorkflowData* workflowData)
{
    Log_Info("Apply called");
    char* updateId = workflow_get_expected_update_id_string(workflowData->WorkflowHandle);
    ADUC_Result result = { ADUC_Result_Apply_Success };

    workflow_set_result(workflowData->WorkflowHandle, result);
    workflow_set_installed_update_id(workflowData->WorkflowHandle, updateId);
    workflow_set_state(workflowData->WorkflowHandle, ADUCITF_State_Idle);

    workflow_free_string(updateId);
    return result;
}

/**
 * @brief Performs 'Cancel' task.
 * @return ADUC_Result The result (always success)
 */
ADUC_Result ComponentsUpdateHandlerImpl::Cancel(const ADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);
    Log_Info("Cancel called - returning success");
    return ADUC_Result{ ADUC_Result_Cancel_Success };
}

/**
 * @brief Mock implementation of IsInstalled check.
 * @return ADUC_Result The result based on evaluating the installed criteria.
 */
ADUC_Result ComponentsUpdateHandlerImpl::IsInstalled(const ADUC_WorkflowData* workflowData)
{
    char* installedCriteria = ADUC_WorkflowData_GetInstalledCriteria(workflowData);
    ADUC_Result result = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria);
    workflow_free_string(installedCriteria);
    return result;
}