/**
 * @file adps2-client-module.c
 * @brief Implementation for the device provisioning using Azure DPS V2.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/adps_gen2.h"
#include "aduc/adu_communication_channel.h"
#include "aduc/adu_types.h"
#include "aduc/agent_state_store.h"
#include "aduc/config_utils.h"
#include "aduc/logging.h"
#include "aduc/retry_utils.h" // ADUC_GetTimeSinceEpochInSeconds
#include "aduc/string_c_utils.h"
#include "du_agent_sdk/agent_module_interface.h"

#include "parson.h"
#include <aducpal/time.h> // time_t
#include <mosquitto.h> // mosquitto related functions

#define ADPS_MQTT_TOPIC_REGISTRATIONS_RESULT "$dps/registrations/res/#"
#define ADPS_MQTT_TOPIC_REGISTRATIONS_RESULT_200 "$dps/registrations/res/200/#"
#define ADPS_MQTT_TOPIC_REGISTRATIONS_RESULT_202 "$dps/registrations/res/202/#"

#define ADPS_DEFAULT_REGISTER_REQUEST_DELAY_SECONDS 600
#define ADPS_DEFAULT_REGISTER_STATUS_POLLING_INTERVAL_SECONDS 5

/**
 * @brief Enumeration of registration states for Azure DPS device registration management.
 */
typedef enum tagADPS_REGISTER_STATE
{
    ADPS_REGISTER_STATE_FAILED = -1, /**< Device registration failed. */
    ADPS_REGISTER_STATE_UNKNOWN = 0, /**< Registration state is unknown. */
    ADPS_REGISTER_STATE_REGISTERING = 1, /**< Device is currently registering. */
    ADPS_REGISTER_STATE_WAIT_TO_POLL = 2, /**< Waiting for a polling event. */
    ADPS_REGISTER_STATE_POLLING = 3, /**< Device is currently polling. */
    ADPS_REGISTER_STATE_REGISTERED = 4 /**< Device registration is successful. */
} ADPS_REGISTER_STATE;

/**
 * @brief The module state.
 */
typedef struct tagADPS_MQTT_CLIENT_MODULE_STATE
{
    bool initialized; //!< Module is initialized
    bool subscribed; //!< Device is subscribed to DPS topics

    ADPS_REGISTER_STATE registerState; //!< Registration state
    char* operationId; //!< Operation ID for registration
    long int requestId; //!< Request ID for registration
    time_t lastRegisterAttemptTime; //!< Last time a registration attempt was made
    time_t lastRegisterResponseTime; //!< Last time a registration response was received
    time_t nextRegisterAttemptTime; //!< Next time to attempt registration
    time_t nextPollingAttemptTime; //!< Next time to attempt polling
    int registerRetries; //!< Number of registration retries
    int pollingRetries; //!< Number of polling retries

    JSON_Value* registrationData; //!< Registration data

    time_t lastErrorTime; //!< Last time an error occurred
    AZURE_DPS_2_MQTT_SETTINGS settings; //!< DPS settings
    time_t nextOperationTime; //!< Next time to perform an operation

    ADUC_AGENT_MODULE_HANDLE commChannelModule; //!< Communication channel module

} ADPS_MQTT_CLIENT_MODULE_STATE;

/**
 * @brief The contract info for the module.
 */
ADUC_AGENT_CONTRACT_INFO g_adpsMqttClientContractInfo = {
    "Microsoft", "Azure DPS2 MQTT Client Module", 1, "Microsoft/AzureDPS2MQTTClientModule:1"
};

// Forward declarations
ADUC_AGENT_MODULE_HANDLE ADPS_MQTT_Client_Module_Create();
void ADPS_MQTT_Client_Module_Destroy(ADUC_AGENT_MODULE_HANDLE handle);
const ADUC_AGENT_CONTRACT_INFO* ADPS_MQTT_Client_Module_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle);
int ADPS_MQTT_Client_Module_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* moduleInitData);
int ADPS_MQTT_Client_Module_Deinitialize(ADUC_AGENT_MODULE_HANDLE handle);
int ADPS_MQTT_Client_Module_DoWork(ADUC_AGENT_MODULE_HANDLE handle);

static ADPS_MQTT_CLIENT_MODULE_STATE* ModuleStateFromModuleHandle(ADUC_AGENT_MODULE_HANDLE handle)
{
    if ((handle != NULL) && (((ADUC_AGENT_MODULE_INTERFACE*)handle)->moduleData != NULL))
    {
        return (ADPS_MQTT_CLIENT_MODULE_STATE*)(((ADUC_AGENT_MODULE_INTERFACE*)handle)->moduleData);
    }
    return NULL;
}

/**
 * @brief Create a Device Update Agent Module for IoT Hub PnP Client.
 *
 * @return ADUC_AGENT_MODULE_HANDLE The handle to the module.
 */
ADUC_AGENT_MODULE_HANDLE ADPS_MQTT_Client_Module_Create()
{
    ADUC_AGENT_MODULE_HANDLE handle = NULL;

    ADPS_MQTT_CLIENT_MODULE_STATE* moduleState = NULL;
    ADUC_AGENT_MODULE_INTERFACE* moduleInterface = NULL;

    moduleState = calloc(1, sizeof(*moduleState));
    if (moduleState == NULL)
    {
        goto done;
    }

    moduleInterface = calloc(1, sizeof(*moduleInterface));
    if (moduleInterface == NULL)
    {
        goto done;
    }

    moduleInterface->destroy = ADPS_MQTT_Client_Module_Destroy;
    moduleInterface->getContractInfo = ADPS_MQTT_Client_Module_GetContractInfo;
    moduleInterface->doWork = ADPS_MQTT_Client_Module_DoWork;
    moduleInterface->initializeModule = ADPS_MQTT_Client_Module_Initialize;
    moduleInterface->deinitializeModule = ADPS_MQTT_Client_Module_Deinitialize;

    moduleInterface->moduleData = moduleState;
    moduleState = NULL; // transfer ownership

    moduleInterface->initialized = true;

    // successful, so only now transfer moduleInterface to the output handle.
    handle = moduleInterface;
    moduleInterface = NULL; // transfer ownership

done:
    // Always free. If successfully transferred to output handle, then these are NOOPs;
    // otherwise, the output handle is NULL and these get freed if they were allocated.
    free(moduleState);
    free(moduleInterface);

    return handle;
}

/*
 * @brief Destroy the Device Update Agent Module for IoT Hub PnP Client.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 */
void ADPS_MQTT_Client_Module_Destroy(ADUC_AGENT_MODULE_HANDLE handle)
{
    Log_Info("DPS mqtt model destroy");
    ADUC_AGENT_MODULE_INTERFACE* moduleInterface = (ADUC_AGENT_MODULE_INTERFACE*)handle;
    if (moduleInterface != NULL)
    {
        free(moduleInterface->moduleData);
        free(moduleInterface);
    }
    else
    {
        Log_Error("Invalid handle");
    }
}

#ifndef MIN
#    define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

static void SetRegisterState(ADPS_MQTT_CLIENT_MODULE_STATE* moduleState, ADPS_REGISTER_STATE state, const char* reason)
{
    if (moduleState->registerState == state)
    {
        return;
    }

    Log_Info("Register state changed from %d to %d (%s)", moduleState->registerState, state, reason);
    moduleState->registerState = state;
}

/**
 * @brief Gets the extension contract info.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 * @return ADUC_AGENT_CONTRACT_INFO The extension contract info.
 */
const ADUC_AGENT_CONTRACT_INFO* ADPS_MQTT_Client_Module_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle)
{
    IGNORED_PARAMETER(handle);
    return &g_adpsMqttClientContractInfo;
}

// Forward declarations
void ADPS_MQTT_Client_Module_OnMessage(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props);
void ADPS_MQTT_Client_Module_OnPublish(
    struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* props);

const char* topicArray[1] = { "$dps/registrations/res/#" };

static bool ADPS_MQTT_Client_Module_GetSubscriptTopics(void* obj, int* count, char*** topics)
{
    bool success = false;
    if (count == NULL || topics == NULL)
    {
        Log_Error("Invalid parameter");
        goto done;
    }

    *count = 1;
    *topics = (char**)topicArray;

    success = true;
done:
    return success;
}

/**
 * @brief Callbacks for various event from the communication channel.
 */
ADUC_MQTT_CALLBACKS s_commChannelCallbacks = {
    NULL /*on_connect*/,
    NULL /*on_disconnect*/,
    ADPS_MQTT_Client_Module_GetSubscriptTopics /*get_subscription_topics*/,
    NULL /*on_subscribe*/,
    NULL /*on_unsubscribe*/,
    ADPS_MQTT_Client_Module_OnPublish,
    ADPS_MQTT_Client_Module_OnMessage,
    NULL /*on_log*/
};

/**
 * @brief A helper function used for processing a device registration response payload.
 *
 * This function processes the given payload to determine the device's registration status.
 * If the registration status is "assigned", then the device is considered registered.
 *
 * Example payload:
 * @code
 * {
 *     "operationId": "4.e38bd7086c69f038.a02a0838-a76c-4a21-89da-f776a58245ac",
 *     "status": "assigned",
 *     "registrationState": {
 *         "x509": {
 *             "enrollmentGroupId": "contoso-violet-devbox-cusc"
 *         },
 *         "registrationId": "contoso-violet-dev02-devbox-cusc",
 *         "createdDateTimeUtc": "2023-09-26T01:48:19.1296493Z",
 *         "assignedEndpoint": {
 *             "type": "mqttBroker",
 *             "hostName": "contosl-violet-devbox-cusc-eg.centraluseuap-1.ts.eventgrid.azure.net"
 *         },
 *         "deviceId": "contoso-violet-dev02-devbox-cusc",
 *         "status": "assigned",
 *         "substatus": "initialAssignment",
 *         "lastUpdatedDateTimeUtc": "2023-09-26T01:48:19.3224804Z",
 *         "etag": "IjA1MDA1Yjg5LTAwMDAtMzMwMC0wMDAwLTY1MTIzODYzMDAwMCI="
 *     }
 * }
 * @endcode
 * @param moduleState The module state.
 * @param payload The payload to process.
 * @param payload_len The length of the payload.
 * @return true if the payload was processed successfully; false otherwise.
 */
bool ProcessDeviceRegistrationResponse(
    ADPS_MQTT_CLIENT_MODULE_STATE* moduleState, const char* payload, int payload_len)
{
    bool result = false;

    JSON_Value* root_value = json_parse_string(payload);
    if (root_value == NULL)
    {
        Log_Error("Failed to parse JSON payload");
        goto done;
    }

    const char* status = json_object_get_string(json_object(root_value), "status");
    if (status == NULL)
    {
        Log_Error("Failed to get 'status' from DPS payload");
        goto done;
    }

    if (strcmp(status, "assigned") == 0)
    {
        bool errorOccurred = false;
        Log_Info("Device is registered.");

        SetRegisterState(moduleState, ADPS_REGISTER_STATE_REGISTERED, "received assigned status");

        const char* deviceId = json_object_dotget_string(json_object(root_value), "registrationState.deviceId");

        if (deviceId == NULL)
        {
            Log_Error("Failed to get 'registrationState.deviceId' from DPS payload:\n%s", payload);
            errorOccurred = true;
        }
        else if (ADUC_StateStore_SetExternalDeviceId(deviceId) != ADUC_STATE_STORE_RESULT_OK)
        {
            Log_Error("Failed to set externalDeviceID");
            errorOccurred = true;
        }

        const char* mqttBrokerHostname =
            json_object_dotget_string(json_object(root_value), "registrationState.assignedEndpoint.hostName");

        if (mqttBrokerHostname == NULL)
        {
            Log_Error("Failed to get 'registrationState.assignedEndpoint.hostName' from DPS payload");
            errorOccurred = true;
        }
        else
        {
            Log_Info("DPS->MQTTbroker: %s", mqttBrokerHostname);
            if (ADUC_StateStore_SetMQTTBrokerHostname(mqttBrokerHostname) != ADUC_STATE_STORE_RESULT_OK)
            {
                Log_Error("Failed to set MQTT broker hostname");
                errorOccurred = true;
            }
        }

        if (errorOccurred)
        {
            // Set registration state to 'unknown' so that we can retry again.
            SetRegisterState(moduleState, ADPS_REGISTER_STATE_UNKNOWN, "failed to set registration data");
            goto done;
        }
    }
    else if (strcmp(status, "assigning") == 0)
    {
        Log_Info("Device is registering.");
        SetRegisterState(moduleState, ADPS_REGISTER_STATE_REGISTERING, "received assigning status");
    }
    else
    {
        Log_Error("Unknown status: %s", status);
        SetRegisterState(moduleState, ADPS_REGISTER_STATE_FAILED, "received unknown status");
        // TODO (nox-msft) : determine whether to retry or bail
    }

done:
    json_value_free(root_value);
    return result;
}

/* Callback called when the client receives a message. */
void ADPS_MQTT_Client_Module_OnMessage(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    bool result = false;
    char** topics = NULL;
    int count = 0;

    Log_Debug("<-- MSG RECV topic: '%s' qos: %d msgid: %d", msg->topic, msg->qos, msg->mid);

    ADPS_MQTT_CLIENT_MODULE_STATE* moduleState = ModuleStateFromModuleHandle(obj);
    if (moduleState == NULL)
    {
        Log_Error("Invalid module state");
        goto done;
    }

    if (msg != NULL && msg->topic != NULL)
    {
        time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();

        int mqtt_ret = mosquitto_topic_matches_sub(ADPS_MQTT_TOPIC_REGISTRATIONS_RESULT, msg->topic, &result);
        if (mqtt_ret != MQTT_RC_SUCCESS || !result)
        {
            goto done;
        }

        // Get http status code from topic sub segment
        // e.g., "$dps/registrations/res/200/?$rid=1&..."
        mqtt_ret = mosquitto_sub_topic_tokenise(msg->topic, &topics, &count);
        if (mqtt_ret != MQTT_RC_SUCCESS && count <= 3)
        {
            // Cant get the http status code.
            goto done;
        }

        int http_status_code = atoi(topics[3]);
        if (http_status_code == 200)
        {
            Log_Info("Registration request completed successfully.");
            moduleState->lastRegisterResponseTime = nowTime;
            ProcessDeviceRegistrationResponse(moduleState, msg->payload, msg->payloadlen);
        }
        else if (http_status_code == 202)
        {
            // If we're polling, then we dont need to change operation id and request id.
            if (moduleState->registerState == ADPS_REGISTER_STATE_REGISTERING)
            {
                Log_Info("Preparing status polling request...");
                JSON_Value* root_value = json_parse_string(msg->payload);
                if (root_value != NULL)
                {
                    free(moduleState->operationId);
                    moduleState->operationId = NULL;
                    const char* optId = json_object_get_string(json_object(root_value), "operationId");
                    moduleState->operationId = strdup(optId);
                    json_value_free(root_value);
                    root_value = NULL;
                }
                SetRegisterState(moduleState, ADPS_REGISTER_STATE_WAIT_TO_POLL, "received 202 response");
                moduleState->nextPollingAttemptTime = nowTime + ADPS_DEFAULT_REGISTER_STATUS_POLLING_INTERVAL_SECONDS;
                goto done;
            }
        }
        else if (http_status_code == 404)
        {
            // TODO (nox-msft) : determine whether to retry or bail
            Log_Info("Unhandled http status code: %d", http_status_code);
        }
        else
        {
            // TODO (nox-msft) : determine whether to retry or bail
            Log_Info("Unhandled http status code: %d", http_status_code);
        }
    }

done:

    free(topics);
    Log_Debug("\tPayload: %s\n", (char*)msg->payload);
}

/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
void ADPS_MQTT_Client_Module_OnPublish(
    struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* props)
{
    Log_Info(
        "<-- PUBACK (qos 1) msgid: %d, reason_code: %d => '%s'",
        mid,
        reason_code,
        mosquitto_reason_string(reason_code));
}

/**
 * @brief Initialize the IoTHub Client module.
*/
int ADPS_MQTT_Client_Module_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* moduleInitData)
{
    IGNORED_PARAMETER(moduleInitData);
    int ret = -1;

    ADPS_MQTT_CLIENT_MODULE_STATE* moduleState = ModuleStateFromModuleHandle(handle);
    if (moduleState == NULL)
    {
        Log_Error("Invalid module state");
        goto done;
    }

    // Need to set ret and goto done after this to ensure proper shutdown and deinitialization.
    // TODO (nox-msft) - set log level according to global config.
    ADUC_Logging_Init(0, "adps2-mqtt-client-module");

    if (!ReadAzureDPS2MqttSettings(&moduleState->settings))
    {
        Log_Error("Failed to read Azure DPS2 MQTT settings");
        goto done;
    }

    moduleState->commChannelModule = ADUC_Communication_Channel_Create();
    if (moduleState->commChannelModule == NULL)
    {
        Log_Error("Failed to create communication channel");
        goto done;
    }

    ADUC_AGENT_MODULE_INTERFACE* commInterface = (ADUC_AGENT_MODULE_INTERFACE*)moduleState->commChannelModule;

    ADUC_COMMUNICATION_CHANNEL_INIT_DATA commInitData = { moduleState->settings.registrationId,
                                                          handle,
                                                          &moduleState->settings.mqttSettings,
                                                          &s_commChannelCallbacks,
                                                          NULL /* key file password callback */,
                                                          NULL /* connection retry param */ };

    ret = commInterface->initializeModule(moduleState->commChannelModule, &commInitData);

    if (ret != 0)
    {
        Log_Error("Failed to initialize the communication channel");
        goto done;
    }

done:
    return ret;
}

/*
 Azure Device Registration Process
 =================================
 See : https://learn.microsoft.com/azure/iot/iot-mqtt-connect-to-iot-dps?toc=%2Fazure%2Fiot-dps%2Ftoc.json&bc=%2Fazure%2Fiot-dps%2Fbreadcrumb%2Ftoc.json

 Polling for registration operation status
 =========================================
    - The device must poll the service periodically to receive the result of the device registration operation.
    - Subscribe to the $dps/registrations/res/# topic
    - Publish a get operation status message to $dps/registrations/GET/iotdps-get-operationstatus/?$rid={request_id}&operationId={operationId}
        - The operation ID in this message should be the value received in the RegistrationOperationStatus
          response message in the previous step.
    - In the successful case, the service responds on the $dps/registrations/res/200/?$rid={request_id} topic.
    - The payload of the response contains the RegistrationOperationStatus object.
    - The device should keep polling the service if the response code is 202 after a delay equal to the retry-after period.
    - The device registration operation is successful if the service returns a 200 status code.
*/

bool DeviceRegistration_DoWork(ADUC_AGENT_MODULE_HANDLE handle)
{
    bool result = false;
    ADPS_MQTT_CLIENT_MODULE_STATE* moduleState = ModuleStateFromModuleHandle(handle);
    if (moduleState == NULL)
    {
        Log_Error("Invalid module state");
        goto done;
    }

    if (moduleState->registerState == ADPS_REGISTER_STATE_REGISTERED)
    {
        result = true;
        goto done;
    }

    time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();

    if (moduleState->registerState == ADPS_REGISTER_STATE_REGISTERING
        || moduleState->registerState == ADPS_REGISTER_STATE_POLLING)
    {
        if (nowTime > moduleState->nextRegisterAttemptTime)
        {
            SetRegisterState(moduleState, ADPS_REGISTER_STATE_UNKNOWN, "time out");
        }
    }

    if (moduleState->registerState == ADPS_REGISTER_STATE_WAIT_TO_POLL)
    {
        if (nowTime > moduleState->nextPollingAttemptTime)
        {
            char* pollTopic = NULL;
            if (moduleState->pollingRetries > 10)
            {
                Log_Error("Exceeded maximum polling retries");
                SetRegisterState(moduleState, ADPS_REGISTER_STATE_FAILED, "exceeded maximum polling retries");
                goto done;
            }

            pollTopic = ADUC_StringFormat(
                "$dps/registrations/GET/iotdps-get-operationstatus/?$rid=%ld&operationId=%s",
                moduleState->requestId,
                moduleState->operationId);
            if (pollTopic == NULL)
            {
                Log_Error("Cannot allocate memory for registration status polling topic");
            }
            else
            {
                int mid = 0;
                int mqtt_ret = ADUC_Communication_Channel_MQTT_Publish(
                    moduleState->commChannelModule,
                    pollTopic,
                    &mid,
                    0,
                    NULL,
                    moduleState->settings.mqttSettings.qos,
                    false /* retain */,
                    NULL /* props */);

                if (mqtt_ret != MOSQ_ERR_SUCCESS)
                {
                    Log_Error("Failed to publish registration polling request (%d)", mqtt_ret);
                    // TODO (nox-msft) : determine whether to retry or bail
                }
                else
                {
                    SetRegisterState(moduleState, ADPS_REGISTER_STATE_POLLING, "publishing message");
                    moduleState->nextPollingAttemptTime =
                        nowTime + ADPS_DEFAULT_REGISTER_STATUS_POLLING_INTERVAL_SECONDS;
                    moduleState->pollingRetries++;
                }
            }
            free(pollTopic);
        }
        else
        {
            goto done;
        }
    }

    if (moduleState->registerState == ADPS_REGISTER_STATE_FAILED)
    {
        result = false;
        goto done;
    }

    if (moduleState->registerState == ADPS_REGISTER_STATE_UNKNOWN && nowTime > moduleState->nextRegisterAttemptTime)
    {
        char topic_name[256];
        char device_registration_json[1024];
        moduleState->requestId = nowTime;
        snprintf(
            topic_name,
            sizeof(topic_name),
            "$dps/registrations/PUT/iotdps-register/?$rid=%ld",
            moduleState->requestId);
        snprintf(
            device_registration_json,
            sizeof(device_registration_json),
            "{\"registrationId\":\"%s\"}",
            moduleState->settings.registrationId);
        int mid = 0;
        int mqtt_ret = ADUC_Communication_Channel_MQTT_Publish(
            moduleState->commChannelModule,
            topic_name,
            &mid,
            (int)strlen(device_registration_json),
            device_registration_json,
            moduleState->settings.mqttSettings.qos,
            false /* retain */,
            NULL /* props */);

        if (mqtt_ret != MOSQ_ERR_SUCCESS)
        {
            Log_Error("Failed to publish registration request (%d)", mqtt_ret);
            // TODO (nox-msft) : determine whether to retry or bail
            goto done;
        }
        else
        {
            SetRegisterState(moduleState, ADPS_REGISTER_STATE_REGISTERING, "publishing message");
            moduleState->nextRegisterAttemptTime = nowTime + ADPS_DEFAULT_REGISTER_REQUEST_DELAY_SECONDS;
            moduleState->registerRetries++;
            moduleState->pollingRetries = 0;
        }
    }

    result = true;

done:
    return result;
}

/**
 * @brief Deinitialize the IoTHub Client module.
 *
 * @param module The handle to the module. This is the same handle that was returned by the Create function.
 *
 * @return int 0 on success.
 */
int ADPS_MQTT_Client_Module_Deinitialize(ADUC_AGENT_MODULE_HANDLE module)
{
    Log_Info("Deinitialize");

    ADPS_MQTT_CLIENT_MODULE_STATE* moduleState = ModuleStateFromModuleHandle(module);
    if (moduleState == NULL)
    {
        Log_Error("Invalid moduleState");
        return -1;
    }

    ADUC_AGENT_MODULE_INTERFACE* commInterface = (ADUC_AGENT_MODULE_INTERFACE*)moduleState->commChannelModule;
    commInterface->deinitializeModule(moduleState->commChannelModule);

    FreeAzureDPS2MqttSettings(&moduleState->settings);
    free(moduleState->operationId);
    json_value_free(moduleState->registrationData);

    ADUC_Logging_Uninit();
    return 0;
}

void OnCommandReceived(const char* commandName, const char* commandPayload, size_t payloadSize, void* context)
{
    IGNORED_PARAMETER(context);
    Log_Info("Received command %s with payload %s (size:%d)", commandName, commandPayload, payloadSize);
}

/**
 * @brief Perform the work for the extension. This must be a non-blocking operation.
 *
 * @return ADUC_Result
 */
int ADPS_MQTT_Client_Module_DoWork(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADPS_MQTT_CLIENT_MODULE_STATE* moduleState = ModuleStateFromModuleHandle(handle);

    ADUC_AGENT_MODULE_INTERFACE* commInterface = (ADUC_AGENT_MODULE_INTERFACE*)moduleState->commChannelModule;
    if (commInterface != NULL)
    {
        commInterface->doWork(commInterface);
    }

    if (ADUC_Communication_Channel_IsConnected(moduleState->commChannelModule))
    {
        DeviceRegistration_DoWork(handle);
    }

    return 0;
}
