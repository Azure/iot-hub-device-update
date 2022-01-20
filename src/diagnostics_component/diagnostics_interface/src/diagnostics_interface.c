/**
 * @file diagnostics_interface.c
 * @brief Methods to communicate with "dtmi:azure:iot:deviceUpdateDiagnosticModel;1" interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <diagnostics_interface.h>

#include "aduc/c_utils.h"
#include "aduc/client_handle_helper.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h" // atoint64t
#include <ctype.h> // isalnum
#include <diagnostics_workflow.h>
#include <pnp_protocol.h>
#include <stdlib.h>

// Name of the DiagnosticsInformation component that this device implements.
static const char g_diagnosticsPnPComponentName[] = "diagnosticInformation";

// this is the device-to-cloud property for the diagnostics interface
// the diagnostic client sends up the status of the upload to the service for
// it to interpret.
static const char g_diagnosticsPnPComponentAgentPropertyName[] = "agent";

// this is the cloud-to-device property for the diagnostics interface
// the diagnostics manager sends down properties necessary for the log upload
static const char g_diagnosticsPnPComponentOrchestratorPropertyName[] = "service";

/**
 * @brief Handle for Diagnostics component to communicate to service.
 */
ADUC_ClientHandle g_iotHubClientHandleForDiagnosticsComponent = NULL;

//
// DiagnosticsInterface methods
//

/**
 * @brief Create a DeviceInfoInterface object.
 *
 * @param componentContext Context object to use for related calls.
 * @param argc Count of arguments in @p argv
 * @param argv Command line parameters.
 * @return _Bool True on success.
 */
_Bool DiagnosticsInterface_Create(void** componentContext, int argc, char** argv)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    _Bool succeeded = false;

    *componentContext = NULL;

    DiagnosticsWorkflowData* workflowData = calloc(1, sizeof(DiagnosticsWorkflowData));

    if (workflowData == NULL)
    {
        goto done;
    }

    if (!DiagnosticsWorkflow_InitFromFile(workflowData, DIAGNOSTICS_CONFIG_FILE_PATH))
    {
        Log_Error("Unable to initialize the diagnostic workflow data.");
        goto done;
    }

    succeeded = true;

done:

    if (!succeeded)
    {
        DiagnosticsWorkflow_UnInit(workflowData);
        free(workflowData);
        workflowData = NULL;
    }

    *componentContext = workflowData;

    return succeeded;
}

/**
 * @brief Called after connected to IoTHub (device client handler is valid).
 *
 * @param componentContext Context object from Create.
 */
void DiagnosticsInterface_Connected(void* componentContext)
{
    UNREFERENCED_PARAMETER(componentContext);

    Log_Info("DiagnosticsInterface is connected");
}

void DiagnosticsInterface_Destroy(void** componentContext)
{
    if (componentContext == NULL)
    {
        Log_Error("DiagnosticsInterface_Destroy called before initialization");
        return;
    }

    DiagnosticsWorkflowData* workflowData = (DiagnosticsWorkflowData*)*componentContext;

    DiagnosticsWorkflow_UnInit(workflowData);
    free(workflowData);
    *componentContext = NULL;
}

void DiagnosticsClientReportedStateCallback(int statusCode, void* context)
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

/**
 * @brief Sends status message to IotHub
 * @param clientHandle handle for the IotHub client handle to send the message to
 * @param msgToSend message to send to IotHub
 * @param msgSize size of @p msgToSend
 * @returns the result of the IotHub message send action
 */
static IOTHUB_CLIENT_RESULT
SendMessageToIotHub(ADUC_ClientHandle clientHandle, const char* msgToSend, const size_t msgSize)
{
    IOTHUB_CLIENT_RESULT iothubClientResult = IOTHUB_CLIENT_ERROR;

    if (g_iotHubClientHandleForDiagnosticsComponent == NULL)
    {
        Log_Error("SendMessageToIotHub called before registration. Can't report!");
        goto done;
    }

    iothubClientResult = ClientHandle_SendReportedState(
        clientHandle, (const unsigned char*)msgToSend, msgSize, DiagnosticsClientReportedStateCallback, NULL);

    if (iothubClientResult != IOTHUB_CLIENT_OK)
    {
        goto done;
    }

done:

    return iothubClientResult;
}

/**
 * @brief Function for sending a PnP message to the IotHub
 * @param clientHandle handle for the IotHub client handle to send the message to
 * @param jsonString message to send to the iothub
 * @returns the result of the IotHub message send action
 */
static IOTHUB_CLIENT_RESULT SendPnPMessageToIotHub(ADUC_ClientHandle clientHandle, const char* jsonString)
{
    IOTHUB_CLIENT_RESULT iothubClientResult = IOTHUB_CLIENT_ERROR;

    // Reporting just a message
    STRING_HANDLE jsonToSend = PnP_CreateReportedProperty(
        g_diagnosticsPnPComponentName, g_diagnosticsPnPComponentAgentPropertyName, jsonString);

    if (jsonToSend == NULL)
    {
        Log_Error("Unable to serialize JSON passed to SendPnPMessageToIotHub");
        goto done;
    }

    const char* jsonToSendStr = STRING_c_str(jsonToSend);
    size_t jsonToSendStrLen = STRING_length(jsonToSend);

    iothubClientResult = SendMessageToIotHub(clientHandle, jsonToSendStr, jsonToSendStrLen);

    if (iothubClientResult != IOTHUB_CLIENT_OK)
    {
        goto done;
    }

done:

    STRING_delete(jsonToSend);

    return iothubClientResult;
}

/**
 * @brief Function for sending a PnP message to the IotHub with status
 * @param clientHandle handle for the IotHub client handle to send the message to
 * @param jsonString message to send to the iothub
 * @param status value to set as the status to send up to the iothub
 * @param propertyVersion value for the version to send up to the iothub
 * @returns the result of the IotHub message send action
 */
static IOTHUB_CLIENT_RESULT SendPnPMessageToIotHubWithStatus(
    ADUC_ClientHandle clientHandle, const char* jsonString, int status, int propertyVersion)
{
    STRING_HANDLE jsonToSend = NULL;
    IOTHUB_CLIENT_RESULT iothubClientResult = IOTHUB_CLIENT_ERROR;

    if (g_iotHubClientHandleForDiagnosticsComponent == NULL)
    {
        Log_Error("Diagnostics ReportStateAsync called before registration. Can't report!");
        goto done;
    }

    jsonToSend = PnP_CreateReportedPropertyWithStatus(
        g_diagnosticsPnPComponentName,
        g_diagnosticsPnPComponentOrchestratorPropertyName,
        jsonString,
        status,
        "", // Description for this acknowledgement.
        propertyVersion);

    if (jsonToSend == NULL)
    {
        Log_Error("Unable to serialize JSON passed to SendPnPMessagetoIotHub");
        goto done;
    }
    const char* jsonToSendStr = STRING_c_str(jsonToSend);
    const size_t jsonToSendStrLen = STRING_length(jsonToSend);

    iothubClientResult = SendMessageToIotHub(clientHandle, jsonToSendStr, jsonToSendStrLen);

    if (iothubClientResult != IOTHUB_CLIENT_OK)
    {
        goto done;
    }

done:

    STRING_delete(jsonToSend);

    return iothubClientResult;
}

void DiagnosticsOrchestratorUpdateCallback(
    ADUC_ClientHandle clientHandle, JSON_Value* propertyValue, int propertyVersion, void* context)
{
    char* jsonString = json_serialize_to_string(propertyValue);

    if (jsonString == NULL)
    {
        Log_Error(
            "DiagnosticsOrchestratorUpdateCallback failed to convert property JSON value to string, property version %d",
            propertyVersion);
        goto done;
    }

    DiagnosticsWorkflow_DiscoverAndUploadLogsAsync(context, jsonString);

    // Ack the request
    IOTHUB_CLIENT_RESULT iothubClientResult =
        SendPnPMessageToIotHubWithStatus(clientHandle, jsonString, PNP_STATUS_SUCCESS, propertyVersion);

    if (iothubClientResult != IOTHUB_CLIENT_OK)
    {
        Log_Error(
            "Unable to send acknowledgement of property to IoT Hub for component=%s, error=%d",
            g_diagnosticsPnPComponentName,
            iothubClientResult);
        goto done;
    }

done:

    json_free_serialized_string(jsonString);
}

/**
 * @brief A callback for the diagnostic component's property update events.
 */
void DiagnosticsInterface_PropertyUpdateCallback(
    ADUC_ClientHandle clientHandle, const char* propertyName, JSON_Value* propertyValue, int version, void* context)
{
    if (strcmp(propertyName, g_diagnosticsPnPComponentOrchestratorPropertyName) == 0)
    {
        DiagnosticsOrchestratorUpdateCallback(clientHandle, propertyValue, version, context);
    }
    else
    {
        Log_Info("DiagnosticsInterface received unsupported property. (%s)", propertyName);
    }
}

/**
 * @brief Report a new state to the server.
 * @param result the result to be reported to the service
 * @param operationId the operation id associated with the result being reported
 */
void DiagnosticsInterface_ReportStateAndResultAsync(const Diagnostics_Result result, const char* operationId)
{
    _Bool succeeded = false;

    Log_Info(
        "DiagnosticsInterface_ReportStateAndResultAsync Reporting result: %s", DiagnosticsResult_ToString(result));
    JSON_Value* rootValue = json_value_init_object();
    JSON_Object* rootObject = json_value_get_object(rootValue);

    char* jsonString = NULL;
    JSON_Status jsonStatus = json_object_set_number(rootObject, DIAGNOSTICSITF_FIELDNAME_RESULTCODE, result);

    if (jsonStatus != JSONSuccess)
    {
        goto done;
    }

    jsonStatus = json_object_set_string(rootObject, DIAGNOSTICSITF_FIELDNAME_OPERATIONID, operationId);

    if (jsonStatus != JSONSuccess)
    {
        goto done;
    }

    jsonString = json_serialize_to_string(rootValue);

    if (jsonString == NULL)
    {
        goto done;
    }

    IOTHUB_CLIENT_RESULT iotHubResult =
        SendPnPMessageToIotHub(g_iotHubClientHandleForDiagnosticsComponent, jsonString);

    if (iotHubResult != IOTHUB_CLIENT_OK)
    {
        Log_Error(
            "Diagnostics Interface unable to report state, %s, error: %d, %s",
            jsonString,
            iotHubResult,
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_RESULT, iotHubResult));
        goto done;
    }

    succeeded = true;

done:

    if (!succeeded)
    {
        Log_Error("DiagnosticsInterface_ReportStateAndResultAsync failed");
    }

    json_value_free(rootValue);

    json_free_serialized_string(jsonString);
}
