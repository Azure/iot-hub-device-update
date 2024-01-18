/**
 * @file adu_integration.h
 * @brief The code in this file is code from gen1 but initially necessary for gen2
 * and has been adapted to work with gen2 where appropriate.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_integration.h"

#include <aduc/calloc_wrapper.hpp>
#include <aduc/content_handler.hpp>
#include <aduc/download_handler_factory.h> // ADUC_DownloadHandlerFactory_LoadDownloadHandler
#include <aduc/download_handler_plugin.h>
#include <aduc/download_handler_plugin.h> // ADUC_DownloadHandlerPlugin_OnUpdateWorkflowCompleted
#include <aduc/extension_manager.hpp>
#include <aduc/logging.h>
#include <aduc/parser_utils.h>
#include <aduc/string_c_utils.h>
#include <aduc/system_utils.h>
#include <aduc/types/update_content.h>
#include <aduc/workflow_data_utils.h>
#include <aduc/workflow_utils.h>

#include <parson.h>

using ADUC::StringUtils::cstr_wrapper;

/**
 * @brief For each update payload that has a DownloadHandlerId, load the handler and call OnUpdateWorkflowCompleted.
 *
 * @param workflowHandle The workflow handle.
 * @details This function will not fail but if a download handler's OnUpdateWorkflowCompleted fails, side effects include logging the error result codes and saving the extended result code that can be reported along with a successful workflow deployment.
 */
void ADU_Integration_CallDownloadHandlerOnUpdateWorkflowCompleted(const ADUC_WorkflowHandle workflowHandle)
{
    size_t payloadCount = workflow_get_update_files_count(workflowHandle);
    for (size_t i = 0; i < payloadCount; ++i)
    {
        ADUC_Result result;
        memset(&result, 0, sizeof(result));
        ADUC_FileEntity fileEntity;
        memset(&fileEntity, 0, sizeof(fileEntity));

        if (!workflow_get_update_file(workflowHandle, i, &fileEntity))
        {
            continue;
        }

        if (IsNullOrEmpty(fileEntity.DownloadHandlerId))
        {
            ADUC_FileEntity_Uninit(&fileEntity);
            continue;
        }

        // NOTE: do not free the handle as it is owned by the DownloadHandlerFactory.
        DownloadHandlerHandle handle = ADUC_DownloadHandlerFactory_LoadDownloadHandler(fileEntity.DownloadHandlerId);
        if (handle == NULL)
        {
            Log_Error("Failed to load download handler.");
        }
        else
        {
            result = ADUC_DownloadHandlerPlugin_OnUpdateWorkflowCompleted(handle, workflowHandle);
            if (IsAducResultCodeFailure(result.ResultCode))
            {
                Log_Warn(
                    "OnupdateWorkflowCompleted, result 0x%08x, erc 0x%08x",
                    result.ResultCode,
                    result.ExtendedResultCode);

                workflow_set_success_erc(workflowHandle, result.ExtendedResultCode);
            }
        }

        ADUC_FileEntity_Uninit(&fileEntity);
    }
}

/**
 * @brief Called when entering Idle state.
 * Idle state is the "ready for new workflow state"
 *
 * @param workflowData Workflow metadata.
 */
void ADU_Integration_Workflow_MethodCall_Idle(ADUC_WorkflowData* workflowData)
{
    ADUCITF_State lastReportedState = ADUC_WorkflowData_GetLastReportedState(workflowData);

    // Can reach Idle state from ApplyStarted as there isn't an ApplySucceeded state.
    if (lastReportedState != ADUCITF_State_Idle && lastReportedState != ADUCITF_State_ApplyStarted
        && lastReportedState != ADUCITF_State_Failed)
    {
        // Likely nothing we can do about this, but try setting Idle state again.
        Log_Warn("Idle UpdateAction called in unexpected state: %s!", ADUCITF_StateToString(lastReportedState));
    }

    //
    // Clean up the sandbox.  It will be re-created when download starts.
    //
    char* workflowId = ADUC_WorkflowData_GetWorkflowId(workflowData);
    char* workFolder = ADUC_WorkflowData_GetWorkFolder(workflowData);

    if (workflowId != NULL)
    {
        Log_Info("UpdateAction: Idle. Ending workflow with WorkflowId: %s", workflowId);
        if (workFolder != NULL)
        {
            Log_Info("Calling SandboxDestroy");
            ADU_Integration_SandboxDestroy(workflowId, workFolder);
        }
    }
    else
    {
        Log_Info("UpdateAction: Idle. WorkFolder is not valid. Nothing to destroy.");
    }

    //
    // Notify callback that we're now back to idle.
    //

    workflow_free_string(workflowId);
    workflow_free_string(workFolder);

    workflow_free(workflowData->WorkflowHandle);
    workflowData->WorkflowHandle = NULL;
}

/**
 * @brief Sets installedUpdateId to the given update ID and sets state to Idle.
 *
 * @param[in,out] workflowData The workflow data.
 * @param[in] updateId The updateId for the installed content.
 */
void ADU_Integration_SetInstalledUpdateIdAndGoToIdle(ADUC_WorkflowData* workflowData, const char* updateId)
{
    ADUC_Result idleResult;
    idleResult.ResultCode = ADUC_Result_Apply_Success;
    idleResult.ExtendedResultCode = 0;

    if (!workflowData->ReportStateAndResultAsyncCallback(
            (ADUC_WorkflowDataToken)workflowData, ADUCITF_State_Idle, &idleResult, updateId))
    {
        Log_Error("Failed to report last installed updateId. Going to idle state.");
    }

    ADUC_WorkflowData_SetLastReportedState(ADUCITF_State_Idle, workflowData);

    if (!ADUC_WorkflowData_SetLastCompletedWorkflowId(workflow_peek_id(workflowData->WorkflowHandle), workflowData))
    {
        Log_Error("Failed to set last completed workflow id. Going to idle state.");
    }

    ADU_Integration_CallDownloadHandlerOnUpdateWorkflowCompleted(workflowData->WorkflowHandle);

    ADU_Integration_Workflow_MethodCall_Idle(workflowData);

    workflowData->SystemRebootState = ADUC_SystemRebootState_None;
    workflowData->AgentRestartState = ADUC_AgentRestartState_None;
}

ContentHandler* ADU_Integration_GetUpdateManifestHandler(const ADUC_WorkflowData* workflowData, ADUC_Result* result)
{
    ContentHandler* contentHandler = nullptr;

    ADUC_Result loadResult = {};

    // Starting from version 4, the top-level update manifest doesn't contains the 'updateType' property.
    // The manifest contains an Instruction (steps) data, which requires special processing.
    // For backward compatibility and avoid code complexity, for V4, we will process the top level update content
    // using 'microsoft/update-manifest:4'
    int updateManifestVersion = workflow_get_update_manifest_version(workflowData->WorkflowHandle);
    if (updateManifestVersion >= 4)
    {
        const cstr_wrapper updateManifestHandler{ ADUC_StringFormat(
            "microsoft/update-manifest:%d", updateManifestVersion) };

        Log_Info(
            "Try to load a handler for current update manifest version %d (handler: '%s')",
            updateManifestVersion,
            updateManifestHandler.get());

        loadResult = ExtensionManager::LoadUpdateContentHandlerExtension(updateManifestHandler.get(), &contentHandler);

        // If handler for the current manifest version is not available,
        // fallback to the V4 default handler.
        if (IsAducResultCodeFailure(loadResult.ResultCode))
        {
            loadResult =
                ExtensionManager::LoadUpdateContentHandlerExtension("microsoft/update-manifest", &contentHandler);
        }
    }
    else
    {
        loadResult.ResultCode = ADUC_Result_Failure;
        loadResult.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_UNSUPPORTED_UPDATE_MANIFEST_VERSION;
    }

    if (IsAducResultCodeFailure(loadResult.ResultCode))
    {
        contentHandler = nullptr;
        *result = loadResult;
    }

    return contentHandler;
}

//
// Reporting
//
static JSON_Status _json_object_set_update_result(
    JSON_Object* object, int32_t resultCode, int32_t extendedResultCode, const char* resultDetails)
{
    JSON_Status status = json_object_set_number(object, ADUCITF_FIELDNAME_RESULTCODE, resultCode);
    if (status != JSONSuccess)
    {
        Log_Error("Could not set value for field: %s", ADUCITF_FIELDNAME_RESULTCODE);
        goto done;
    }

    status = json_object_set_number(object, ADUCITF_FIELDNAME_EXTENDEDRESULTCODE, extendedResultCode);
    if (status != JSONSuccess)
    {
        Log_Error("Could not set value for field: %s", ADUCITF_FIELDNAME_EXTENDEDRESULTCODE);
        goto done;
    }

    if (resultDetails != NULL)
    {
        status = json_object_set_string(object, ADUCITF_FIELDNAME_RESULTDETAILS, resultDetails);
        if (status != JSONSuccess)
        {
            Log_Error("Could not set value for field: %s", ADUCITF_FIELDNAME_RESULTDETAILS);
        }
    }
    else
    {
        status = json_object_set_null(object, ADUCITF_FIELDNAME_RESULTDETAILS);
        if (status != JSONSuccess)
        {
            Log_Error("Could not set field %s to 'null'", ADUCITF_FIELDNAME_RESULTDETAILS);
        }
    }

done:
    return status;
}

/**
 * @brief Get the Reporting Json Value object
 *
 * @param workflowData The workflow data.
 * @param updateState The workflow state machine state.
 * @param result The pointer to the result. If NULL, then the result will be retrieved from the opaque handle object in the workflow data.
 * @param installedUpdateId The installed Update ID string.
 * @return JSON_Value* The resultant json value object. Caller must free using json_value_free().
 */
JSON_Value* ADU_Integration_GetReportingJsonValue(
    ADUC_WorkflowData* workflowData,
    ADUCITF_State updateState,
    const ADUC_Result* result,
    const char* installedUpdateId)
{
    JSON_Value* resultValue = NULL;

    //
    // Get result from current workflow if exists.
    // (Note: on startup, update workflow is not started, unless there is an existing Update Action in the twin.)
    //
    // If not, try to use specified 'result' param.
    //
    // If there's no result details, we'll report only 'updateState'.
    //
    ADUC_Result rootResult;
    ADUC_WorkflowHandle handle = workflowData->WorkflowHandle;
    ADUC_Result_t successErc = workflow_get_success_erc(handle);

    if (result != NULL)
    {
        rootResult = *result;
    }
    else
    {
        rootResult = workflow_get_result(handle);
    }

    // Allow reporting of extended result code of soft-failing mechanisms, such as download handler, that have a
    // fallback mechanism (e.g. full content download) that can ultimately become an overall success.
    if (IsAducResultCodeSuccess(rootResult.ResultCode) && successErc != 0)
    {
        rootResult.ExtendedResultCode = successErc;
    }

    JSON_Value* rootValue = json_value_init_object();
    JSON_Object* rootObject = json_value_get_object(rootValue);
    size_t stepsCount = workflow_get_children_count(handle);

    //
    // Prepare 'lastInstallResult', 'stepResults' data.
    //
    // Example schema:
    //
    // {
    //     "state" : ###,
    //     "workflowId": "...",
    //     "installedUpdateId" : "...",
    //
    //     "lastInstallResult" : {
    //         "resultCode" : ####,
    //         "extendedResultCode" : ####,
    //         "resultDetails" : "...",
    //         "stepResults" : {
    //             "step_0" : {
    //                 "resultCode" : ####,
    //                 "extendedResultCode" : ####,
    //                 "resultDetails" : "..."
    //             },
    //             ...
    //             "step_N" : {
    //                 "resultCode" : ####,
    //                 "extendedResultCode" : ####,
    //                 "resultDetails" : "..."
    //             }
    //         }
    //     }
    // }

    JSON_Value* lastInstallResultValue = json_value_init_object();
    JSON_Object* lastInstallResultObject = json_object(lastInstallResultValue);

    JSON_Value* stepResultsValue = json_value_init_object();
    JSON_Object* stepResultsObject = json_object(stepResultsValue);

    JSON_Status jsonStatus;

    if (lastInstallResultValue == NULL || stepResultsValue == NULL)
    {
        Log_Error("Failed to init object for json value");
        goto done;
    }

    jsonStatus = json_object_set_value(rootObject, ADUCITF_FIELDNAME_LASTINSTALLRESULT, lastInstallResultValue);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_LASTINSTALLRESULT);
        goto done;
    }

    lastInstallResultValue = NULL; // rootObject owns the value now.

    //
    // State
    //
    jsonStatus = json_object_set_number(rootObject, ADUCITF_FIELDNAME_STATE, updateState);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_STATE);
        goto done;
    }

    //
    // WorkflowId top-level property
    //
    if (IsNullOrEmpty(workflow_peek_id(handle)))
    {
        Log_Warn("null or empty workflow id");
    }
    else
    {
        if (json_object_set_string(rootObject, ADUCITF_FIELDNAME_WORKFLOWID, workflow_peek_id(handle)) != JSONSuccess)
        {
            Log_Error("Could not add JSON : %s", ADUCITF_FIELDNAME_WORKFLOWID);
            goto done;
        }
    }

    //
    // Install Update Id
    //
    if (installedUpdateId != NULL)
    {
        jsonStatus = json_object_set_string(rootObject, ADUCITF_FIELDNAME_INSTALLEDUPDATEID, installedUpdateId);
        if (jsonStatus != JSONSuccess)
        {
            Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_INSTALLEDUPDATEID);
            goto done;
        }
    }

    // If reporting 'downloadStarted' or 'ADUCITF_State_DeploymentInProgress' state, we must clear previous 'stepResults' map, if exists.
    if (updateState == ADUCITF_State_DownloadStarted || updateState == ADUCITF_State_DeploymentInProgress)
    {
        if (json_object_set_null(lastInstallResultObject, ADUCITF_FIELDNAME_STEPRESULTS) != JSONSuccess)
        {
            /* Note: continue the 'download' phase if we could not clear the previous results. */
            Log_Warn("Could not clear 'stepResults' property. The property may contains previous install results.");
        }
    }
    // Otherwise, we will only report 'stepResults' property if we have one or more step.
    else if (stepsCount > 0)
    {
        jsonStatus = json_object_set_value(lastInstallResultObject, ADUCITF_FIELDNAME_STEPRESULTS, stepResultsValue);
        if (jsonStatus != JSONSuccess)
        {
            Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_STEPRESULTS);
            goto done;
        }

        stepResultsValue = NULL; // rootObject owns the value now.
    }

    //
    // Report both state and result
    //

    // Set top-level update state and result.
    jsonStatus = _json_object_set_update_result(
        lastInstallResultObject,
        rootResult.ResultCode,
        rootResult.ExtendedResultCode,
        workflow_peek_result_details(handle));

    if (jsonStatus != JSONSuccess)
    {
        goto done;
    }

    // Report all steps result.
    if (updateState != ADUCITF_State_DownloadStarted)
    {
        stepsCount = workflow_get_children_count(handle);
        for (size_t i = 0; i < stepsCount; i++)
        {
            ADUC_WorkflowHandle childHandle = workflow_get_child(handle, i);
            ADUC_Result childResult;
            JSON_Value* childResultValue = NULL;
            JSON_Object* childResultObject = NULL;
            STRING_HANDLE childUpdateId = NULL;

            if (childHandle == NULL)
            {
                Log_Error("Could not get components #%d update result", i);
                continue;
            }

            childResult = workflow_get_result(childHandle);

            childResultValue = json_value_init_object();
            childResultObject = json_object(childResultValue);
            if (childResultValue == NULL)
            {
                Log_Error("Could not create components update result #%d", i);
                goto childDone;
            }

            // Note: IoTHub twin doesn't support some special characters in a map key (e.g. ':', '-').
            // Let's name the result using "step_" +  the array index.
            childUpdateId = STRING_construct_sprintf("step_%d", i);
            if (childUpdateId == NULL)
            {
                Log_Error("Could not create proper child update id result key.");
                goto childDone;
            }

            jsonStatus = json_object_set_value(stepResultsObject, STRING_c_str(childUpdateId), childResultValue);
            if (jsonStatus != JSONSuccess)
            {
                Log_Error("Could not add step #%d update result", i);
                goto childDone;
            }
            childResultValue = NULL; // stepResultsValue owns it now.

            jsonStatus = _json_object_set_update_result(
                childResultObject,
                childResult.ResultCode,
                childResult.ExtendedResultCode,
                workflow_peek_result_details(childHandle));

            if (jsonStatus != JSONSuccess)
            {
                goto childDone;
            }

        childDone:
            STRING_delete(childUpdateId);
            childUpdateId = NULL;
            json_value_free(childResultValue);
            childResultValue = NULL;
        }
    }

    resultValue = rootValue;
    rootValue = NULL;

done:
    json_value_free(rootValue);
    json_value_free(lastInstallResultValue);
    json_value_free(stepResultsValue);

    return resultValue;
}

char* ADU_Integration_GetReportingJson(
    ADUC_WorkflowData* workflowData,
    ADUCITF_State updateState,
    const ADUC_Result* result,
    const char* installedUpdateId)
{
    char* jsonString = NULL;
    JSON_Value* rootValue =
        ADU_Integration_GetReportingJsonValue(workflowData, updateState, result, installedUpdateId);
    if (rootValue == NULL)
    {
        Log_Error("Failed to get reporting json value");
        goto done;
    }

    jsonString = json_serialize_to_string(rootValue);
    if (jsonString == NULL)
    {
        Log_Error("Serializing JSON to string failed");
        goto done;
    }

done:

    return jsonString;
}

void ADU_Integration_SandboxDestroy(const char* workflowId, const char* workFolder)
{
    // If SandboxCreate failed or didn't specify a workfolder, we'll get nullptr here.
    if (workFolder == nullptr)
    {
        return;
    }

    Log_Info("Destroying sandbox %s. workflowId: %s", workFolder, workflowId);

    struct stat st = {};
    bool statOk = stat(workFolder, &st) == 0;
    if (statOk && S_ISDIR(st.st_mode))
    {
        int ret = ADUC_SystemUtils_RmDirRecursive(workFolder);
        if (ret != 0)
        {
            // Not a fatal error.
            Log_Error("Unable to remove sandbox, error %d", ret);
        }
    }
    else
    {
        Log_Info("Can not access folder '%s', or doesn't exist. Ignored...", workFolder);
    }
}

void ADU_Integration_Install_Complete(ADUC_WorkflowData* workflowData)
{
    if (workflow_is_immediate_reboot_requested(workflowData->WorkflowHandle)
        || workflow_is_reboot_requested(workflowData->WorkflowHandle))
    {
        // If 'install' indicated a reboot required result from apply, go ahead and reboot.
        Log_Info("Install indicated success with RebootRequired - rebooting system now");
        workflowData->SystemRebootState = ADUC_SystemRebootState_Required;

        Log_Info("Calling ADUC_RebootSystem");
        if (ADUC_RebootSystem() == 0)
        {
            workflowData->SystemRebootState = ADUC_SystemRebootState_InProgress;
        }
        else
        {
            Log_Error("Reboot attempt failed.");
            workflow_set_operation_in_progress(workflowData->WorkflowHandle, false);
        }
    }
    else if (
        workflow_is_immediate_agent_restart_requested(workflowData->WorkflowHandle)
        || workflow_is_agent_restart_requested(workflowData->WorkflowHandle))
    {
        // If 'install' indicated a restart is required, go ahead and restart the agent.
        Log_Info("Install indicated success with AgentRestartRequired - restarting the agent now");
        workflowData->SystemRebootState = ADUC_SystemRebootState_Required;

        Log_Info("Calling ADUC_RestartAgent");
        if (ADUC_RestartAgent() == 0)
        {
            workflowData->AgentRestartState = ADUC_AgentRestartState_InProgress;
        }
        else
        {
            Log_Error("Agent restart attempt failed.");
            workflow_set_operation_in_progress(workflowData->WorkflowHandle, false);
        }
    }
}

void ADU_Integration_Apply_Complete(ADUC_WorkflowData* workflowData, ADUC_Result result)
{
    if (workflow_is_immediate_reboot_requested(workflowData->WorkflowHandle)
        || workflow_is_reboot_requested(workflowData->WorkflowHandle))
    {
        // If apply indicated a reboot required result from apply, go ahead and reboot.
        Log_Info("Apply indicated success with RebootRequired - rebooting system now");
        workflowData->SystemRebootState = ADUC_SystemRebootState_Required;

        if (ADUC_RebootSystem() == 0)
        {
            workflowData->SystemRebootState = ADUC_SystemRebootState_InProgress;
        }
        else
        {
            Log_Error("Reboot attempt failed.");
            workflow_set_operation_in_progress(workflowData->WorkflowHandle, false);
        }
    }
    else if (
        workflow_is_immediate_agent_restart_requested(workflowData->WorkflowHandle)
        || workflow_is_agent_restart_requested(workflowData->WorkflowHandle))
    {
        // If apply indicated a restart is required, go ahead and restart the agent.
        Log_Info("Apply indicated success with AgentRestartRequired - restarting the agent now");
        workflowData->SystemRebootState = ADUC_SystemRebootState_Required;

        if (ADUC_RestartAgent() == 0)
        {
            workflowData->AgentRestartState = ADUC_AgentRestartState_InProgress;
        }
        else
        {
            Log_Error("Agent restart attempt failed.");
            workflow_set_operation_in_progress(workflowData->WorkflowHandle, false);
        }
    }
    else if (result.ResultCode == ADUC_Result_Apply_Success)
    {
        // An Apply action completed successfully. Continue to the next step.
        workflow_set_operation_in_progress(workflowData->WorkflowHandle, false);
    }
}
