/**
 * @file bundle_handler.cpp
 * @brief Implementation of ContentHandler API for 'microsoft/bundle:1' update type.
 *
 * @copyright Copyright (c), Microsoft Corporation.
 */
#include "aduc/bundle_handler.hpp"

#include "aduc/adu_core_exports.h"
#include "aduc/component_enumerator_extension.hpp"
#include "aduc/extension_manager.hpp"
#include "aduc/extension_utils.h"
#include "aduc/installed_criteria_utils.hpp"
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

EXTERN_C_BEGIN

/**
 * @brief Instantiates an Update Content Handler for 'microsoft/bundle:1' update type.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel)
{
    ADUC_Logging_Init(logLevel, "bundle-handler");
    Log_Info("Instantiating an Update Content Handler for 'microsoft/bundle:1'");
    try
    {
        return BundleHandlerImpl::CreateContentHandler();
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
 * @brief Creates a new BundleHandlerImpl object and casts to a ContentHandler.
 * Note that there is no way to create a BundleHandlerImpl directly.
 *
 * @return ContentHandler* BundleHandlerImpl object as a ContentHandler.
 */
ContentHandler* BundleHandlerImpl::CreateContentHandler()
{
    return new BundleHandlerImpl();
}

/**
 * @brief Performs 'Download' task by downloading all 'Components-Update' manifest files.
 * For each manifest file, invoke a Component Update Handler to performs 'Download' action.
 * 
 * @return ADUC_Result The result.
 */
ADUC_Result BundleHandlerImpl::Download(const ADUC_WorkflowData* workflowData)
{
    Log_Debug("Bundle_Handler download task begin.");
    ADUC_Result result{ ADUC_Result_Failure };

    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    ADUC_WorkflowHandle childHandle = nullptr;

    auto fileCount = static_cast<unsigned int>(workflow_get_bundle_updates_count(handle));
    char* workflowId = workflow_get_id(handle);
    char* workingFolder = workflow_get_workfolder(handle);
    char* compatibilityString = nullptr;
    STRING_HANDLE leafManifestFile = nullptr;
    STRING_HANDLE childId = nullptr;

    for (size_t i = 0; i < fileCount; i++)
    {
        ADUC_FileEntity* entity = nullptr;
        if (!workflow_get_bundle_updates_file(handle, i, &entity))
        {
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_BUNDLE_CONTENT_HANDLER_GET_FILE_ENTITY_FAILURE };
            goto done;
        }

        Log_Info("Downloading Components-Update manifest file#%d (id:%s).", i, entity->FileId);

        try
        {
            result = ExtensionManager::Download(entity, workflowId, workingFolder, DO_RETRY_TIMEOUT_DEFAULT, nullptr);
        }
        catch (...)
        {
            result.ExtendedResultCode = ADUC_ERC_UPDATE_CONTENT_HANDLER_DOWNLOAD_FAILURE_UNKNOWNEXCEPTION;
        }

        workflow_free_file_entity(entity);

        // For 'microsoft/bundle:1' implementation, abort download task as soon as an error occurs.
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }
    }

    //
    // For each components manifest, invoke download action.
    //
    for (int i = 0; i < fileCount; i++)
    {
        // Container workflow to hold a childHandle.
        ADUC_WorkflowData componentWorkflow;
        int selectedComponentCount = 0;
        ADUC_FileEntity* entity = nullptr;
        if (!workflow_get_bundle_updates_file(handle, i, &entity))
        {
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_BUNDLE_CONTENT_HANDLER_GET_FILE_ENTITY_FAILURE };
            goto done;
        }

        leafManifestFile = STRING_construct_sprintf("%s/%s", workingFolder, entity->TargetFilename);

        workflow_free_file_entity(entity);
        entity = nullptr;

        if (leafManifestFile == nullptr)
        {
            goto done;
        }

        // Create child workflow.
        result = workflow_init_from_file(STRING_c_str(leafManifestFile), false, &childHandle);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }

        childId = STRING_construct_sprintf("%d", i);
        workflow_set_id(childHandle, STRING_c_str(childId));

        if (!workflow_insert_child(handle, -1, childHandle))
        {
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_BUNDLE_HANDLER_CREATE_LEAF_WORKFLOW_CANT_ADD_TO_PARENT };
            goto done;
        }

        // Using same core functions.
        componentWorkflow.WorkflowHandle = childHandle;

        Log_Info(
            "Loading handler for leaf update ('%s')",
            workflow_peek_update_manifest_string(childHandle, ADUCITF_FIELDNAME_UPDATETYPE));

        ContentHandler* contentHandler = nullptr;
        result = ContentHandlerFactory::LoadUpdateContentHandlerExtension(
            workflow_peek_update_manifest_string(childHandle, ADUCITF_FIELDNAME_UPDATETYPE), &contentHandler);

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            Log_Error("Cannot load Update Content Handler for components-update #%d", i);
            goto done;
        }

        // NOTE: For MCU V1. We're only matching the first compatibility map.
        compatibilityString = workflow_get_update_manifest_compatibility(childHandle, 0);
        if (compatibilityString == nullptr)
        {
            Log_Error("Cannot get compatibility info for components-update #%d", i);
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_BUNDLE_CONTENT_HANDLER_GET_LEAF_COMPAT_FAILURE };
            goto done;
        }

        std::string output;
        result = ExtensionManager::SelectComponents(compatibilityString, output);

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            Log_Error("Cannot select components for components-update #%d", i);
            goto done;
        }

        {
            JSON_Value* compsValue = json_parse_string(output.c_str());
            selectedComponentCount =
                json_array_get_count(json_object_get_array(json_object(compsValue), "components"));
            json_value_free(compsValue);
        }

        if (!workflow_set_selected_components(childHandle, output.c_str()))
        {
            result.ResultCode = ADUC_Result_Failure;
            result.ExtendedResultCode = ADUC_ERC_BUNDLE_CONTENT_HANDLER_SET_SELECTED_COMPONENTS_FAILURE;
        }

        Log_Debug("Selected components: %s", output.c_str());

        // Only download if there's matching component(s).
        if (selectedComponentCount > 0)
        {
            // Download files requried for components update.
            try
            {
                result = contentHandler->Download(&componentWorkflow);
            }
            catch (...)
            {
                result.ExtendedResultCode = ADUC_ERC_BUNDLE_CONTENT_HANDLER_DOWNLOAD_FAILURE_UNKNOWNEXCEPTION;
            }

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                goto done;
            }
        }
        else
        {
            // This leaf update is meant for component(s) that hasn't yet connected or regcoginzed by the Device Enumerator.
            // We need to report that the download has been skipped.
            result = { .ResultCode = ADUC_Result_Download_Skipped_NoMatchingComponents };

            // And save the component workflow state.
            char* updateId = workflow_get_expected_update_id_string(handle);
            workflow_set_result(componentWorkflow.WorkflowHandle, result);
            workflow_set_result_details(
                componentWorkflow.WorkflowHandle, "No matching components. (%s)", compatibilityString);
            workflow_set_state(componentWorkflow.WorkflowHandle, ADUCITF_State_DownloadSucceeded);
            workflow_free_string(updateId);
        }

        workflow_free_string(compatibilityString);
        compatibilityString = nullptr;
        STRING_delete(leafManifestFile);
        leafManifestFile = nullptr;
        STRING_delete(childId);
        childId = nullptr;
    }

    result = { ADUC_Result_Download_Success };

done:
    STRING_delete(leafManifestFile);
    leafManifestFile = nullptr;
    STRING_delete(childId);
    childId = nullptr;

    workflow_free_string(compatibilityString);

    // NOTE: Do not free child workflow here, so that it can be reused in the next phase.
    // Only free child handle when the workflow is done.

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

    Log_Debug("Bundle_Handler download task end.");
    return result;
}

/**
 * @brief Make sure that all child components update workflows are created.
 * 
 * @param handle A workflow data object handle.
 * @return ADUC_Result 
 */
ADUC_Result EnsureComponentsWorkflowsCreated(const ADUC_WorkflowHandle handle)
{
    ADUC_Result result{ ADUC_Result_Failure };
    ADUC_WorkflowHandle childHandle = nullptr;

    auto fileCount = static_cast<unsigned int>(workflow_get_bundle_updates_count(handle));
    char* workFolder = workflow_get_workfolder(handle);
    unsigned int childWorkflowCount = workflow_get_children_count(handle);
    int totalInstalledCount = 0;

    // Child workflow should be either 0 (resuming i(nstall phase after agent restarted),
    // or equal to fileCount (already created children during download phase)
    if (childWorkflowCount != fileCount)
    {
        // Remove existing child workflow handle(s)
        while (workflow_get_children_count(handle) > 0)
        {
            ADUC_WorkflowHandle child = workflow_remove_child(handle, 0);
            workflow_free(child);
        }

        for (unsigned int i = 0; i < fileCount; i++)
        {
            STRING_HANDLE childId = nullptr;
            ADUC_FileEntity* entity = nullptr;

            if (!workflow_get_bundle_updates_file(handle, i, &entity))
            {
                result = { .ResultCode = ADUC_Result_Failure,
                           .ExtendedResultCode = ADUC_ERC_BUNDLE_CONTENT_HANDLER_GET_FILE_ENTITY_FAILURE };
                goto done;
            }

            std::stringstream leafManifestFile;
            leafManifestFile << workFolder << "/" << entity->TargetFilename;

            workflow_free_file_entity(entity);
            entity = nullptr;

            // Create child workflow.
            result = workflow_init_from_file(leafManifestFile.str().c_str(), false, &childHandle);

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                goto done;
            }

            childId = STRING_construct_sprintf("%d", i);
            workflow_set_id(childHandle, STRING_c_str(childId));
            STRING_delete(childId);
            childId = nullptr;

            if (!workflow_insert_child(handle, -1, childHandle))
            {
                result.ExtendedResultCode = ADUC_ERC_BUNDLE_HANDLER_CREATE_LEAF_WORKFLOW_CANT_ADD_TO_PARENT;
                goto done;
            }
        }
    }
    result = { ADUC_Result_Success };

done:
    workflow_free_string(workFolder);
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
ADUC_Result BundleHandlerImpl::Install(const ADUC_WorkflowData* workflowData)
{
    Log_Debug("Bundle 'install' phase begin.");
    ADUC_Result result{ ADUC_Result_Failure };

    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    ADUC_WorkflowHandle childHandle = nullptr;

    auto fileCount = static_cast<unsigned int>(workflow_get_bundle_updates_count(handle));
    char* workflowId = workflow_get_id(handle);
    char* workingFolder = workflow_get_workfolder(handle);
    char* compatibilityString = nullptr;

    result = EnsureComponentsWorkflowsCreated(handle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    //
    // For each components manifest, invoke install and apply actions.
    //
    for (int i = 0; i < fileCount; i++)
    {
        // Dummy workflow to hold a childHandle.
        ADUC_WorkflowData componentWorkflow;

        childHandle = workflow_get_child(handle, i);
        if (childHandle == nullptr)
        {
            Log_Error("Cannot install components update #&d due to missing (child) workflow data.", i);
            result.ExtendedResultCode = ADUC_ERC_BUNDLE_CONTENT_HANDLER_INSTALL_FAILURE_MISSING_CHILD_WORKFLOW;
            goto done;
        }

        // Using same core functions.
        componentWorkflow.WorkflowHandle = childHandle;

        Log_Info(
            "Loading handler for leaf update ('%s')",
            workflow_peek_update_manifest_string(childHandle, ADUCITF_FIELDNAME_UPDATETYPE));

        ContentHandler* contentHandler = nullptr;
        const char* childUpdateType = workflow_peek_update_manifest_string(childHandle, ADUCITF_FIELDNAME_UPDATETYPE);
        result = ContentHandlerFactory::LoadUpdateContentHandlerExtension(childUpdateType, &contentHandler);

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            const char* errorFmt = "Cannot load Update Content Handler for components-update #%d, updateType:%s";
            Log_Error(errorFmt, i, childUpdateType);
            workflow_set_result_details(handle, errorFmt, i, childUpdateType == nullptr ? "NULL" : childUpdateType);
            goto done;
        }

        // NOTE: For MCU V1. We're only matching the first compatibility map.
        compatibilityString = workflow_get_update_manifest_compatibility(childHandle, 0);
        if (compatibilityString == nullptr)
        {
            Log_Error("Cannot get compatibility info for components-update #%d", i);
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_BUNDLE_CONTENT_HANDLER_GET_LEAF_COMPAT_FAILURE };
            goto done;
        }

        std::string output;
        result = ExtensionManager::SelectComponents(compatibilityString, output);

        if (!workflow_set_selected_components(childHandle, output.c_str()))
        {
            result.ResultCode = ADUC_Result_Failure;
            result.ExtendedResultCode = ADUC_ERC_BUNDLE_CONTENT_HANDLER_SET_SELECTED_COMPONENTS_FAILURE;
            goto done;
        }

        Log_Debug("Selected components: %s", output.c_str());

        // Perform 'install' action.
        try
        {
            result = contentHandler->Install(&componentWorkflow);
        }
        catch (...)
        {
            result.ExtendedResultCode = ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_UNKNOWNEXCEPTION;
        }

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            // Propagate components' resultDetails.
            workflow_set_result_details(handle, workflow_peek_result_details(componentWorkflow.WorkflowHandle));
            goto done;
        }

        workflow_free_string(compatibilityString);
        compatibilityString = nullptr;

        if (workflow_is_immediate_reboot_requested(childHandle)
            || workflow_is_immediate_agent_restart_requested(childHandle))
        {
            Log_Info("Device reboot or Agent restart required. Stopping current update progress...");
            goto done;
        }
    }

    result = { ADUC_Result_Install_Success };

done:

    // NOTE: Do not free child workflow here, so that it can be reused in the next phase.
    // Only free child handle when the workflow is done.
    //
    // Alternatively, we can persist child workflow state, to free up some memery, and
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

    Log_Debug("Bundle_Handler install task end.");
    return result;
}

/**
 * @brief Perform 'Apply' action.
 * @return ADUC_Result The result (always success)
 */
ADUC_Result BundleHandlerImpl::Apply(const ADUC_WorkflowData* workflowData)
{
    Log_Debug("Bundle 'apply' phase begin.");
    ADUC_Result result{ ADUC_Result_Failure };

    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    ADUC_WorkflowHandle childHandle = nullptr;

    auto fileCount = static_cast<unsigned int>(workflow_get_bundle_updates_count(handle));
    char* workflowId = workflow_get_id(handle);
    char* workingFolder = workflow_get_workfolder(handle);
    char* compatibilityString = nullptr;
    char* installedCriteria = workflow_get_installed_criteria(handle);
    int totalInstalledCount = 0;

    result = EnsureComponentsWorkflowsCreated(handle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    //
    // For each components manifest, invoke install and apply actions.
    //
    for (int i = 0; i < fileCount; i++)
    {
        // Dummy workflow to hold a childHandle.
        ADUC_WorkflowData componentWorkflow;

        childHandle = workflow_get_child(handle, i);
        if (childHandle == nullptr)
        {
            Log_Error("Cannot install components update #&d due to missing (child) workflow data.", i);
            result.ExtendedResultCode = ADUC_ERC_BUNDLE_CONTENT_HANDLER_INSTALL_FAILURE_MISSING_CHILD_WORKFLOW;
            goto done;
        }

        // Using same core functions.
        componentWorkflow.WorkflowHandle = childHandle;

        Log_Info(
            "Loading handler for leaf update ('%s')",
            workflow_peek_update_manifest_string(childHandle, ADUCITF_FIELDNAME_UPDATETYPE));

        ContentHandler* contentHandler = nullptr;
        result = ContentHandlerFactory::LoadUpdateContentHandlerExtension(
            workflow_peek_update_manifest_string(childHandle, ADUCITF_FIELDNAME_UPDATETYPE), &contentHandler);

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            Log_Error("Cannot load Update Content Handler for components-update #%d", i);
            goto done;
        }

        // NOTE: For MCU V1. We're only matching the first compatibility map.
        compatibilityString = workflow_get_update_manifest_compatibility(childHandle, 0);
        if (compatibilityString == nullptr)
        {
            Log_Error("Cannot get compatibility info for components-update #%d", i);
            result = { .ResultCode = ADUC_Result_Failure,
                       .ExtendedResultCode = ADUC_ERC_BUNDLE_CONTENT_HANDLER_GET_LEAF_COMPAT_FAILURE };
            goto done;
        }

        std::string output;
        result = ExtensionManager::SelectComponents(compatibilityString, output);

        if (!workflow_set_selected_components(childHandle, output.c_str()))
        {
            result.ResultCode = ADUC_Result_Failure;
            result.ExtendedResultCode = ADUC_ERC_BUNDLE_CONTENT_HANDLER_SET_SELECTED_COMPONENTS_FAILURE;
            goto done;
        }

        // Perform 'apply' action.
        try
        {
            result = contentHandler->Apply(&componentWorkflow);
        }
        catch (...)
        {
            result.ExtendedResultCode = ADUC_ERC_UPDATE_CONTENT_HANDLER_INSTALL_FAILURE_UNKNOWNEXCEPTION;
        }

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }

        workflow_free_string(compatibilityString);
        compatibilityString = nullptr;
    }

    if (!PersistInstalledCriteria(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria))
    {
        result = { .ResultCode = ADUC_Result_Failure,
                   .ExtendedResultCode = ADUC_ERC_BUNDLE_HANDLER_INSTALLCRITERIA_PERSIST_FAILURE };
        goto done;
    }

    result = { ADUC_Result_Apply_Success };

done:

    // NOTE: Do not free child workflow here, so that it can be reused in the next phase.
    // Only free child handle when the workflow is done.
    //
    // Alternatively, we can persist child workflow state, to free up some memery, and
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
    workflow_free_string(installedCriteria);

    Log_Debug("Bundle_Handler apply task end.");
    return result;
}

/**
 * @brief Perform 'Cancel' action
 */
ADUC_Result BundleHandlerImpl::Cancel(const ADUC_WorkflowData* workflowData)
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
ADUC_Result BundleHandlerImpl::IsInstalled(const ADUC_WorkflowData* workflowData)
{
    char* installedCriteria = workflow_get_installed_criteria(workflowData->WorkflowHandle);
    ADUC_Result result = GetIsInstalled(ADUC_INSTALLEDCRITERIA_FILE_PATH, installedCriteria);
    workflow_free_string(installedCriteria);
    return result;
}