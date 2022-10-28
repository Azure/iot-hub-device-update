/**
 * @file adu_core_interface.c
 * @brief Methods to communicate with "urn:azureiot:AzureDeviceUpdateCore:1" interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_core_interface.h"
#include "aduc/adu_core_export_helpers.h" // ADUC_Workflow_SetUpdateStateWithResult
#include "aduc/agent_orchestration.h"
#include "aduc/agent_workflow.h"
#include "aduc/c_utils.h"
#include "aduc/client_handle_helper.h"
#include "aduc/config_utils.h"
#include "aduc/d2c_messaging.h"
#include "aduc/hash_utils.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h"
#include "aduc/types/update_content.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_utils.h"

#include "startup_msg_helper.h"

#include <azure_c_shared_utility/strings.h> // STRING_*
#include <iothub_client_version.h>
#include <parson.h>
#include <pnp_protocol.h>

// Name of an Device Update Agent component that this device implements.
static const char g_aduPnPComponentName[] = "deviceUpdate";

// Name of properties that Device Update Agent component supports.

// This is the device-to-cloud property.
// An agent communicates its state and other data to ADU Management service by reporting this property to IoTHub.
static const char g_aduPnPComponentAgentPropertyName[] = "agent";

// This is the cloud-to-device property.
// ADU Management send an 'Update Action' to this device by setting this property on IoTHub.
static const char g_aduPnPComponentServicePropertyName[] = "service";

/**
 * @brief Handle for Device Update Agent component to communication to service.
 */
ADUC_ClientHandle g_iotHubClientHandleForADUComponent;

/**
 * @brief This function is called when the message is no longer being process.
 *
 * @param context The ADUC_D2C_Message object
 * @param status The message status.
 */
static void OnUpdateResultD2CMessageCompleted(void* context, ADUC_D2C_Message_Status status)
{
    UNREFERENCED_PARAMETER(context);
    Log_Debug("Send message completed (status:%d)", status);
}

/**
 * @brief Initialize a ADUC_WorkflowData object.
 *
 * @param[out] workflowData Workflow metadata.
 * @param argc Count of arguments in @p argv
 * @param argv Command line parameters.
 * @return _Bool True on success.
 */
_Bool ADUC_WorkflowData_Init(ADUC_WorkflowData* workflowData, int argc, char** argv)
{
    _Bool succeeded = false;

    memset(workflowData, 0, sizeof(*workflowData));

    ADUC_Result result = ADUC_MethodCall_Register(&(workflowData->UpdateActionCallbacks), argc, (const char**)argv);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ADUC_RegisterPlatformLayer failed %d, %d", result.ResultCode, result.ExtendedResultCode);
        goto done;
    }

    // Only call Unregister if register succeeded.
    workflowData->IsRegistered = true;

    workflowData->DownloadProgressCallback = ADUC_Workflow_DefaultDownloadProgressCallback;

    workflowData->ReportStateAndResultAsyncCallback = AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync;

    workflowData->LastCompletedWorkflowId = NULL;

    workflow_set_cancellation_type(workflowData->WorkflowHandle, ADUC_WorkflowCancellationType_None);

    succeeded = true;

done:
    return succeeded;
}

/**
 * @brief Free members of ADUC_WorkflowData object.
 *
 * @param workflowData Object whose members should be freed.
 */
void ADUC_WorkflowData_Uninit(ADUC_WorkflowData* workflowData)
{
    if (workflowData == NULL)
    {
        return;
    }

    if (workflowData->IsRegistered)
    {
        ADUC_MethodCall_Unregister(&(workflowData->UpdateActionCallbacks));
    }

    workflow_free_string(workflowData->LastCompletedWorkflowId);
    memset(workflowData, 0, sizeof(*workflowData));
}

/**
 * @brief Reports the client json via PnP so it ends up in the reported section of the twin.
 *
 * @param json_value The json value to be reported.
 * @param workflowData The workflow data.
 * @return _Bool true if call succeeded.
 */
static _Bool ReportClientJsonProperty(const char* json_value, ADUC_WorkflowData* workflowData)
{
    _Bool success = false;

    if (g_iotHubClientHandleForADUComponent == NULL)
    {
        Log_Error("ReportClientJsonProperty called with invalid IoTHub Device Client handle! Can't report!");
        return false;
    }

    STRING_HANDLE jsonToSend =
        PnP_CreateReportedProperty(g_aduPnPComponentName, g_aduPnPComponentAgentPropertyName, json_value);

    if (jsonToSend == NULL)
    {
        Log_Error("Unable to create Reported property for ADU client.");
        goto done;
    }

    if (!ADUC_D2C_Message_SendAsync(
            ADUC_D2C_Message_Type_Device_Update_Result,
            &g_iotHubClientHandleForADUComponent,
            STRING_c_str(jsonToSend),
            NULL /* responseCallback */,
            OnUpdateResultD2CMessageCompleted,
            NULL /* statusChangedCallback */,
            NULL /* userData */))
    {
        Log_Error("Unable to send update result.");
        goto done;
    }

    success = true;

done:
    STRING_delete(jsonToSend);

    return success;
}

/**
 * @brief Reports values to the cloud which do not change throughout ADUs execution
 * @details the current expectation is to report these values after the successful
 * connection of the AzureDeviceUpdateCoreInterface
 * @param workflowData the workflow data.
 * @returns true when the report is sent and false when reporting fails.
 */
_Bool ReportStartupMsg(ADUC_WorkflowData* workflowData)
{
    if (g_iotHubClientHandleForADUComponent == NULL)
    {
        Log_Error("ReportStartupMsg called before registration! Can't report!");
        return false;
    }

    _Bool success = false;

    char* jsonString = NULL;

    JSON_Value* startupMsgValue = json_value_init_object();

    if (startupMsgValue == NULL)
    {
        goto done;
    }

    JSON_Object* startupMsgObj = json_value_get_object(startupMsgValue);

    if (startupMsgObj == NULL)
    {
        goto done;
    }

    ADUC_ConfigInfo config = {};

    if (!ADUC_ConfigInfo_Init(&config, ADUC_CONF_FILE_PATH))
    {
        goto done;
    }

    const ADUC_AgentInfo* agent = ADUC_ConfigInfo_GetAgent(&config, 0);

    if (!StartupMsg_AddDeviceProperties(startupMsgObj, agent))
    {
        Log_Error("Could not add Device Properties to the startup message");
        goto done;
    }

    if (!StartupMsg_AddCompatPropertyNames(startupMsgObj, &config))
    {
        Log_Error("Could not add compatPropertyNames to the startup message");
        goto done;
    }

    jsonString = json_serialize_to_string(startupMsgValue);

    if (jsonString == NULL)
    {
        Log_Error("Serializing JSON to string failed!");
        goto done;
    }

    ReportClientJsonProperty(jsonString, workflowData);

    success = true;
done:
    json_value_free(startupMsgValue);
    json_free_serialized_string(jsonString);

    ADUC_ConfigInfo_UnInit(&config);
    return success;
}

//
// AzureDeviceUpdateCoreInterface methods
//

_Bool AzureDeviceUpdateCoreInterface_Create(void** context, int argc, char** argv)
{
    _Bool succeeded = false;

    ADUC_WorkflowData* workflowData = calloc(1, sizeof(ADUC_WorkflowData));
    if (workflowData == NULL)
    {
        goto done;
    }

    Log_Info("ADUC agent started. Using IoT Hub Client SDK %s", IoTHubClient_GetVersionString());

    if (!ADUC_WorkflowData_Init(workflowData, argc, argv))
    {
        Log_Error("Workflow data initialization failed");
        goto done;
    }

    succeeded = true;

done:

    if (!succeeded)
    {
        ADUC_WorkflowData_Uninit(workflowData);
        free(workflowData);
        workflowData = NULL;
    }

    // Set out parameter.
    *context = workflowData;

    return succeeded;
}

void AzureDeviceUpdateCoreInterface_Connected(void* componentContext)
{
    ADUC_WorkflowData* workflowData = (ADUC_WorkflowData*)componentContext;

    if (workflowData->WorkflowHandle == NULL)
    {
        // Only perform startup logic here, if no workflows has been created.
        ADUC_Workflow_HandleStartupWorkflowData(workflowData);
    }

    if (!ReportStartupMsg(workflowData))
    {
        Log_Warn("ReportStartupMsg failed");
    }
}

void AzureDeviceUpdateCoreInterface_DoWork(void* componentContext)
{
    ADUC_WorkflowData* workflowData = (ADUC_WorkflowData*)componentContext;
    ADUC_Workflow_DoWork(workflowData);
}

void AzureDeviceUpdateCoreInterface_Destroy(void** componentContext)
{
    ADUC_WorkflowData* workflowData = (ADUC_WorkflowData*)(*componentContext);

    Log_Info("ADUC agent stopping");

    ADUC_WorkflowData_Uninit(workflowData);
    free(workflowData);

    *componentContext = NULL;
}

void OrchestratorUpdateCallback(
    ADUC_ClientHandle clientHandle,
    JSON_Value* propertyValue,
    int propertyVersion,
    ADUC_PnPComponentClient_PropertyUpdate_Context* sourceContext,
    void* context)
{
    ADUC_WorkflowData* workflowData = (ADUC_WorkflowData*)context;
    STRING_HANDLE jsonToSend = NULL;

    // Reads out the json string so we can Log Out what we've got.
    // The value will be parsed and handled in ADUC_Workflow_HandlePropertyUpdate.
    char* jsonString = json_serialize_to_string(propertyValue);
    if (jsonString == NULL)
    {
        Log_Error(
            "OrchestratorUpdateCallback failed to convert property JSON value to string, property version (%d)",
            propertyVersion);
        goto done;
    }

    // To reduce TWIN size, remove UpdateManifestSignature and fileUrls before ACK.
    char* ackString = NULL;
    JSON_Object* signatureObj = json_value_get_object(propertyValue);
    if (signatureObj != NULL)
    {
        json_object_set_null(signatureObj, "updateManifestSignature");
        json_object_set_null(signatureObj, "fileUrls");
        ackString = json_serialize_to_string(propertyValue);
    }

    Log_Debug("Update Action info string (%s), property version (%d)", ackString, propertyVersion);

    ADUC_Workflow_HandlePropertyUpdate(workflowData, (const unsigned char*)jsonString, sourceContext->forceUpdate);
    free(jsonString);
    jsonString = ackString;

    // ACK the request.
    jsonToSend = PnP_CreateReportedPropertyWithStatus(
        g_aduPnPComponentName,
        g_aduPnPComponentServicePropertyName,
        jsonString,
        PNP_STATUS_SUCCESS,
        "", // Description for this acknowledgement.
        propertyVersion);

    if (jsonToSend == NULL)
    {
        Log_Error("Unable to build reported property ACK response.");
        goto done;
    }

    if (!ADUC_D2C_Message_SendAsync(
            ADUC_D2C_Message_Type_Device_Update_ACK,
            &g_iotHubClientHandleForADUComponent,
            STRING_c_str(jsonToSend),
            NULL /* responseCallback */,
            OnUpdateResultD2CMessageCompleted,
            NULL /* statusChangedCallback */,
            NULL /* userData */))
    {
        Log_Error("Unable to send update result.");
        goto done;
    }

done:
    STRING_delete(jsonToSend);

    free(jsonString);

    Log_Info("OrchestratorPropertyUpdateCallback ended");
}

/**
 * @brief This function is invoked when Device Update PnP Interface property is updated.
 *
 * @param clientHandle A Device Update Client handle object.
 * @param propertyName The name of the property that changed.
 * @param propertyValue The new property value.
 * @param version Property version.
 * @param sourceContext An information about the source of the property update notificaion.
 * @param context An ADUC_WorkflowData object.
 */
void AzureDeviceUpdateCoreInterface_PropertyUpdateCallback(
    ADUC_ClientHandle clientHandle,
    const char* propertyName,
    JSON_Value* propertyValue,
    int version,
    ADUC_PnPComponentClient_PropertyUpdate_Context* sourceContext,
    void* context)
{
    if (strcmp(propertyName, g_aduPnPComponentServicePropertyName) == 0)
    {
        OrchestratorUpdateCallback(clientHandle, propertyValue, version, sourceContext, context);
    }
    else
    {
        Log_Info("Unsupported property. (%s)", propertyName);
    }
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
 * @brief Sets workflow properties on the workflow json value.
 *
 * @param[in,out] workflowValue The workflow json value to set properties on.
 * @param[in] updateAction The updateAction for the action field.
 * @param[in] workflowId The workflow id of the update deployment.
 * @param[in] retryTimestamp optional. The retry timestamp that's present for service-initiated retries.
 * @return true if all properties were set successfully; false, otherwise.
 */
static _Bool set_workflow_properties(
    JSON_Value* workflowValue, ADUCITF_UpdateAction updateAction, const char* workflowId, const char* retryTimestamp)
{
    _Bool succeeded = false;

    JSON_Object* workflowObject = json_value_get_object(workflowValue);
    if (json_object_set_number(workflowObject, ADUCITF_FIELDNAME_ACTION, updateAction) != JSONSuccess)
    {
        Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_ACTION);
        goto done;
    }

    if (json_object_set_string(workflowObject, ADUCITF_FIELDNAME_ID, workflowId) != JSONSuccess)
    {
        Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_ID);
        goto done;
    }

    if (!IsNullOrEmpty(retryTimestamp))
    {
        if (json_object_set_string(workflowObject, ADUCITF_FIELDNAME_RETRYTIMESTAMP, retryTimestamp) != JSONSuccess)
        {
            Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_RETRYTIMESTAMP);
            goto done;
        }
    }

    succeeded = true;

done:

    return succeeded;
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
JSON_Value* GetReportingJsonValue(
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
    int stepsCount = workflow_get_children_count(handle);

    //
    // Prepare 'lastInstallResult', 'stepResults' data.
    //
    // Example schema:
    //
    // {
    //     "state" : ###,
    //     "workflow": {
    //         "action": 3,
    //         "id": "..."
    //     },
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

    JSON_Value* workflowValue = json_value_init_object();

    if (lastInstallResultValue == NULL || stepResultsValue == NULL || workflowValue == NULL)
    {
        Log_Error("Failed to init object for json value");
        goto done;
    }

    JSON_Status jsonStatus =
        json_object_set_value(rootObject, ADUCITF_FIELDNAME_LASTINSTALLRESULT, lastInstallResultValue);
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
    // Workflow
    //
    if (!IsNullOrEmpty(workflow_peek_id(handle)))
    {
        _Bool success = set_workflow_properties(
            workflowValue,
            ADUC_WorkflowData_GetCurrentAction(workflowData),
            workflow_peek_id(handle),
            workflow_peek_retryTimestamp(handle));

        if (!success)
        {
            goto done;
        }

        if (json_object_set_value(rootObject, ADUCITF_FIELDNAME_WORKFLOW, workflowValue) != JSONSuccess)
        {
            Log_Error("Could not add JSON : %s", ADUCITF_FIELDNAME_WORKFLOW);
            goto done;
        }

        workflowValue = NULL; // rootObject owns the value now.
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
        for (int i = 0; i < stepsCount; i++)
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
    json_value_free(workflowValue);

    return resultValue;
}

/**
 * @brief Report state, and optionally result to service.
 *
 * @param workflowDataToken A workflow data object.
 * @param updateState state to report.
 * @param result Result to report (optional, can be NULL).
 * @param installedUpdateId Installed update id (if update completed successfully).
 * @return true if succeeded.
 */
_Bool AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync(
    ADUC_WorkflowDataToken workflowDataToken,
    ADUCITF_State updateState,
    const ADUC_Result* result,
    const char* installedUpdateId)
{
    _Bool success = false;
    ADUC_WorkflowData* workflowData = (ADUC_WorkflowData*)workflowDataToken;

    JSON_Value* rootValue = NULL;
    char* jsonString = NULL;

    if (g_iotHubClientHandleForADUComponent == NULL)
    {
        Log_Error("ReportStateAsync called before registration! Can't report!");
        return false;
    }

    if (AgentOrchestration_ShouldNotReportToCloud(updateState))
    {
        Log_Debug("Skipping report of state '%s'", ADUCITF_StateToString(updateState));
        return true;
    }

    if (result == NULL && updateState == ADUCITF_State_DeploymentInProgress)
    {
        ADUC_Result resultForSet = { ADUC_Result_DeploymentInProgress_Success };
        workflow_set_result(workflowData->WorkflowHandle, resultForSet);
    }

    rootValue = GetReportingJsonValue(workflowData, updateState, result, installedUpdateId);
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

    if (!ReportClientJsonProperty(jsonString, workflowData))
    {
        goto done;
    }

    success = true;

done:
    json_value_free(rootValue);
    json_free_serialized_string(jsonString);
    // Don't free the persistenceData as that will be done by the startup logic that owns it.

    return success;
}
