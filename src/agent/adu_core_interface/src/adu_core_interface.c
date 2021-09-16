/**
 * @file adu_core_interface.c
 * @brief Methods to communicate with "urn:azureiot:AzureDeviceUpdateCore:1" interface.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */

#include "aduc/adu_core_interface.h"
#include "aduc/adu_core_export_helpers.h" // ADUC_SetUpdateStateWithResult
#include "aduc/agent_orchestration.h"
#include "aduc/agent_workflow.h"
#include "aduc/agent_workflow_utils.h"
#include "aduc/c_utils.h"
#include "aduc/client_handle_helper.h"
#include "aduc/hash_utils.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h"
#include "aduc/types/update_content.h"
#include "aduc/workflow_utils.h"

#include "startup_msg_helper.h"

#include <azure_c_shared_utility/strings.h> // STRING_*
#include <iothub_client_version.h>
#include <parson.h>
#include <pnp_protocol.h>

// Name of an Azure Device Update Agent component that this device implements.
static const char g_aduPnPComponentName[] = "deviceUpdate";

// Name of properties that Azure Device Update Agent component supports.

// This is the device-to-cloud property.
// An agent communicates its state and other data to ADU Management service by reporting this property to IoTHub.
static const char g_aduPnPComponentClientPropertyName[] = "agent";

// This is the cloud-to-device property.
// ADU Management send an 'Update Action' to this device by setting this property on IoTHub.
static const char g_aduPnPComponentOrchestratorPropertyName[] = "service";

/**
 * @brief Handle for Azure Device Update Agent component to communication to service.
 */
ADUC_ClientHandle g_iotHubClientHandleForADUComponent;

void ClientReportedStateCallback(int statusCode, void* context)
{
    UNREFERENCED_PARAMETER(context);

    if (statusCode < 200 || statusCode >= 300)
    {
        Log_Error(
            "Failed to report ADU agent's state, error: %d, %s",
            statusCode,
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_RESULT, statusCode));
    }
}

static void ReportClientJsonProperty(const char* json_value)
{
    if (g_iotHubClientHandleForADUComponent == NULL)
    {
        Log_Error("ReportClientJsonProperty called with invalid IoTHub Device Client handle! Can't report!");
        return;
    }

    IOTHUB_CLIENT_RESULT iothubClientResult;
    STRING_HANDLE jsonToSend =
        PnP_CreateReportedProperty(g_aduPnPComponentName, g_aduPnPComponentClientPropertyName, json_value);

    if (jsonToSend == NULL)
    {
        Log_Error("Unable to create Reported property for ADU client.");
        goto done;
    }

    const char* jsonToSendStr = STRING_c_str(jsonToSend);
    size_t jsonToSendStrLen = strlen(jsonToSendStr);

    Log_Debug("Reporting agent state:\n%s", jsonToSendStr);

    iothubClientResult = ClientHandle_SendReportedState(
        g_iotHubClientHandleForADUComponent,
        (const unsigned char*)jsonToSendStr,
        jsonToSendStrLen,
        ClientReportedStateCallback,
        NULL);

    if (iothubClientResult != IOTHUB_CLIENT_OK)
    {
        Log_Error(
            "Unable to report state, %s, error: %d, %s",
            json_value,
            iothubClientResult,
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_RESULT, iothubClientResult));
        goto done;
    }

done:
    STRING_delete(jsonToSend);
}

/**
 * @brief Reports values to the cloud which do not change throughout ADUs execution
 * @details the current expectation is to report these values after the successful
 * connection of the AzureDeviceUpdateCoreInterface
 * @returns true when the report is sent and false when reporting fails.
 */
_Bool ReportStartupMsg()
{
    if (g_iotHubClientHandleForADUComponent == NULL)
    {
        Log_Error("ReportStartupMsg called before registration! Can't report!");
        return false;
    }

    _Bool success = false;

    char* jsonString = NULL;
    char* manufacturer = NULL;
    char* model = NULL;

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

    if (!StartupMsg_AddDeviceProperties(startupMsgObj))
    {
        Log_Error("Could not add Device Properties to the startup message");
        goto done;
    }

    if (!StartupMsg_AddCompatPropertyNames(startupMsgObj))
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

    ReportClientJsonProperty(jsonString);

    success = true;
done:
    free(model);
    free(manufacturer);
    json_value_free(startupMsgValue);
    json_free_serialized_string(jsonString);

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
    ADUC_Workflow_HandleStartupWorkflowData(workflowData);

    if (!ReportStartupMsg())
    {
        Log_Warn("ReportStartupMsg failed");
    }
}

void AzureDeviceUpdateCoreInterface_DoWork(void* componentContext)
{
    ADUC_WorkflowData* workflowData = (ADUC_WorkflowData*)componentContext;
    ADUC_WorkflowData_DoWork(workflowData);
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
    ADUC_ClientHandle clientHandle, JSON_Value* propertyValue, int propertyVersion, void* context)
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

    ADUC_Workflow_HandlePropertyUpdate(workflowData, (const unsigned char*)jsonString);
    free(jsonString);
    jsonString = ackString;

    // ACK the request.
    jsonToSend = PnP_CreateReportedPropertyWithStatus(
        g_aduPnPComponentName,
        g_aduPnPComponentOrchestratorPropertyName,
        jsonString,
        PNP_STATUS_SUCCESS,
        "", // Description for this acknowledgement.
        propertyVersion);

    if (jsonToSend == NULL)
    {
        Log_Error("Unable to build reported property ACK response.");
        goto done;
    }

    const char* jsonToSendStr = STRING_c_str(jsonToSend);
    size_t jsonToSendStrLen = strlen(jsonToSendStr);
    IOTHUB_CLIENT_RESULT iothubClientResult = ClientHandle_SendReportedState(
        clientHandle, (const unsigned char*)jsonToSendStr, jsonToSendStrLen, NULL, NULL);

    if (iothubClientResult != IOTHUB_CLIENT_OK)
    {
        Log_Error(
            "Unable to send acknowledgement of property to IoT Hub for component=%s, error=%d",
            g_aduPnPComponentName,
            iothubClientResult);
        goto done;
    }

done:
    STRING_delete(jsonToSend);

    free(jsonString);

    Log_Info("OrchestratorPropertyUpdateCallback ended");
}

void AzureDeviceUpdateCoreInterface_PropertyUpdateCallback(
    ADUC_ClientHandle clientHandle, const char* propertyName, JSON_Value* propertyValue, int version, void* context)
{
    if (strcmp(propertyName, g_aduPnPComponentOrchestratorPropertyName) == 0)
    {
        OrchestratorUpdateCallback(clientHandle, propertyValue, version, context);
    }
    else
    {
        Log_Info("Unsupported property. (%s)", propertyName);
    }
}

//
// Reporting
//

JSON_Status _json_object_set_update_result(
    JSON_Object* object, int32_t resultCode, int32_t extendedResultCode, const char* resultDetails)
{
    JSON_Status status = json_object_set_number(object, ADUCITF_FIELDNAME_RESULTCODE, resultCode);
    if (status == JSONSuccess)
    {
        status = json_object_set_number(object, ADUCITF_FIELDNAME_EXTENDEDRESULTCODE, extendedResultCode);
    }
    if (status == JSONSuccess)
    {
        if (resultDetails)
        {
            status = json_object_set_string(object, ADUCITF_FIELDNAME_RESULTDETAILS, resultDetails);
        }
        else
        {
            status = json_object_set_null(object, ADUCITF_FIELDNAME_RESULTDETAILS);
        }
    }
    return status;
}

/**
 * @brief Sets workflow properties on the workflow json value.
 *
 * @param[in,out] workflowValue The workflow json value to set properties on.
 * @param[in] workflowId The workflow id of the update deployment.
 * @param[in] retryTimestamp optional. The retry timestamp that's present for service-initiated retries.
 * @return true if all properties were set successfully; false, otherwise.
 */
static _Bool set_workflow_properties(JSON_Value* workflowValue, const char* workflowId, const char* retryTimestamp)
{
    _Bool succeeded = false;

    JSON_Object* workflowObject = json_value_get_object(workflowValue);
    if (json_object_set_number(workflowObject, ADUCITF_FIELDNAME_ACTION, ADUCITF_UpdateAction_ProcessDeployment) != JSONSuccess)
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
 * @brief Report state, and optionally result to service.
 *
 * @param workflowData A workflow data object.
 * @param updateState state to report.
 * @param result Result to report (optional, can be NULL).
 * @param installedUpdateId Installed update id (if update completed successfully).
 */
void AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync(
    ADUC_WorkflowData* workflowData, ADUCITF_State updateState, const ADUC_Result* result, const char* installedUpdateId)
{
    if (g_iotHubClientHandleForADUComponent == NULL)
    {
        Log_Error("ReportStateAsync called before registration! Can't report!");
        return;
    }

    if (AgentOrchestration_IsWorkflowOrchestratedByAgent(workflowData))
    {
        if (AgentOrchestration_ShouldNotReportToCloud(updateState)) {
            return;
        }
    }
    // TODO(jewelden): Remove following once disabling support for CBO-driven orchestration
    else if (updateState == ADUCITF_State_InstallStarted || updateState == ADUCITF_State_ApplyStarted)
    {
        return;
    }

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

    if (result != NULL)
    {
        rootResult = *result;
    }
    else
    {
        rootResult = workflow_get_result(handle);
    }

    JSON_Value* rootValue = json_value_init_object();
    JSON_Object* rootObject = json_value_get_object(rootValue);
    char* jsonString = NULL;
    int leafCount = workflow_get_children_count(handle);

    //
    // Prepare 'lastInstallResult', 'updateInstallResult', and 'bundledUpdates' data.
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
    //         "updateInstallResult" : {
    //             "resultCode" : ####,
    //             "extendedResultCode" : ####,
    //             "resultDetails" : "..."
    //         },
    //         "bundledUpdates" : {
    //             "<leaf#0 update id>" : {
    //                 "resultCode" : ####,
    //                 "extendedResultCode" : ####,
    //                 "resultDetails" : "..."
    //             },
    //             ...
    //             "<leaf#N update id>" : {
    //                 "resultCode" : ####,
    //                 "extendedResultCode" : ####,
    //                 "resultDetails" : "..."
    //             }
    //         }
    //     }
    // }

    JSON_Value* lastInstallResultValue = json_value_init_object();
    JSON_Object* lastInstallResultObject = json_object(lastInstallResultValue);

    JSON_Value* updateInstallResultValue = json_value_init_object();
    JSON_Object* updateInstallResultObject = json_object(updateInstallResultValue);

    JSON_Value* bundledResultsValue = json_value_init_object();
    JSON_Object* bundledResultsObject = json_object(bundledResultsValue);

    JSON_Value* workflowValue = json_value_init_object();

    if (lastInstallResultValue == NULL || updateInstallResultValue == NULL || bundledResultsValue == NULL || workflowValue == NULL)
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
        Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_UPDATEINSTALLRESULT);
        goto done;
    }

    //
    // Workflow
    //
    char *workflowId = workflow_get_id (handle);
    if (!IsNullOrEmpty(workflowId))
    {
        _Bool success = set_workflow_properties(
            workflowValue,
            workflowId,
            NULL); // retryTimestamp - TODO(JeffW): call workflow_get_retryTimestamp(handle) once available

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

    // Top level result
    jsonStatus = json_object_set_value(
        lastInstallResultObject, ADUCITF_FIELDNAME_UPDATEINSTALLRESULT, updateInstallResultValue);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_UPDATEINSTALLRESULT);
        goto done;
    }

    updateInstallResultValue = NULL; // rootObject owns the value now.

    // If reporting 'downloadStarted' state, we must clear previous 'bundledUpdates' map, if exists.
    if (updateState == ADUCITF_State_DownloadStarted)
    {
        if (json_object_set_null(lastInstallResultObject, ADUCITF_FIELDNAME_BUNDLEDUPDATES) != JSONSuccess)
        {
            /* Note: continue the 'download' phase if we could not clear the previous results. */
            Log_Warn("Could not clear 'bundledUpdates' property. The property may contains previous install results.");
        }
    }
    // Otherwise, we will only report 'bundledUpdates' result if we have one ore moe Components Update.
    else if (leafCount > 0)
    {
        jsonStatus =
            json_object_set_value(lastInstallResultObject, ADUCITF_FIELDNAME_BUNDLEDUPDATES, bundledResultsValue);
        if (jsonStatus != JSONSuccess)
        {
            Log_Error("Could not add JSON field: %s", ADUCITF_FIELDNAME_BUNDLEDUPDATES);
            goto done;
        }

        bundledResultsValue = NULL; // rootObject owns the value now.
    }

    //
    // Report both state and result
    //

    // Set top-level update state and result.
    jsonStatus = _json_object_set_update_result(
        updateInstallResultObject,
        rootResult.ResultCode,
        rootResult.ExtendedResultCode,
        workflow_peek_result_details(handle));

    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not set values for field: %s", ADUCITF_FIELDNAME_UPDATEINSTALLRESULT);
        goto done;
    }

    // Report all leaf-updates result.
    if (updateState != ADUCITF_State_DownloadStarted)
    {
        leafCount = workflow_get_children_count(handle);
        if (leafCount == 0)
        {
            // Assuming this is not a Bundled Update, let's clear 'bundledUpdates' field.
            jsonStatus = json_object_clear(bundledResultsObject);
            if (jsonStatus != JSONSuccess)
            {
                Log_Warn("Could not clear JSON field: %s", ADUCITF_FIELDNAME_BUNDLEDUPDATES);
            }
        }

        for (int i = 0; i < leafCount; i++)
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
            // Let's name the result using "leaf_" +  the array index.
            childUpdateId = STRING_construct_sprintf("leaf_%d", i);
            if (childUpdateId == NULL)
            {
                Log_Error("Could not create proper child update id result key.");
                goto childDone;
            }

            jsonStatus = json_object_set_value(bundledResultsObject, STRING_c_str(childUpdateId), childResultValue);
            if (jsonStatus != JSONSuccess)
            {
                Log_Error("Could not add leaf #%d update result", i);
                goto childDone;
            }
            childResultValue = NULL; // bundledResultsValue owns it now.

            jsonStatus = _json_object_set_update_result(
                childResultObject,
                childResult.ResultCode,
                childResult.ExtendedResultCode,
                workflow_peek_result_details(childHandle));

            if (jsonStatus != JSONSuccess)
            {
                Log_Error("Could not add update result values for components #%d", i);
                goto childDone;
            }

        childDone:
            STRING_delete(childUpdateId);
            childUpdateId = NULL;
            json_value_free(childResultValue);
            childResultValue = NULL;
        }
    }

    jsonString = json_serialize_to_string(rootValue);
    if (jsonString == NULL)
    {
        Log_Error("Serializing JSON to string failed");
        goto done;
    }

    ReportClientJsonProperty(jsonString);

done:
    json_free_serialized_string(jsonString);
    json_value_free(rootValue);
    json_value_free(lastInstallResultValue);
    json_value_free(updateInstallResultValue);
    json_value_free(bundledResultsValue);
    json_value_free(workflowValue);
}

/**
 * @brief Report Idle State and update ID to service.
 *
 * This method handles reporting values after a successful apply.
 * After a successful apply, we need to report State as Idle and
 * we need to also update the installedUpdateId property.
 * We want to set both of these in the Digital Twin at the same time.
 *
 * @param[in] workflowData A workflow data object.
 * @param[in] updateId Id of and update installed on the device.
 */
void AzureDeviceUpdateCoreInterface_ReportUpdateIdAndIdleAsync(
    ADUC_WorkflowData* workflowData,
    const char* updateId)
{
    if (g_iotHubClientHandleForADUComponent == NULL)
    {
        Log_Error("ReportUpdateIdAndIdleAsync called before registration! Can't report!");
        return;
    }

    ADUC_Result result = { .ResultCode = ADUC_Result_Apply_Success, .ExtendedResultCode = 0 };

    AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync(workflowData, ADUCITF_State_Idle, &result, updateId);
}
