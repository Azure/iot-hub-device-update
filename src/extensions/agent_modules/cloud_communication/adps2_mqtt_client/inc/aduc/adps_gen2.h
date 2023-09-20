/**
 * @file adps_gen2.h
 * @brief Implements the Azure DPS Gen2 communication utility.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADPS_GEN2_H
#define ADPS_GEN2_H

#include "aduc/mqtt_client.h"
#include "du_agent_sdk/agent_common.h"
#include "stdbool.h"

EXTERN_C_BEGIN

/**
 * @brief Struct containing the connection settings for the Azure DPS .
 * This is a data structure that is used for unit testing only.
 * The actual MQTT broker connection settings are defined in src/extensions/agent_modules/communication_modules/inc.
 * This data structure is used to compare the values read from the config file with the expected values.
 */
typedef struct AZURE_DPS_2_MQTT_SETTINGS_TAG
{
    char* idScope; /*< Device Provisioning Service Id Scope */
    char* registrationId; /*< Device Provisioning Service Registration Id */
    char* dpsApiVersion; /*< Device Provisioning Service API Version */
    ADUC_MQTT_SETTINGS mqttSettings; /*< MQTT Settings */
} AZURE_DPS_2_MQTT_SETTINGS;

/**
 * @brief Read the Azure DPS Gen2 connection data from the config file.
 *        This follow MQTT protocol describe at https://learn.microsoft.com/azure/iot/iot-mqtt-connect-to-iot-dps
 * @param settings The Azure DPS Gen2 MQTT settings to populate. The caller is responsible for freeing the returned settings by calling FreeAzureDPS2MqttSettings function.
 * @return true if the settings were read successfully, false otherwise.
 */
bool ReadAzureDPS2MqttSettings(AZURE_DPS_2_MQTT_SETTINGS* settings);

/**
 * @brief Free resources allocated for the Azure DPS Gen2 MQTT settings.
 * @param settings The Azure DPS Gen2 MQTT settings to free.
 */
void FreeAzureDPS2MqttSettings(AZURE_DPS_2_MQTT_SETTINGS* settings);

EXTERN_C_END

#endif
