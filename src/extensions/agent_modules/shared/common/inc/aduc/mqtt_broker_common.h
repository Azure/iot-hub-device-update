#ifndef MQTT_BROKER_COMMON
#define MQTT_BROKER_COMMON

#include <stddef.h> // size_t
#include <stdint.h> // int32_t

/**
 * @brief The length of a correlation ID. (including null terminator)
 */
#define CORRELATION_ID_LENGTH 37

/**
 * @brief The MQTT message context
 *
 */
typedef struct tagADUC_MQTT_Message_Context
{
    int messageId;
    char* correlationId;
    char* publishTopic;
    char* responseTopic;
    char* payload;
    size_t payloadLen;
    int qos;
} ADUC_MQTT_Message_Context;


/**
 * @brief The parsed MQTT Resonse user properties.
 *
 */
typedef struct tagADUC_Common_Response_User_Properties
{
    int32_t pid; /**< The protocol id for enrollment response. */
    int32_t resultcode; /**< The result code of the enrollment response. */
    int32_t extendedresultcode; /**< The extended result code of the enrollment response. */
} ADUC_Common_Response_User_Properties;

#endif // MQTT_BROKER_COMMON
