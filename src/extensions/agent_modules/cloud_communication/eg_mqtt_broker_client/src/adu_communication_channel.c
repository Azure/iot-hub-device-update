/**
 * @file adu_enrollment_management.c
 * @brief Implementation for device enrollment status management.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_communication_channel.h" // ADUC_EnsureValidCommunicationChannel
#include "aduc/adu_mosquitto_utils.h"
#include "aduc/adu_mqtt_protocol.h" // ADUC_MQTT_TOPICS
#include "aduc/config_utils.h"
#include "aduc/logging.h"
#include "aduc/mqtt_client.h"
#include "aduc/string_c_utils.h" // IsNullOrEmpty, ADUC_StringFormat

#include "du_agent_sdk/agent_module_interface.h" // ADUC_AGENT_MODULE_INTERFACE and related functions
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aducpal/time.h> // time_t
#include <errno.h> // errno
#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h> // mosquitto_property

// Default MQTT protocol version for Event Grid MQTT Broker is v5 (5)
#define DEFAULT_EG_MQTT_PROTOCOL_VERSION 5

#define REQUIRED_TLS_SET_CERT_PATH "L"

#define DEFAULT_CONNECT_RETRY_DELAY_SECONDS (60 * 5) /* 5 seconds */

/**
 * @brief Structure to hold MQTT communication manager state.
 */
typedef struct ADU_MQTT_COMMUNICATION_MGR_STATE_TAG
{
    bool initialized; /**< Indicates if the MQTT communication manager is initialized. */
    bool topicsSubscribed; /**< Indicates if MQTT topics are subscribed. */
    struct mosquitto* mqttClient; /**< Pointer to the MQTT client. */
    ADUC_MQTT_TOPICS mqttTopics; /**< MQTT topics configuration. */
    ADUC_MQTT_SETTINGS mqttSettings; /**< MQTT settings. */
    ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE commState; /**< Communication channel state. */
    time_t commStateUpdatedTime; /**< Time when the communication channel state was last updated. */
    time_t commLastConnectedTime; /**< Time when the communication channel was last connected. */
    time_t commLastAttemptTime; /**< Time when the last connection attempt was made. */
    time_t commNextRetryTime; /**< Time when the next connection attempt should be made. */
    ADUC_MQTT_CALLBACKS mqttCallbacks; /**< MQTT callback functions. */
} ADU_MQTT_COMMUNICATION_MGR_STATE;

static ADU_MQTT_COMMUNICATION_MGR_STATE s_commMgrState = { 0 };

// Forward declarations.
void Free_ADU_MQTT_Topics();
static time_t ADUC_SetCommunicationChannelState(ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE state);
void ADUC_MQTT_CLient_OnDisconnect(struct mosquitto* mosq, void* obj, int rc, const mosquitto_property* props);
void ADUC_CommunicationChannel_OnConnect(
    struct mosquitto* mosq, void* obj, int reason_code, int flags, const mosquitto_property* props);
void ADUC_CommunicationChannel_OnMessage(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props);
void ADUC_CommunicationChannel_OnPublish(
    struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* props);
void ADUC_CommunicationChannel_OnSubscribe(
    struct mosquitto* mosq,
    void* obj,
    int mid,
    int qos_count,
    const int* granted_qos,
    const mosquitto_property* props);
void ADUC_CommunicationChannel_OnLog(struct mosquitto* mosq, void* obj, int level, const char* str);

/**
 * @brief Get the device ID to use for MQTT client identification.
 *
 * This function retrieves the device ID to be used as the MQTT client's identifier. It checks
 * various sources, including the client ID, username, and falls back to "unknown" if no
 * suitable device ID is found.
 *
 * @return A pointer to a null-terminated string representing the device ID.
 */
static const char* GetDeviceIdHelper()
{
    if (!IsNullOrEmpty(s_commMgrState.mqttSettings.clientId))
    {
        return s_commMgrState.mqttSettings.clientId;
    }

    if (!IsNullOrEmpty(s_commMgrState.mqttSettings.username))
    {
        return s_commMgrState.mqttSettings.username;
    }

    return "unknown";
}

/**
 * @brief Prepare MQTT topics for Azure Device Update communication.
 *
 * This function prepares MQTT topics for communication between an Azure Device Update client and server.
 * It constructs topics based on templates and the device ID, ensuring that the required topics are ready for use.
 *
 * @return `true` if MQTT topics were successfully prepared; otherwise, `false`.
 */
bool Prepare_ADU_MQTT_Topics()
{
    bool result = false;
    const char* deviceId = GetDeviceIdHelper();

    if (IsNullOrEmpty(deviceId))
    {
        Log_Error("Invalid device id.");
        goto done;
    }

    // Create string for agent to service topic, from PUBLISH_TOPIC_TEMPLATE_ADU_OTO and the device id.
    s_commMgrState.mqttTopics.agentOTO = ADUC_StringFormat(PUBLISH_TOPIC_TEMPLATE_ADU_OTO, deviceId);
    if (s_commMgrState.mqttTopics.agentOTO == NULL)
    {
        goto done;
    }

    // Create string for service to agent topic, from SUBSCRIBE_TOPIC_TEMPLATE_ADU_OTO and the device id.
    s_commMgrState.mqttTopics.serviceOTO = ADUC_StringFormat(SUBSCRIBE_TOPIC_TEMPLATE_ADU_OTO, deviceId);
    if (s_commMgrState.mqttTopics.serviceOTO == NULL)
    {
        goto done;
    }

    result = true;

done:
    if (!result)
    {
        Free_ADU_MQTT_Topics();
    }
    return result;
}

/**
 * @brief Free resources allocated for MQTT topics.
*/
void Free_ADU_MQTT_Topics()
{
    if (s_commMgrState.mqttTopics.agentOTO != NULL)
    {
        free(s_commMgrState.mqttTopics.agentOTO);
        s_commMgrState.mqttTopics.agentOTO = NULL;
    }

    if (s_commMgrState.mqttTopics.serviceOTO != NULL)
    {
        free(s_commMgrState.mqttTopics.serviceOTO);
        s_commMgrState.mqttTopics.serviceOTO = NULL;
    }
}

/**
 * @brief Free resources allocated for MQTT broker settings.
 */
void FreeMqttBrokerSettings(ADUC_MQTT_SETTINGS* settings)
{
    if (settings == NULL)
    {
        return;
    }

    free(settings->hostname);
    free(settings->clientId);
    free(settings->username);
    free(settings->password);
    free(settings->caFile);
    free(settings->certFile);
    free(settings->keyFile);
    free(settings->keyFilePassword);
    memset(settings, 0, sizeof(*settings));
}

/**
 * @brief Read the MQTT broker connection data from the config file.
 *
 * @remark The caller is responsible for freeing the returned settings by calling FreeMqttBrokerSettings function.
*/
bool ReadMqttBrokerSettings(ADUC_MQTT_SETTINGS* settings)
{
    bool result = false;
    const ADUC_ConfigInfo* config = NULL;
    const ADUC_AgentInfo* agent_info = NULL;

    if (settings == NULL)
    {
        goto done;
    }

    memset(settings, 0, sizeof(*settings));

    config = ADUC_ConfigInfo_GetInstance();
    if (config == NULL)
    {
        goto done;
    }

    agent_info = ADUC_ConfigInfo_GetAgent(config, 0);

    // Currently only support 'ADPS2/MQTT' connection data.
    if (strcmp(agent_info->connectionType, ADUC_CONNECTION_TYPE_MQTTBROKER) != 0)
    {
        goto done;
    }

    // Read the x.506 certificate authentication data.
    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.caFile", &settings->caFile))
    {
        settings->caFile = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.certFile", &settings->certFile))
    {
        settings->certFile = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.keyFile", &settings->keyFile))
    {
        settings->keyFile = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(
            agent_info, "mqttBroker.keyFilePassword", &settings->keyFilePassword))
    {
        settings->keyFilePassword = NULL;
    }

    // TODO (nox-msft) - For MQTT broker, userName must match the device Id (usually the CN of the device's certificate)
    if (IsNullOrEmpty(settings->username))
    {
        if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.username", &settings->username))
        {
            settings->username = NULL;
            Log_Error("Username field is not specified");
            goto done;
        }
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.password", &settings->password))
    {
        settings->password = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.hostname", &settings->hostname))
    {
        Log_Info("MQTT Broker hostname is not specified");
    }

    // Common MQTT connection data fields.
    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(
            agent_info, "mqttBroker.mqttVersion", &settings->mqttVersion))
    {
        Log_Info("Using default MQTT protocol version: %d", DEFAULT_MQTT_BROKER_PROTOCOL_VERSION);
        settings->mqttVersion = DEFAULT_MQTT_BROKER_PROTOCOL_VERSION;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(agent_info, "mqttBroker.tcpPort", &settings->tcpPort))
    {
        Log_Info("Using default TCP port: %d", DEFAULT_TCP_PORT);
        settings->tcpPort = DEFAULT_TCP_PORT;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetBooleanField(agent_info, "mqttBroker.useTLS", &settings->useTLS))
    {
        Log_Info("UseTLS: %d", DEFAULT_USE_TLS);
        settings->useTLS = DEFAULT_USE_TLS;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetBooleanField(agent_info, "mqttBroker.cleanSession", &settings->cleanSession))
    {
        Log_Info("CleanSession: %d", DEFAULT_ADPS_CLEAN_SESSION);
        settings->cleanSession = DEFAULT_ADPS_CLEAN_SESSION;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(
            agent_info, "mqttBroker.keepAliveInSeconds", &settings->keepAliveInSeconds))
    {
        Log_Info("Keep alive: %d sec.", DEFAULT_KEEP_ALIVE_IN_SECONDS);
        settings->keepAliveInSeconds = DEFAULT_KEEP_ALIVE_IN_SECONDS;
    }

    result = true;

done:
    ADUC_ConfigInfo_ReleaseInstance(config);
    if (!result)
    {
        FreeMqttBrokerSettings(settings);
    }
    return result;
}

/* Callback called when the broker has received the DISCONNECT command and has disconnected the
 * client. */
void ADUC_CommunicationChannel_OnDisconnect(struct mosquitto* mosq, void* obj, int rc, const mosquitto_property* props)
{
    Log_Info("on_disconnect: reason=%s", mosquitto_strerror(rc));
    ADUC_SetCommunicationChannelState(ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_DISCONNECTED);

    // TODO (nox-msft) - select retry strategy based on error code.
    //
    // ADUC_MQTT_DISCONNECTION_CATEGORY category = CategorizeMQTTDisconnection(rc);
    // switch (category)
    // {
    // case ADUC_MQTT_DISCONNECTION_CATEGORY_TRANSIENT:
    //     break;
    // case ADUC_MQTT_DISCONNECTION_CATEGORY_NON_RECOVERABLE:
    //     // This may require the application restart
    //     break;
    // case ADUC_MQTT_DISCONNECTION_CATEGORY_OTHER:
    // default:
    //     break;
    // }
    //
    // For now, wait 30 seconds before retrying.
    s_commMgrState.commNextRetryTime = ADUC_GetTimeSinceEpochInSeconds() + DEFAULT_CONNECT_RETRY_DELAY_SECONDS;

    ADUC_SetCommunicationChannelState(ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_DISCONNECTED);

    if (s_commMgrState.mqttCallbacks.on_disconnect_v5 != NULL)
    {
        s_commMgrState.mqttCallbacks.on_disconnect_v5(mosq, obj, rc, props);
    }
}

/*
 * @brief  Initialize the communication channel.
 * @param[in] session_id The session ID to use for the MQTT client.
 * @param[in] module_state The module state to use for the MQTT client.
 * @param[in] callbacks The MQTT callbacks to use for the MQTT client.
 * @return true if the communication channel was successfully initialized; otherwise, false.
 */
bool ADUC_CommunicationChannel_Initialize(const char* session_id, void* module_state, ADUC_MQTT_CALLBACKS* callbacks)
{
    int mqtt_res = 0;
    bool use_OS_cert = false;

    if (s_commMgrState.initialized)
    {
        return true;
    }

    // Copy all callbacks address.
    memcpy(&s_commMgrState.mqttCallbacks, callbacks, sizeof(ADUC_MQTT_CALLBACKS));

    if (!ReadMqttBrokerSettings(&s_commMgrState.mqttSettings))
    {
        Log_Error("Failed to read Azure DPS2 MQTT settings");
        goto done;
    }

    if (!Prepare_ADU_MQTT_Topics())
    {
        goto done;
    }

    Log_Info("Initialize Mosquitto library");
    mosquitto_lib_init();

    s_commMgrState.mqttClient = mosquitto_new(session_id, s_commMgrState.mqttSettings.cleanSession, module_state);
    if (!s_commMgrState.mqttClient)
    {
        Log_Error("Failed to create Mosquitto client");
        return -1; // Initialization failed
    }

    mqtt_res = mosquitto_int_option(
        s_commMgrState.mqttClient, MOSQ_OPT_PROTOCOL_VERSION, s_commMgrState.mqttSettings.mqttVersion);
    if (mqtt_res != MOSQ_ERR_SUCCESS)
    {
        Log_Error("Failed to set MQTT protocol version (%d)", mqtt_res);
        goto done;
    }

    if (!IsNullOrEmpty(s_commMgrState.mqttSettings.username))
    {
        mqtt_res = mosquitto_username_pw_set(
            s_commMgrState.mqttClient, s_commMgrState.mqttSettings.username, s_commMgrState.mqttSettings.password);
    }

    if (s_commMgrState.mqttSettings.useTLS)
    {
        use_OS_cert = IsNullOrEmpty(s_commMgrState.mqttSettings.caFile);
        if (use_OS_cert)
        {
            mqtt_res = mosquitto_int_option(s_commMgrState.mqttClient, MOSQ_OPT_TLS_USE_OS_CERTS, true);
            if (mqtt_res != MOSQ_ERR_SUCCESS)
            {
                Log_Error("Failed to set TLS use OS certs (%d)", mqtt_res);
                // TODO (nox-msft) - Handle all error codes.
                goto done;
            }
        }

        // TODO (nox-msft) - Set X.509 certificate here (if using certificate-based authentication)
        mqtt_res = mosquitto_tls_set(
            s_commMgrState.mqttClient,
            s_commMgrState.mqttSettings.caFile,
            use_OS_cert ? REQUIRED_TLS_SET_CERT_PATH : NULL,
            s_commMgrState.mqttSettings.certFile,
            s_commMgrState.mqttSettings.keyFile,
            NULL);

        if (mqtt_res != MOSQ_ERR_SUCCESS)
        {
            // Handle all error codes.
            switch (mqtt_res)
            {
            case MOSQ_ERR_INVAL:
                Log_Error("Invalid parameter");
                break;

            case MOSQ_ERR_NOMEM:
                Log_Error("Out of memory");
                break;

            default:
                Log_Error("Failed to set TLS settings (%d)", mqtt_res);
                break;
            }

            goto done;
        }
    }

    // Register callbacks.
    mosquitto_connect_v5_callback_set(s_commMgrState.mqttClient, ADUC_CommunicationChannel_OnConnect);
    mosquitto_disconnect_v5_callback_set(s_commMgrState.mqttClient, ADUC_CommunicationChannel_OnDisconnect);
    mosquitto_subscribe_v5_callback_set(s_commMgrState.mqttClient, ADUC_CommunicationChannel_OnSubscribe);
    mosquitto_publish_v5_callback_set(s_commMgrState.mqttClient, ADUC_CommunicationChannel_OnPublish);
    mosquitto_message_v5_callback_set(s_commMgrState.mqttClient, ADUC_CommunicationChannel_OnMessage);
    mosquitto_log_callback_set(s_commMgrState.mqttClient, ADUC_CommunicationChannel_OnLog);

    s_commMgrState.initialized = true;
done:
    return s_commMgrState.initialized;
}

void ADUC_CommunicationChannel_Reset()
{
    // Disconnect the client. (best effort)
    mosquitto_disconnect(s_commMgrState.mqttClient);
    ADUC_SetCommunicationChannelState(ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_UNKNOWN);

    // TODO (nox-msft) - compute the next retry time.
    s_commMgrState.commNextRetryTime = ADUC_GetTimeSinceEpochInSeconds() + DEFAULT_CONNECT_RETRY_DELAY_SECONDS;
}

void ADUC_CommunicationChannel_Deinitialize()
{
    if (s_commMgrState.initialized)
    {
        if (s_commMgrState.mqttClient != NULL)
        {
            Log_Info("Disconnecting MQTT client");
            mosquitto_disconnect(s_commMgrState.mqttClient);
            Log_Info("Destroying MQTT client");
            mosquitto_destroy(s_commMgrState.mqttClient);
            s_commMgrState.mqttClient = NULL;
        }

        mosquitto_lib_cleanup();
        s_commMgrState.initialized = false;
    }
    Free_ADU_MQTT_Topics();
}

/**
 * @brief Set the communication channel state.
 * @param state The new state.
 * @return The current time.
 */
static time_t ADUC_SetCommunicationChannelState(ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE state)
{
    time_t nowTime = time(NULL);
    if (s_commMgrState.commState != state)
    {
        Log_Info("Comm channel state changed %d --> %d", s_commMgrState.commState, state);
        s_commMgrState.commState = state;
    }

    return nowTime;
}

/**
 * @brief Callback called when the client receives a CONNACK message from the broker.
 * @param mosq The mosquitto instance making the callback.
 * @param obj The user data provided in mosquitto_new.
 * @param reason_code The return code from the broker.
 * @param flags The Connect flags.
 * @param props The MQTT v5 properties returned with the CONNACK.
 * @return void
 *
 * @remark This callback is called even if the connection fails.
 * @remark This callback is called by the network thread.
 * @remark Do not use this callback for general message processing. Use the on_message callback instead.
 * @remark Do not use blocking functions in this callback.
 *
 * @remark If the connection status is successful, the client is connected to the broker.
 *         The DU communication channel state is set to ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED.
 */
void ADUC_CommunicationChannel_OnConnect(
    struct mosquitto* mosq, void* obj, int reason_code, int flags, const mosquitto_property* props)
{
    // Print out the connection result. mosquitto_connack_string() produces an
    // appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
    // clients is mosquitto_reason_string().
    if ((int)s_commMgrState.mqttSettings.mqttVersion == MQTT_PROTOCOL_V5)
    {
        Log_Info("on_connect: %s", mosquitto_reason_string(reason_code));
    }
    else
    {
        Log_Info("on_connect: %s", mosquitto_connack_string(reason_code));
    }

    if (reason_code == 0)
    {
        Log_Info("Connection accepted - flags: %d", flags);
        s_commMgrState.commLastConnectedTime =
            ADUC_SetCommunicationChannelState(ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED);
        goto done;
    }

    switch (reason_code)
    {
    case MQTT_RC_QUOTA_EXCEEDED:
        Log_Error("Connection refused - quota exceeded");
        break;
    }

    // TODO (nox-msft) - Handle all error codes.
    // For now, just retry after 5 minutes.
    s_commMgrState.commNextRetryTime = ADUC_GetTimeSinceEpochInSeconds() + DEFAULT_CONNECT_RETRY_DELAY_SECONDS;

    // For now, just disconnect to prevent the client from attempting to reconnect.
    int rc;
    if ((rc = mosquitto_disconnect_v5(mosq, reason_code, NULL)) != MOSQ_ERR_SUCCESS)
    {
        Log_Error("Failure on disconnect: %s", mosquitto_strerror(rc));
    }

done:
    if (s_commMgrState.mqttCallbacks.on_connect_v5 != NULL)
    {
        s_commMgrState.mqttCallbacks.on_connect_v5(mosq, obj, reason_code, flags, props);
    }
    return;
}

/**
 * @brief Callback function to handle incoming MQTT messages.
 *
 * This function is called when an MQTT message is received from the broker.
 * It provides information about the received message, including the message structure,
 * and MQTT properties if available.
 *
 * For more details, refer to the MQTT specification on incoming messages:
 * [MQTT Incoming Messages](https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc488848792)
 *
 * @param mosq Pointer to the MQTT client instance receiving the message.
 * @param obj User data or context associated with the MQTT client (if any).
 * @param msg Pointer to the received MQTT message.
 * @param props MQTT properties associated with the received message (may be NULL).
 */
void ADUC_CommunicationChannel_OnMessage(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    char* msg_type = NULL;

    Log_Info("Topic: %s; QOS: %d; mid: %d", msg->topic, msg->qos, msg->mid);

    // All parameters are required.
    if (mosq == NULL || obj == NULL || msg == NULL || props == NULL)
    {
        Log_Error("Null parameter (mosq=%p, obj=%p, msg=%p, props=%p)", mosq, obj, msg, props);
        goto done;
    }

    // Get the message type. (mt = message type)
    if (!ADU_mosquitto_read_user_property_string(props, "mt", &msg_type) || IsNullOrEmpty(msg_type))
    {
        Log_Warn("Failed to read the message type. Ignore the message.");
        goto done;
    }

    // Only support protocol version 1
    if (!ADU_mosquitto_has_user_property(props, "pid", "1"))
    {
        Log_Warn("Ignore the message. (pid != 1)");
        goto done;
    }

    /* This blindly prints the payload, but the payload can be anything so take care. */
    Log_Debug("\tPayload: %s\n", (char*)msg->payload);

done:
    free(msg_type);
    if (s_commMgrState.mqttCallbacks.on_message_v5 != NULL)
    {
        s_commMgrState.mqttCallbacks.on_message_v5(mosq, obj, msg, props);
    }
}

/**
 * @brief Callback function to handle MQTT publish acknowledgments.
 *
 * This function is called when an MQTT publish acknowledgment is received from the broker.
 * It provides information about the acknowledgment, including the message identifier (mid),
 * the reason code, and MQTT properties if available.
 *
 * For more details, refer to the MQTT specification on publish acknowledgments:
 * [MQTT Publish Acknowledgment](https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc488848793)
 *
 * @param mosq Pointer to the MQTT client instance receiving the acknowledgment.
 * @param obj User data or context associated with the MQTT client (if any).
 * @param mid Message identifier of the publish acknowledgment.
 * @param reason_code The reason code indicating the result of the publish operation.
 * @param props MQTT properties associated with the acknowledgment (may be NULL).
 */
void ADUC_CommunicationChannel_OnPublish(
    struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* props)
{
    Log_Info("on_publish: Message with mid %d has been published.", mid);

    if (s_commMgrState.mqttCallbacks.on_publish_v5 != NULL)
    {
        s_commMgrState.mqttCallbacks.on_publish_v5(mosq, obj, mid, reason_code, props);
    }
}

/**
 * @brief Callback function to handle MQTT subscribe acknowledgments.
 *
 * This function is called when an MQTT subscribe acknowledgment is received from the broker.
 * It provides information about the acknowledgment, including the message identifier (mid), the
 * number of granted QoS levels, the granted QoS levels, and MQTT properties if available.
 *
 * For more details, refer to the MQTT specification on subscribe acknowledgments:
 * [MQTT Subscribe Acknowledgment](https://docs.oasis-open.org/mqtt/mqtt/v5.0/os/mqtt-v5.0-os.html#_Toc488848794)
 *
 * @param mosq Pointer to the MQTT client instance receiving the acknowledgment.
 * @param obj User data or context associated with the MQTT client (if any).
 * @param mid Message identifier of the subscribe acknowledgment.
 * @param qos_count The number of granted QoS levels.
 * @param granted_qos An array of granted QoS levels for each subscribed topic.
 * @param props MQTT properties associated with the acknowledgment (may be NULL).
 */
void ADUC_CommunicationChannel_OnSubscribe(
    struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos, const mosquitto_property* props)
{
    Log_Info("on_subscribe: Subscribed with mid %d; %d topics.", mid, qos_count);

    /* In this example we only subscribe to a single topic at once, but a
   * SUBSCRIBE can contain many topics at once, so this is one way to check
   * them all. */
    for (int i = 0; i < qos_count; i++)
    {
        Log_Info("\tQoS %d\n", granted_qos[i]);
    }

    if (s_commMgrState.mqttCallbacks.on_subscribe_v5 != NULL)
    {
        s_commMgrState.mqttCallbacks.on_subscribe_v5(mosq, obj, mid, qos_count, granted_qos, props);
    }
}

/**
 * @brief Callback function to handle MQTT log messages.
 *
 * This function is called when MQTT log messages are generated. It provides information about the
 * MQTT log message, including the message level and the log message text.
 *
 * @param mosq Pointer to the MQTT client instance generating the log message.
 * @param obj User data or context associated with the MQTT client (if any).
 * @param level The log message level indicating the severity of the message.
 * @param str The text of the log message.
 */
void ADUC_CommunicationChannel_OnLog(struct mosquitto* mosq, void* obj, int level, const char* str)
{
#ifndef LOG_ALL_MOSQUITTO
    if (level == MOSQ_LOG_ERR || strstr(str, "PINGREQ") != NULL || strstr(str, "PINGRESP") != NULL)
    {
        Log_Info("%s", str);
    }
#else
    {
        char* log_level_str;
        switch (level)
        {
        case MOSQ_LOG_DEBUG:
            log_level_str = "DEBUG";
            break;
        case MOSQ_LOG_INFO:
            log_level_str = "INFO";
            break;
        case MOSQ_LOG_NOTICE:
            log_level_str = "NOTICE";
            break;
        case MOSQ_LOG_WARNING:
            log_level_str = "WARNING";
            break;
        case MOSQ_LOG_ERR:
            log_level_str = "ERROR";
            break;
        default:
            log_level_str = "";
            break;
        }
        Log_Info("[%s] %s", log_level_str, str);
    }
#endif
    if (s_commMgrState.mqttCallbacks.on_log != NULL)
    {
        s_commMgrState.mqttCallbacks.on_log(mosq, obj, level, str);
    }
}

/**
 * @brief Stores the timestamp until which DoWork calls should be suppressed.
 */
static time_t g_suppressDoWorkCallsUntil = 0;

/**
 * @brief Suppresses DoWork calls for a specified duration.
 *
 * @param seconds The duration in seconds for which DoWork calls should be suppressed.
 */
void SuppressDoWorkCallsFor(int seconds)
{
    g_suppressDoWorkCallsUntil = time(NULL) + seconds;
}

/**
 * @brief Checks if DoWork calls are currently suppressed.
 *
 * @return True if DoWork calls are suppressed, false otherwise.
 */
bool IsDoWorkCallsSuppressed()
{
    return ADUC_GetTimeSinceEpochInSeconds() < g_suppressDoWorkCallsUntil;
}

/**
 * @brief Performs MQTT-related work using the Mosquitto library.
 *
 * This function is responsible for executing `mosquitto_loop` to process network traffic,
 * callbacks, and connection maintenance as needed. It also handles various error conditions.
 */
void PerformMosquittoDoWork()
{
    int mqtt_ret = MOSQ_ERR_UNKNOWN;

    // Must call mosquitto_loop() to process network traffic, callbacks and connection maintenance as needed.
    if (s_commMgrState.mqttClient != NULL && !IsDoWorkCallsSuppressed())
    {
        mqtt_ret = mosquitto_loop(s_commMgrState.mqttClient, 100 /*timeout*/, 1 /*max_packets*/);
        if (mqtt_ret != MOSQ_ERR_SUCCESS)
        {
            switch (mqtt_ret)
            {
            case MOSQ_ERR_NOMEM:
            case MOSQ_ERR_INVAL:
            case MOSQ_ERR_PROTOCOL:
                // Unrecoverable errors. Agent configuration may be invalid.
                Log_Error("Failed to execute mosquitto loop (%d - %s)", mqtt_ret, mosquitto_strerror(mqtt_ret));
                SuppressDoWorkCallsFor(60 /*seconds*/);
                break;
            case MOSQ_ERR_ERRNO: {
                int errno_copy = errno;
                Log_Error("mosquitto_loop failed (%d) - %s", mqtt_ret, mosquitto_strerror(errno_copy));
                SuppressDoWorkCallsFor(60 /*seconds*/);
                break;
            }
            case MOSQ_ERR_NO_CONN:
            case MOSQ_ERR_CONN_LOST:
                // This could be transient or permanent.
                // Will be handle by ADUC_EnsureValidCommunicationChannel()
                break;
            default:
                // Unexpected error
                Log_Error(
                    "Failed to execute mosquitto loop (unexpected error) (%d - %s)",
                    mqtt_ret,
                    mosquitto_strerror(mqtt_ret));
                break;
            }
        }
    }
}

/**
 * @brief Ensure that the device is subscribed to DPS topics.
 * @return true if the device is subscribed to DPS topics.
 */
bool EnsureADUTopicsSubscriptionValid()
{
    if (s_commMgrState.topicsSubscribed)
    {
        return true;
    }

    if (s_commMgrState.mqttClient == NULL)
    {
        return false;
    }

    if (s_commMgrState.commState != ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED)
    {
        return false;
    }

    // Sub scribe to the service-to-device topics.
    int mid = 0;
    int mqtt_ret =
        mosquitto_subscribe(s_commMgrState.mqttClient, &mid, s_commMgrState.mqttTopics.serviceOTO, 0 /* qos */);
    if (mqtt_ret != MOSQ_ERR_SUCCESS)
    {
        Log_Error("Failed to subscribe to '%s'. (err:%d)", s_commMgrState.mqttTopics.serviceOTO, mqtt_ret);
    }
    else
    {
        Log_Info("Subscribing to '%s'. (mid:%d)", s_commMgrState.mqttTopics.serviceOTO, mid);
        s_commMgrState.topicsSubscribed = true;
    }

    return s_commMgrState.topicsSubscribed;
}

/**
 * @brief Manages the state of the communication channel by attempting to establish a connection if necessary.
 *
 * This function performs state management of the communication channel based on the current state. If the
 * communication channel is in a disconnected state, it attempts to re-establish the connection. It also handles
 * various states and connection results, logging any relevant errors.
 *
 * @return `true` if the communication channel state is already connected.
 *         `false` for all other states or if the function encounters any issues.
 *
 * @note This function might return false even if no errors are encountered (e.g., when waiting to reconnect).
 */
bool PerformChannelStateManagement()
{
    int mqtt_res = MOSQ_ERR_UNKNOWN;
    if (s_commMgrState.commState == ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED)
    {
        return true;
    }

    if (!s_commMgrState.initialized || s_commMgrState.mqttClient == NULL)
    {
        Log_Debug("MQTT client is not initialized");
        return false;
    }

    if (s_commMgrState.commState == ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTING)
    {
        // TODO (nox-msft) : Check for timeout
        // If time out, set state to UNKNOWN and try again.
        // If not time out, let's wait, return false.
        return false;
    }

    if (s_commMgrState.commState == ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_DISCONNECTED)
    {
        // TODO (nox-msft) : Check if the disconnect was due to an non-recoverable error.
        //   If so, do nothing and return false.
        //
        //   If the error is recoverable, set state to UNKNOWN and try again when the nextRetryTimestamp is reached.
        //
        // For now, let's try to reconnect after 5 seconds.
        if (s_commMgrState.commStateUpdatedTime + 5 > ADUC_GetTimeSinceEpochInSeconds())
        {
            return false;
        }
        else
        {
            ADUC_SetCommunicationChannelState(ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_UNKNOWN);
        }
    }

    if (s_commMgrState.commState == ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_UNKNOWN)
    {
        // Connect to an MQTT broker.
        // It is valid to use this function for clients using all MQTT protocol versions.
        //
        // If you need to set MQTT v5 CONNECT properties, use <mosquitto_connect_bind_v5>
        // instead.
        mqtt_res = mosquitto_connect(
            s_commMgrState.mqttClient,
            s_commMgrState.mqttSettings.hostname,
            (int)s_commMgrState.mqttSettings.tcpPort,
            (int)s_commMgrState.mqttSettings.keepAliveInSeconds);

        switch (mqtt_res)
        {
        case MOSQ_ERR_SUCCESS:
            s_commMgrState.commLastAttemptTime =
                ADUC_SetCommunicationChannelState(ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTING);
            break;
        case MOSQ_ERR_INVAL:
            Log_Error(
                "Invalid parameter (client: %d, host:%s, port:%d, keepalive:%d)",
                s_commMgrState.mqttClient,
                s_commMgrState.mqttSettings.hostname,
                s_commMgrState.mqttSettings.tcpPort,
                s_commMgrState.mqttSettings.keepAliveInSeconds);
            break;
        case MOSQ_ERR_EAI:
            // NOTE: Common reasons for this error include:
            // - The hostname is not resolvable.
            // - An intermediate router or firewall is blocking the connection.
            // - The hostname is valid but the client is unable to connect.

            // TODO (nox-msft) - compute retry time based on error code.
            // This could be transient error, e.g., connectivity issue.
            Log_Error("Lookup error (%s)", mosquitto_strerror(mqtt_res));
            break;
        case MOSQ_ERR_ERRNO: {
            int errno_copy = errno;
            if (mqtt_res == MOSQ_ERR_ERRNO)
            {
                // TODO (nox-msft) - use FormatMessage() on WinIoT
                Log_Error("mosquitto_connect failed (%d) - %s", mqtt_res, strerror(errno_copy));
            }
            else
            {
                Log_Error("mosquitto_connect failed (%d)", mqtt_res);
            }
            break;
        }
        default:
            Log_Error("mosquitto_connect failed (%d)", mqtt_res);
            break;
        }
    }

    return false;
}

/**
 * @brief Ensure that the communication channel to the DU service is valid.
 *
 * @remark This function requires that the MQTT client is initialized.
 * @remark This function requires that the MQTT connection settings are valid.
 *
 * @return true if the communication channel state is ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED.
 */
bool ADUC_CommunicationChannel_DoWork()
{
    // Always call PerformMosquittoDoWork() to process network traffic, callbacks and connection maintenance as needed.
    PerformMosquittoDoWork();

    EnsureADUTopicsSubscriptionValid();

    PerformChannelStateManagement();

    return true;
}

/**
 * @brief Publishes a message to a the 'adu/oto/#/a' topic using version 5 of the MQTT protocol.
 *
 * This function is a wrapper around `mosquitto_publish_v5`, simplifying the process of
 * publishing messages to an MQTT broker.
 *
 * @param[out] mid Pointer to an integer where the message ID will be stored. Can be NULL.
 * @param[in]  payload_len Length of the payload to be published.
 * @param[in]  payload Pointer to the data payload to be published.
 * @param[in]  qos Quality of Service level for the message. Valid values are 0, 1, or 2.
 * @param[in]  retain If set to `true`, the message will be retained by the broker. Otherwise, it won't be retained.
 * @param[in]  props MQTT v5 properties to be included in the message. Can be NULL if no properties are to be sent.
 *
 * @return MOSQ_ERR_SUCCESS upon successful completion.
 *         For other return values, refer to the documentation of `mosquitto_publish_v5`.
 */
int ADUC_CommunicationChannel_MQTT_Publish(
    int* mid, size_t payload_len, const void* payload, int qos, bool retain, const mosquitto_property* props)
{
    return mosquitto_publish_v5(
        s_commMgrState.mqttClient,
        mid /* mid */,
        s_commMgrState.mqttTopics.agentOTO,
        payload_len /* payload len */,
        payload /* payload */,
        qos /* qos */,
        retain /* retain */,
        props);
}

/*
 * @brief Check if the communication channel is in connected state.
 */
bool ADUC_CommunicationChannel_IsConnected()
{
    return s_commMgrState.commState == ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED;
}
