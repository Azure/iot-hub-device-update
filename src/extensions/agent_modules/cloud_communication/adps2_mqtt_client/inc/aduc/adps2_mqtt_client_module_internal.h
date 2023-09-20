#ifndef __ADPS2_MQTT_CLIENT_MODULE_INTERNAL_H__
#define __ADPS2_MQTT_CLIENT_MODULE_INTERNAL_H__

/**
 * @file adps2-mqtt-client-module-internal.h
 * @brief Implementation for the private APIs for the Azure Device Provisioning MQTT client module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adps2_mqtt_client_module.h"

bool ADPS_MQTT_CLIENT_MODULE_IsDeviceRegistered(ADUC_AGENT_MODULE_HANDLE handle);

const char* ADPS_MQTT_CLIENT_MODULE_GetMQTTBrokerEndpoint(ADUC_AGENT_MODULE_HANDLE handle);

#endif // __ADPS2_MQTT_CLIENT_MODULE_H__
