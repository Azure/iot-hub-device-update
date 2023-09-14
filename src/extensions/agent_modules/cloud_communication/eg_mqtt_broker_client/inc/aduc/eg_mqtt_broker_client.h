#ifndef __ADUC_ADUC_MQTT_Client_CLIENT_H__
#define __ADUC_ADUC_MQTT_Client_CLIENT_H__

/**
 * @file ADUC_MQTT_Client_client.h
 * @brief Implements the MQTT client for the Event Grid MQTT Broker.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/mqtt_client.h"
#include "stdbool.h"

EXTERN_C_BEGIN

/**
 * @brief Read the MQTT broker connection data from the config file.
 *
 * @
 * remark The caller is responsible for freeing the returned settings by calling FreeMqttBrokerSettings function.
 */
bool ReadMqttBrokerSettings(ADUC_MQTT_SETTINGS* settings);

/**
 * @brief Frees the MQTT broker connection settings.
*/
void FreeMqttBrokerSettings(ADUC_MQTT_SETTINGS* settings);


EXTERN_C_END

#endif // __ADUC_ADUC_MQTT_Client_CLIENT_H__
