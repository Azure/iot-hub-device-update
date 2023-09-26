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
 * @brief Initialize the enrollment management.
 * @return true if the enrollment management was successfully initialized; otherwise, false.
 */
bool ADUC_Enrollment_Management_Initialize(ADUC_AGENT_MODULE_HANDLE communicationModule);

/**
 * @brief Deinitialize the enrollment management.
 */
void ADUC_Enrollment_Management_Deinitialize();

/**
 * @brief Ensure that the device is enrolled with the Device Update service.
 */
bool ADUC_Enrollment_Management_DoWork();

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

/**
 * @brief Retrieve the Azure Device Update service instance.
 * @return The Azure Device Update service instance.
 */
const char* ADUC_Enrollment_Management_GetDUInstance();

/**
 * @brief Set the MQTT publishing topic for communication from the Agent to the Service.
 *
 * This function allows setting the MQTT publishing topic for messages sent from the Agent to the Service.
 * If the previous topic was set, it is freed to avoid memory leaks. If the provided 'topic' is not empty or NULL,
 * it is duplicated and assigned to the internal MQTT topic variable.
 *
 * @param topic A pointer to the MQTT publishing topic string to be set.
 *
 * @return true if the MQTT topic was successfully set or updated; false if 'topic' is empty or NULL.
 *
 * @note The memory allocated for the previous topic (if any) is freed to prevent memory leaks.
 * @note If 'topic' is empty or NULL, the function returns false.
 */
bool ADUC_Enrollment_Management_SetAgentToServiceTopic(const char* topic);

#endif // __ADU_ENROLLMENT_MANAGEMENT_H__
