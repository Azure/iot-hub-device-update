
#ifndef ADU_AGENTINFO_H
#define ADU_AGENTINFO_H

#include "aduc/mqtt_broker_common.h" // ADUC_MQTT_Message_Context
#include <aduc/retry_utils.h> // ADUC_Retriable_Operation_Context

EXTERN_C_BEGIN

/**
 * @brief Describes the agentInfo state of the ADU client.
 *
 * This enumeration provides different agentInfo states for the ADU client
 * to better represent and track the agentInfo status in the system.
 */
typedef enum tagADU_AGENTINFO_STATE
{
    ADU_AGENTINFO_STATE_UNKNOWN = 0, /**< The agentInfo state of the client is unknown. */
    ADU_AGENTINFO_STATE_SUBSCRIBED = 1, /**< The client is subscribed. */
    ADU_AGENTINFO_STATE_REQUESTING = 2, /**< The client is requesting an agentInfo status. */
    ADU_AGENTINFO_STATE_ACKNOWLEDGED = 3, /**< The client agent info report is successfully acknowledged. */
} ADU_AGENTINFO_STATE;

/**
 * @brief Struct to represent agentInfo status request operation for an Azure Device Update service agentInfo acknowledgement.
 */
typedef struct tagADUC_AgentInfo_Request_Operation_Data
{
    ADUC_Common_Response_User_Properties respUserProps; /**< Common Response User Properties. */
    ADU_AGENTINFO_STATE agentInfoState; /**< Current agentInfo acknowledgement state. */
    ADUC_MQTT_Message_Context ainfoReqMessageContext; /**< AgentInfo request message context. */
} ADUC_AgentInfo_Request_Operation_Data;

EXTERN_C_END

#endif // ADU_AGENTINFO_H
