/**
 * @file adu_updates_management.c
 * @brief Implementation for the deployments and updates management related functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_updates_management.h"
#include "aduc/adu_mosquitto_utils.h"
#include "aduc/adu_mqtt_protocol.h"
#include "aduc/logging.h"

#include <aducpal/time.h> // time_t
#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h> // mosquitto_property

typedef enum ADU_MQTT_UPDATE_SYNC_STATE_TAG
{
    ADU_MQTT_UPDATE_SYNC_STATE_FAILED = -2,
    ADU_MQTT_UPDATE_SYNC_STATE_PROCESS_FAILED = -2,
    ADU_MQTT_UPDATE_SYNC_STATE_REQUEST_FAILED = -1,
    ADU_MQTT_UPDATE_SYNC_STATE_UNKNOWN = 0,
    ADU_MQTT_UPDATE_SYNC_STATE_SENDING = 1,
    ADU_MQTT_UPDATE_SYNC_STATE_PUBLISHED = 2,
    ADU_MQTT_UPDATE_SYNC_STATE_RESPONDED = 3,
    ADU_MQTT_UPDATE_SYNC_STATE_PROCESSING = 4,
    ADU_MQTT_UPDATE_SYNC_STATE_PROCESS_COMPLETED = 5,
} ADU_MQTT_UPDATE_SYNC_STATE;

typedef struct ADU_UPDATES_MANAGEMENT_STATE_TAG
{
    ADU_MQTT_UPDATE_SYNC_STATE upd_req_state;
    char* upd_req_correlation_id;
    time_t upd_req_attempt_time;
    time_t next_upd_req_attempt_time;
    time_t last_upd_req_success_time;
    time_t last_upd_req_error_time;

    time_t upd_resp_time;
    char* upd_resp_content;

    time_t last_upd_cn_time;
    char*
        last_upd_cn_correlation_id; //!< The correlation ID for the last 'update change notification'. If this value is not NULL, the client will resent an 'upd_req' message with this correlation ID.
} ADU_UPDATES_MANAGEMENT_STATE;

static ADU_UPDATES_MANAGEMENT_STATE s_updMgrState = { 0 };

/**
 * @brief Initialize the updates management component.
 */
int ADUC_Updates_Management_Initialize()
{
    return true;
}

/**
 * @brief Deinitialize the updates management component.
 */
void ADUC_Updates_Management_Deinitialize()
{
}

/**
 * @brief Callback called when the client receives an 'Update Request' response message from the Device Update service.
 *
 * @param mosq The mosquitto instance making the callback.
 * @param obj The user data provided in mosquitto_new. This is the module state
 * @param msg The message data.
 * @param props The MQTT v5 properties returned with the message.
 *
 * @remark This callback is called by the network thread. Usually by the same thread that calls mosquitto_loop function.
 * IMPORTANT - Do not use blocking functions in this callback.
 */
void OnMessage_upd_resp(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    // TODO (nox-msft) : Parse update data and send it to the agent update workflow_utils for processing.
}

/**
 * @brief Callback invoked upon receiving an 'upd_cn' (update notification) message from the MQTT broker.
 *
 * This function is called when the client receives an 'upd_cn' message from the broker.
 * Upon successful validation of the message type and protocol id, this function updates
 * the state with the latest update notification time and logs the correlation id (cid)
 * if provided in the MQTT v5 properties of the message.
 *
 * @param[in] mosq Pointer to the mosquitto instance making the callback.
 * @param[in] obj User-defined data, provided during callback registration.
 * @param[in] msg Pointer to the structure containing details about the received message.
 * @param[in] props MQTT v5 properties associated with the message.
 *
 * @remark If the message or its payload is NULL, an error will be logged.
 *         If the message type is not 'upd_cn' or the protocol id is not '1', the function will return after logging a warning or info.
 *         If a correlation id is provided in the MQTT v5 properties, it's logged. Otherwise, a default empty cid is used.
 */
void OnMessage_upd_cn(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    if (msg == NULL || props == NULL)
    {
        Log_Error("Invalid param - msg:%d, props:%d", msg, props);
        return;
    }
    // Check that message type is 'upd_cn'
    if (!ADU_mosquitto_has_user_property(props, "mt", "upd_cn"))
    {
        Log_Info("IGNORE: Message type is not 'upd_cn'");
        return;
    }
    // Check that protocol id is '1'
    if (!ADU_mosquitto_has_user_property(props, "pid", "1"))
    {
        Log_Warn("IGNORE: Protocol is not '1'");
        return;
    }

    s_updMgrState.last_upd_cn_time = time(NULL);

    // This message doesn't contain any payload, we'll publish the 'upd_req' message using the CID from the notification.
    char* cid = NULL;
    if (!ADU_mosquitto_get_correlation_data(props, &cid))
    {
        cid = GenerateCorrelationIdFromTime(s_updMgrState.last_upd_cn_time);
        Log_Warn("No cid. Generating one. cid:%s", cid);
    }

    Log_Info(
        "Received 'upd_cn' message. time:%s, cid:%s",
        s_updMgrState.last_upd_cn_time,
        s_updMgrState.last_upd_cn_correlation_id = cid);
}

void OnPublished_upd_req(struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* props)
{
    Log_Info("on_publish: Message with mid %d has been published.", mid);
}

/**
 * @brief Ensure that the device is enrolled with the Device Update service.
 * @param createNew If true, create a new enrollment request. Otherwise, manage state of the existing one.
 * @param correlationId The correlation ID to use for the enrollment request.
 * @return True if the enrollment request was successfully sent; otherwise, false.
 */
bool ADUC_Updates_Management_DoWork_Helper(bool createNew, const char* correlationId)
{
    int result = false;
    // TODO (nox-msft) : implement the update management workflow.
    return result;
}

/**
 * @brief Perform the work of the updates management component.
 * @return True if the work was successfully performed; otherwise, false.
 */
bool ADUC_Updates_Management_DoWork()
{
    return ADUC_Updates_Management_DoWork_Helper(false, NULL);
}
