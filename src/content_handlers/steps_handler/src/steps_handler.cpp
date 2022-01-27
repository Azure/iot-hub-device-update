/**
 * @file steps_handler.cpp
 * @brief Implementation of ContentHandler API for the MSOE (Multi Steps Ordered Execution) Steps.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/steps_handler.hpp"

#include "aduc/component_enumerator_extension.hpp"
#include "aduc/extension_manager.hpp"
#include "aduc/extension_utils.h"
#include "aduc/logging.h"
#include "aduc/string_utils.hpp"
#include "aduc/system_utils.h"
#include "aduc/workflow_utils.h"

#include "parson.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include <azure_c_shared_utility/crt_abstractions.h> // mallocAndStrcpy
#include <azure_c_shared_utility/strings.h> // STRING_*

#include <dirent.h>

// Note: this requires ${CMAKE_DL_LIBS}
#include <dlfcn.h>

#define DEFAULT_REF_STEP_HANDLER "microsoft/steps:1"

EXTERN_C_BEGIN

/**
 * @brief Instantiates a special handler that performs multi-steps ordered execution.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel)
{
    ADUC_Logging_Init(logLevel, "steps-handler");
    Log_Info("Instantiating an Update Content Handler for MSOE");
    try
    {
        return StepsHandlerImpl::CreateContentHandler();
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

EXTERN_C_END

/**
 * @brief Destructor for the Steps Handler Impl class.
 */
StepsHandlerImpl::~StepsHandlerImpl() // override
{
    ADUC_Logging_Uninit();
}

/**
 * @brief Make sure that all step workflows are created.
 *
 * @param handle A workflow data object handle.
 * @return ADUC_Result
 */
ADUC_Result EnsureStepsWorkflowsCreated(const ADUC_WorkflowHandle handle)
{
    ADUC_Result result{ ADUC_Result_Failure };
    ADUC_WorkflowHandle childHandle = nullptr;

    auto stepCount = static_cast<unsigned int>(workflow_get_instructions_steps_count(handle));
    const char* workflowId = workflow_peek_id(handle);
    char* workFolder = workflow_get_workfolder(handle);
    unsigned int childWorkflowCount = workflow_get_children_count(handle);
    ADUC_FileEntity* entity = nullptr;
    int workflowLevel = workflow_get_level(handle);

    int createResult = ADUC_SystemUtils_MkSandboxDirRecursive(workFolder);
    if (createResult != 0)
    {
        Log_Error("Unable to create folder %s, error %d", workFolder, createResult);
        result = { ADUC_Result_Failure, ADUC_ERC_STEPS_HANDLER_CREATE_SANDBOX_FAILURE };
        goto done;
    }

    // Child workflow should be either 0 (resuming install phase after agent restarted),
    // or equal to fileCount (already created children during download phase)
    if (childWorkflowCount != stepCount)
    {
        // Remove existing child workflow handle(s)
        while (workflow_get_children_count(handle) > 0)
        {
            ADUC_WorkflowHandle child = workflow_remove_child(handle, 0);
            workflow_free(child);
        }

        Log_Debug("Creating workflow for %d step(s). Parent's level: %d", stepCount, workflowLevel);
        for (unsigned int i = 0; i < stepCount; i++)
        {
            STRING_HANDLE childId = nullptr;
            ADUC_FileEntity* entity = nullptr;

            if (workflow_is_inline_step(handle, i))
            {
                const char* selectedComponents = workflow_peek_selected_components(handle);

                Log_Debug("Creating workflow for level#%d step#%d. Selected components:\n=====\n%s\n=====", workflowLevel, i, selectedComponents);

                // Create child workflow using inline step data.
                result = workflow_create_from_inline_step(handle, i, &childHandle);

                if (IsAducResultCodeSuccess(result.ResultCode))
                {
                    // Inherit parent's selected components.
                    workflow_set_selected_components(childHandle, workflow_peek_selected_components(handle));
                }
            }
            else
            {
                // Download detached update manifest file.
                if (!workflow_get_step_detached_manifest_file(handle, i, &entity))
                {
                    result = { .ResultCode = ADUC_Result_Failure,
                            .ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_GET_FILE_ENTITY_FAILURE };
                    Log_Error("Cannot get a detached Update manifest file entity for level#%d step#%d", workflowLevel, i);
                    goto done;
                }

                Log_Info("Downloading a detached Update manifest file for level#%d step#%d (file id:%s).", workflowLevel, i, entity->FileId);

                try
                {
                    result = ExtensionManager::Download(entity, workflowId, workFolder, DO_RETRY_TIMEOUT_DEFAULT, nullptr);
                }
                catch (...)
                {
                    Log_Error("Exception occured while downloading a detached Update Manifest file for level#%d step#%d (file id:%s).", workflowLevel, i, entity->FileId);
                    result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_DOWNLOAD_FAILURE_UNKNOWNEXCEPTION;
                }

                std::stringstream childManifestFile;
                childManifestFile << workFolder << "/" << entity->TargetFilename;

                workflow_free_file_entity(entity);
                entity = nullptr;

                // For 'microsoft/steps:1' implementation, abort download task as soon as an error occurs.
                if (IsAducResultCodeFailure(result.ResultCode))
                {
                    Log_Error("An error occured while downloading manifest file for step#%d (erc:%d)",i, result.ExtendedResultCode);
                    goto done;
                }

                // Create child workflow from file.
                result = workflow_init_from_file(childManifestFile.str().c_str(), false, &childHandle);

                if (IsAducResultCodeSuccess(result.ResultCode))
                {
                    // Select components based on the first pair of compatibility properties.
                    char* compatibilityString = workflow_get_update_manifest_compatibility(childHandle, 0);
                    JSON_Value* compsValue = nullptr;
                    if (compatibilityString == nullptr)
                    {
                        Log_Error("Cannot get compatibility info for components-update #%d", i);
                        result = { .ResultCode = ADUC_Result_Failure,
                                .ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_GET_REF_STEP_COMPATIBILITY_FAILED };
                        goto done;
                    }

                    std::string output;
                    result = ExtensionManager::SelectComponents(compatibilityString, output);

                    if (IsAducResultCodeFailure(result.ResultCode))
                    {
                        Log_Error("Cannot select components for components-update #%d", i);
                        goto done;
                    }

                    compsValue = json_parse_string(output.c_str());
                    json_value_free(compsValue);

                    if (!workflow_set_selected_components(childHandle, output.c_str()))
                    {
                        result.ResultCode = ADUC_Result_Failure;
                        result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_SET_SELECTED_COMPONENTS_FAILURE;
                    }

                    Log_Debug("Set child handle's selected components: %s", workflow_peek_selected_components(childHandle));
                }
            }

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                Log_Error("ERROR: failed to create workflow for level:%d step#%d.", workflowLevel, i);
                goto done;
            }
            else
            {
                childId = STRING_construct_sprintf("%d", i);
                workflow_set_id(childHandle, STRING_c_str(childId));
                STRING_delete(childId);
                childId = nullptr;
#if _ADU_DEBUG
                char* childManifest = workflow_get_serialized_update_manifest(childHandle, true);
                Log_Debug("##########\n# Successfully created workflow object for child#%s\n# Handle:0x%x\n# Manifest:\n%s\n", workflow_peek_id(childHandle),
                childHandle,
                childManifest);
                workflow_free_string(childManifest);
#endif
            }

            if (!workflow_insert_child(handle, -1, childHandle))
            {
                result = { .ResultCode = ADUC_Result_Failure,
                           .ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_CHILD_WORKFLOW_INSERT_FAILED };
                goto done;
            }
        }
    }

    result = { ADUC_Result_Success };

done:
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        workflow_free(childHandle);
    }

    workflow_free_string(workFolder);
    workflow_free_file_entity(entity);
    return result;
}

/**
 * @brief Creates a new StepsHandlerImpl object and casts to a ContentHandler.
 * Note that there is no way to create a StepsHandlerImpl directly.
 *
 * @return ContentHandler* StepsHandlerImpl object as a ContentHandler.
 */
ContentHandler* StepsHandlerImpl::CreateContentHandler()
{
    return new StepsHandlerImpl();
}

/**
 * @brief Return a json string containing a 'components' array with one component.
 *
 * @param components The source components array.
 * @param index Index of the component to be returned.
 * @return char* Returns a json string containing serialized 'components' data.
 */
static char* CreateComponentSerializedString(JSON_Array* components, size_t index)
{
    JSON_Value* root = json_value_init_object();
    JSON_Value* componentClone = json_value_deep_copy(json_array_get_value(components, index));
    JSON_Array* array = json_array(json_value_init_array());
    json_array_append_value(array, componentClone);
    json_object_set_value(json_object(root), "components", json_array_get_wrapping_value(array));
    return json_serialize_to_string_pretty(root);
}

/**
 * @brief Get a list of selected components for specified workflow.
 *
 * @param handle A workflow data object handle.
 * @param componentsArray Array of selected components.
 * @return ADUC_Result Returns ADUC_Result_Success is succeeded.
 *         Otherwise, returns ADUC_ERC_STEPS_HANDLER_INVALID_COMPONENTS_DATA.
 */
static ADUC_Result GetSelectedComponentsArray(const ADUC_WorkflowHandle handle, JSON_Array** componentsArray)
{
    ADUC_Result result = { ADUC_Result_Failure };
    JSON_Value* rootValue = nullptr;
    JSON_Object* rootObject = nullptr;

    if (componentsArray == nullptr)
    {
        workflow_set_result_details(handle, "Invalid parameter - componentsArray");
        return result;
    }

    *componentsArray = nullptr;

    // Parse componenets list. If the list is empty, nothing to install.
    const char* selectedComponents = workflow_peek_selected_components(handle);
    if (selectedComponents == nullptr)
    {
        result = { .ResultCode = ADUC_Result_Failure };
        goto done;
    }

    rootValue = json_parse_string(selectedComponents);
    if (rootValue == nullptr)
    {

        result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_INVALID_COMPONENTS_DATA;
        goto done;
    }

    rootObject = json_object(rootValue);
    *componentsArray = json_object_get_array(rootObject, "components");
    if (*componentsArray == nullptr)
    {
        result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_INVALID_COMPONENTS_DATA;
        goto done;
    }

    result = { .ResultCode = ADUC_Result_Success,
               .ExtendedResultCode = 0 };

done:
    return result;
}

/**
 * @brief Perform download phase for specified step.
 *
 * @param workflowData A workflow data object.
 * @param stepIndex A step index.
 * @return ADUC_Result
 */
static ADUC_Result ProcessStepDownloadPhase(const tagADUC_WorkflowData* workflowData, int stepIndex)
{
    Log_Info("Step's download phase begin.");
    ADUC_Result result = { ADUC_Result_Failure };
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    ContentHandler* contentHandler = nullptr;
    ADUC_WorkflowHandle childHandler = workflow_get_child(handle, stepIndex);
    ADUC_WorkflowData childWorkflow {
        .WorkflowHandle = childHandler
    };

    const char* stepHandlerName = workflow_is_inline_step(handle, stepIndex) ?
                                    workflow_peek_update_manifest_step_handler(handle, stepIndex):
                                    DEFAULT_REF_STEP_HANDLER;

    Log_Info("Loading handler for step #%d ('%s')", stepIndex, stepHandlerName);

    char* workFolder = workflow_get_workfolder(childHandler);
    int createResult = ADUC_SystemUtils_MkSandboxDirRecursive(workFolder);
    if (createResult != 0)
    {
        Log_Error("Unable to create folder %s, error %d", workFolder, createResult);
        result = { ADUC_Result_Failure, ADUC_ERC_STEPS_HANDLER_CREATE_SANDBOX_FAILURE };
        goto done;
    }

    result = ExtensionManager::LoadUpdateContentHandlerExtension(stepHandlerName, &contentHandler);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("Cannot load the step handler for step #%d", stepIndex);
    }
    else
    {
        // Step is completed, no files required.
        result = contentHandler->Download(&childWorkflow);
    }

done:
    workflow_set_result(childHandler, result);

    workflow_set_state(
        handle, IsAducResultCodeSuccess(result.ResultCode) ? ADUCITF_State_DownloadSucceeded : ADUCITF_State_Failed);


    workflow_free_string(workFolder);
    return result;
}

/**
 * @brief Performs 'Download' task by iterating through all steps and invoke each step's handler
 * to download file(s), if needed.
 *
 * It is a step's handler's responsibility to determine whether
 * files are needed for 'install' and 'apply' phases.
 *
 * @param workflowData A workflow data object.
 *
 * @return ADUC_Result The result.
 */
static ADUC_Result StepsHandler_Download(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result{ ADUC_Result_Failure };
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    ADUC_WorkflowHandle stepHandle = nullptr;

    char* workflowId = workflow_get_id(handle);
    char* workingFolder = workflow_get_workfolder(handle);
    char* compatibilityString = nullptr;
    JSON_Array* selectedComponentsArray = nullptr;
    char* currentComponent;
    int workflowLevel = workflow_get_level(handle);
    int selectedComponentsCount = 0;

    Log_Debug("\n##########\n#\n# Steps_Handler Download begin (level %d, id: %d, addr:0x%x\n#\n##########\n", workflowLevel, workflowId, handle);

    result = EnsureStepsWorkflowsCreated(handle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        workflow_set_result_details(handle, "Invalid steps workflow collection.");
        goto done;
    }

    if (workflowLevel > 0)
    {
        // If this is not a top level workflow, selected component array must exists (okay to be empty).
        result = GetSelectedComponentsArray(handle, &selectedComponentsArray);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            Log_Error("Missing selected components. workflow level #%d", workflowLevel);
            workflow_set_result_details(handle, "Cannot select target components.");
            goto done;
        }

        selectedComponentsCount = json_array_get_count(selectedComponentsArray);
    }
    else
    {
        // Process all steps once.
        selectedComponentsCount = 1;
    }

    // If any of the targetted components is not up-to-date, download the update payloads.
    for (int iCom = 0; iCom < selectedComponentsCount; iCom++)
    {
        char* componentJson = nullptr;
        int childCount = workflow_get_children_count(handle);

        if (workflowLevel > 0)
        {
            componentJson = CreateComponentSerializedString(selectedComponentsArray, iCom);
            Log_Debug(
                "Processing %d step(s) for component #%d.\nComponent Json Data:%s\n",
                iCom,
                childCount,
                componentJson);
        }
        else
        {
            Log_Debug("Processing %d step(s) on host device.", childCount);
        }

        //
        // For each step (child workflow), invoke download actions, if not already installed.
        //
        for (int i = 0; i < childCount; i++)
        {
            Log_Info("Processing step #%d on component #%d.", i, iCom);

            // Dummy workflow to hold a childHandle.
            ADUC_WorkflowData stepWorkflow = {};
            std::string selectedComponentsJson;

            stepHandle = workflow_get_child(handle, i);
            if (stepHandle == nullptr)
            {
                const char* errorFmt = "Cannot process step #%d due to missing (child) workflow data.";
                Log_Error(errorFmt, i);
                result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_DOWNLOAD_FAILURE_MISSING_CHILD_WORKFLOW;
                workflow_set_result_details(handle, errorFmt, i);
                goto done;
            }

            // For inline step - set current component info on the workflow.
            if (workflow_is_inline_step(handle, i))
            {
                if (!workflow_set_selected_components(stepHandle, componentJson))
                {
                    result.ResultCode = ADUC_Result_Failure;
                    result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_SET_SELECTED_COMPONENTS_FAILURE;
                    workflow_set_result_details(handle, "Cannot select target component(s) for step #%d", i);
                    goto done;
                }
            }

            // Using same core functions.
            stepWorkflow.WorkflowHandle = stepHandle;

            ContentHandler* contentHandler = nullptr;
            const char* stepUpdateType = workflow_is_inline_step(handle, i) ?
                                workflow_peek_update_manifest_step_handler(handle, i):
                                DEFAULT_REF_STEP_HANDLER;

            Log_Info("Loading handler for step #%d (handler: '%s')", i, stepUpdateType);

            result = ExtensionManager::LoadUpdateContentHandlerExtension(stepUpdateType, &contentHandler);

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                const char* errorFmt = "Cannot load a handler for step #%d (handler :%s)";
                Log_Error(errorFmt, i, stepUpdateType);
                workflow_set_result_details(handle, errorFmt, i, stepUpdateType == nullptr ? "NULL" : stepUpdateType);
                goto done;
            }

            // If this item is already installed, skip to the next one.
            try
            {
                result = contentHandler->IsInstalled(&stepWorkflow);
            }
            catch (...)
            {
                // Cannot determine whether the step has been applied, so, we'll try to process the step.
                result = { .ResultCode = ADUC_Result_IsInstalled_NotInstalled,
                           .ExtendedResultCode = 0 };
            }

            if (IsAducResultCodeSuccess(result.ResultCode) && result.ResultCode == ADUC_Result_IsInstalled_Installed)
            {
                result.ResultCode = ADUC_Result_Install_Skipped_UpdateAlreadyInstalled;
                result.ExtendedResultCode = 0;
                // The current instance is already up-to-date, continue checking the next instance.
                goto instanceDone;
            }

            // Try to download content for current instance and step.
            try
            {
                result = contentHandler->Download(&stepWorkflow);
            }
            catch (...)
            {
                result = { .ResultCode = ADUC_Result_Failure,
                           .ExtendedResultCode =
                               ADUC_ERC_STEPS_HANDLER_DOWNLOAD_UNKNOWN_EXCEPTION_DOWNLOAD_CONTENT };
                goto done;
            }

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                // Propagate item's resultDetails to parent.
                workflow_set_result_details(handle, workflow_peek_result_details(stepHandle));
            }

        instanceDone:
            stepHandle = nullptr;

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                goto componentDone;
            }
        } // instances loop

    componentDone:
        json_free_serialized_string(componentJson);
        componentJson = nullptr;

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }

        // Set step's result.
    }

    result = { ADUC_Result_Download_Success };

done:

    // NOTE: Do not free child workflow here, so that it can be reused in the next phase.
    // Only free child handle when the workflow is done.
    //
    // Alternatively, we can persist child workflow state, to free up some memory, and
    // load the state when needed in the next phase

    workflow_set_result(handle, result);

    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        workflow_set_state(handle, ADUCITF_State_DownloadSucceeded);
    }
    else
    {
        workflow_set_state(handle, ADUCITF_State_Failed);
    }

    workflow_free_string(workflowId);
    workflow_free_string(workingFolder);
    workflow_free_string(compatibilityString);

    Log_Debug("Steps_Handler Download end (level %d).", workflowLevel);
    return result;
}

/**
 * @brief Performs 'Download' task by iterating through all steps and invoke each step's handler
 * to download file(s), if needed.
 *
 * It is a step's handler's responsibility to determine whether
 * any files is needed for 'install' and 'apply' phases.
 *
 * @param workflowData A workflow data object.
 *
 * @return ADUC_Result The result.
 */
ADUC_Result StepsHandlerImpl::Download(const tagADUC_WorkflowData* workflowData)
{
    return StepsHandler_Download(workflowData);
}


/**
 * @brief Performs 'Install' phase.
 * All files required for installation must be downloaded in to sandbox.
 * During this phase, we will not re-download any file.
 * If file(s) missing, install will be aborted.
 *
 * @return ADUC_Result The result (always success)
 */
static ADUC_Result StepsHandler_Install(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result{ ADUC_Result_Failure };

    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    ADUC_WorkflowHandle stepHandle = nullptr;

    char* workflowId = workflow_get_id(handle);
    char* workingFolder = workflow_get_workfolder(handle);
    char* compatibilityString = nullptr;
    JSON_Array* selectedComponentsArray = nullptr;
    char* currentComponent;
    int workflowLevel = workflow_get_level(handle);
    int selectedComponentsCount = 0;

    Log_Debug("\n##########\n#\n# Steps_Handler Install begin (level %d, id: %s, addr:0x%x\n#\n##########\n", workflowLevel, workflowId, handle);

    result = EnsureStepsWorkflowsCreated(handle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        workflow_set_result_details(handle, "Invalid steps workflow collection.");
        goto done;
    }

    if (workflowLevel > 0)
    {
        // If this is not a top level workflow, selected component array must exists (okay to be empty).
        result = GetSelectedComponentsArray(handle, &selectedComponentsArray);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            Log_Error("Missing selected components. workflow level #%d", workflowLevel);
            workflow_set_result_details(handle, "Cannot select target components.");
            goto done;
        }

        selectedComponentsCount = json_array_get_count(selectedComponentsArray);
    }
    else
    {
        // Process all steps once.
        selectedComponentsCount = 1;
    }

    // For each targetted component, perform step's install & apply phase, in order.
    for (int iCom = 0; iCom < selectedComponentsCount; iCom++)
    {
        char* componentJson = nullptr;
        bool skipRemainingSteps = false;
        int childCount = workflow_get_children_count(handle);

        if (workflowLevel > 0)
        {
            componentJson = CreateComponentSerializedString(selectedComponentsArray, iCom);
            Log_Debug(
                "Processing %d step(s) for component #%d.\nComponent Json Data:%s\n",
                iCom,
                childCount,
                componentJson);
        }
        else
        {
            Log_Debug("Processing %d step(s) on host device.", workflow_get_children_count(handle));
        }

        //
        // For each step (child workflow), invoke install and apply actions.
        //
        for (int i = 0; i < childCount && !skipRemainingSteps; i++)
        {
            Log_Info("Processing step #%d on component #%d.", i, iCom);

            // Dummy workflow to hold a childHandle.
            ADUC_WorkflowData stepWorkflow = {};
            std::string selectedComponentsJson;

            stepHandle = workflow_get_child(handle, i);
            if (stepHandle == nullptr)
            {
                const char* errorFmt = "Cannot process step #%d due to missing (child) workflow data.";
                Log_Error(errorFmt, i);
                result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_INSTALL_FAILURE_MISSING_CHILD_WORKFLOW;
                workflow_set_result_details(handle, errorFmt, i);
                goto done;
            }

            // For inline step - set current component info on the workflow.
            if (workflow_is_inline_step(handle, i))
            {
                if (!workflow_set_selected_components(stepHandle, componentJson))
                {
                    result.ResultCode = ADUC_Result_Failure;
                    result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_SET_SELECTED_COMPONENTS_FAILURE;
                    workflow_set_result_details(handle, "Cannot select target component(s) for step #%d", i);
                    goto done;
                }
            }

            // Using same core functions.
            stepWorkflow.WorkflowHandle = stepHandle;

            ContentHandler* contentHandler = nullptr;
            const char* stepUpdateType = workflow_is_inline_step(handle, i) ?
                                workflow_peek_update_manifest_step_handler(handle, i):
                                DEFAULT_REF_STEP_HANDLER;

            Log_Info("Loading handler for step #%d (handler: '%s')", i, stepUpdateType);

            result = ExtensionManager::LoadUpdateContentHandlerExtension(stepUpdateType, &contentHandler);

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                const char* errorFmt = "Cannot load a handler for step #%d (handler :%s)";
                Log_Error(errorFmt, i, stepUpdateType);
                workflow_set_result_details(handle, errorFmt, i, stepUpdateType == nullptr ? "NULL" : stepUpdateType);
                goto done;
            }

            // If this item is already installed, skip to the next one.
            try
            {
                result = contentHandler->IsInstalled(&stepWorkflow);
            }
            catch (...)
            {
                // Cannot determine whether the step has been applied, so, we'll try to process the step.
                result = { .ResultCode = ADUC_Result_IsInstalled_NotInstalled,
                           .ExtendedResultCode = 0 };
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
                result = contentHandler->Install(&stepWorkflow);
            }
            catch (...)
            {
                result = { .ResultCode = ADUC_Result_Failure,
                           .ExtendedResultCode =
                               ADUC_ERC_STEPS_HANDLER_INSTALL_UNKNOWN_EXCEPTION_INSTALL_CHILD_STEP };
                goto done;
            }

            switch (result.ResultCode)
            {
            case ADUC_Result_Install_RequiredImmediateReboot:
                workflow_request_immediate_reboot(handle);
                // We can skip another instances.
                goto done;

            case ADUC_Result_Install_RequiredReboot:
                workflow_request_reboot(handle);
                break;

            case ADUC_Result_Install_RequiredImmediateAgentRestart:
                workflow_request_immediate_agent_restart(handle);
                goto done;

            case ADUC_Result_Install_RequiredAgentRestart:
                workflow_request_agent_restart(handle);
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
                // Propagate item's resultDetails to parent.
                workflow_set_result_details(
                    handle, workflow_peek_result_details(stepHandle));

                goto done;
            }

            //
            // Perform 'apply' action.
            //
            try
            {
                result = contentHandler->Apply(&stepWorkflow);
            }
            catch (...)
            {
                result = { .ResultCode = ADUC_Result_Failure,
                           .ExtendedResultCode =
                               ADUC_ERC_STEPS_HANDLER_INSTALL_UNKNOWN_EXCEPTION_APPLY_CHILD_STEP };
                goto done;
            }

            if (iCom == (selectedComponentsCount - 1))
            {
                // This is the last instance of the selected component.
                if (!IsAducResultCodeFailure(result.ResultCode))
                {
                    workflow_set_result(stepHandle, result);
                    workflow_set_result_details(stepHandle, "");
                }
            }

            switch (result.ResultCode)
            {
            case ADUC_Result_Apply_RequiredImmediateReboot:
                workflow_request_immediate_reboot(handle);
                // Translate into components-level result.
                result.ResultCode = ADUC_Result_Install_RequiredImmediateReboot;
                // We can skip another instances.
                goto done;

            case ADUC_Result_Apply_RequiredReboot:
                workflow_request_reboot(handle);
                // Translate into 'install' result.
                result.ResultCode = ADUC_Result_Install_RequiredReboot;
                break;

            case ADUC_Result_Apply_RequiredImmediateAgentRestart:
                workflow_request_immediate_agent_restart(handle);
                // Translate into components-level result.
                result.ResultCode = ADUC_Result_Install_RequiredImmediateAgentRestart;
                goto done;

            case ADUC_Result_Apply_RequiredAgentRestart:
                workflow_request_agent_restart(handle);
                // Translate into components-level result.
                result.ResultCode = ADUC_Result_Install_RequiredAgentRestart;
                break;
            }

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                // Propagate item's resultDetails to parent.
                workflow_set_result_details(handle, workflow_peek_result_details(stepHandle));
            }

        instanceDone:
            stepHandle = nullptr;

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                goto componentDone;
            }
        } // installItems loop

    componentDone:
        json_free_serialized_string(componentJson);
        componentJson = nullptr;

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }

        // Set step's result.
    }

    result = { ADUC_Result_Install_Success };

done:

    // NOTE: Do not free child workflow here, so that it can be reused in the next phase.
    // Only free child handle when the workflow is done.
    //
    // Alternatively, we can persist child workflow state, to free up some memory, and
    // load the state when needed in the next phase

    workflow_set_result(handle, result);

    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        workflow_set_state(handle, ADUCITF_State_InstallSucceeded);
    }
    else
    {
        workflow_set_state(handle, ADUCITF_State_Failed);
    }

    workflow_free_string(workflowId);
    workflow_free_string(workingFolder);
    workflow_free_string(compatibilityString);

    Log_Debug("Steps_Handler Install end (level %d).", workflowLevel);
    return result;
}

/**
 * @brief Performs 'Install' phase.
 * All files required for installation must be downloaded in to sandbox.
 * During this phase, we will not re-download any file.
 * If file(s) missing, install will be aborted.
 *
 * @return ADUC_Result The result (always success)
 */
ADUC_Result StepsHandlerImpl::Install(const tagADUC_WorkflowData* workflowData)
{
    return StepsHandler_Install(workflowData);
}

/**
 * @brief Perform 'Apply' action.
 * @return ADUC_Result The result (always success)
 */
static ADUC_Result StepsHandler_Apply(const tagADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);
    Log_Debug("Steps_Handler 'apply' phase begin.");
    ADUC_Result result{ ADUC_Result_Apply_Success };

    Log_Debug("Steps_Handler apply phase end.");
    return result;
}

/**
 * @brief Perform 'Apply' action.
 * @return ADUC_Result The result (always success)
 */
ADUC_Result StepsHandlerImpl::Apply(const tagADUC_WorkflowData* workflowData)
{
    return StepsHandler_Apply(workflowData);
}

/**
 * @brief Perform 'Cancel' action
 */
ADUC_Result StepsHandlerImpl::Cancel(const tagADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);
    Log_Info("Cancel called - returning success");
    return ADUC_Result{ ADUC_Result_Cancel_UnableToCancel };
}

/**
 * @brief Checks if the installed content matches the installed criteria.
 *
 * @param installedCriteria The installed criteria string. e.g. The firmware version or APT id.
 *  installedCriteria has already been checked to be non-empty before this call.
 *
 * @return ADUC_Result
 */
static ADUC_Result StepsHandler_IsInstalled(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result{ ADUC_Result_Failure };

    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    ADUC_WorkflowHandle stepHandle = nullptr;

    char* workflowId = workflow_get_id(handle);
    char* workingFolder = workflow_get_workfolder(handle);
    char* compatibilityString = nullptr;
    JSON_Array* selectedComponentsArray = nullptr;
    char* currentComponent;
    int workflowLevel = workflow_get_level(handle);
    int selectedComponentsCount = 0;

    Log_Debug("Steps_Handler IsInstall begin (level %d).", workflowLevel);

    result = EnsureStepsWorkflowsCreated(handle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        workflow_set_result_details(handle, "Invalid steps workflow collection.");
        goto done;
    }

    if (workflowLevel > 0)
    {
        // If this is not a top level workflow, selected component array must exists (okay to be empty).
        result = GetSelectedComponentsArray(handle, &selectedComponentsArray);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            Log_Error("Missing selected components. workflow level #%d", workflowLevel);
            workflow_set_result_details(handle, "Cannot select target components.");
            goto done;
        }

        selectedComponentsCount = json_array_get_count(selectedComponentsArray);
    }
    else
    {
        // Process all steps once.
        selectedComponentsCount = 1;
    }

    // For each targetted component, perform step's install & apply phase, in order.
    for (int iCom = 0; iCom < selectedComponentsCount; iCom++)
    {
        char* componentJson = nullptr;
        bool skipRemainingSteps = false;
        int childCount = workflow_get_children_count(handle);

        if (workflowLevel > 0)
        {
            componentJson = CreateComponentSerializedString(selectedComponentsArray, iCom);
            Log_Debug(
                "Processing %d step(s) for component #%d.\nComponent Json Data:%s\n",
                iCom,
                childCount,
                componentJson);
        }
        else
        {
            Log_Debug("Processing %d step(s) on host device.", childCount);
        }

        //
        // For each step (child workflow), invoke install and apply actions.
        //
        for (int i = 0; i < childCount && !skipRemainingSteps; i++)
        {
            Log_Info("Processing step #%d on component #%d.\n#### Component ####\n%s\n###################\n", i, iCom, componentJson);

            // Dummy workflow to hold a childHandle.
            ADUC_WorkflowData stepWorkflow = {};
            std::string selectedComponentsJson;

            stepHandle = workflow_get_child(handle, i);
            if (stepHandle == nullptr)
            {
                const char* errorFmt = "Cannot process step #%d due to missing (child) workflow data.";
                Log_Error(errorFmt, i);
                result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_INSTALL_FAILURE_MISSING_CHILD_WORKFLOW;
                workflow_set_result_details(handle, errorFmt, i);
                goto done;
            }

            // For inline step - set current component info on the workflow.
            if (workflow_is_inline_step(handle, i))
            {
                if (!workflow_set_selected_components(stepHandle, componentJson))
                {
                    result.ResultCode = ADUC_Result_Failure;
                    result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_SET_SELECTED_COMPONENTS_FAILURE;
                    workflow_set_result_details(handle, "Cannot select target component(s) for step #%d", i);
                    goto done;
                }
            }

            // Using same core functions.
            stepWorkflow.WorkflowHandle = stepHandle;

            ContentHandler* contentHandler = nullptr;
            const char* stepUpdateType = workflow_is_inline_step(handle, i) ?
                                workflow_peek_update_manifest_step_handler(handle, i):
                                DEFAULT_REF_STEP_HANDLER;

            Log_Info("Loading handler for step #%d (handler: '%s')", i, stepUpdateType);

            result = ExtensionManager::LoadUpdateContentHandlerExtension(stepUpdateType, &contentHandler);

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                const char* errorFmt = "Cannot load a handler for step #%d (handler :%s)";
                Log_Error(errorFmt, i, stepUpdateType);
                workflow_set_result_details(handle, errorFmt, i, stepUpdateType == nullptr ? "NULL" : stepUpdateType);
                goto done;
            }

            // If this item is already installed, skip to the next one.
            try
            {
                result = contentHandler->IsInstalled(&stepWorkflow);
            }
            catch (...)
            {
                // Cannot determine whether the step has been applied, so, we'll try to process the step.
                result = { .ResultCode = ADUC_Result_IsInstalled_NotInstalled,
                           .ExtendedResultCode = 0 };
            }

            if (IsAducResultCodeFailure(result.ResultCode) ||
                result.ResultCode == ADUC_Result_IsInstalled_NotInstalled)
            {
                Log_Info("Step #%d is not installed.", i);
                goto done;
            }

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                // Propagate item's resultDetails to parent.
                workflow_set_result_details(handle, workflow_peek_result_details(stepHandle));
            }

        instanceDone:
            stepHandle = nullptr;

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                goto componentDone;
            }
        } // installItems loop

    componentDone:
        json_free_serialized_string(componentJson);
        componentJson = nullptr;

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }

        // Set step's result.
    }

    result = { ADUC_Result_IsInstalled_Installed };

done:

    workflow_free_string(workflowId);
    workflow_free_string(workingFolder);
    workflow_free_string(compatibilityString);

    Log_Debug("Steps_Handler IsInstall end (level %d).", workflowLevel);

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
ADUC_Result StepsHandlerImpl::IsInstalled(const tagADUC_WorkflowData* workflowData)
{
    return StepsHandler_IsInstalled(workflowData);
}
