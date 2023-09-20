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

#define ADPS_DEFAULT_REGISTER_REQUEST_DELAY_SECONDS 60

/**
 * @brief Enumeration of registration states for Azure DPS device registration management.
 */
typedef enum ADPS_REGISTER_STATE_TAG
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
typedef struct ADPS_MQTT_CLIENT_MODULE_STATE_TAG
{
    struct mosquitto* mqttClient; //!< Mosquitto client
    bool initialized; //!< Module is initialized

    bool connected; //!< Device is connected to DPS
    time_t lastConnectedTime; //!< Last time connected to DPS

    bool subscribed; //!< Device is subscribed to DPS topics

    ADPS_REGISTER_STATE registerState; //!< Registration state
    char* operationId; //!< Operation ID for registration
    time_t lastRegisterAttemptTime; //!< Last time a registration attempt was made
    time_t lastRegisterResponseTime; //!< Last time a registration response was received
    time_t nextRegisterAttemptTime; //!< Next time to attempt registration
    int registerRetries; //!< Number of registration retries

    JSON_Value* registrationData; //!< Registration data

    time_t lastErrorTime; //!< Last time an error occurred
    ADUC_Result lastAducResult; //!< Last ADUC result
    int lastError; //!< Last error code
    AZURE_DPS_2_MQTT_SETTINGS settings; //!< DPS settings
    time_t nextOperationTime; //!< Next time to perform an operation

} ADPS_MQTT_CLIENT_MODULE_STATE;

static ADPS_MQTT_CLIENT_MODULE_STATE s_moduleState = { 0 };

/**
 * @brief The contract info for the module.
 */
ADUC_AGENT_CONTRACT_INFO g_adpsMqttClientContractInfo = {
    "Microsoft", "Azure DPS2 MQTT Client  Module", 1, "Microsoft/AzureDPS2MQTTClientModule:1"
};

#ifndef MIN
#    define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

static void SetRegisterState(ADPS_REGISTER_STATE state, const char* reason)
{
    if (s_moduleState.registerState == state)
    {
        return;
    }

    Log_Info("Register state changed from %d to %d (%s)", s_moduleState.registerState, state, reason);
    s_moduleState.registerState = state;
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

enum ADPS_ERROR
{
    ADPS_ERROR_NONE = 0,
    ADPS_ERROR_INVALID_PARAMETER = 1,
    ADPS_ERROR_INVALID_STATE = 2,
    ADPS_ERROR_OUT_OF_MEMORY = 3,
    ADPS_ERROR_MQTT_ERROR = 4,
    ADPS_ERROR_DPS_ERROR = 5,
    ADPS_ERROR_TIMEOUT = 6,
    ADPS_ERROR_UNKNOWN = 7,
};

// Forward declarations
void ADPS_MQTT_Client_Module_OnMessage(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props);
void ADPS_MQTT_Client_Module_OnPublish(
    struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* props);

const char* topicArray[1] = { "$dps/registrations/res/#" };

static bool ADPS_MQTT_Client_Module_GetSubscriptTopics(int* count, char*** topics)
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
 * Example payload:
 *   {
 *      "operationId": "5.4501502c51dab402.6a8fa100-82f6-44fc-ae51-f90c2811abf5",
 *      "status": "assigned",
 *      "registrationState": {
 *      "x509": {
 *        "enrollmentGroupId": "contoso-blue-devbox-wus3"
 *      },
 *      "registrationId": "contoso-blue-device03-devbox-wus3",
 *      "createdDateTimeUtc": "2023-09-18T23:29:30.4838175Z",
 *      "assignedEndpoint": {
 *          "type": "mqttBroker",
 *          "hostName": "contoso-blue-devbox-wus3-eg.westus2-1.ts.eventgrid.azure.net"
 *      },
 *      "deviceId": "contoso-blue-device03-devbox-wus3",
 *      "status": "assigned",
 *      "substatus": "initialAssignment",
 *      "lastUpdatedDateTimeUtc": "2023-09-18T23:29:53.4296572Z",
 *      "etag": "IjJmMDBjNjEzLTAwMDAtNGQwMC0wMDAwLTY1MDhkZDcxMDAwMCI="
 *   }
 *
 * If the registration status is "assigned", then the device is registered
 *
 *
 */
bool ProcessDeviceRegistrationResponse(const char* payload, size_t payload_len)
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
        Log_Error("Failed to get status from JSON payload");
        goto done;
    }

    if (strncmp(status, "assigned", 8) == 0)
    {
        Log_Info("Device is registered.");
        SetRegisterState(ADPS_REGISTER_STATE_REGISTERED, "received assigned status");
    }
    else if (strncmp(status, "assigning", 9) == 0)
    {
        Log_Info("Device is registering.");
        SetRegisterState(ADPS_REGISTER_STATE_REGISTERING, "received assigning status");
    }
    else
    {
        Log_Error("Unknown status: %s", status);
        SetRegisterState(ADPS_REGISTER_STATE_FAILED, "received unknown status");
        // TODO (nox-msft) : determine whether to retry or bail
    }

done:
    return result;
}

/* Callback called when the client receives a message. */
void ADPS_MQTT_Client_Module_OnMessage(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    bool result = false;
    char** topics = NULL;
    int count = 0;
    Log_Info("on_message: Topic: %s; QOS: %d; mid: %d", msg->topic, msg->qos, msg->mid);

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
            Log_Info("Device is registered.");
            s_moduleState.lastRegisterResponseTime = nowTime;
            ProcessDeviceRegistrationResponse(msg->payload, msg->payloadlen);
        }
        else if (http_status_code == 202)
        {
            Log_Info("Sending status polling request.");
            // const char* status = NULL;
            JSON_Value* root_value = json_parse_string(msg->payload);
            if (root_value != NULL)
            {
                free(s_moduleState.operationId);
                s_moduleState.operationId = NULL;
                const char* optId = json_object_get_string(json_object(root_value), "operationId");
                s_moduleState.operationId = strdup(optId);
                json_value_free(root_value);
                root_value = NULL;

                //status = json_object_get_string(json_object(root_value), "status");

                SetRegisterState(ADPS_REGISTER_STATE_WAIT_TO_POLL, "received 202 response");
                // TODO (nox-msft) parst next-retry time from the topic string.
                s_moduleState.nextRegisterAttemptTime = nowTime + 3;
                goto done;
            }
        }
        else if (http_status_code == 404)
        {
            // TODO (nox-msft) : determine whether to retry or bail
        }
        else
        {
            // TODO (nox-msft) : determine whether to retry or bail
        }
    }

done:

    free(topics);
    Log_Info("\tPayload: %s\n", (char*)msg->payload);
}

/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
void ADPS_MQTT_Client_Module_OnPublish(
    struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* props)
{
    Log_Info("on_publish: Message with mid %d has been published.", mid);
}

/**
 * @brief Initialize the IoTHub Client module.
*/
int ADPS_MQTT_Client_Module_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* moduleInitData)
{
    bool success = false;

    IGNORED_PARAMETER(handle);
    IGNORED_PARAMETER(moduleInitData);

    // Need to set ret and goto done after this to ensure proper shutdown and deinitialization.
    // TODO (nox-msft) - set log level according to global config.
    ADUC_Logging_Init(0, "adps2-mqtt-client-module");

    if (!ReadAzureDPS2MqttSettings(&s_moduleState.settings))
    {
        Log_Error("Failed to read Azure DPS2 MQTT settings");
        goto done;
    }

    success = ADUC_CommunicationChannel_Initialize(
        s_moduleState.settings.registrationId,
        handle,
        &s_moduleState.settings.mqttSettings,
        &s_commChannelCallbacks,
        NULL);

    if (!success)
    {
        Log_Error("Failed to initialize the communication channel");
        goto done;
    }

done:
    return success ? 0 : -1;
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

bool DeviceRegistration_DoWork(/*const char* request_id, const char* connect_json*/)
{
    if (s_moduleState.registerState == ADPS_REGISTER_STATE_REGISTERED)
    {
        return true;
    }

    time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();

    if (s_moduleState.registerState == ADPS_REGISTER_STATE_REGISTERING)
    {
        if (nowTime > s_moduleState.nextRegisterAttemptTime)
        {
            SetRegisterState(ADPS_REGISTER_STATE_UNKNOWN, "time out");
        }
    }

    if (s_moduleState.registerState == ADPS_REGISTER_STATE_WAIT_TO_POLL)
    {
        if (nowTime > s_moduleState.nextRegisterAttemptTime)
        {
            char* pollTopic = ADUC_StringFormat(
                "$dps/registrations/GET/iotdps-get-operationstatus/?$rid=%s&operationId=%s",
                s_moduleState.settings.registrationId,
                s_moduleState.operationId);
            if (pollTopic == NULL)
            {
                Log_Error("Cannot allocate memory for registration status polling topic");
            }
            else
            {
                int mid = 0;
                int mqtt_ret = ADUC_CommunicationChannel_MQTT_Publish(
                    pollTopic,
                    &mid,
                    0,
                    NULL,
                    s_moduleState.settings.mqttSettings.qos,
                    false /* retain */,
                    NULL /* props */);

                if (mqtt_ret != MOSQ_ERR_SUCCESS)
                {
                    Log_Error("Failed to publish registration polling request (%d)", mqtt_ret);
                    // TODO (nox-msft) : determine whether to retry or bail
                }
                else
                {
                    SetRegisterState(ADPS_REGISTER_STATE_POLLING, "publishing message");
                    s_moduleState.nextRegisterAttemptTime = nowTime + ADPS_DEFAULT_REGISTER_REQUEST_DELAY_SECONDS;
                    s_moduleState.registerRetries++;
                }
            }
            free(pollTopic);
        }
        else
        {
            goto done;
        }
    }

    if (s_moduleState.registerState == ADPS_REGISTER_STATE_FAILED)
    {
        // TODO (nox-msft) : determine whether to retry or bail
        return false;
    }

    if (s_moduleState.registerState == ADPS_REGISTER_STATE_UNKNOWN && nowTime > s_moduleState.nextRegisterAttemptTime)
    {
        char topic_name[256];
        char device_registration_json[1024];
        time_t request_id = nowTime;
        snprintf(
            topic_name, sizeof(topic_name), "$dps/registrations/PUT/iotdps-register/?$rid=adps_req_%ld", request_id);
        snprintf(
            device_registration_json,
            sizeof(device_registration_json),
            "{\"registrationId\":\"%s\"}",
            s_moduleState.settings.registrationId);
        int mid = 0;
        int mqtt_ret = ADUC_CommunicationChannel_MQTT_Publish(
            topic_name,
            &mid,
            strlen(device_registration_json),
            device_registration_json,
            s_moduleState.settings.mqttSettings.qos,
            false /* retain */,
            NULL /* props */);

        if (mqtt_ret != MOSQ_ERR_SUCCESS)
        {
            Log_Error("Failed to publish registration request (%d)", mqtt_ret);
            // TODO (nox-msft) : determine whether to retry or bail
            return false;
        }
        else
        {
            SetRegisterState(ADPS_REGISTER_STATE_REGISTERING, "publishing message");
            s_moduleState.nextRegisterAttemptTime = nowTime + ADPS_DEFAULT_REGISTER_REQUEST_DELAY_SECONDS;
            s_moduleState.registerRetries++;
        }
    }

done:
    return false;
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

    ADUC_CommunicationChannel_Deinitialize();

    FreeAzureDPS2MqttSettings(&s_moduleState.settings);
    free(s_moduleState.operationId);
    json_value_free(s_moduleState.registrationData);

    ADUC_Logging_Uninit();
    return 0;
}

/**
 * @brief Create a Device Update Agent Module for IoT Hub PnP Client.
 *
 * @return ADUC_AGENT_MODULE_HANDLE The handle to the module.
 */
ADUC_AGENT_MODULE_HANDLE ADPS_MQTT_Client_Module_Create()
{
    ADUC_AGENT_MODULE_HANDLE handle = &s_moduleState;
    return handle;
}

/*
 * @brief Destroy the Device Update Agent Module for IoT Hub PnP Client.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 */
void ADPS_MQTT_Client_Module_Destroy(ADUC_AGENT_MODULE_HANDLE handle)
{
    IGNORED_PARAMETER(handle);
    Log_Info("DPS mqtt model destroy");
}

void OnCommandReceived(const char* commandName, const char* commandPayload, size_t payloadSize, void* context)
{
    IGNORED_PARAMETER(context);
    Log_Info("Received command %s with payload %s (size:%d)", commandName, commandPayload, payloadSize);
}

bool SubscribeToDSPModuleCommands()
{
    // TODO (nox-msft): subscribe to DPS module commands.
    // return InterModuleCommand_Subscribe("module.microsoft.dpsclient.ipcCommands", OnCommandReceived, NULL);
    return true;
}

void UnsubScribeToDSPModuleCommands()
{
    // TODO (nox-msft): unsubscribe to DPS module commands.
    // InterModuleCommand_Unsubscribe("module.microsoft.dpsclient.ipcCommands", OnCommandReceived, NULL);
}

bool IsConnectionResetRequested()
{
    // TODO (nox-msft): listen for a signal to reset the connection.
    return false;
}

/**
 * @brief Reset the connection.
 */
void ResetConnection()
{
    // TODO (nox-msft): ensure that we tear down the connection and re-establish it.
    s_moduleState.connected = s_moduleState.subscribed = false;
    s_moduleState.registerState = ADPS_REGISTER_STATE_UNKNOWN;
}

/**
 * @brief Perform the work for the extension. This must be a non-blocking operation.
 *
 * @return ADUC_Result
 */
int ADPS_MQTT_Client_Module_DoWork(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_CommunicationChannel_DoWork();

    if (ADUC_CommunicationChannel_IsConnected())
    {
        DeviceRegistration_DoWork();
    }

    return 0;
}

/**
 * @brief Get the Data object for the specified key.
 *
 * @param handle agent module handle
 * @param dataType data type
 * @param key data key/name
 * @param data return data
 * @param size return size of the return value
 * @return int 0 on success
*/
int ADPS_MQTT_Client_Module_GetData(
    ADUC_AGENT_MODULE_HANDLE handle, ADUC_MODULE_DATA_TYPE dataType, int key, void* data, int* size)
{
    IGNORED_PARAMETER(handle);
    IGNORED_PARAMETER(dataType);
    IGNORED_PARAMETER(key);
    IGNORED_PARAMETER(data);
    IGNORED_PARAMETER(size);
    int ret = 0;
    return ret;
}

/**
 * @brief Set the Data object for the specified key.
 *
 * @param handle agent module handle
 * @param dataType data type
 * @param key data key/name
 * @param data data buffer
 * @package size size of the data buffer
 */
int ADPS_MQTT_Client_Module_SetData(
    ADUC_AGENT_MODULE_HANDLE handle, ADUC_MODULE_DATA_TYPE dataType, int key, void* data, int size)
{
    IGNORED_PARAMETER(handle);
    IGNORED_PARAMETER(dataType);
    IGNORED_PARAMETER(key);
    IGNORED_PARAMETER(data);
    IGNORED_PARAMETER(size);
    int ret = 0;
    return ret;
}

ADUC_AGENT_MODULE_INTERFACE ADPS_MQTT_Client_ModuleInterface = {
    &s_moduleState,
    ADPS_MQTT_Client_Module_Create,
    ADPS_MQTT_Client_Module_Destroy,
    ADPS_MQTT_Client_Module_GetContractInfo,
    ADPS_MQTT_Client_Module_DoWork,
    ADPS_MQTT_Client_Module_Initialize,
    ADPS_MQTT_Client_Module_Deinitialize,
    ADPS_MQTT_Client_Module_GetData,
    ADPS_MQTT_Client_Module_SetData,
};

bool ADPS_MQTT_CLIENT_MODULE_IsDeviceRegistered(ADUC_AGENT_MODULE_HANDLE handle)
{
    return (s_moduleState.registerState == ADPS_REGISTER_STATE_REGISTERED);
}

const char* ADPS_MQTT_CLIENT_MODULE_GetMQTTBrokerEndpoint(ADUC_AGENT_MODULE_HANDLE handle)
{
    return json_object_dotget_string(
        json_value_get_object(s_moduleState.registrationData), "assignedEnpoint.hostName");
}
