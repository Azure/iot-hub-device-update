/**
 * @file adu_mqtt_protocol.h
 * @brief A header file for the Device Update client and service protocol definitions for MQTT broker.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef __ADU_MQTT_PROTOCOL_H__
#define __ADU_MQTT_PROTOCOL_H__

#include "aducpal/time.h"

/**
 * @brief Enrollment status request topic
 *
 * Topic : "adu/oto/{deviceId}/a"
 *
 * Example Message:
 *      {
 *      }
 *
 * User Properties:
 *      {
 *          "pid" : 1               // Protocol version
 *          "mt" : "enr_req"        // Message type
 *      }
 *
 * Content Type: "json"
 * Corelation Data" [uuid]
 */

// This is a topic template for the device to publish messages to the broker.
#define PUBLISH_TOPIC_TEMPLATE_ADU_OTO "adu/oto/%s/a"

// This is a topic template for the device to subscribe to listen for messages from the broker.
#define SUBSCRIBE_TOPIC_TEMPLATE_ADU_OTO "adu/oto/%s/s"

#define ADU_MQTT_PROTOCOL_VERSION "1"

#define ADU_MQTT_PROTOCOL_VERSION_PROPERTY_NAME "pid"

#define ADU_MQTT_PROTOCOL_MESSAGE_TYPE_PROPERTY_NAME "mt"
#define ADU_MQTT_PROTOCOL_MESSAGE_CONTENT_TYPE_JSON "application/json"

#define ADU_MQTT_PROTOCOL_MESSAGE_TYPE_ENROLLMENT_REQUEST "enr_req"

#define ADU_MQTT_PROTOCOL_MESSAGE_TYPE_ENROLLMENT_RESPONSE "enr_resp"

#define ADU_MQTT_PROTOCOL_MESSAGE_TYPE_ENROLLMENT_CHANGE_NOTIFICATION "enr_cn"

#define ADU_MQTT_PROTOCOL_MESSAGE_TYPE_AGENT_INFORMATION_REPORT_REQUEST "ainfo_req"

#define ADU_MQTT_PROTOCOL_MESSAGE_TYPE_AGENT_INFORMATION_REPORT_CONFIRMATION "ainfo_resp"

#define ADU_MQTT_PROTOCOL_MESSAGE_TYPE_UPDATE_AVAILABLE_NOTIFICATION "upd_cn"

#define ADU_MQTT_PROTOCOL_MESSAGE_TYPE_UPDATE_SYNC_REQUEST "upd_req"

#define ADU_MQTT_PROTOCOL_MESSAGE_TYPE_UPDATE_SYNC_RESPONSE "upd_resp"

#define ADU_MQTT_PROTOCOL_MESSAGE_TYPE_UPDATE_RESULT_REPORT_REQUEST "updrslt_req"

#define ADU_MQTT_PROTOCOL_MESSAGE_TYPE_UPDATE_RESULT_REPORT_CONFIRMATION "updrslt_resp"

#define ADU_MQTT_PROTOCOL_MESSAGE_TYPE_UPDATE_RESULT_REPORT_ACK "updrslt_ack"


typedef struct ADU_MQTT_MESSAGE_INFO_TAG
{
    char* correlationData; //!< Correlation data. This a epoch time in seconds.
    time_t sentTime; //!< Time the message was sent.
    int mid; //!< Message ID.
    int qos; //!< QOS level.
    int code; //!< Result code.
} ADU_MQTT_MESSAGE_INFO;

/**
 * @brief Describes the connection state of the ADU communication channel.
 *
 * This enumeration provides different connection states of the ADU communication channel
 * to better handle and track the state of the connection in the system.
 */
typedef enum ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_TAG
{
    ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_DISCONNECTED = -1, /**< The communication channel is disconnected. */
    ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_UNKNOWN = 0,       /**< The communication channel state is unknown. */
    ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTING = 1,    /**< The communication channel is currently connecting. */
    ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED = 2,     /**< The communication channel is connected. */
} ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE;

/**
 * @brief Describes the enrollment state of the ADU client.
 *
 * This enumeration provides different enrollment states for the ADU client
 * to better represent and track the enrollment status in the system.
 */
typedef enum ADU_ENROLLMENT_STATE_TAG
{
    ADU_ENROLLMENT_STATE_NOT_ENROLLED = -1, /**< The client is not enrolled. */
    ADU_ENROLLMENT_STATE_UNKNOWN = 0,       /**< The enrollment state of the client is unknown. */
    ADU_ENROLLMENT_STATE_ENROLLING = 1,     /**< The client is currently in the process of enrolling. */
    ADU_ENROLLMENT_STATE_ENROLLED = 2,      /**< The client is successfully enrolled. */
} ADU_ENROLLMENT_STATE;

/*
 * @brief Represents the MQTT topics used by the ADU client.
 *
 * This structure contains the MQTT topics required for the ADU client's
 * communication, including topics for sending messages to the service
 * and for receiving messages from the service.
 */
typedef struct ADUC_MQTT_TOPICS_TAG
{
    char* agentOTO;     //!< An MQTT topic for the agent to send messages to the service.
    char* serviceOTO;   //!< An MQTT topic for the agent to receive messages from the service.
} ADUC_MQTT_TOPICS;

#endif /* __ADU_MQTT_PROTOCOL_H__ */