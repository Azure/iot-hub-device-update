/**
 * @file adu_mqtt_client_module.c
 * @brief Implementation for the Device Update agent module for the Event Grid MQTT broker.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_mqtt_client_module.h"
#include "aduc/adu_agent_info_management.h"
#include "aduc/adu_communication_channel.h"
#include "aduc/adu_enrollment_management.h"
#include "aduc/adu_mosquitto_utils.h"
#include "aduc/adu_mqtt_protocol.h"
#include "aduc/adu_types.h"
#include "aduc/adu_updates_management.h"
#include "aduc/config_utils.h"
#include "aduc/eg_mqtt_broker_client.h"
#include "aduc/logging.h"
#include "aduc/retry_utils.h"
#include "aduc/string_c_utils.h"
#include "du_agent_sdk/agent_module_interface.h"

#include "aducpal/time.h" // time_t
#include "errno.h"
#include "string.h"
#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h> // mosquitto_property

// Forward declarations
static int ADUC_MQTT_Client_Module_Initialize_DoWork(ADUC_MQTT_CLIENT_MODULE_STATE* state);

#define DEFAULT_OPERATION_INTERVAL_SECONDS (5)

static ADUC_MQTT_CLIENT_MODULE_STATE s_moduleState = { 0 };

/**
 * @brief Get the device ID to use for MQTT client identification.
 *
 * This function retrieves the device ID to be used as the MQTT client's identifier. It checks
 * various sources, including the client ID, username, and falls back to "unknown" if no
 * suitable device ID is found.
 *
 * @return A pointer to a null-terminated string representing the device ID.
 * @remark The caller is responsible for freeing the returned string by calling free().
 */
static char* GetDeviceIdHelper()
{
    char* deviceId = NULL;

    // Get device id setting from the du-config.json file.
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    if (config == NULL)
    {
        goto done;
    }

    const ADUC_AgentInfo* agent = ADUC_ConfigInfo_GetAgent(config, 0);
    if (agent != NULL)
    {
        // NOTE - For MQTT broker, userName must match the device Id (usually the CN of the device's certificate)
        if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent, "mqttBroker.username", &deviceId))
        {
            deviceId = NULL;
        }
    }

done:
    return deviceId;
}

/**
 * @brief Deinitialize the MQTT topics used for Azure Device Update communication.
 * @return `true` if MQTT topics were successfully de-initialized; otherwise, `false`.
*/
static void DeinitMqttTopics()
{
    free(s_moduleState.mqtt_topic_service2agent);
    s_moduleState.mqtt_topic_service2agent = NULL;
    free(s_moduleState.mqtt_topic_agent2service);
    s_moduleState.mqtt_topic_agent2service = NULL;
}

/**
 * @brief Initializes MQTT topics for Azure Device Update communication.
 *
 * This function prepares MQTT topics for communication between an Azure Device Update client and server.
 * It constructs topics based on templates and the device ID, ensuring that the required topics are ready for use.
 *
 * @return `true` if MQTT topics were successfully prepared; otherwise, `false`.
 */
bool InitMqttTopics()
{
    bool result = false;
    const char* deviceId;

    if (!IsNullOrEmpty(s_moduleState.mqtt_topic_service2agent)
        && !IsNullOrEmpty(s_moduleState.mqtt_topic_agent2service))
    {
        return true;
    }

    deviceId = GetDeviceIdHelper();

    if (IsNullOrEmpty(deviceId))
    {
        Log_Error("Invalid device id.");
        goto done;
    }

    // Create string for agent to service topic, from PUBLISH_TOPIC_TEMPLATE_ADU_OTO and the device id.
    s_moduleState.mqtt_topic_agent2service = ADUC_StringFormat(PUBLISH_TOPIC_TEMPLATE_ADU_OTO, deviceId);
    if (s_moduleState.mqtt_topic_agent2service == NULL)
    {
        goto done;
    }

    // Create string for service to agent topic, from SUBSCRIBE_TOPIC_TEMPLATE_ADU_OTO and the device id.
    s_moduleState.mqtt_topic_service2agent = ADUC_StringFormat(SUBSCRIBE_TOPIC_TEMPLATE_ADU_OTO, deviceId);
    if (s_moduleState.mqtt_topic_service2agent == NULL)
    {
        goto done;
    }

    result = true;

done:
    if (!result)
    {
        DeinitMqttTopics();
    }
    return result;
}

/**
 * @brief Free resources allocated for the module state.
 * @param state The module state to free.
 */
void ADUC_MQTT_CLIENT_MODULE_STATE_Free(ADUC_MQTT_CLIENT_MODULE_STATE* state)
{
    DeinitMqttTopics();
    FreeMqttBrokerSettings(&state->mqttSettings);
}

/**
 * @brief The contract info for the module.
 */
ADUC_AGENT_CONTRACT_INFO g_egMqttClientContractInfo = {
    "Microsoft", "Event Grid MQTT Client  Module", 1, "Microsoft/AzureEGMQTTClientModule:1"
};

/**
 * @brief Gets the extension contract info.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 * @return ADUC_AGENT_CONTRACT_INFO The extension contract info.
 */
const ADUC_AGENT_CONTRACT_INFO* ADUC_MQTT_Client_Module_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle)
{
    IGNORED_PARAMETER(handle);
    return &g_egMqttClientContractInfo;
}

/* Callback called when the client receives a message. */
void ADUC_MQTT_Client_OnMessage(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    char* msg_type = NULL;

    Log_Info("on_message: Topic: %s; QOS: %d; mid: %d", msg->topic, msg->qos, msg->mid);

    // All parameters are required.
    if (mosq == NULL || obj == NULL || msg == NULL || props == NULL)
    {
        Log_Error("OnMessage: Null parameter (mosq=%p, obj=%p, msg=%p, props=%p)", mosq, obj, msg, props);
        goto done;
    }

    // Get the message type. (mt = message type)
    if (!ADU_mosquitto_read_user_property_string(props, "mt", &msg_type) || IsNullOrEmpty(msg_type))
    {
        Log_Warn("on_message: Failed to read the message type. Ignore the message.");
        goto done;
    }

    // Only support protocol version 1
    if (!ADU_mosquitto_has_user_property(props, "pid", "1"))
    {
        Log_Warn("on_message: Ignore the message. (pid != 1)");
        goto done;
    }

    // Route message to the appropriate component.

    if (strcmp(msg_type, "enr_resp") == 0)
    {
        OnMessage_enr_resp(mosq, obj, msg, props);
        goto done;
    }

    if (strcmp(msg_type, "enr_cn") == 0)
    {
        OnMessage_enr_cn(mosq, obj, msg, props);
        goto done;
    }

    if (strcmp(msg_type, "ainfo_resp") == 0)
    {
        OnMessage_ainfo_resp(mosq, obj, msg, props);
        goto done;
    }

    if (strcmp(msg_type, "upd_cn") == 0)
    {
        OnMessage_upd_cn(mosq, obj, msg, props);
        goto done;
    }

    if (strcmp(msg_type, "upd_resp") == 0)
    {
        OnMessage_upd_resp(mosq, obj, msg, props);
        goto done;
    }

    /* This blindly prints the payload, but the payload can be anything so take care. */
    Log_Info("\tPayload: %s\n", (char*)msg->payload);

done:
    free(msg_type);
}

static char* s_subscriptionTopics[1];

/**
 * @brief Get the MQTT subscription topics for the MQTT client.
 *
 * This function retrieves the MQTT subscription topics used by the MQTT client for message reception.
 * It initializes the MQTT topics if not already initialized and returns a pointer to the array of topics
 * along with the count of topics.
 *
 * @param count A pointer to an integer where the count of subscription topics will be stored.
 * @param topics A pointer to a pointer to char arrays (strings) where the subscription topics will be stored.
 *
 * @return true if the operation was successful; false if any of the input parameters are NULL or if initialization fails.
 *
 * @note The caller MUST NOTE freeing the memory allocated for 'topics' and its contents.
 */
bool ADUC_MQTT_Client_GetSubscriptionTopics(int* count, char*** topics)
{
    if (count == NULL || topics == NULL)
    {
        Log_Error("NULL parameter");
        return false;
    }

    if (!InitMqttTopics())
    {
        Log_Error("Failed to initialize MQTT topics");
        return false;
    }

    // Always have only 1 topic to subscribe.
    *count = 1;
    s_subscriptionTopics[0] = s_moduleState.mqtt_topic_service2agent;
    *topics = s_subscriptionTopics;
    return true;
}

/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
void ADUC_MQTT_Client_OnPublish(
    struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* props)
{
    // TODO (nox-msft) - route the message to the appropriate component.
    Log_Info("on_publish: Message with mid %d has been published.", mid);
}

/**
 * @brief Callbacks for various event from the communication channel.
 */
ADUC_MQTT_CALLBACKS s_commChannelCallbacks = {
    NULL /*on_connect*/,
    NULL /*on_disconnect*/,
    ADUC_MQTT_Client_GetSubscriptionTopics,
    NULL /*on_subscribe*/,
    NULL /*on_unsubscribe*/,
    ADUC_MQTT_Client_OnPublish,
    ADUC_MQTT_Client_OnMessage,
    NULL /*on_log*/
};

/**
 * @brief Initialize the IoTHub Client module.
 */
int ADUC_MQTT_Client_Module_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* moduleInitData)
{
    ADUC_MQTT_CLIENT_MODULE_STATE* state = (ADUC_MQTT_CLIENT_MODULE_STATE*)handle;
    if (state == NULL)
    {
        Log_Error("handle is NULL");
        return -1;
    }

    state->moduleInitData = moduleInitData;

    // Need to set ret and goto done after this to ensure proper shutdown and deinitialization.
    // TODO (nox-msft) - set log level according to global config.
    ADUC_Logging_Init(0, "du-mqtt-client-module");

    bool success = ReadMqttBrokerSettings(&s_moduleState.mqttSettings);
    if (!success)
    {
        Log_Error("Failed to read MQTT settings");
        state->initializeState = ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE_FAILED;
        goto done;
    }

    if (s_moduleState.mqttSettings.hostnameSource == ADUC_MQTT_HOSTNAME_SOURCE_DPS)
    {
        Log_Info("Hostname source will be provided by DPS.");
        state->initializeState = ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE_PARTIAL;
        success = true;
    }

    success = ADUC_MQTT_Client_Module_Initialize_DoWork(state);
done:
    return success ? 0 : -1;
}

static int ADUC_MQTT_Client_Module_Initialize_DoWork(ADUC_MQTT_CLIENT_MODULE_STATE* state)
{
    bool success = false;
    if (state->initializeState == ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE_FAILED)
    {
        goto done;
    }

    if (state->mqttSettings.hostnameSource == ADUC_MQTT_HOSTNAME_SOURCE_DPS)
    {
        // Ensure that we have hostName.
        if (IsNullOrEmpty(state->mqttSettings.hostname))
        {
            Log_Info("MQTT broker hostname is empty. Retry in %d seconds", DEFAULT_OPERATION_INTERVAL_SECONDS);
            state->initializeState = ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE_PARTIAL;
            // TODO (nox-msft) : use retry_utils
            state->nextOperationTime = ADUC_GetTimeSinceEpochInSeconds() + DEFAULT_OPERATION_INTERVAL_SECONDS;
            goto done;
        }
    }

    success = InitMqttTopics();
    if (!success)
    {
        Log_Error("Failed to initialize MQTT topics");
        state->initializeState = ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE_FAILED;
        goto done;
    }

    // Ensure that the communication channel is valid.
    if (IsNullOrEmpty(state->mqttSettings.hostname))
    {
        Log_Error("MQTT broker hostname is not specified");
        state->initializeState = ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE_PARTIAL;
        // TODO (nox-msft) : use retry_utils
        state->nextOperationTime = ADUC_GetTimeSinceEpochInSeconds() + DEFAULT_OPERATION_INTERVAL_SECONDS;
        success = false;
        goto done;
    }

    success = ADUC_CommunicationChannel_Initialize(
        GetDeviceIdHelper(), state, &s_moduleState.mqttSettings, &s_commChannelCallbacks, NULL);
    if (!success)
    {
        Log_Error("Failed to initialize the communication channel");
        goto done;
    }

    success = ADUC_Enrollment_Management_Initialize();

    if (!success)
    {
        Log_Error("Failed to initialize the enrollment management");
        goto done;
    }

    success = ADUC_Enrollment_Management_SetAgentToServiceTopic(s_moduleState.mqtt_topic_agent2service);
    if (!success)
    {
        Log_Error("Failed to set the agent to service topic");
        goto done;
    }

    success = ADUC_Agent_Info_Management_Initialize();
    if (!success)
    {
        Log_Error("Failed to initialize the agent info management");
        goto done;
    }

    success = ADUC_Updates_Management_Initialize();
    if (!success)
    {
        Log_Error("Failed to initialize the updates management");
        goto done;
    }

done:
    return success ? 0 : -1;
}

/**
 * @brief Sets the MQTT broker endpoint.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 * @param mqttBrokerEndpoint The MQTT broker endpoint to use.
 */
int ADUC_MQTT_CLIENT_MODULE_SetMQTTBrokerEndpoint(ADUC_AGENT_MODULE_HANDLE* handle, const char* mqttBrokerEndpoint)
{
    ADUC_MQTT_CLIENT_MODULE_STATE* state = (ADUC_MQTT_CLIENT_MODULE_STATE*)handle;
    if (state == NULL || IsNullOrEmpty(mqttBrokerEndpoint))
    {
        Log_Error("Invalid parameter (state=%p, mqttBrokerEndpoint=%p)", state, mqttBrokerEndpoint);
        return -1;
    }

    free(state->mqttSettings.hostname);
    state->mqttSettings.hostname = NULL;

    state->mqttSettings.hostname = strdup(mqttBrokerEndpoint);
    if (state->mqttSettings.hostname == NULL)
    {
        Log_Error("Failed to allocate memory for the MQTT broker endpoint");
        return -1;
    }
    return 0;
}

/**
 * @brief Deinitialize the IoTHub Client module.
 *
 * @param module The handle to the module. This is the same handle that was returned by the Create function.
 *
 * @return int 0 on success.
 */
int ADUC_MQTT_Client_Module_Deinitialize(ADUC_AGENT_MODULE_HANDLE module)
{
    Log_Info("Deinitialize");

    ADUC_CommunicationChannel_Deinitialize();

    ADUC_MQTT_CLIENT_MODULE_STATE_Free(&s_moduleState);

    ADUC_Logging_Uninit();
    return 0;
}

/**
 * @brief Create a Device Update Agent Module for IoT Hub PnP Client.
 *
 * @return ADUC_AGENT_MODULE_HANDLE The handle to the module.
 */
ADUC_AGENT_MODULE_HANDLE ADUC_MQTT_Client_Module_Create()
{
    ADUC_AGENT_MODULE_HANDLE handle = &s_moduleState;
    return handle;
}

/*
 * @brief Destroy the Device Update Agent Module for IoT Hub PnP Client.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 */
void ADUC_MQTT_Client_Module_Destroy(ADUC_AGENT_MODULE_HANDLE handle)
{
    IGNORED_PARAMETER(handle);
    Log_Info("Destroy");
}

/**
 * @brief Perform the work for the extension. This must be a non-blocking operation.
 *
 * @return ADUC_Result
 */
int ADUC_MQTT_Client_Module_DoWork(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_MQTT_CLIENT_MODULE_STATE* state = (ADUC_MQTT_CLIENT_MODULE_STATE*)handle;
    if (handle == NULL)
    {
        Log_Error("handle is NULL");
        return -1;
    }

    time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();
    if (state->mqttSettings.hostnameSource == ADUC_MQTT_HOSTNAME_SOURCE_DPS
        && state->initializeState == ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE_PARTIAL)
    {
        if (state->nextOperationTime > nowTime)
        {
            goto done;
        }

        if (ADUC_MQTT_Client_Module_Initialize_DoWork(state) != 0)
        {
            state->nextOperationTime = nowTime + DEFAULT_OPERATION_INTERVAL_SECONDS;
            goto done;
        }
        else
        {
            state->nextOperationTime = 0;
        }
    }

    ADUC_CommunicationChannel_DoWork();
    if (!ADUC_CommunicationChannel_IsConnected())
    {
        // TODO: (nox-msft) - use retry utils.
        state->nextOperationTime = nowTime + 5;
        goto done;
    }

    ADUC_Enrollment_Management_DoWork();
    ADUC_Agent_Info_Management_DoWork();
    ADUC_Updates_Management_DoWork();
done:
    return 0;
}

/**
 * @brief Get the Data object for the specified key.
 *
 * @param handle agent module handle
 * @param dataType data type
 * @param key data key/name
 * @param data return buffer (call must free the memory of the return value once done with it)
 * @param size return size of the return value
 * @return int 0 on success
*/
int ADUC_MQTT_Client_Module_GetData(
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
int ADUC_MQTT_Client_Module_SetData(
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

ADUC_AGENT_MODULE_INTERFACE ADUC_MQTT_Client_ModuleInterface = {
    &s_moduleState,
    ADUC_MQTT_Client_Module_Create,
    ADUC_MQTT_Client_Module_Destroy,
    ADUC_MQTT_Client_Module_GetContractInfo,
    ADUC_MQTT_Client_Module_DoWork,
    ADUC_MQTT_Client_Module_Initialize,
    ADUC_MQTT_Client_Module_Deinitialize,
    ADUC_MQTT_Client_Module_GetData,
    ADUC_MQTT_Client_Module_SetData,
};