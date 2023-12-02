/**
 * @file adu_communication_channel.h
 * @brief Header file for the Device Update communication channel (MQTT broker) management.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef __ADUC_COMMUNICATION_CHANNEL_H__
#define __ADUC_COMMUNICATION_CHANNEL_H__

#include "aduc/adu_mqtt_protocol.h" // ADUC_COMMUNICATION_*, ADUC_ENROLLMENT_*, ADUC_MQTT_MESSAGE_*, ADUC_MQTT_SETTINGS, ADUC_Result
#include "aduc/c_utils.h" // EXTERN_C_* macros
#include "aduc/retry_utils.h" // ADUC_Retry_Params
#include "du_agent_sdk/agent_module_interface.h" // ADUC_AGENT_MODULE_HANDLE
#include "du_agent_sdk/mqtt_client_settings.h" // ADUC_MQTT_SETTINGS
#include "stdbool.h" // bool
#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h> // mosquitto_property

#include "azure_c_shared_utility/map.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/vector.h"

#include <mqtt_protocol.h>

EXTERN_C_BEGIN

#define ADUC_DU_SERVICE_COMMUNICATION_CHANNEL_ID "du_service_communication_channel"

/**
 * @brief Typedef for the MQTT Keyfile Password Callback.
 * @param buf Buffer to store the password.
 * @param size Size of the buffer.
 * @param rwflag Flag indicating whether the password is being used for reading or writing.
 * @param userdata Pointer to user-specific data.
*/
typedef int (*ADUC_MQTT_KEYFILE_PASSWORD_CALLBACK)(char* buf, int size, int rwflag, void* userdata);

/**
 * @brief Typedef for the MQTT On Connect V5 Callback.
 *
 * This callback function is invoked when the MQTT client successfully connects.
 *
 * @param mosq A pointer to the MQTT client structure.
 * @param userdata A pointer to user-specific data.
 * @param rc The return code from the MQTT broker.
 * @param flags Flags indicating connection status.
 * @param props Pointer to MQTT properties associated with the connection.
 */
typedef void (*MQTT_ON_CONNECT_V5_CALLBACK)(
    struct mosquitto* mosq, void* userData, int rc, int flags, const mosquitto_property* props);

/**
 * @brief Typedef for the MQTT On Disconnect V5 Callback.
 *
 * This callback function is invoked when the MQTT client disconnects.
 */
typedef void (*MQTT_ON_DISCONNECT_V5_CALLBACK)(struct mosquitto*, void*, int, const mosquitto_property* props);

/**
 * @brief Typedef for a function pointer that fetches MQTT subscription topics.
 *
 * This typedef defines a function pointer type that represents a function
 * which, when called, fetches a list of MQTT subscription topics.
 *
 * @param[out] count   Pointer to an integer that will be populated with
 *                     the number of topics returned.
 * @param[out] topics  Pointer to a char pointer array that will hold
 *                     the list of topics. It is assumed that the caller
 *                     has allocated sufficient memory for the topics.
 * @return true if the topics were successfully fetched; otherwise, false.
 */
typedef bool (*GET_MQTT_SUBSCRIPTION_TOPICS_FUNC)(void* obj, int* count, char*** topics);

/**
 * @brief Typedef for the MQTT on unsubscribe callback.
 *
 * This is called when the broker responds to a unsubscription request.
 *
 * @param mosq  the mosquitto instance making the callback.
 * @param obj   the user data provided in mosquitto_new
 * @param mid   the message id of the unsubscribe message.
 *
 */
typedef void (*MQTT_UNSUBSCRIBE_CALLBACK)(struct mosquitto* mosq, void* obj, int msgid);

/**
 * @brief Typedef for the MQTT On Subscribe V5 Function.
 *
 * This function is provided by the app. It is invoked by manager when the manager is ready to subscribe to a topic.
 *
 * @param mosq  the mosquitto instance making the callback.
 * @param obj   the user data provided in mosquitto_new
 * @param mid   the message id of the unsubscribe message.
 * @param qos_count the number of granted subscriptions (size of granted_qos).
 * @param granted_qos an array of integers indicating the granted QoS for each of the subscriptions.
 * @param props list of MQTT 5 properties, or NULL
 */
typedef void (*MQTT_UNSUBSCRIBE_V5_CALLBACK)(
    struct mosquitto* mosq,
    void* userdata,
    int msgid,
    int qos_count,
    const int* granted_qos,
    const mosquitto_property* props);

/**
 * @brief Typedef for the MQTT On Subscribe V5 Callback.
 *
 * This callback function is invoked when the MQTT client subscribes to a topic.
 *
 * @param mosq the mosquitto instance making the callback.
 * @param userdata the user data provided in mosquitto_new
 * @param mid the message id of the subscribe message.
 * @param qos_count the number of granted subscriptions (size of granted_qos).
 * @param granted_qos an array of integers indicating the granted QoS for each of the subscriptions.
 * @param props list of MQTT 5 properties, or NULL
 */
typedef void (*MQTT_ON_SUBSCRIBE_V5_CALLBACK)(
    struct mosquitto* mosq,
    void* userdata,
    int mid,
    int qos_count,
    const int* granted_qos,
    const mosquitto_property* props);

/**
 * @brief Typedef for the MQTT On Publish V5 Callback.
 *
 * This callback function is invoked when a message is successfully published.
 *
 * @param mosq A pointer to the MQTT client structure.
 * @param userdata A pointer to user-specific data.
 * @param mid The MQTT message ID.
 * @param qos The QoS level of the published message.
 * @param props Pointer to MQTT properties associated with the message.
 */
typedef void (*MQTT_ON_PUBLISH_V5_CALLBACK)(
    struct mosquitto* mosq, void* userdata, int mid, int qos, const mosquitto_property* props);

/**
 * @brief Typedef for the MQTT On Message V5 Callback.
 *
 * This callback function is invoked when a message is received.
 *
 * @param mosq A pointer to the MQTT client structure.
 * @param userdata A pointer to user-specific data.
 * @param message Pointer to the received MQTT message.
 * @param props Pointer to MQTT properties associated with the message.
 */
typedef void (*MQTT_ON_MESSAGE_V5_CALLBACK)(
    struct mosquitto*, void*, const struct mosquitto_message*, const mosquitto_property* props);

/**
 * @brief Typedef for the MQTT On Log Callback.
 *
 * This callback function is invoked for logging MQTT-related information.
 *
 * @param mosq  the mosquitto instance making the callback.
 * @param obj   the user data provided in mosquitto_new
 * @param level the log message level from the values: MOSQ_LOG_INFO MOSQ_LOG_NOTICE MOSQ_LOG_WARNING MOSQ_LOG_ERR MOSQ_LOG_DEBUG
 * @param str the message string.
 */
typedef void (*MQTT_ON_LOG_CALLBACK)(struct mosquitto* mosq, void* obj, int level, const char* str);

/**
 * @struct ADUC_MQTT_MQTT_CALLBACKS_TAG
 * @brief Struct containing MQTT callback functions.
 *
 * This struct holds pointers to various MQTT callback functions used for handling MQTT events.
 */
typedef struct ADUC_MQTT_MQTT_CALLBACKS_TAG
{
    /// Callback for MQTT On Connect V5 event.
    MQTT_ON_CONNECT_V5_CALLBACK on_connect_v5;

    /// Callback for MQTT On Disconnect V5 event.
    MQTT_ON_DISCONNECT_V5_CALLBACK on_disconnect_v5;

    /// A function pointer to the MQTT subscribe function.
    GET_MQTT_SUBSCRIPTION_TOPICS_FUNC get_subscription_topics;

    /// Callback for MQTT On Subscribe V5 event.
    MQTT_ON_SUBSCRIBE_V5_CALLBACK on_subscribe_v5;

    /// Callback for MQTT On Unsubscribe event.
    MQTT_UNSUBSCRIBE_V5_CALLBACK on_unsubscribe_v5;

    /// Callback for MQTT On Publish V5 event.
    MQTT_ON_PUBLISH_V5_CALLBACK on_publish_v5;

    /// Callback for MQTT On Message V5 event.
    MQTT_ON_MESSAGE_V5_CALLBACK on_message_v5;

    /// Callback for MQTT On Log event.
    MQTT_ON_LOG_CALLBACK on_log;
} ADUC_MQTT_CALLBACKS;

typedef struct ADUC_COMMUNICATION_CHANNEL_INIT_DATA_TAG
{
    const char* sessionId;
    void* ownerModuleContext;
    const ADUC_MQTT_SETTINGS* mqttSettings;
    ADUC_MQTT_CALLBACKS* callbacks;
    ADUC_MQTT_KEYFILE_PASSWORD_CALLBACK passwordCallback;
    ADUC_Retry_Params* connectionRetryParams; //!< [optional] Retry parameters for connection attempts.
} ADUC_COMMUNICATION_CHANNEL_INIT_DATA;

typedef struct ADUC_MQTT_SUBSCRIBE_CALLBACK_INFO_TAG
{
    char* topic;
    MQTT_ON_SUBSCRIBE_V5_CALLBACK callback;
    void* userData;
    int messageId;
    bool isScopedTopic;
} ADUC_MQTT_SUBSCRIBE_CALLBACK_INFO;

/**
 * @brief Structure to hold MQTT communication management state.
 */
typedef struct ADU_MQTT_COMMUNICATION_MGR_STATE_TAG
{
    bool initialized; /**< Indicates if the MQTT communication manager is initialized. */
    STRING_HANDLE sessionId; /**< Session ID. */
    bool topicsSubscribed; /**< Indicates if MQTT topics are subscribed. */
    struct mosquitto* mqttClient; /**< Pointer to the MQTT client. */
    ADUC_MQTT_SETTINGS mqttSettings; /**< MQTT settings. */
    ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE commState; /**< Communication channel state. */
    time_t commStateUpdatedTime; /**< Time when the communication channel state was last updated. */
    time_t commLastConnectedTime; /**< Time when the communication channel was last connected. */
    time_t commLastAttemptTime; /**< Time when the last connection attempt was made. */
    time_t commNextRetryTime; /**< Time when the next connection attempt should be made. */
    ADUC_MQTT_CALLBACKS mqttCallbacks; /**< MQTT callback functions. */
    ADUC_MQTT_KEYFILE_PASSWORD_CALLBACK keyFilePasswordCallback;
    void* ownerModuleContext; /**< Pointer to the owner module context. */
    ADUC_Retry_Params connectionRetryParams;
    VECTOR_HANDLE pendingSubscriptions; /**< List of subscribing topics. */
    ADUC_MQTT_SUBSCRIBE_CALLBACK_INFO subscribeTopicInfo; /**< Subscribed topic info. There exists only 1 response topic in adu protocol v1. */
} ADU_MQTT_COMMUNICATION_MGR_STATE;

ADU_MQTT_COMMUNICATION_MGR_STATE* CommunicationManagerStateFromModuleHandle(ADUC_AGENT_MODULE_HANDLE commHandle);

int ADUC_Communication_Channel_MQTT_Publish(
    ADUC_AGENT_MODULE_HANDLE commChannelModule,
    const char* topic,
    int* mid,
    int payload_len,
    const void* payload,
    int qos,
    bool retain,
    const mosquitto_property* props);

int ADUC_Communication_Channel_MQTT_Subscribe(
    ADUC_AGENT_MODULE_HANDLE commHandle,
    const char* topic,
    bool isTopicScoped,
    int qos,
    int options,
    const mosquitto_property* props,
    void* userData,
    MQTT_ON_SUBSCRIBE_V5_CALLBACK callback,
    int* outMessageId);

ADUC_AGENT_MODULE_HANDLE ADUC_Communication_Channel_Create();
void ADUC_Communication_Channel_Destroy();
bool ADUC_Communication_Channel_IsConnected(ADUC_AGENT_MODULE_HANDLE handle);

EXTERN_C_END

#endif // __ADUC_COMMUNICATION_CHANNEL_H__
