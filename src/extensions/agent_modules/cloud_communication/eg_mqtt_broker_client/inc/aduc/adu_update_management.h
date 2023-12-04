/**
 * @file adu_update_management.h
 * @brief A header file for the Device Update service update status management.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADU_UPDATE_MANAGEMENT_H
#define ADU_UPDATE_MANAGEMENT_H

#include "aduc/c_utils.h" // EXTERN_C_* macros
#include "du_agent_sdk/agent_module_interface.h"
#include <mosquitto.h>
#include <mqtt_protocol.h>

EXTERN_C_BEGIN

void ADUC_update_Management_Destroy(ADUC_AGENT_MODULE_HANDLE module);
void OnMessage_upd_resp(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props);
void OnPublish_upd_resp(struct mosquitto* mosq, void* obj, const mosquitto_property* props, int reason_code);

EXTERN_C_END

#endif // ADU_UPDATE_MANAGEMENT_H
