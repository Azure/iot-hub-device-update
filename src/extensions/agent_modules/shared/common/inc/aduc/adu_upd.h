#ifndef ADU_UPD_H
#define ADU_UPD_H

#include "aduc/mqtt_broker_common.h" // ADUC_MQTT_Message_Context

EXTERN_C_BEGIN

// clang-format off
/**
 * @brief THe update state of the the sending of 'upd_req' and handling of the 'upd_resp' response as per the adu protocol.
 */
typedef enum tagADU_UPD_STATE
{
    ADU_UPD_STATE_READY             =  0, /**< The update module is ready to start requesting in the next frame of execution. */
    ADU_UPD_STATE_IDLEWAIT          =  1, /**< The upd state is waiting for wait timer timeout to poll for updates. */
    ADU_UPD_STATE_REQUESTING        =  2, /**< In the process of requesting for updates. */
    ADU_UPD_STATE_RETRYWAIT         =  3, /**< The upd state is waiting for wait timer timeout to poll for updates. */
    ADU_UPD_STATE_REQUEST_ACK       =  4, /**< Received PUBACK from the MQTT broker and is awaiting a response message. */
    ADU_UPD_STATE_PROCESSING_UPDATE =  6, /**< In the process of processing the update for installation on the device. */
} ADU_UPD_STATE;
// clang-format on

/**
 * @brief Struct to represent upd status request operation for an Azure Device Update service upd.
 */
typedef struct tagADUC_Update_Request_Operation_Data
{
    ADUC_Common_Response_User_Properties respUserProps; /**< Common Response User Properties. */
    ADU_UPD_STATE updState; /**< Current upd state. */
    ADUC_MQTT_Message_Context updReqMessageContext; /**< upd request message context. */
} ADUC_Update_Request_Operation_Data;

const char* AduUpdState_str(ADU_UPD_STATE updateModuleState);

EXTERN_C_END

#endif // ADU_UPD_H
