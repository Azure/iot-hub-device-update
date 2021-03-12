/**
 * @file adu_core_interface.c
 * @brief Methods to communicate with "urn:azureiot:AzureDeviceUpdateCore:1" interface.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */

#include "aduc/adu_core_interface.h"
#include "aduc/adu_core_export_helpers.h" // ADUC_SetUpdateStateWithResult
#include "aduc/agent_workflow.h"
#include "aduc/c_utils.h"
#include "aduc/client_handle_helper.h"
#include "aduc/hash_utils.h"
#include "startup_msg_helper.h"
#include <aduc/logging.h>
#include <aduc/string_c_utils.h>

#include <iothub_client_version.h>
#include <parson.h>
#include <pnp_protocol.h>
#include <stdlib.h>

// Name of an Azure Device Update Agent component that this device implements.
static const char g_aduPnPComponentName[] = "azureDeviceUpdateAgent";

// Name of properties that Azure Device Update Agent component supports.

// This is the device-to-cloud property.
// An agent communicates its state and other data to ADU Management service by reporting this property to IoTHub.
static const char g_aduPnPComponentClientPropertyName[] = "client";

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
    if (jsonToSend != NULL)
    {
        STRING_delete(jsonToSend);
    }
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
// AzureDeviceUpdateCoreInterface  methods
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
        ADUC_WorkflowData_Free(workflowData);
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

    ADUC_WorkflowData_Free(workflowData);
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

    Log_Debug(
        "OrchestratorUpdateCallback received property JSON string (%s), property version (%d)",
        jsonString,
        propertyVersion);

    ADUC_Workflow_HandlePropertyUpdate(workflowData, (const unsigned char*)jsonString);

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
    if (jsonToSend != NULL)
    {
        STRING_delete(jsonToSend);
    }

    if (jsonString != NULL)
    {
        free(jsonString);
    }

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

/**
 * @brief Report state, and optionally result to service.
 *
 * @param updateState state to report.
 * @param result Result to report (optional, can be NULL).
 */
void AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync(ADUCITF_State updateState, const ADUC_Result* result)
{
    if (g_iotHubClientHandleForADUComponent == NULL)
    {
        Log_Error("ReportStateAsync called before registration! Can't report!");
        return;
    }

    // As a optimization to reduce network traffic, these states are not reported.
    if (updateState == ADUCITF_State_DownloadStarted || updateState == ADUCITF_State_InstallStarted
        || updateState == ADUCITF_State_ApplyStarted)
    {
        return;
    }

    JSON_Value* rootValue = json_value_init_object();
    JSON_Object* rootObject = json_value_get_object(rootValue);
    char* jsonString = NULL;

    JSON_Status jsonStatus = json_object_set_number(rootObject, ADUCITF_FIELDNAME_STATE, updateState);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not serialize JSON field: %s value: %d", ADUCITF_FIELDNAME_STATE, updateState);
        goto done;
    }

    if (result != NULL)
    {
        // Report state and result.

        const int httpResultCode = IsAducResultCodeSuccess(result->ResultCode) ? 200 : 500;
        const int extendedResultCode = result->ExtendedResultCode;

        Log_Info(
            "Reporting state: %d, %s (%u); HTTP %d; result %d, %d",
            updateState,
            ADUCITF_StateToString(updateState),
            updateState,
            httpResultCode,
            result->ResultCode,
            extendedResultCode);

        jsonStatus = json_object_set_number(rootObject, ADUCITF_FIELDNAME_RESULTCODE, httpResultCode);
        if (jsonStatus != JSONSuccess)
        {
            Log_Error("Could not serialize JSON field: %s value: %d", ADUCITF_FIELDNAME_RESULTCODE, httpResultCode);
            goto done;
        }

        jsonStatus = json_object_set_number(rootObject, ADUCITF_FIELDNAME_EXTENDEDRESULTCODE, extendedResultCode);
        if (jsonStatus != JSONSuccess)
        {
            Log_Error(
                "Could not serialize JSON field: %s value: %d",
                ADUCITF_FIELDNAME_EXTENDEDRESULTCODE,
                extendedResultCode);
            goto done;
        }
    }
    else
    {
        // Only report state.

        Log_Info("Reporting state: %s (%u)", ADUCITF_StateToString(updateState), updateState);
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
}

/**
 * @brief Report Idle State and update ID to service.
 *
 * This method handles reporting values after a successful apply.
 * After a successful apply, we need to report State as Idle and
 * we need to also update the installedUpdateId property.
 * We want to set both of these in the Digital Twin at the same time.
 *
 * @param[in] updateId update ID to report as installed.
 */
void AzureDeviceUpdateCoreInterface_ReportUpdateIdAndIdleAsync(const ADUC_UpdateId* updateId)
{
    if (g_iotHubClientHandleForADUComponent == NULL)
    {
        Log_Error("ReportUpdateIdAndIdleAsync called before registration! Can't report!");
        return;
    }

    if (!ADUC_IsValidUpdateId(updateId))
    {
        Log_Error("ReportUpdateIdAndIdleAsync called with invalid update ID");
        return;
    }

    Log_Info(
        "Reporting state: %s (%u); UpdateId %s:%s:%s",
        ADUCITF_StateToString(ADUCITF_State_Idle),
        ADUCITF_State_Idle,
        updateId->Provider,
        updateId->Name,
        updateId->Version);

    JSON_Value* rootValue = json_value_init_object();
    JSON_Object* rootObject = json_value_get_object(rootValue);
    char* jsonString = NULL;

    char* installedUpdateIdJsonString = ADUC_UpdateIdToJsonString(updateId);

    if (installedUpdateIdJsonString == NULL)
    {
        Log_Error("Serializing installedUpdateId JSON to string failed");
        goto done;
    }

    JSON_Status jsonStatus =
        json_object_set_string(rootObject, ADUCITF_FIELDNAME_INSTALLEDUPDATEID, installedUpdateIdJsonString);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error(
            "Could not serialize JSON field: %s value %s",
            ADUCITF_FIELDNAME_INSTALLEDUPDATEID,
            installedUpdateIdJsonString);
    }

    jsonStatus = json_object_set_number(rootObject, ADUCITF_FIELDNAME_STATE, ADUCITF_State_Idle);
    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not serialize JSON field: %s value: %d", ADUCITF_FIELDNAME_STATE, ADUCITF_State_Idle);
        goto done;
    }

    jsonStatus = json_object_set_number(rootObject, ADUCITF_FIELDNAME_RESULTCODE, 200); // Success
    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not serialize JSON field: %s value: %d", ADUCITF_FIELDNAME_RESULTCODE, 200);
        goto done;
    }

    jsonStatus = json_object_set_number(rootObject, ADUCITF_FIELDNAME_EXTENDEDRESULTCODE, 0); // Success
    if (jsonStatus != JSONSuccess)
    {
        Log_Error("Could not serialize JSON field: %s value: %d", ADUCITF_FIELDNAME_EXTENDEDRESULTCODE, 0);
        goto done;
    }

    jsonString = json_serialize_to_string(rootValue);
    if (jsonString == NULL)
    {
        Log_Error("Serializing JSON to string failed");
        goto done;
    }

    ReportClientJsonProperty(jsonString);

done:

    json_free_serialized_string(installedUpdateIdJsonString);

    json_free_serialized_string(jsonString);
    json_value_free(rootValue);
}
