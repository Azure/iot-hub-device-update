/**
 * @file adu_communication_channel.h
 * @brief Header file for the Device Update communication channel (MQTT broker) management.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef __ADUC_COMMUNICATION_CHANNEL_H__
#define __ADUC_COMMUNICATION_CHANNEL_H__

#include "aduc/c_utils.h" // EXTERN_C_* macros
#include "aduc/mqtt_client.h" // ADUC_MQTT_SETTINGS
#include "stdbool.h" // bool
#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h> // mosquitto_property

#include <mqtt_protocol.h>

EXTERN_C_BEGIN

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
typedef void (*MQTT_ON_CONNECT_V5_CALLBACK)(struct mosquitto*, void*, int, int, const mosquitto_property* props);

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
typedef bool (*GET_MQTT_SUBSCRIPTION_TOPICS_FUNC)(int* count, char*** topics);

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

/*
 * @brief  Initialize the communication channel.
 * @param[in] session_id The session ID to use for the MQTT client.
 * @param[in] module_state The module state to use for the MQTT client.
 * @param[in] mqtt_settings The MQTT settings to use for the MQTT client.
 * @param[in] callbacks The MQTT callbacks to use for the MQTT client.
 * @param[in] pw_callback The password callback to use for the MQTT client.
 * @return true if the communication channel was successfully initialized; otherwise, false.
 */
bool ADUC_CommunicationChannel_Initialize(
    const char* session_id,
    void* module_state,
    const ADUC_MQTT_SETTINGS* mqtt_settings,
    ADUC_MQTT_CALLBACKS* callbacks,
    ADUC_MQTT_KEYFILE_PASSWORD_CALLBACK pw_callback);

/*
 * @brief Deinitialize the communication channel.
 */
void ADUC_CommunicationChannel_Deinitialize();

/**
 * @brief Ensure that the communication channel to the DU service is valid.
 *
 * @remark This function requires that the MQTT client is initialized.
 * @remark This function requires that the MQTT connection settings are valid.
 *
 * @return true if the communication channel state is ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED.
 */
bool ADUC_CommunicationChannel_DoWork();

/**
 * @brief Publishes a message to a the 'adu/oto/#/a' topic using version 5 of the MQTT protocol.
 *
 * This function is a wrapper around `mosquitto_publish_v5`, simplifying the process of
 * publishing messages to an MQTT broker.
 *
 * @param[in]  topic The topic to which the message will be published.
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
    const char* topic,
    int* mid,
    size_t payload_len,
    const void* payload,
    int qos,
    bool retain,
    const mosquitto_property* props);

/*
 * @brief Check if the communication channel is in connected state.
 */
bool ADUC_CommunicationChannel_IsConnected();

EXTERN_C_END

#endif // __ADUC_COMMUNICATION_CHANNEL_H__