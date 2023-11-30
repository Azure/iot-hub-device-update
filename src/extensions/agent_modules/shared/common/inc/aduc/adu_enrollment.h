#ifndef ADU_ENROLLMENT_H
#define ADU_ENROLLMENT_H

#include "aduc/mqtt_broker_common.h" // ADUC_MQTT_Message_Context

EXTERN_C_BEGIN

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
    ADU_ENROLLMENT_STATE_REQUESTING = 1, /**< The client is requesting an enrollment status. */
    ADU_ENROLLMENT_STATE_REQUESTED = 2, /**< The client has received an on_publish callback from the MQTT library and is awaiting a message on the subscribed response topic. */
    ADU_ENROLLMENT_STATE_ENROLLED = 3, /**< The client is successfully enrolled. */
} ADU_ENROLLMENT_STATE;

/**
 * @brief Struct to represent enrollment status request operation for an Azure Device Update service enrollment.
 */
typedef struct tagADUC_Enrollment_Request_Operation_Data
{
    ADUC_Common_Response_User_Properties respUserProps; /**< Common Response User Properties. */
    ADU_ENROLLMENT_STATE enrollmentState; /**< Current enrollment state. */
    ADUC_MQTT_Message_Context enrReqMessageContext; /**< Enrollment request message context. */
} ADUC_Enrollment_Request_Operation_Data;

EXTERN_C_END

#endif // ADU_ENROLLMENT_H
