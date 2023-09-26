/**
 * @file adu_updates_management.h
 * @brief A header file for utility functions for mosquitto library.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef __ADU_UPDATES_MANAGEMENT_H__
#define __ADU_UPDATES_MANAGEMENT_H__

#include <mosquitto.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
 * @brief Initialize the updates management component.
 */
    int ADUC_Updates_Management_Initialize();

    /**
 * @brief Deinitialize the updates management component.
 */
    void ADUC_Updates_Management_Deinitialize();

    /**
 * @brief Perform the work of the updates management component.
 * @return True if the work was successfully performed; otherwise, false.
 */
    bool ADUC_Updates_Management_DoWork();

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
        struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props);

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
        struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props);

#ifdef __cplusplus
}
#endif

#endif // __ADU_UPDATES_MANAGEMENT_H__
