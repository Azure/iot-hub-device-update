/**
 * @file adu_communication_channel.h
 * @brief Header file for the Device Update communication channel (MQTT broker) management.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef __ADUC_COMMUNICATION_CHANNEL_H__
#define __ADUC_COMMUNICATION_CHANNEL_H__

#include <mosquitto.h>      // mosquitto related functions
#include <mqtt_protocol.h>  // mosquitto_property
#include "stdbool.h"        // bool

#ifdef __cplusplus
extern "C" {
#endif

#include <mqtt_protocol.h>

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
typedef void (*MQTT_ON_CONNECT_V5_CALLBACK)(struct mosquitto *, void *, int, int, const mosquitto_property *props);

/**
 * @brief Typedef for the MQTT On Disconnect V5 Callback.
 *
 * This callback function is invoked when the MQTT client disconnects.
 *
 * @param mosq A pointer to the MQTT client structure.
 * @param userdata A pointer to user-specific data.
 * @param rc The return code from the MQTT broker.
 * @param props Pointer to MQTT properties associated with the disconnection.
 */
typedef void (*MQTT_ON_DISCONNECT_V5_CALLBACK)(struct mosquitto *, void *, int, const mosquitto_property *props);

/**
 * @brief Typedef for the MQTT On Subscribe V5 Callback.
 *
 * This callback function is invoked when the MQTT client subscribes to a topic.
 *
 * @param mosq A pointer to the MQTT client structure.
 * @param userdata A pointer to user-specific data.
 * @param mid The MQTT message ID.
 * @param qos_count The number of topics successfully subscribed to.
 * @param granted_qos Array of QoS levels for each topic subscription.
 * @param props Pointer to MQTT properties associated with the subscription.
 */
typedef void (*MQTT_ON_SUBSCRIBE_V5_CALLBACK)(struct mosquitto *, void *, int, int, const int *, const mosquitto_property *props);

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
typedef void (*MQTT_ON_PUBLISH_V5_CALLBACK)(struct mosquitto *, void *, int, int, const mosquitto_property *props);

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
typedef void (*MQTT_ON_MESSAGE_V5_CALLBACK)(struct mosquitto *, void *, const struct mosquitto_message *, const mosquitto_property *props);

/**
 * @brief Typedef for the MQTT On Log Callback.
 *
 * This callback function is invoked for logging MQTT-related information.
 *
 * @param mosq A pointer to the MQTT client structure.
 * @param userdata A pointer to user-specific data.
 * @param level The log level.
 * @param message The log message.
 */
typedef void (*MQTT_ON_LOG_CALLBACK)(struct mosquitto *, void *, int, const char *);

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

    /// Callback for MQTT On Subscribe V5 event.
    MQTT_ON_SUBSCRIBE_V5_CALLBACK on_subscribe_v5;

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
 * @param[in] callbacks The MQTT callbacks to use for the MQTT client.
 * @return true if the communication channel was successfully initialized; otherwise, false.
 */
bool ADUC_CommunicationChannel_Initialize(const char* session_id, void *module_state, ADUC_MQTT_CALLBACKS *callbacks);

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
int ADUC_CommunicationChannel_MQTT_Publish(int* mid, size_t payload_len, const void* payload, int qos, bool retain, const mosquitto_property* props);

/*
 * @brief Check if the communication channel is in connected state.
 */
bool ADUC_CommunicationChannel_IsConnected();

#ifdef __cplusplus
}
#endif


#endif // __ADUC_COMMUNICATION_CHANNEL_H__
