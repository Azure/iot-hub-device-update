#ifndef ADU_ENROLLMENT_H
#define ADU_ENROLLMENT_H

#include "aduc/mqtt_broker_common.h" // CORRELATION_ID_LENGTH
#include <aduc/retry_utils.h>
#include <mosquitto.h>

/**
 * @brief Describes the enrollment state of the ADU client.
 *
 * This enumeration provides different enrollment states for the ADU client
 * to better represent and track the enrollment status in the system.
 */
typedef enum tagADU_ENROLLMENT_STATE
{
    ADU_ENROLLMENT_STATE_NOT_ENROLLED = -1, /**< The client is not enrolled. */
    ADU_ENROLLMENT_STATE_UNKNOWN = 0, /**< The enrollment state of the client is unknown. */
    ADU_ENROLLMENT_STATE_SUBSCRIBED = 1, /**< The client is subscribed. */
    ADU_ENROLLMENT_STATE_REQUESTING = 2, /**< The client is requesting an enrollment status. */
    ADU_ENROLLMENT_STATE_ENROLLED = 3, /**< The client is successfully enrolled. */
} ADU_ENROLLMENT_STATE;

typedef struct tagADUC_MQTT_Message_Context
{
    int messageId;
    char correlationId[CORRELATION_ID_LENGTH];
    char* publishTopic;
    char* responseTopic;
    char* payload;
    size_t payloadLen;
    mosquitto_property* properties;
    int qos;
} ADUC_MQTT_Message_Context;

/**
 * @brief Struct to represent enrollment status request operation for an Azure Device Update service enrollment.
 */
typedef struct tagADUC_Enrollment_Request_Operation_Data
{
    /// Current enrollment state.
    ADU_ENROLLMENT_STATE enrollmentState;

    ADUC_MQTT_Message_Context enrReqMessageContext;

    /// @brief A reference to the parent context of this enrollment status request operation.
    ADUC_Retriable_Operation_Context* ownerContext;

} ADUC_Enrollment_Request_Operation_Data;

#endif // ADU_ENROLLMENT_H
