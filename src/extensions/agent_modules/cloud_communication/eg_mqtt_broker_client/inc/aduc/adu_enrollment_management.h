/**
 * @file adu_enrollment_management.h
 * @brief A header file for the Device Update service enrollment status management.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef __ADU_ENROLLMENT_MANAGEMENT_H__
#define __ADU_ENROLLMENT_MANAGEMENT_H__

#include "du_agent_sdk/agent_module_interface.h"
#include <mosquitto.h>
#include <mqtt_protocol.h>

/**
 * @brief Create the enrollment management module.
 */
ADUC_AGENT_MODULE_HANDLE ADUC_Enrollment_Management_Create();

/**
 * @brief Destroy the enrollment management module.
 */
void ADUC_Enrollment_Management_Destroy(ADUC_AGENT_MODULE_HANDLE module);

/**
 * @brief Callback should be called when the client receives an enrollment status response message from the Device Update service.
 * @param mosq The mosquitto instance making the callback.
 * @param obj The user data provided in mosquitto_new. This is the module state
 * @param msg The message data.
 * @param props The MQTT v5 properties returned with the message.
 */
void OnMessage_enr_resp(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props);

/**
 * @brief Callback should be called when the client receives an enrollment status change notification message from the Device Update service.
 * @param mosq The mosquitto instance making the callback.
 * @param obj The user data provided in mosquitto_new. This is the module state
 * @param msg The message data.
 * @param props The MQTT v5 properties returned with the message.
 */
void OnMessage_enr_cn(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props);

#endif // __ADU_ENROLLMENT_MANAGEMENT_H__
