/**
 * @file adu_mqtt_client_module.h
 * @brief A header file for the Device Update MQTT client module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef __ADU_MQTT_CLIENT_MODULE_H__
#define __ADU_MQTT_CLIENT_MODULE_H__

#include "aduc/adu_enrollment.h"

#include <aduc/adu_mqtt_protocol.h> // for ADUC_COMMUNICATION_*, ADUC_ENROLLMENT_*, ADUC_MQTT_MESSAGE_*, ADUC_MQTT_SETTINGS, ADUC_Result
#include <aduc/result.h> // for ADUC_Result
#include <aducpal/time.h> // for time_t
#include <du_agent_sdk/agent_module_interface.h> // for ADUC_AGENT_MODULE_HANDLE
#include <du_agent_sdk/mqtt_client_settings.h> // for ADUC_MQTT_SETTINGS
#include <mosquitto.h> // for mosquitto related functions
#include <mqtt_protocol.h> // for mosquitto_property

EXTERN_C_BEGIN

/**
 * @brief Create the MQTT client module instance.
 */
ADUC_AGENT_MODULE_HANDLE ADUC_MQTT_Client_Module_Create();

/**
 * @brief Destroy the MQTT client module instance.
 */
void ADUC_MQTT_Client_Module_Destroy(ADUC_AGENT_MODULE_HANDLE handle);

EXTERN_C_END

#endif /* __ADU_MQTT_CLIENT_MODULE_H__ */
