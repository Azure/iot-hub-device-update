/**
 * @file adu_enrollment_management.h
 * @brief A header file for the Device Update service enrollment status management.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef __ADU_ENROLLMENT_MANAGEMENT_H__
#define __ADU_ENROLLMENT_MANAGEMENT_H__

#include "aduc/c_utils.h" // EXTERN_C_* macros
#include "du_agent_sdk/agent_module_interface.h"
#include <mosquitto.h>
#include <mqtt_protocol.h>

EXTERN_C_BEGIN

void ADUC_Enrollment_Management_Destroy(ADUC_AGENT_MODULE_HANDLE module);
void OnMessage_enr_resp(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props);
void OnPublish_enr_resp(struct mosquitto* mosq, void* obj, const mosquitto_property* props, int reason_code);

EXTERN_C_END

#endif // __ADU_ENROLLMENT_MANAGEMENT_H__
