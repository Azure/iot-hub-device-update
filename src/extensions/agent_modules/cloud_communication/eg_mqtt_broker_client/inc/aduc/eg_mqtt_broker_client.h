
/**
 * @file ADUC_MQTT_Client_client.h
 * @brief Implements the MQTT client for the Event Grid MQTT Broker.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef __ADUC_ADUC_MQTT_Client_CLIENT_H__
#define __ADUC_ADUC_MQTT_Client_CLIENT_H__

#include <du_agent_sdk/mqtt_client_settings.h>
#include <stdbool.h>

EXTERN_C_BEGIN

/**
 * @brief Reads MQTT broker connection settings from the configuration file.
 *
 * This function reads the MQTT client settings for communicating with the Azure Device Update service
 * from the configuration file. The settings are read from the `agent.connectionData.mqttBroker` section
 * of the configuration file.
 *
 * @param settings A pointer to an `ADUC_MQTT_SETTINGS` structure to populate with the MQTT broker settings.
 *
 * @return Returns `true` if the settings were successfully read and populated, otherwise `false`.
 *
 * @remark The caller is responsible for freeing the returned settings by calling `FreeMqttBrokerSettings` function.
 */
bool ReadMqttBrokerSettings(ADUC_MQTT_SETTINGS* settings);

/**
 * @brief Frees the MQTT broker connection settings.
*/
void FreeMqttBrokerSettings(ADUC_MQTT_SETTINGS* settings);

EXTERN_C_END

#endif // __ADUC_ADUC_MQTT_Client_CLIENT_H__
