/**
 * @file adu_agent_info_management.h
 * @brief Header file for Device Update agent info management functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef __ADUC_AGENT_INFO_MANAGEMENT_H__
#define __ADUC_AGENT_INFO_MANAGEMENT_H__

#include "aduc/c_utils.h" // EXTERN_C_* macros
#include "du_agent_sdk/agent_module_interface.h"
#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h> // mosquitto_property

EXTERN_C_BEGIN

DECLARE_AGENT_MODULE_PUBLIC(ADUC_Agent_Info_Management)

/**
 * @brief Destroy the Agent Info management module.
 * @param module The module handle.
 */
void ADUC_AgentInfo_Management_Destroy(ADUC_AGENT_MODULE_HANDLE module);

/**
 * @brief Callback should be called when the client receives an agent info response message from the Device Update service.
 * @param mosq The mosquitto instance making the callback.
 * @param obj The user data provided in mosquitto_new. This is the module state
 * @param msg The message data.
 * @param props The MQTT v5 properties returned with the message.
 */
void OnMessage_ainfo_resp(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props);

EXTERN_C_END

#endif // __ADUC_AGENT_INFO_MANAGEMENT_H__
