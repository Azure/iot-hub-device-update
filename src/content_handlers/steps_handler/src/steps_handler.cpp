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
#include "aduc/string_c_utils.h"    // IsNullOrEmpty
#include "aduc/string_utils.hpp"
#include "aduc/system_utils.h"
#include "aduc/workflow_utils.h"
#include "aduc/calloc_wrapper.hpp"  // cstr_wrapper

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

using ADUC::StringUtils::cstr_wrapper;

#define DEFAULT_REF_STEP_HANDLER "microsoft/steps:1"

/**
 * @brief Check whether to show additional debug logs.
 *
 * @return true if DU_AGENT_ENABLE_STEPS_HANDLER_EXTRA_DEBUG_LOGS is set
 */
static bool IsStepsHandlerExtraDebugLogsEnabled()
{
    return (!IsNullOrEmpty(getenv("DU_AGENT_ENABLE_STEPS_HANDLER_EXTRA_DEBUG_LOGS")));
}

EXTERN_C_BEGIN
/**
 * @brief Instantiates an Update Content Handler for 'microsoft/steps:1' update type.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel)
{
    ADUC_Logging_Init(logLevel, "steps-handler");
    Log_Info("Instantiating an Update Content Handler for 'microsoft/steps:1'");
    try
    {
        return StepsHandlerImpl::CreateContentHandler();
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

/**
 * @brief Destructor for the Steps Handler Impl class.
 */
StepsHandlerImpl::~StepsHandlerImpl() // override
{
    ADUC_Logging_Uninit();
}

/**
 * @brief Ensure all steps' workflow data objects are created.
 *
 * Algorithm:
 *    Start from a given parent workflow ( @p handle )
 *
 *       foreach step in steps {
 *
 *          if in-line step {
 *              - create child workflow for this step (inherit some file entities from parent workflow )
 *              - copy parent workflow's selected components into child workflow
 *          } else {
 *              - download this reference step detached-manifest file
 *              - create child workflor for tis step from manifest file (inherit some file entities from parent workflow)
 *              - select target components based on this step workflow's compatibilities
 *                  Note: components-enumerator extension is not registered, the reference step will be applied to host device (selected component is empty)
 *          }
 *
 *      } // next step
 *
 *
 * @param handle A workflow data object handle.
 * @return ADUC_Result
 */
ADUC_Result PrepareStepsWorkflowDataObject(ADUC_WorkflowHandle handle)
{
    ADUC_Result result{ ADUC_Result_Failure };
    ADUC_WorkflowHandle childHandle = nullptr;

    auto stepCount = static_cast<unsigned int>(workflow_get_instructions_steps_count(handle));
    const char* workflowId = workflow_peek_id(handle);
    char* workFolder = workflow_get_workfolder(handle);
    unsigned int childWorkflowCount = workflow_get_children_count(handle);
    ADUC_FileEntity* entity = nullptr;
    int workflowLevel = workflow_get_level(handle);

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
            childHandle = nullptr;

            if (workflow_is_inline_step(handle, i))
            {
                const char* selectedComponents = workflow_peek_selected_components(handle);

                Log_Debug(
                    "Creating workflow for level#%d step#%d.\nSelected components:\n=====\n%s\n=====\n",
                    workflowLevel,
                    i,
                    selectedComponents);

                // Create child workflow using inline step data.
                result = workflow_create_from_inline_step(handle, i, &childHandle);

                if (IsAducResultCodeSuccess(result.ResultCode))
                {
                    workflow_set_step_index(childHandle, i);

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
                    Log_Error(
                        "Cannot get a detached Update manifest file entity for level#%d step#%d", workflowLevel, i);
                    goto done;
                }

                Log_Info(
                    "Downloading a detached Update manifest file for level#%d step#%d (file id:%s).",
                    workflowLevel,
                    i,
                    entity->FileId);

                try
                {
                    result =
                        ExtensionManager::Download(entity, workflowId, workFolder, DO_RETRY_TIMEOUT_DEFAULT, nullptr);
                }
                catch (...)
                {
                    Log_Error(
                        "Exception occurred while downloading a detached Update Manifest file for level#%d step#%d (file id:%s).",
                        workflowLevel,
                        i,
                        entity->FileId);
                    result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_DOWNLOAD_FAILURE_UNKNOWNEXCEPTION;
                }

                std::stringstream childManifestFile;
                childManifestFile << workFolder << "/" << entity->TargetFilename;

                workflow_free_file_entity(entity);
                entity = nullptr;

                // For 'microsoft/steps:1' implementation, abort download task as soon as an error occurs.
                if (IsAducResultCodeFailure(result.ResultCode))
                {
                    Log_Error(
                        "An error occurred while downloading manifest file for step#%d (erc:%d)",
                        i,
                        result.ExtendedResultCode);
                    goto done;
                }

                // Create child workflow from file.
                result = workflow_init_from_file(childManifestFile.str().c_str(), false, &childHandle);

                if (IsAducResultCodeSuccess(result.ResultCode))
                {
                    workflow_set_step_index(childHandle, i);

                    // If no component enumerator is registered, assume that this reference update is for the host device.
                    // Don't set selected components in the workflow data.
                    if (ExtensionManager::IsComponentsEnumeratorRegistered())
                    {
                        // Select components based on the first pair of compatibility properties.
                        ADUC::StringUtils::cstr_wrapper compatibilityString{ workflow_get_update_manifest_compatibility(childHandle, 0) };
                        JSON_Value* compsValue = nullptr;
                        if (compatibilityString.get() == nullptr)
                        {
                            Log_Error("Cannot get compatibility info for components-update #%d", i);
                            result = { .ResultCode = ADUC_Result_Failure,
                                       .ExtendedResultCode =
                                           ADUC_ERC_STEPS_HANDLER_GET_REF_STEP_COMPATIBILITY_FAILED };
                            goto done;
                        }

                        std::string output;
                        result = ExtensionManager::SelectComponents(compatibilityString.get(), output);

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

                        Log_Debug(
                            "Set child handle's selected components: %s",
                            workflow_peek_selected_components(childHandle));
                    }
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
                Log_Debug(
                    "##########\n# Successfully created workflow object for child#%s\n# Handle:0x%x\n# Manifest:\n%s\n",
                    workflow_peek_id(childHandle),
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
 * @return char* Returns nullptr if @p index is invalid.
 *         Otherwise, return a json string containing serialized 'components' data.
 */
static char* CreateComponentSerializedString(JSON_Array* components, size_t index)
{
    JSON_Value* val = json_array_get_value(components, index);
    if (val == nullptr)
    {
        return nullptr;
    }

    JSON_Value* root = json_value_init_object();
    JSON_Value* componentClone = json_value_deep_copy(val);
    JSON_Array* array = json_array(json_value_init_array());
    json_array_append_value(array, componentClone);
    json_object_set_value(json_object(root), "components", json_array_get_wrapping_value(array));
    return json_serialize_to_string_pretty(root);
}

/**
 * @brief Get a list of selected components for specified workflow @p handle.
 *
 * @param handle A workflow data object handle.
 * @param componentsArray Array of selected components.
 * @return ADUC_Result Returns ADUC_Result_Success is succeeded.
 *         Otherwise, returns ADUC_ERC_STEPS_HANDLER_INVALID_COMPONENTS_DATA.
 */
static ADUC_Result GetSelectedComponentsArray(ADUC_WorkflowHandle handle, JSON_Array** componentsArray)
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
    if (IsNullOrEmpty(selectedComponents))
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

    result = { .ResultCode = ADUC_Result_Success, .ExtendedResultCode = 0 };

done:
    return result;
}

/**
 * @brief Performs 'Download' task by iterating through all steps and invoke each step's handler
 * to download file(s), if needed.
 *
 * Each step's handler is responsible for determine whether to download payload file(s) for
 * for 'install' and 'apply' tasks.
 *
 * @param workflowData A workflow data object.
 *
 * @return ADUC_Result The result.
 *
 */
static ADUC_Result StepsHandler_Download(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result{ ADUC_Result_Failure };
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    ADUC_WorkflowHandle stepHandle = nullptr;
    char* workflowId = workflow_get_id(handle);
    char* workFolder = workflow_get_workfolder(handle);
    JSON_Array* selectedComponentsArray = nullptr;
    int workflowLevel = workflow_get_level(handle);
    int workflowStep = workflow_get_step_index(handle);
    int selectedComponentsCount = 0;
    char* serializedComponentString = nullptr;
    bool isComponentsEnumeratorRegistered = ExtensionManager::IsComponentsEnumeratorRegistered();

    Log_Debug(
        "\n#\n#Download task begin (level: %d, step:%d, wfid:%s, h_addr:0x%x).",
        workflowLevel,
        workflowStep,
        workflowId,
        handle);

    int createResult = ADUC_SystemUtils_MkSandboxDirRecursive(workFolder);
    if (createResult != 0)
    {
        Log_Error("Unable to create folder %s, error %d", workFolder, createResult);
        result = { ADUC_Result_Failure, ADUC_ERC_STEPS_HANDLER_CREATE_SANDBOX_FAILURE };
        goto done;
    }

    result = PrepareStepsWorkflowDataObject(handle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        workflow_set_result_details(handle, "Invalid steps workflow collection");
        goto done;
    }

    if (workflowLevel == 0 || !isComponentsEnumeratorRegistered)
    {
        // If this is a top-level step or component enumerator is not registered, we will assume that this step is
        // intended for the host device and will set selectedComponentCount to 1 to iterate through
        // every step once without setting any component data on the workflow.
        selectedComponentsCount = 1;
    }
    else
    {
        // This is a reference step (workflowLevel == 1), this intended for one or more components.
        result = GetSelectedComponentsArray(handle, &selectedComponentsArray);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            const char* fmt = "Missing selected components. workflow level %d, step %d";
            Log_Error(fmt, workflowLevel, workflowStep);
            workflow_set_result_details(handle, fmt, workflowLevel, workflowStep);
            goto done;
        }

        selectedComponentsCount = json_array_get_count(selectedComponentsArray);

        if (selectedComponentsCount == 0)
        {
            // If there's no matching component, we will consider this step 'optional' and this
            // step is no-op.
            const char* msg = "Optional step (no matching components)";
            Log_Debug(msg);
            result = { ADUC_Result_Download_Skipped_NoMatchingComponents };

            // This is a good opportunity to set the workflow state to indicates that
            // the current component is 'optional'.
            // We do this by setting workflow result code to ADUC_Result_Download_Skipped_NoMatchingComponents
            ADUC_Result currentResult = workflow_get_result(handle);
            if (IsAducResultCodeFailure(currentResult.ResultCode))
            {
                ADUC_Result newResult = { ADUC_Result_Download_Skipped_NoMatchingComponents };
                workflow_set_result(handle, newResult);
                workflow_set_result_details(handle, msg);
            }
        }
    }

    // For each selected component, perform step's backup, install & apply phase, restore phase if needed, in order.
    for (int iCom = 0, stepsCount = workflow_get_children_count(handle); iCom < selectedComponentsCount; iCom++)
    {
        serializedComponentString = CreateComponentSerializedString(selectedComponentsArray, iCom);

        //
        // For each step (child workflow), invoke backup, install and apply actions.
        // if install or apply fails, invoke restore action.
        //
        for (int i = 0; i < stepsCount; i++)
        {
            if (IsStepsHandlerExtraDebugLogsEnabled())
            {
                Log_Debug(
                    "Perform download action of child step #%d on component #%d.\n#### Component ####\n%s\n###################\n",
                    i,
                    iCom,
                    serializedComponentString);
            }

            // Use a wrapper workflow to hold a stepHandle.
            ADUC_WorkflowData stepWorkflow = {};

            stepHandle = workflow_get_child(handle, i);
            if (stepHandle == nullptr)
            {
                const char* errorFmt = "Cannot process step #%d due to missing (child) workflow data.";
                Log_Error(errorFmt, i);
                result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_DOWNLOAD_FAILURE_MISSING_CHILD_WORKFLOW;
                workflow_set_result_details(handle, errorFmt, i);
                goto done;
            }
            stepWorkflow.WorkflowHandle = stepHandle;

            // For inline step - set current component info on the workflow.
            if (serializedComponentString != nullptr && workflow_is_inline_step(handle, i))
            {
                if (!workflow_set_selected_components(stepHandle, serializedComponentString))
                {
                    result.ResultCode = ADUC_Result_Failure;
                    result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_SET_SELECTED_COMPONENTS_FAILURE;
                    workflow_set_result_details(handle, "Cannot select target component(s) for step #%d", i);
                    goto done;
                }
            }

            ContentHandler* contentHandler = nullptr;
            const char* stepUpdateType = workflow_is_inline_step(handle, i)
                ? workflow_peek_update_manifest_step_handler(handle, i)
                : DEFAULT_REF_STEP_HANDLER;

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
                result = { .ResultCode = ADUC_Result_IsInstalled_NotInstalled, .ExtendedResultCode = 0 };
            }

            if (IsAducResultCodeSuccess(result.ResultCode) && result.ResultCode == ADUC_Result_IsInstalled_Installed)
            {
                result.ResultCode = ADUC_Result_Install_Skipped_UpdateAlreadyInstalled;
                result.ExtendedResultCode = 0;
                workflow_set_result(stepHandle, result);
                workflow_set_result_details(handle, workflow_peek_result_details(stepHandle));
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
                           .ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_DOWNLOAD_UNKNOWN_EXCEPTION_DOWNLOAD_CONTENT };
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
        json_free_serialized_string(serializedComponentString);
        serializedComponentString = nullptr;

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

    json_free_serialized_string(serializedComponentString);
    workflow_free_string(workflowId);
    workflow_free_string(workFolder);

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
 * Algorithm:
 *      - Top-level inline step is intended for the host.
 *          - [Process the step]
 *              - Load content handler
 *              - Invoke contentHandler::Install
 *                  - If failed, return with 'Install' result.
 *                  - If success with Reboot or Agent Restart request, then call workflow api accordingly, then return with 'Install' result.
 *                  - Otherise, continue...
 *              - Invoke contentHandler::Apply
 *              - Return 'Apply' result
 *
 *     - For reference step(s):
 *         - If component enumerator is registered...
 *              - The reference step must be installed onto 'selected-components'
 *              - Components are selected using compat properties specified in reference step's update manifest.
 *              - If no components matched, the reference step are considered "optional".
 *                   - Install resultCode for optional step is ADUC_Result_Install_Skipped_NoMatchingComponents (604)
 *              - For each selected component [Process the step]
 *                   - Load content handler
 *                   - Invoke contentHandler::Install
 *                      - If failed, return with 'Install' result.
 *                      - If success with Reboot or Agent Restart request, then call workflow api accordingly, then return with 'Install' result.
 *                      - Otherise, continue...
 *                   - Invoke contentHandler::Apply
 *                      - If failed, invoke contentHandler::Restore, then return with 'Apply' result
 *                      - If success, continue to next *component*
 *                   - Once done with every components, return ADUC_Result_Install_Success (600)
 *
 *         - If component enumerator is not registered, every child steps of this reference step will be installed onto the host.
 *           In this case, if the reference step is intended to be install onto a component, it's likely to be failed, due to missing component info.
 *              - [Process the step] (same as above, but w/o selected component data)
 *
 * @return ADUC_Result The 'install' result
 */
static ADUC_Result StepsHandler_Install(const tagADUC_WorkflowData* workflowData)
{
    ADUC_Result result{ ADUC_Result_Failure };
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    ADUC_WorkflowHandle stepHandle = nullptr;

    char* workflowId = workflow_get_id(handle);
    char* workFolder = workflow_get_workfolder(handle);
    JSON_Array* selectedComponentsArray = nullptr;
    int workflowLevel = workflow_get_level(handle);
    int workflowStep = workflow_get_step_index(handle);
    int selectedComponentsCount = 0;
    char* serializedComponentString = nullptr;
    bool isComponentsEnumeratorRegistered = ExtensionManager::IsComponentsEnumeratorRegistered();

    Log_Debug(
        "\n#\n#Install task begin (level: %d, step:%d, wfid:%s, h_addr:0x%x).",
        workflowLevel,
        workflowStep,
        workflowId,
        handle);

    int createResult = ADUC_SystemUtils_MkSandboxDirRecursive(workFolder);
    if (createResult != 0)
    {
        Log_Error("Unable to create folder %s, error %d", workFolder, createResult);
        result = { ADUC_Result_Failure, ADUC_ERC_STEPS_HANDLER_CREATE_SANDBOX_FAILURE };
        goto done;
    }

    result = PrepareStepsWorkflowDataObject(handle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        workflow_set_result_details(handle, "Invalid steps workflow collection");
        goto done;
    }

    if (workflowLevel == 0 || !isComponentsEnumeratorRegistered)
    {
        // If this is a top-level step or component enumerator is not registered, we will assume that this step is
        // intended for the host device and will set selectedComponentCount to 1 to iterate through
        // every step once  without setting any component data on the workflow.
        selectedComponentsCount = 1;
    }
    else
    {
        // This is a reference step (workflowLevel == 1), this intended for one or more components.
        result = GetSelectedComponentsArray(handle, &selectedComponentsArray);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            const char* fmt = "Missing selected components. workflow level %d, step %d";
            Log_Error(fmt, workflowLevel, workflowStep);
            workflow_set_result_details(handle, fmt, workflowLevel, workflowStep);
            goto done;
        }

        selectedComponentsCount = json_array_get_count(selectedComponentsArray);

        if (selectedComponentsCount == 0)
        {
            // If there's no matching component, we will consider this step 'optional' and this
            // step is no-op.
            const char* msg = "Optional step (no matching components)";
            Log_Debug(msg);
            result = { ADUC_Result_Install_Skipped_NoMatchingComponents };

            // This is a good opportunity to set the workflow state to indicates that
            // the current component is 'optional'.
            // We do this by setting workflow result code to ADUC_Result_Install_Skipped_NoMatchingComponents
            ADUC_Result currentResult = workflow_get_result(handle);
            if (IsAducResultCodeFailure(currentResult.ResultCode))
            {
                ADUC_Result newResult = { ADUC_Result_Install_Skipped_NoMatchingComponents };
                workflow_set_result(handle, newResult);
                workflow_set_result_details(handle, msg);
            }
        }
    }

    // For each selected component, perform step's backup, install & apply phase, restore phase if needed, in order.
    for (int iCom = 0, stepsCount = workflow_get_children_count(handle); iCom < selectedComponentsCount; iCom++)
    {
        serializedComponentString = CreateComponentSerializedString(selectedComponentsArray, iCom);

        //
        // For each step (child workflow), invoke backup, install and apply actions.
        // if install or apply fails, invoke restore action.
        //
        for (int i = 0; i < stepsCount; i++)
        {
            if (IsStepsHandlerExtraDebugLogsEnabled())
            {
                Log_Debug(
                    "Perform install action of child step #%d on component #%d.\n#### Component ####\n%s\n###################\n",
                    i,
                    iCom,
                    serializedComponentString);
            }

            // Use a wrapper workflow to hold a stepHandle.
            ADUC_WorkflowData stepWorkflow = {};

            stepHandle = workflow_get_child(handle, i);
            if (stepHandle == nullptr)
            {
                const char* errorFmt = "Cannot process step #%d due to missing (child) workflow data.";
                Log_Error(errorFmt, i);
                result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_INSTALL_FAILURE_MISSING_CHILD_WORKFLOW;
                workflow_set_result_details(handle, errorFmt, i);
                goto done;
            }
            stepWorkflow.WorkflowHandle = stepHandle;

            // For inline step - set current component info on the workflow.
            if (serializedComponentString != nullptr && workflow_is_inline_step(handle, i))
            {
                if (!workflow_set_selected_components(stepHandle, serializedComponentString))
                {
                    result.ResultCode = ADUC_Result_Failure;
                    result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_SET_SELECTED_COMPONENTS_FAILURE;
                    workflow_set_result_details(handle, "Cannot set target component(s) for step #%d", i);
                    goto done;
                }
            }

            ContentHandler* contentHandler = nullptr;
            const char* stepUpdateType = workflow_is_inline_step(handle, i)
                ? workflow_peek_update_manifest_step_handler(handle, i)
                : DEFAULT_REF_STEP_HANDLER;

            Log_Info("Loading handler for child step #%d (handler: '%s')", i, stepUpdateType);

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
                result = { .ResultCode = ADUC_Result_IsInstalled_NotInstalled, .ExtendedResultCode = 0 };
            }

            if (IsAducResultCodeSuccess(result.ResultCode) && result.ResultCode == ADUC_Result_IsInstalled_Installed)
            {
                result.ResultCode = ADUC_Result_Install_Skipped_UpdateAlreadyInstalled;
                result.ExtendedResultCode = 0;
                workflow_set_result(stepHandle, result);
                workflow_set_result_details(handle, workflow_peek_result_details(stepHandle));
                // Skipping 'backup', 'install' and 'apply'.
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
                           .ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_INSTALL_UNKNOWN_EXCEPTION_INSTALL_CHILD_STEP };
                goto done;
            }

            // If the workflow interuption is required as part of the Install action,
            // we must propagate that request to the wrapping workflow.

            if (workflow_is_immediate_reboot_requested(stepHandle)
                || workflow_is_immediate_agent_restart_requested(stepHandle))
            {
                // Skip remaining tasks for this instance.
                // And then skip remaining instance(s) if requested.
                goto instanceDone;
            }

            // If any step reported that the update is already installed on the
            // selected component, we will skip the 'apply' phase, and skip all
            // remaining step(s).
            switch (result.ResultCode)
            {
            case ADUC_Result_Install_Skipped_UpdateAlreadyInstalled:
            case ADUC_Result_Install_Skipped_NoMatchingComponents:
                goto instanceDone;
            }

            // If Install task failed, try to restore (best effort).
            // The restore result is discarded, since install result is more important to customer.
            if (IsAducResultCodeFailure(result.ResultCode))
            {
                // Propagate item's resultDetails to parent.
                workflow_set_result_details(handle, workflow_peek_result_details(stepHandle));
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
                           .ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_INSTALL_UNKNOWN_EXCEPTION_APPLY_CHILD_STEP };
                goto done;
            }

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                // Propagate item's resultDetails to parent.
                workflow_set_result_details(handle, workflow_peek_result_details(stepHandle));
            }

        instanceDone:
            // If the workflow interuption is required as part of the Install action,
            // we must propagate that request to the wrapping workflow.

            if (workflow_is_immediate_reboot_requested(stepHandle))
            {
                workflow_request_immediate_reboot(handle);
                // We must skip the remaining instance(s).
                goto done;
            }

            if (workflow_is_immediate_agent_restart_requested(stepHandle))
            {
                // We must skip the remaining instance(s).
                workflow_request_immediate_agent_restart(handle);
                goto done;
            }

            if (workflow_is_reboot_requested(stepHandle))
            {
                // Continue with the remaining instance(s).
                workflow_request_reboot(handle);
                break;
            }

            if (workflow_is_agent_restart_requested(stepHandle))
            {
                // Continue with the remaining instance(s).
                workflow_request_agent_restart(handle);
                break;
            }

            workflow_set_result(stepHandle, result);
            stepHandle = nullptr;

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                goto componentDone;
            }
        } // steps

    componentDone:
        json_free_serialized_string(serializedComponentString);
        serializedComponentString = nullptr;

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }
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

    json_free_serialized_string(serializedComponentString);
    workflow_free_string(workflowId);
    workflow_free_string(workFolder);

    Log_Debug("Steps_Handler Install end (level %d).", workflowLevel);
    return result;
}

/**
 * @brief Performs 'Install' phase.
 * All files required for installation must be downloaded in to sandbox.
 * During this phase, we will not re-download any file.
 * If file(s) missing, install will be aborted.
 *
 * @return ADUC_Result The install result.
 */
ADUC_Result StepsHandlerImpl::Install(const tagADUC_WorkflowData* workflowData)
{
    return StepsHandler_Install(workflowData);
}

/**
 * @brief This function is no-op because 'Apply' action for every step
 * were invoked inside the 'StepsHandler::Install' function.
 * @return ADUC_Result The result (ADUC_Result_Apply_Success)
 */
static ADUC_Result StepsHandler_Apply(const tagADUC_WorkflowData* workflowData)
{
    UNREFERENCED_PARAMETER(workflowData);
    ADUC_Result result{ ADUC_Result_Apply_Success };
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
    char* workFolder = workflow_get_workfolder(handle);
    JSON_Array* selectedComponentsArray = nullptr;
    int workflowLevel = workflow_get_level(handle);
    int workflowStep = workflow_get_step_index(handle);
    int selectedComponentsCount = 0;
    char* serializedComponentString = nullptr;
    bool isComponentsEnumeratorRegistered = ExtensionManager::IsComponentsEnumeratorRegistered();

    Log_Debug("Evaluating is-installed state of the workflow (level %d, step %d).", workflowLevel, workflowStep);

    int createResult = ADUC_SystemUtils_MkSandboxDirRecursive(workFolder);
    if (createResult != 0)
    {
        Log_Error("Unable to create folder %s, error %d", workFolder, createResult);
        result = { ADUC_Result_Failure, ADUC_ERC_STEPS_HANDLER_CREATE_SANDBOX_FAILURE };
        goto done;
    }

    result = PrepareStepsWorkflowDataObject(handle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        workflow_set_result_details(handle, "Invalid steps workflow collection");
        goto done;
    }

    if (workflowLevel == 0 || !isComponentsEnumeratorRegistered)
    {
        // To top-level step, or if component enumerator is not registered, we will assume that this reference update is
        // interned for the host device and will set selectedComponentCount to 1 to iterate through
        // every step once.
        selectedComponentsCount = 1;
    }
    else
    {
        // This is a reference step (workflowLevel == 1), this intended for one or more components.
        result = GetSelectedComponentsArray(handle, &selectedComponentsArray);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            const char* fmt = "Missing selected components. workflow level %d, step %d";
            Log_Error(fmt, workflowLevel, workflowStep);
            workflow_set_result_details(handle, fmt, workflowLevel, workflowStep);
            goto done;
        }

        selectedComponentsCount = json_array_get_count(selectedComponentsArray);

        if (selectedComponentsCount == 0)
        {
            // If there's no matching component, we will consider this step 'optional' and this
            // step is no-op. Returning 'installed' to skip this step.
            const char* msg = "Optional step (no matching components)";
            Log_Debug(msg);
            result = { ADUC_Result_IsInstalled_Installed };

            // This is a good opportunity to set the workflow state to indicates that
            // the current component is 'optional'.
            // We do this by setting workflow result code to ADUC_Result_Download_Skipped_NoMatchingComponents.
            ADUC_Result currentResult = workflow_get_result(handle);
            if (IsAducResultCodeFailure(currentResult.ResultCode))
            {
                ADUC_Result newResult = { ADUC_Result_Download_Skipped_NoMatchingComponents };
                workflow_set_result(handle, newResult);
                workflow_set_result_details(handle, msg);
            }

            goto done;
        }
    }

    // For each selected component, check whether the update has been installed.
    for (int iCom = 0, stepsCount = workflow_get_children_count(handle); iCom < selectedComponentsCount; iCom++)
    {
        serializedComponentString = CreateComponentSerializedString(selectedComponentsArray, iCom);

        // For each step (child workflow), invoke isInstalled.
        for (int i = 0; i < stepsCount; i++)
        {
            if (IsStepsHandlerExtraDebugLogsEnabled())
            {
                Log_Debug(
                    "Evaluating child step #%d on component #%d.\n#### Component ####\n%s\n###################\n",
                    i,
                    iCom,
                    serializedComponentString);
            }

            // Use a wrapper workflow to hold a stepHandle.
            ADUC_WorkflowData stepWorkflow = {};

            stepHandle = workflow_get_child(handle, i);
            if (stepHandle == nullptr)
            {
                const char* errorFmt = "Cannot process child step #%d due to missing (child) workflow data.";
                Log_Error(errorFmt, i);
                result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_ISINSTALLED_FAILURE_MISSING_CHILD_WORKFLOW;
                workflow_set_result_details(handle, errorFmt, i);
                goto done;
            }
            stepWorkflow.WorkflowHandle = stepHandle;

            // For inline step - set current component info on the workflow.
            if (serializedComponentString != nullptr && workflow_is_inline_step(handle, i))
            {
                if (!workflow_set_selected_components(stepHandle, serializedComponentString))
                {
                    result.ResultCode = ADUC_Result_Failure;
                    result.ExtendedResultCode = ADUC_ERC_STEPS_HANDLER_SET_SELECTED_COMPONENTS_FAILURE;
                    workflow_set_result_details(handle, "Cannot set target component(s) for child step #%d", i);
                    goto done;
                }
            }

            ContentHandler* contentHandler = nullptr;
            const char* stepUpdateType = workflow_is_inline_step(handle, i)
                ? workflow_peek_update_manifest_step_handler(handle, i)
                : DEFAULT_REF_STEP_HANDLER;

            Log_Debug("Loading handler for child step #%d (handler: '%s')", i, stepUpdateType);

            result = ExtensionManager::LoadUpdateContentHandlerExtension(stepUpdateType, &contentHandler);

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                const char* errorFmt = "Cannot load a handler for child step #%d (handler :%s)";
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
                // Cannot determine whether the step is installed, so, let's assume that it's not install.
                result = { .ResultCode = ADUC_Result_IsInstalled_NotInstalled, .ExtendedResultCode = 0 };
            }

            if (IsAducResultCodeFailure(result.ResultCode)
                || result.ResultCode == ADUC_Result_IsInstalled_NotInstalled)
            {
                Log_Info(
                    "Workflow lvl %d, step #%d, child step #%d, component #%d is not installed.",
                    workflowLevel,
                    workflowStep,
                    i,
                    iCom);
                // We can stop here if we found one component that not installed.
                goto done;
            }
        } // steps
    } // components

    result = { ADUC_Result_IsInstalled_Installed };

    {
        // This is a good opportunity to set the workflow state to indicates that
        // the current component is up-to-date with its goal state.
        // We do this by setting workflow result code to ADUC_Result_Apply_Success.
        ADUC_Result currentResult = workflow_get_result(handle);
        if (IsAducResultCodeFailure(currentResult.ResultCode))
        {
            ADUC_Result newResult = { ADUC_Result_Apply_Success };
            workflow_set_result(handle, newResult);
        }
    }

done:

    json_free_serialized_string(serializedComponentString);
    workflow_free_string(workflowId);
    workflow_free_string(workFolder);

    Log_Debug("Workflow lvl %d step #%d is-installed state %d", workflowLevel, workflowStep, result.ResultCode);

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
