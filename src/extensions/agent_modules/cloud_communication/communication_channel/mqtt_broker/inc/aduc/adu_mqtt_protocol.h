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
#define PUBLISH_TOPIC_TEMPLATE_ADU_OTO_WITH_DU_INSTANCE "adu/oto/%s/a/%s"

// This is a topic template for the device to subscribe to listen for messages from the broker.
#define SUBSCRIBE_TOPIC_TEMPLATE_ADU_OTO "adu/oto/%s/s"
#define SUBSCRIBE_TOPIC_TEMPLATE_ADU_OTO_WITH_DU_INSTANCE "adu/oto/%s/s/%s"

#define ADU_MQTT_PROTOCOL_VERSION "1"

#define ADU_MQTT_PROTOCOL_VERSION_PROPERTY_NAME "pid"

#define ADU_MQTT_PROTOCOL_MESSAGE_TYPE_PROPERTY_NAME "mt"
#define ADU_MQTT_PROTOCOL_MESSAGE_CONTENT_TYPE_JSON "json"

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
    ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_UNKNOWN = 0, /**< The communication channel state is unknown. */
    ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTING =
        1, /**< The communication channel is currently connecting. */
    ADU_COMMUNICATION_CHANNEL_CONNECTION_STATE_CONNECTED = 2, /**< The communication channel is connected. */
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
    ADU_ENROLLMENT_STATE_UNKNOWN = 0, /**< The enrollment state of the client is unknown. */
    ADU_ENROLLMENT_STATE_SUBSCRIBED = 1, /**< The client is subscribed. */
    ADU_ENROLLMENT_STATE_REQUESTING = 2, /**< The client is requesting an enrollment status. */
    ADU_ENROLLMENT_STATE_ENROLLED = 3, /**< The client is successfully enrolled. */
} ADU_ENROLLMENT_STATE;

/**
 * @brief Enumeration representing the initialization states of the ADU MQTT client module.
 *
 * @note This enumeration is used to track the initialization progress of the ADU MQTT client module.
 */
typedef enum ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE_TAG
{
    ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE_FAILED = -1, /**< Initialization has failed. */
    ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE_NONE = 0, /**< No initialization state. */
    ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE_PARTIAL = 1, /**< Partial initialization. */
    ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE_COMPLETED = 2, /**< Initialization has been successfully completed. */
} ADU_MQTT_CLIENT_MODULE_INITIALIZE_STATE;

#define ADU_AINFO_RESP_MESSAGE_RESULT_CODE_PROPERTY_NAME "resultCode"
#define ADU_AINFO_RESP_MESSAGE_EXTENDED_RESULT_CODE_PROPERTY_NAME "extendedResultCode"
#define ADU_AINFO_RESP_MESSAGE_RESULT_DESCRIPTION_PROPERTY_NAME "resultDescription"

/**
 * @brief Enumeration representing result codes for ADU response messages.
 */
typedef enum ADU_RESPONSE_MESSAGE_RESULT_CODE_TAG
{
    ADU_RESPONSE_MESSAGE_RESULT_CODE_SUCCESS = 0, /**< Operation was successful. */
    ADU_RESPONSE_MESSAGE_RESULT_CODE_BAD_REQUEST = 1, /**< The request was invalid or cannot be served. */
    ADU_RESPONSE_MESSAGE_RESULT_CODE_BUSY = 2, /**< The server is busy and cannot process the request. */
    ADU_RESPONSE_MESSAGE_RESULT_CODE_CONFLICT = 3, /**< There is a conflict with the current state of the system. */
    ADU_RESPONSE_MESSAGE_RESULT_CODE_SERVER_ERROR = 4, /**< The server encountered an internal error. */
    ADU_RESPONSE_MESSAGE_RESULT_CODE_AGENT_NOT_ENROLLED = 5 /**< The agent is not enrolled. */
} ADU_RESPONSE_MESSAGE_RESULT_CODE;

/**
 * @brief Enumeration representing extended result codes for ADU response messages.
 */
typedef enum ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_TAG
{
    ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_NONE = 0, /**< No extended error. */
    ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_UNABLE_TO_PARSE_MESSAGE =
        1, /**< Unable to parse the provided message. */
    ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_MISSING_OR_INVALID_VALUE =
        2, /**< A required value is missing or invalid. */
    ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_MISSING_OR_INVALID_CORRELATION_ID =
        3, /**< Missing or invalid correlation ID. */
    ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_MISSING_OR_INVALID_MESSAGE_TYPE =
        4, /**< Missing or invalid message type. */
    ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_MISSING_OR_INVALID_PROTOCOL_VERSION =
        5, /**< Missing or invalid protocol version. */
    ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_PROTOCOL_VERSION_MISMATCH =
        6, /**< Mismatch in protocol versions between client and server. */
    ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE_MISSING_OR_INVALID_CONTENT_TYPE =
        7, /**< Missing or invalid content type. */
} ADU_RESPONSE_MESSAGE_EXTENDED_RESULT_CODE;

#endif /* __ADU_MQTT_PROTOCOL_H__ */
