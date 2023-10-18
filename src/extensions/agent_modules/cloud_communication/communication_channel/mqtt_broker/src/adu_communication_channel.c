/**
 * @file adu_communication_channel.c
 * @brief Implementation for the Device Update communication channel (MQTT broker) management.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_communication_channel.h" // ADUC_EnsureValidCommunicationChannel
#include "aduc/adu_communication_channel_internal.h"
#include "aduc/adu_mosquitto_utils.h"
#include "aduc/adu_mqtt_protocol.h" // ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE
#include "aduc/agent_state_store.h"
#include "aduc/config_utils.h"
#include "aduc/d2c_messaging.h"
#include "aduc/logging.h"
#include "aduc/mqtt_client.h"
#include "aduc/retry_utils.h" // ADUC_GetTimeSinceEpochInSeconds
#include "aduc/string_c_utils.h" // IsNullOrEmpty, ADUC_StringFormat

#include "du_agent_sdk/agent_module_interface.h" // ADUC_AGENT_MODULE_INTERFACE and related functions
#include <stdarg.h> // va_list, va_start, va_end
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/vector.h"
#include <aducpal/time.h> // time_t
#include <errno.h> // errno
#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h> // mosquitto_property

#define REQUIRED_TLS_SET_CERT_PATH "L"

#define DEFAULT_CONNECT_RETRY_DELAY_SECONDS (60 * 5) /* 5 seconds */

#define DEFAULT_CONNECT_UNRECOVERABLE_ERROR_RETRY_DELAY_SECONDS (60 * 60) /* 1 hours */

#define DEFAULT_MQTT_BROKER_HOSTNAME_QUERY_INTERVAL_SECONDS 5

// Forward declarations
ADUC_AGENT_MODULE_HANDLE ADUC_Communication_Channel_Create();
void ADUC_Communication_Channel_Destroy(ADUC_AGENT_MODULE_HANDLE handle);
const ADUC_AGENT_CONTRACT_INFO* ADUC_Communication_Channel_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle);
int ADUC_Communication_Channel_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* moduleInitData);
int ADUC_Communication_Channel_Deinitialize(ADUC_AGENT_MODULE_HANDLE handle);
int ADUC_Communication_Channel_DoWork(ADUC_AGENT_MODULE_HANDLE handle);

static time_t ADUC_SetCommunicationChannelState(
    ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState, ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE state);

void ADUC_MQTT_CLient_OnDisconnect(struct mosquitto* mosq, void* obj, int rc, const mosquitto_property* props);
void ADUC_Communication_Channel_OnConnect(
    struct mosquitto* mosq, void* obj, int reason_code, int flags, const mosquitto_property* props);
void ADUC_Communication_Channel_OnMessage(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props);
void ADUC_Communication_Channel_OnPublish(
    struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* props);
void ADUC_Communication_Channel_OnSubscribe(
    struct mosquitto* mosq,
    void* obj,
    int mid,
    int qos_count,
    const int* granted_qos,
    const mosquitto_property* props);
void ADUC_Communication_Channel_OnLog(struct mosquitto* mosq, void* obj, int level, const char* str);

#define LOG_FREQUENCY_SECONDS_5 5
#define LOG_FREQUENCY_SECONDS_10 10
#define LOG_FREQUENCY_SECONDS_30 30
#define LOG_FREQUENCY_SECONDS_60 60

static const char* LOG_MESSAGE_MQTT_CLIENT_IS_NO_INITIALIZE = "MQTT client is not initialized";

static ADUC_Retry_Params defaultConnectionRetryParams = {
    UINT16_MAX /*maxRetries*/,
    ADUC_RETRY_DEFAULT_INITIAL_DELAY_MS /*maxDelaySecs*/,
    TIME_SPAN_ONE_MINUTE_IN_SECONDS /* fallbackWaitTimeSec */,
    ADUC_RETRY_DEFAULT_INITIAL_DELAY_MS /* initialDelayUnitMilliSecs (Backoff factor) */,
    ADUC_RETRY_DEFAULT_MAX_JITTER_PERCENT /* double maxJitterPercent - the maximum number of jitter percent (0 - 100) */
};

/**
 * @brief Logs a message with a specified frequency.
 * @param freqSeconds The frequency in seconds.
 * @param format The format string.
 * @param ... The format arguments.
 * @return void
*/
static inline void Log_Debug_With_Frequency(int freqSeconds, const char* format, ...)
{
    static time_t lastLogTime = 0;
    static int logCount = 0;
    static int logFrequency = TIME_SPAN_FIVE_MINUTES_IN_SECONDS;

    logFrequency = freqSeconds;

    if (logFrequency <= 0)
    {
        return;
    }

    time_t now = time(NULL);
    if (now != lastLogTime)
    {
        lastLogTime = now;
        logCount = 0;
    }

    if (logCount < logFrequency)
    {
        logCount++;
        return;
    }

    va_list args;
    va_start(args, format);
    Log_Debug(format, args);
    va_end(args);
}

static ADU_MQTT_COMMUNICATION_MGR_STATE* CommunicationManagerStateFromModuleHandle(ADUC_AGENT_MODULE_HANDLE commHandle)
{
    ADUC_AGENT_MODULE_INTERFACE* interface = (ADUC_AGENT_MODULE_INTERFACE*)commHandle;
    if (!interface)
        return NULL;
    ADU_MQTT_COMMUNICATION_MGR_STATE* state = (ADU_MQTT_COMMUNICATION_MGR_STATE*)interface->moduleData;
    return state;
}

/**
 * @brief Create the communication channel module.
 */
ADUC_AGENT_MODULE_HANDLE ADUC_Communication_Channel_Create()
{
    ADUC_AGENT_MODULE_INTERFACE* interface = NULL;
    ADU_MQTT_COMMUNICATION_MGR_STATE* state = malloc(sizeof(ADU_MQTT_COMMUNICATION_MGR_STATE));
    if (state == NULL)
    {
        Log_Error("Failed to allocate memory for communication channel state");
        goto done;
    }
    memset(state, 0, sizeof(ADU_MQTT_COMMUNICATION_MGR_STATE));
    state->pendingSubscriptions = VECTOR_create(sizeof(ADUC_MQTT_SUBSCRIBE_CALLBACK_INFO));
    if (state->pendingSubscriptions == NULL)
    {
        Log_Error("Failed to create pending subscriptions vector");
        goto done;
    }

    state->subscribedTopicsMap = Map_Create(NULL);
    if (state->subscribedTopicsMap == NULL)
    {
        Log_Error("Failed to create subscribed topics map");
        goto done;
    }

    state->connectionRetryParams = defaultConnectionRetryParams;

    interface = malloc(sizeof(ADUC_AGENT_MODULE_INTERFACE));
    if (interface == NULL)
    {
        Log_Error("Failed to allocate memory for communication channel interface");
        goto done;
    }
    memset(interface, 0, sizeof(ADUC_AGENT_MODULE_INTERFACE));

    interface->moduleData = state;
    interface->initializeModule = ADUC_Communication_Channel_Initialize;
    interface->deinitializeModule = ADUC_Communication_Channel_Deinitialize;
    interface->doWork = ADUC_Communication_Channel_DoWork;
    interface->getContractInfo = ADUC_Communication_Channel_GetContractInfo;
done:
    if (interface == NULL)
    {
        if (state->pendingSubscriptions != NULL)
        {
            VECTOR_destroy(state->pendingSubscriptions);
        }
        if (state->subscribedTopicsMap != NULL)
        {
            Map_Destroy(state->subscribedTopicsMap);
        }
        free(state);
    }
    return interface;
}

/**
 * @brief Destroy the communication channel module.
 */
void ADUC_Communication_Channel_Destroy(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_AGENT_MODULE_INTERFACE* interface = (ADUC_AGENT_MODULE_INTERFACE*)handle;
    if (interface == NULL)
    {
        return;
    }

    ADU_MQTT_COMMUNICATION_MGR_STATE* state = (ADU_MQTT_COMMUNICATION_MGR_STATE*)interface->moduleData;
    if (state != NULL)
    {
        VECTOR_destroy(state->pendingSubscriptions);
        Map_Destroy(state->subscribedTopicsMap);
    }

    free(state);
    free(interface);
}

/* Callback called when the broker has received the DISCONNECT command and has disconnected the
 * client. */
void ADUC_Communication_Channel_OnDisconnect(
    struct mosquitto* mosq, void* obj, int rc, const mosquitto_property* props)
{
    Log_Info("on_disconnect: reason=%s", mosquitto_strerror(rc));

    ADUC_AGENT_MODULE_INTERFACE* interface = (ADUC_AGENT_MODULE_INTERFACE*)obj;

    if (interface == NULL)
    {
        Log_Error("Null parameter (interface=%p)", interface);
        return;
    }

    ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState = (ADU_MQTT_COMMUNICATION_MGR_STATE*)interface->moduleData;
    if (commMgrState == NULL)
    {
        Log_Error("commMgrState is NULL");
        return;
    }

    ADUC_SetCommunicationChannelState(commMgrState, ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_DISCONNECTED);

    // TODO (nox-msft) - select retry , strategy based on error code.
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
    commMgrState->commNextRetryTime = ADUC_GetTimeSinceEpochInSeconds() + DEFAULT_CONNECT_RETRY_DELAY_SECONDS;

    ADUC_SetCommunicationChannelState(commMgrState, ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_DISCONNECTED);

    if (commMgrState->mqttCallbacks.on_disconnect_v5 != NULL)
    {
        commMgrState->mqttCallbacks.on_disconnect_v5(mosq, obj, rc, props);
    }
}

/*
 * @brief  Initialize the communication channel.
 * @param handle The communication channel module handle.
 * @param moduleInitData The module initialization data.
 */
int ADUC_Communication_Channel_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* moduleInitData)
{
    int mqtt_res = 0;
    bool use_OS_cert = false;
    ADUC_AGENT_MODULE_INTERFACE* interface = (ADUC_AGENT_MODULE_INTERFACE*)handle;

    // For
    ADUC_STATE_STORE_RESULT storeRes = ADUC_StateStore_Initialize(NULL);
    if (storeRes != ADUC_STATE_STORE_RESULT_OK)
    {
        Log_Error("Failed to initialize state store (%d)", storeRes);
        return false;
    }

    if (interface == NULL)
    {
        Log_Error("Null parameter (interface=%p)", interface);
        return false;
    }

    ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState = (ADU_MQTT_COMMUNICATION_MGR_STATE*)interface->moduleData;
    if (commMgrState == NULL)
    {
        Log_Error("commMgrState is NULL");
        return false;
    }

    ADUC_COMMUNICATION_CHANNEL_INIT_DATA* initData = (ADUC_COMMUNICATION_CHANNEL_INIT_DATA*)moduleInitData;
    if (initData == NULL)
    {
        Log_Error("moduleInitData is NULL");
        return false;
    }

    ADUC_STATE_STORE_RESULT stateRes = ADUC_StateStore_SetCommunicationChannelHandle(initData->sessionId, handle);
    if (stateRes != ADUC_STATE_STORE_RESULT_OK)
    {
        Log_Error("Failed to set communication channel handle in state store (%d)", stateRes);
        return false;
    }

    commMgrState->ownerModuleContext = initData->ownerModuleContext;

    if (commMgrState->initialized)
    {
        return true;
    }

    // Copy all callbacks address.
    memcpy(&commMgrState->mqttCallbacks, initData->callbacks, sizeof(ADUC_MQTT_CALLBACKS));

    memcpy(&commMgrState->mqttSettings, initData->mqttSettings, sizeof(commMgrState->mqttSettings));

    commMgrState->sessionId = STRING_construct(initData->sessionId);
    if (commMgrState->sessionId == NULL)
    {
        Log_Error("sessionId construct");
        goto done;
    }

    commMgrState->keyFilePasswordCallback = initData->passwordCallback;

    if (initData->connectionRetryParams != NULL)
    {
        memcpy(
            &commMgrState->connectionRetryParams,
            initData->connectionRetryParams,
            sizeof(commMgrState->connectionRetryParams));
    }
    else
    {
        memcpy(
            &commMgrState->connectionRetryParams,
            &defaultConnectionRetryParams,
            sizeof(commMgrState->connectionRetryParams));
    }

    Log_Info("Initialize Mosquitto library");
    mqtt_res = mosquitto_lib_init();
    if (mqtt_res != MOSQ_ERR_SUCCESS)
    {
        Log_Error("Failed to initialize Mosquitto library (%d)", mqtt_res);
        goto done;
    }

    commMgrState->mqttClient = mosquitto_new(
        initData->sessionId,
        commMgrState->mqttSettings.cleanSession,
        handle /* Communication Channel Module Handle */);
    if (!commMgrState->mqttClient)
    {
        Log_Error("Failed to create Mosquitto client");
        goto done;
    }

    mqtt_res = mosquitto_int_option(
        commMgrState->mqttClient, MOSQ_OPT_PROTOCOL_VERSION, commMgrState->mqttSettings.mqttVersion);
    if (mqtt_res != MOSQ_ERR_SUCCESS)
    {
        Log_Error("Failed to set MQTT protocol version (%d)", mqtt_res);
        goto done;
    }

    if (!IsNullOrEmpty(commMgrState->mqttSettings.username))
    {
        mqtt_res = mosquitto_username_pw_set(commMgrState->mqttClient, commMgrState->mqttSettings.username, NULL);
    }

    if (commMgrState->mqttSettings.useTLS)
    {
        use_OS_cert = IsNullOrEmpty(commMgrState->mqttSettings.caFile);
        if (use_OS_cert)
        {
            mqtt_res = mosquitto_int_option(commMgrState->mqttClient, MOSQ_OPT_TLS_USE_OS_CERTS, true);
            if (mqtt_res != MOSQ_ERR_SUCCESS)
            {
                Log_Error("Failed to set TLS use OS certs (%d - %s)", mosquitto_strerror(mqtt_res));
                goto done;
            }
        }

        mqtt_res = mosquitto_tls_set(
            commMgrState->mqttClient,
            commMgrState->mqttSettings.caFile,
            use_OS_cert ? REQUIRED_TLS_SET_CERT_PATH : NULL,
            commMgrState->mqttSettings.certFile,
            commMgrState->mqttSettings.keyFile,
            commMgrState->keyFilePasswordCallback);

        if (mqtt_res != MOSQ_ERR_SUCCESS)
        {
            // Handle all error codes.
            switch (mqtt_res)
            {
            case MOSQ_ERR_INVAL:
            case MOSQ_ERR_NOMEM:
            default:
                Log_Error("TLS setting failed. (%d:%s)", mqtt_res, mosquitto_strerror(mqtt_res));
                break;
            }

            goto done;
        }
    }

    // Register callbacks.
    mosquitto_connect_v5_callback_set(commMgrState->mqttClient, ADUC_Communication_Channel_OnConnect);
    mosquitto_disconnect_v5_callback_set(commMgrState->mqttClient, ADUC_Communication_Channel_OnDisconnect);
    mosquitto_subscribe_v5_callback_set(commMgrState->mqttClient, ADUC_Communication_Channel_OnSubscribe);
    mosquitto_publish_v5_callback_set(commMgrState->mqttClient, ADUC_Communication_Channel_OnPublish);
    mosquitto_message_v5_callback_set(commMgrState->mqttClient, ADUC_Communication_Channel_OnMessage);
    mosquitto_log_callback_set(commMgrState->mqttClient, ADUC_Communication_Channel_OnLog);

    commMgrState->initialized = true;
done:
    return commMgrState->initialized ? 0 : -1;
}

int ADUC_Communication_Channel_Deinitialize(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_AGENT_MODULE_INTERFACE* interface = (ADUC_AGENT_MODULE_INTERFACE*)handle;

    if (interface == NULL)
    {
        Log_Error("Null parameter (interface=%p)", interface);
        return -1;
    }

    ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState = (ADU_MQTT_COMMUNICATION_MGR_STATE*)interface->moduleData;
    if (commMgrState == NULL)
    {
        Log_Error("commMgrState is NULL");
        return -1;
    }

    if (commMgrState->initialized)
    {
        if (commMgrState->mqttClient != NULL)
        {
            Log_Info("Disconnecting MQTT client");
            mosquitto_disconnect(commMgrState->mqttClient);
            Log_Info("Destroying MQTT client");
            mosquitto_destroy(commMgrState->mqttClient);
            commMgrState->mqttClient = NULL;
        }

        mosquitto_lib_cleanup();
        commMgrState->initialized = false;
    }

    return 0;
}

/**
 * @brief Set the communication channel state.
 * @param state The new state.
 * @return The current time.
 */
static time_t ADUC_SetCommunicationChannelState(
    ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState, ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE state)
{
    time_t nowTime = time(NULL);
    if (commMgrState->commState != state)
    {
        Log_Info("Comm channel state changed %d --> %d", commMgrState->commState, state);
        commMgrState->commState = state;
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
void ADUC_Communication_Channel_OnConnect(
    struct mosquitto* mosq, void* commChannelModuleHandle, int reason_code, int flags, const mosquitto_property* props)
{
    ADUC_AGENT_MODULE_INTERFACE* interface = (ADUC_AGENT_MODULE_INTERFACE*)commChannelModuleHandle;

    if (interface == NULL)
    {
        Log_Error("Null parameter (interface=%p)", interface);
        return;
    }

    ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState = (ADU_MQTT_COMMUNICATION_MGR_STATE*)interface->moduleData;
    if (commMgrState == NULL)
    {
        Log_Error("commMgrState is NULL");
        return;
    }

    // Print out the connection result. mosquitto_connack_string() produces an
    // appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
    // clients is mosquitto_reason_string().
    if ((int)commMgrState->mqttSettings.mqttVersion == MQTT_PROTOCOL_V5)
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
        commMgrState->commLastConnectedTime =
            ADUC_SetCommunicationChannelState(commMgrState, ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED);
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
    commMgrState->commNextRetryTime = ADUC_GetTimeSinceEpochInSeconds() + DEFAULT_CONNECT_RETRY_DELAY_SECONDS;

    // For now, just disconnect to prevent the client from attempting to reconnect.
    int rc;
    if ((rc = mosquitto_disconnect_v5(mosq, reason_code, NULL)) != MOSQ_ERR_SUCCESS)
    {
        Log_Error("Failure on disconnect: %s", mosquitto_strerror(rc));
    }

done:
    if (commMgrState->mqttCallbacks.on_connect_v5 != NULL)
    {
        commMgrState->mqttCallbacks.on_connect_v5(mosq, commMgrState->ownerModuleContext, reason_code, flags, props);
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
void ADUC_Communication_Channel_OnMessage(
    struct mosquitto* mosq,
    void* commChannelModuleHandle,
    const struct mosquitto_message* msg,
    const mosquitto_property* props)
{
    ADUC_AGENT_MODULE_INTERFACE* interface = (ADUC_AGENT_MODULE_INTERFACE*)commChannelModuleHandle;

    if (interface == NULL)
    {
        Log_Error("Null parameter (interface=%p)", interface);
        return;
    }

    ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState = (ADU_MQTT_COMMUNICATION_MGR_STATE*)interface->moduleData;
    if (commMgrState == NULL)
    {
        Log_Error("commMgrState is NULL");
        return;
    }

    char* msg_type = NULL;

    Log_Info("Topic: %s; QOS: %d; mid: %d", msg->topic, msg->qos, msg->mid);

    // All parameters are required.
    if (mosq == NULL || commChannelModuleHandle == NULL || msg == NULL)
    {
        Log_Error("Null parameter (mosq=%p, obj=%p, msg=%p, props=%p)", mosq, commChannelModuleHandle, msg, props);
        goto done;
    }

    if (commMgrState->mqttSettings.mqttVersion == MQTT_PROTOCOL_V5)
    {
        if (props == NULL)
        {
            Log_Error("Null parameter (props=%p)", props);
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
    }

    /* This blindly prints the payload, but the payload can be anything so take care. */
    Log_Debug("\tPayload: %s\n", (char*)msg->payload);

done:
    free(msg_type);
    if (commMgrState->mqttCallbacks.on_message_v5 != NULL)
    {
        commMgrState->mqttCallbacks.on_message_v5(mosq, commMgrState->ownerModuleContext, msg, props);
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
void ADUC_Communication_Channel_OnPublish(
    struct mosquitto* mosq, void* commChannelModuleHandle, int mid, int reason_code, const mosquitto_property* props)
{
    Log_Info("on_publish: Message with mid %d has been published.", mid);
    ADUC_AGENT_MODULE_INTERFACE* interface = (ADUC_AGENT_MODULE_INTERFACE*)commChannelModuleHandle;

    if (interface == NULL)
    {
        Log_Error("Null parameter (interface=%p)", interface);
        return;
    }

    ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState = (ADU_MQTT_COMMUNICATION_MGR_STATE*)interface->moduleData;
    if (commMgrState == NULL)
    {
        Log_Error("commMgrState is NULL");
        return;
    }
    if (commMgrState->mqttCallbacks.on_publish_v5 != NULL)
    {
        commMgrState->mqttCallbacks.on_publish_v5(mosq, commMgrState->ownerModuleContext, mid, reason_code, props);
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
void ADUC_Communication_Channel_OnSubscribe(
    struct mosquitto* mosq,
    void* commChannelModuleHandle,
    int mid,
    int qos_count,
    const int* granted_qos,
    const mosquitto_property* props)
{
    Log_Info("on_subscribe: Subscribed with mid %d; %d topics.", mid, qos_count);
    ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState =
        CommunicationManagerStateFromModuleHandle(commChannelModuleHandle);
    if (commMgrState == NULL)
    {
        Log_Error("State is NULL. (handle=%p)", commChannelModuleHandle);
        return;
    }

    for (int i = 0; i < qos_count; i++)
    {
        Log_Info("\tQoS %d\n", granted_qos[i]);
    }

    // Remove the subscription from the pending list.
    for (size_t i = 0; i < VECTOR_size(commMgrState->pendingSubscriptions); i++)
    {
        ADUC_MQTT_SUBSCRIBE_CALLBACK_INFO* subInfo =
            (ADUC_MQTT_SUBSCRIBE_CALLBACK_INFO*)VECTOR_element(commMgrState->pendingSubscriptions, i);
        if (subInfo->messageId == mid)
        {
            VECTOR_erase(commMgrState->pendingSubscriptions, subInfo, 1);

            // Add the subscription to the subscribed list.
            MAP_RESULT mapRes = Map_AddOrUpdate(commMgrState->subscribedTopicsMap, subInfo->topic, NULL);
            if (mapRes != MAP_OK)
            {
                Log_Error("Failed to add topic to subscribed topics map (%d)", mapRes);
            }
            break;
        }
    }

    if (commMgrState->mqttCallbacks.on_subscribe_v5 != NULL)
    {
        commMgrState->mqttCallbacks.on_subscribe_v5(
            mosq, commMgrState->ownerModuleContext, mid, qos_count, granted_qos, props);
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
void ADUC_Communication_Channel_OnLog(
    struct mosquitto* mosq, void* commChannelModuleHandle, int level, const char* str)
{
    ADUC_AGENT_MODULE_INTERFACE* interface = (ADUC_AGENT_MODULE_INTERFACE*)commChannelModuleHandle;

    if (interface == NULL)
    {
        Log_Error("Null parameter (interface=%p)", interface);
        return;
    }

    ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState = (ADU_MQTT_COMMUNICATION_MGR_STATE*)interface->moduleData;
    if (commMgrState == NULL)
    {
        Log_Error("commMgrState is NULL");
        return;
    }

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
    if (commMgrState->mqttCallbacks.on_log != NULL)
    {
        commMgrState->mqttCallbacks.on_log(mosq, commMgrState->ownerModuleContext, level, str);
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
void PerformMosquittoDoWork(ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState)
{
    if (commMgrState == NULL)
    {
        Log_Error("commMgrState is NULL");
        return;
    }

    int mqtt_ret = MOSQ_ERR_UNKNOWN;

    // Must call mosquitto_loop() to process network traffic, callbacks and connection maintenance as needed.
    if (commMgrState->mqttClient != NULL && !IsDoWorkCallsSuppressed())
    {
        mqtt_ret = mosquitto_loop(commMgrState->mqttClient, 100 /*timeout*/, 1 /*max_packets*/);
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
bool EnsureADUTopicsSubscriptionValid(ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState)
{
    if (commMgrState->topicsSubscribed)
    {
        return true;
    }

    if (commMgrState->mqttClient == NULL)
    {
        return false;
    }

    if (commMgrState->commState != ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED)
    {
        return false;
    }

    char** topics = NULL;
    int topic_count = 0;
    int mid = 0;
    int mqtt_ret = MOSQ_ERR_UNKNOWN;

    if (commMgrState->mqttCallbacks.get_subscription_topics != NULL)
    {
        if (!commMgrState->mqttCallbacks.get_subscription_topics(
                commMgrState->ownerModuleContext, &topic_count, &topics))
        {
            Log_Info("GetTopics failed - no topics to subscribe to.");
        }
    }

    if (topic_count <= 0)
    {
        Log_Warn("No topics to subscribe to.");
        goto done;
    }

    mqtt_ret = mosquitto_subscribe_multiple(
        commMgrState->mqttClient, &mid, topic_count, topics, commMgrState->mqttSettings.qos, 0, NULL /*props*/);
    if (mqtt_ret != MOSQ_ERR_SUCCESS)
    {
        Log_Error("Failed to subscribe to topic. (err:%d)", mqtt_ret);
        goto done;
    }

    commMgrState->topicsSubscribed = true;
done:
    return commMgrState->topicsSubscribed;
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
bool PerformChannelStateManagement(ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState)
{
    int mqtt_res = MOSQ_ERR_UNKNOWN;
    if (commMgrState->commState == ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED)
    {
        return true;
    }

    if (!commMgrState->initialized || commMgrState->mqttClient == NULL)
    {
        Log_Debug_With_Frequency(LOG_FREQUENCY_SECONDS_10, LOG_MESSAGE_MQTT_CLIENT_IS_NO_INITIALIZE);
        return false;
    }

    if (commMgrState->commState == ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTING)
    {
        // TODO (nox-msft) : Check for timeout
        // If time out, set state to UNKNOWN and try again.
        // If not time out, let's wait, return false.
        return false;
    }

    if (commMgrState->commState == ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_DISCONNECTED)
    {
        // TODO (nox-msft) : Check if the disconnect was due to an non-recoverable error.
        //   If so, do nothing and return false.
        //
        //   If the error is recoverable, set state to UNKNOWN and try again when the nextRetryTimestamp is reached.
        //
        // For now, let's try to reconnect after 5 seconds.
        if (commMgrState->commStateUpdatedTime + 5 > ADUC_GetTimeSinceEpochInSeconds())
        {
            return false;
        }
        else
        {
            ADUC_SetCommunicationChannelState(commMgrState, ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_UNKNOWN);
        }
    }

    if (commMgrState->commState == ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_UNKNOWN)
    {
        // Connect to an MQTT broker.
        // It is valid to use this function for clients using all MQTT protocol versions.
        //
        // If you need to set MQTT v5 CONNECT properties, use <mosquitto_connect_bind_v5>
        // instead.
        mqtt_res = mosquitto_connect(
            commMgrState->mqttClient,
            commMgrState->mqttSettings.hostname,
            (int)commMgrState->mqttSettings.tcpPort,
            (int)commMgrState->mqttSettings.keepAliveInSeconds);

        switch (mqtt_res)
        {
        case MOSQ_ERR_SUCCESS:
            commMgrState->commLastAttemptTime =
                ADUC_SetCommunicationChannelState(commMgrState, ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTING);
            break;
        case MOSQ_ERR_INVAL:
            Log_Error(
                "Invalid parameter (client: %d, host:%s, port:%d, keepalive:%d)",
                commMgrState->mqttClient,
                commMgrState->mqttSettings.hostname,
                commMgrState->mqttSettings.tcpPort,
                commMgrState->mqttSettings.keepAliveInSeconds);
            // Unrecoverable error. Agent configuration may be invalid.
            // TODO (nox-msft) : use retry_utils
            commMgrState->commNextRetryTime =
                ADUC_GetTimeSinceEpochInSeconds() + DEFAULT_CONNECT_UNRECOVERABLE_ERROR_RETRY_DELAY_SECONDS;
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

bool EnsureHostnameValid(ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState)
{
    if (commMgrState == NULL)
    {
        Log_Error("commMgrState is NULL");
        return false;
    }

    if (IsNullOrEmpty(commMgrState->mqttSettings.hostname))
    {
        if (commMgrState->mqttSettings.hostnameSource == ADUC_MQTT_HOSTNAME_SOURCE_DPS)
        {
            const char* hostName = ADUC_StateStore_GetMQTTBrokerHostname();
            if (!IsNullOrEmpty(hostName))
            {
                Log_Info("Applying hostname from DPS: '%s'", hostName);
                if (mallocAndStrcpy_s(&commMgrState->mqttSettings.hostname, hostName) != 0)
                {
                    Log_Error("Failed to allocate memory for hostname");
                }
            }
        }
        else
        {
            Log_Error("Host name is empty");
        }
    }

    return !IsNullOrEmpty(commMgrState->mqttSettings.hostname);
}

/**
 * @brief Ensure that the communication channel to the DU service is valid.
 *
 * @remark This function requires that the MQTT client is initialized.
 * @remark This function requires that the MQTT connection settings are valid.
 *
 * @return true if the communication channel state is ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED.
 */
int ADUC_Communication_Channel_DoWork(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_AGENT_MODULE_INTERFACE* interface = (ADUC_AGENT_MODULE_INTERFACE*)handle;

    if (interface == NULL)
    {
        Log_Error("Null parameter (interface=%p)", interface);
        return false;
    }

    ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState = (ADU_MQTT_COMMUNICATION_MGR_STATE*)interface->moduleData;
    if (commMgrState == NULL)
    {
        Log_Error("commMgrState is NULL");
        return false;
    }

    // Always call PerformMosquittoDoWork() to process network traffic, callbacks and connection maintenance as needed.
    PerformMosquittoDoWork(commMgrState);

    if (EnsureHostnameValid(commMgrState))
    {
        EnsureADUTopicsSubscriptionValid(commMgrState);
        PerformChannelStateManagement(commMgrState);
    }
    else
    {
        commMgrState->commNextRetryTime =
            ADUC_GetTimeSinceEpochInSeconds() + DEFAULT_MQTT_BROKER_HOSTNAME_QUERY_INTERVAL_SECONDS;
    }

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
int ADUC_Communication_Channel_MQTT_Publish(
    ADUC_AGENT_MODULE_HANDLE commHandle,
    const char* topic,
    int* mid,
    int payload_len,
    const void* payload,
    int qos,
    bool retain,
    const mosquitto_property* props)
{
    ADUC_AGENT_MODULE_INTERFACE* interface = (ADUC_AGENT_MODULE_INTERFACE*)commHandle;

    if (interface == NULL)
    {
        Log_Error("Null parameter (interface=%p)", interface);
        return -1;
    }

    ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState = (ADU_MQTT_COMMUNICATION_MGR_STATE*)interface->moduleData;
    if (commMgrState == NULL)
    {
        Log_Error("commMgrState is NULL");
        return -1;
    }

    return mosquitto_publish_v5(
        commMgrState->mqttClient,
        mid /* mid */,
        topic,
        payload_len /* payload len */,
        payload /* payload */,
        qos /* qos */,
        retain /* retain */,
        props);
}

/**
 * @brief Subscribes to a topic using version 5 of the MQTT protocol.
 * @param[in] commHandle The handle to the communication channel.
 * @param[in] topic The topic to subscribe to.
 * @param[out] mid Pointer to an integer where the message ID will be stored. Can be NULL.
 * @param[in] qos Quality of Service level for the message. Valid values are 0, 1, or 2.
 * @param[in] options Subscription options. Valid values are 0 or 1.
 * @param[in] props MQTT v5 properties to be included in the message. Can be NULL if no properties are to be sent.
 * @param[in] userData Pointer to user-specific data.
 * @param[in] callback Pointer to the callback function to be invoked when the subscription is complete.
 * @return MOSQ_ERR_SUCCESS upon successful completion.
 */
int ADUC_Communication_Channel_MQTT_Subscribe(
    ADUC_AGENT_MODULE_HANDLE commHandle,
    const char* topic,
    int* mid,
    int qos,
    int options,
    const mosquitto_property* props,
    void* userData,
    MQTT_ON_SUBSCRIBE_V5_CALLBACK callback)
{
    if (commHandle == NULL || topic == NULL)
    {
        Log_Error("Null parameter (commHandle=%p, topic=%p)", commHandle, topic);
        return MOSQ_ERR_INVAL;
    }

    int mosqResult = MOSQ_ERR_UNKNOWN;
    ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState = CommunicationManagerStateFromModuleHandle(commHandle);
    ADUC_MQTT_SUBSCRIBE_CALLBACK_INFO* callbackInfo = NULL;

    // Create a tracking data.
    callbackInfo = (ADUC_MQTT_SUBSCRIBE_CALLBACK_INFO*)malloc(sizeof(ADUC_MQTT_SUBSCRIBE_CALLBACK_INFO));
    if (callbackInfo == NULL)
    {
        mosqResult = MOSQ_ERR_NOMEM;
        goto done;
    }

    callbackInfo->messageId = -1;
    if (mallocAndStrcpy_s(&callbackInfo->topic, topic) != 0)
    {
        mosqResult = MOSQ_ERR_NOMEM;
        goto done;
    }

    callbackInfo->callback = callback;
    callbackInfo->userData = userData;

    // Add the tracking data to the list.
    if (VECTOR_push_back(commMgrState->pendingSubscriptions, callbackInfo, 1) != 0)
    {
         goto done;
    }

    mosqResult =
        mosquitto_subscribe_v5(commMgrState->mqttClient, &callbackInfo->messageId, topic, qos, options, props);
    if (mosqResult != MOSQ_ERR_SUCCESS)
    {
        Log_Error("mosquitto_subscribe_v5 failed (mosqRes:%d)", mosqResult);
        goto done;
    }

    Log_Info("Subscribing to topic: %s, mid: %d", topic, callbackInfo->messageId);
    mosqResult = MOSQ_ERR_SUCCESS;
done:

    if (mosqResult != MOSQ_ERR_SUCCESS)
    {
        if (callbackInfo != NULL)
        {
            free(callbackInfo->topic);
            free(callbackInfo);
        }
    }
    else
    {
        if (mid != NULL)
        {
            *mid = callbackInfo->messageId;
        }
    }

    return mosqResult;
}

/**
 * @brief Check whether the specified @p topic is subscribed.
 * @param[in] commHandle The communication channel handle.
 * @param[in] topic The topic to check.
 * @return true if the topic is subscribed, false otherwise.
 */
bool ADUC_Communication_Channel_MQTT_IsSubscribed(ADUC_AGENT_MODULE_HANDLE commHandle, const char* topic)
{
    if (commHandle == NULL || topic == NULL)
    {
        Log_Error("Null parameter (commHandle=%p, topic=%p)", commHandle, topic);
        return false;
    }

    ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState = CommunicationManagerStateFromModuleHandle(commHandle);
    if (commMgrState == NULL)
    {
        Log_Error("commMgrState is NULL");
        return false;
    }

    ADUC_MQTT_SUBSCRIBE_CALLBACK_INFO* subInfo =
        (ADUC_MQTT_SUBSCRIBE_CALLBACK_INFO*)Map_GetValueFromKey(commMgrState->subscribedTopicsMap, topic);

    return subInfo != NULL;
}

/*
 * @brief Check if the communication channel is in connected state.
 */
bool ADUC_Communication_Channel_IsConnected(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_AGENT_MODULE_INTERFACE* interface = (ADUC_AGENT_MODULE_INTERFACE*)handle;

    if (interface == NULL)
    {
        Log_Error("Null parameter (interface=%p)", interface);
        return false;
    }

    ADU_MQTT_COMMUNICATION_MGR_STATE* commMgrState = (ADU_MQTT_COMMUNICATION_MGR_STATE*)interface->moduleData;
    if (commMgrState == NULL)
    {
        Log_Error("commMgrState is NULL");
        return false;
    }

    return commMgrState->commState == ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED;
}

/**
 * @brief The contract info for the module.
 */
static ADUC_AGENT_CONTRACT_INFO s_contractInfo = {
    "Microsoft", "Communication Channel Management Module", 1, "Microsoft/CommManger:1"
};

/**
 * @brief Gets the extension contract info.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 * @return ADUC_AGENT_CONTRACT_INFO The extension contract info.
 */
const ADUC_AGENT_CONTRACT_INFO* ADUC_Communication_Channel_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle)
{
    return &s_contractInfo;
}

/**
 * @brief The default function used for sending message content to the IoT Hub.
 *
 * @param cloudServiceHandle A pointer to ADUC_ClientHandle
 * @param context A pointer to the ADUC_D2C_Message_Processing_Context.
 * @param handleResponseMessageFunc A callback function to be called when the device received a response from the IoT Hub.
 * @return int Returns 0 if success. Otherwise, return implementation specific error code.
 *         For default function, this is equivalent to IOTHUB_CLIENT_RESULT.
 */
int ADUC_MQTTBroker_D2C_Message_Transport_Function(
    void* cloudServiceHandle, void* context, ADUC_C2D_RESPONSE_HANDLER_FUNCTION c2dResponseHandlerFunc)
{
    ADUC_AGENT_MODULE_HANDLE handle = (ADUC_AGENT_MODULE_HANDLE)cloudServiceHandle;
    if (handle == NULL)
    {
        Log_Error("cloudServiceHandle is NULL");
        return -1;
    }

    // ADUC_D2C_Message_Processing_Context* message_processing_context = (ADUC_D2C_Message_Processing_Context*)context;
    // if (message_processing_context->message.cloudServiceHandle == NULL
    //     || *((ADUC_AGENT_MODULE_HANDLE*)message_processing_context->message.cloudServiceHandle) == NULL)
    // {
    //     Log_Warn(
    //         "Try to send D2C message but cloudServiceHandle is NULL. Skipped. (content:0x%x)",
    //         message_processing_context->message.content);
    //     return 1;
    // }
    // else
    // {
    //     // Send content.
    //     Log_Debug("Sending D2C (MQTT) message:\n%s", (const char*)message_processing_context->message.content);

    //     TODO: Publish message.
    //     if (success)
    //     {
    //         ADUC_D2C_Messaging_SetMessageStatus(&message_processing_context->message, ADUC_D2C_Message_Status_Waiting_For_Response);
    //     }
    //     else
    //     {
    //         Log_Error("ClientHandle_SendReportedState return %d. Stop processing the message.", result);
    //         ADUC_D2C_Messaging_OnMessageProcessingCompleted(&message_processing_context->message, ADUC_D2C_Message_Status_Failed);
    //     }

    //     return iotHubClientResult;
    // }
    return 0;
}
