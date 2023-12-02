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

void ADUC_AgentInfo_Management_Destroy(ADUC_AGENT_MODULE_HANDLE module);
void OnMessage_ainfo_resp(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props);
void OnPublish_ainfo_resp(struct mosquitto* mosq, void* obj, const mosquitto_property* props, int reason_code);

EXTERN_C_END

#endif // __ADUC_AGENT_INFO_MANAGEMENT_H__
