/**
 * @file adu_enrollment_management.h
 * @brief A header file for the Device Update service enrollment status management.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef __ADU_ENROLLMENT_MANAGEMENT_H__
#define __ADU_ENROLLMENT_MANAGEMENT_H__

#include <mosquitto.h>
#include <mqtt_protocol.h>

/**
 * @brief Initialize the enrollment management.
 * @return true if the enrollment management was successfully initialized; otherwise, false.
 */
bool ADUC_Enrollment_Management_Initialize();

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
 * @brief Retrieve the device enrollment state.
 *
 * @note This function will be replaced with the Inter-module communication (IMC) mechanism.
 */
bool IsDeviceEnrolled();

#endif // __ADU_ENROLLMENT_MANAGEMENT_H__
