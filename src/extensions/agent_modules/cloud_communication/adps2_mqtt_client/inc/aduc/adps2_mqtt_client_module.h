/**
 * @file adps2_mqtt_client_module.h
 * @brief Implementation for the device provisioning using Azure DPS V2.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef __ADPS2_MQTT_CLIENT_MODULE_H__
#define __ADPS2_MQTT_CLIENT_MODULE_H__

#include "aduc/adps_gen2.h"
#include "aduc/adu_types.h"
#include "du_agent_sdk/agent_module_interface.h"
#include <mosquitto.h> // mosquitto related functions

EXTERN_C_BEGIN

typedef enum ADPS_MQTT_CLIENT_MODULE_DATA_KEY_TAG
{
    ADPS_MQTT_CLIENT_MODULE_DATA_KEY_REGISTER_STATE = 0,
    ADPS_MQTT_CLIENT_MODULE_DATA_KEY_MQTT_BROKER_ENDPOINT = 100
} ADPS_MQTT_CLIENT_MODULE_DATA_KEY;

/**
 * @brief Create a Device Update Agent Module for IoT Hub PnP Client.
 * @return ADUC_AGENT_MODULE_HANDLE The handle to the module.
 */
ADUC_AGENT_MODULE_HANDLE ADPS_MQTT_Client_Module_Create();

/*
 * @brief Destroy the Device Update Agent Module for IoT Hub PnP Client.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 */
void ADPS_MQTT_Client_Module_Destroy(ADUC_AGENT_MODULE_HANDLE handle);


EXTERN_C_END

#endif // __ADPS2_MQTT_CLIENT_MODULE_H__
