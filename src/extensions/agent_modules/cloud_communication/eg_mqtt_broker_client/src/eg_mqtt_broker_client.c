/**
 * @file adps_gen2.c
 * @brief Implements the Azure DPS Gen2 communication utility.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/mqtt_client.h"
#include "aduc/ADUC_MQTT_Client_client.h"
#include "aduc/config_utils.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h" // ADUC_StringFormat
#include "stdlib.h" // memset

// Default MQTT protocol version for Event Grid MQTT Broker is v5 (5)
#define DEFAULT_EG_MQTT_PROTOCOL_VERSION 5

/**
 * @brief Read the MQTT broker connection data from the config file.
 *
 * @remark The caller is responsible for freeing the returned settings by calling FreeMqttBrokerSettings function.
*/
bool ReadMqttBrokerSettings(ADUC_MQTT_SETTINGS* settings)
{
    bool result = false;
    const ADUC_ConfigInfo* config = NULL;
    const ADUC_AgentInfo* agent_info = NULL;

    if (settings == NULL)
    {
        goto done;
    }

    memset(settings, 0, sizeof(*settings));

    config = ADUC_ConfigInfo_GetInstance();
    if (config == NULL)
    {
        goto done;
    }

    agent_info = ADUC_ConfigInfo_GetAgent(config, 0);

    // Currently only support 'ADPS2/MQTT' connection data.
    if (strcmp(agent_info->connectionType, ADUC_CONNECTION_TYPE_MQTTBROKER) != 0)
    {
        goto done;
    }

    // Read the x.506 certificate authentication data.
    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.caFile", &settings->caFile))
    {
        settings->caFile = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.certFile", &settings->certFile))
    {
        settings->certFile = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.keyFile", &settings->keyFile))
    {
        settings->keyFile = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(
            agent_info, "mqttBroker.keyFilePassword", &settings->keyFilePassword))
    {
        settings->keyFilePassword = NULL;
    }


    // TODO (nox-msft) - For MQTT broker, userName must match the device Id (usually the CN of the device's certificate)
    if (IsNullOrEmpty(settings->username))
    {
        if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.username", &settings->username))
        {
            settings->username = NULL;
            Log_Error("Username field is not specified");
            goto done;
        }
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.password", &settings->password))
    {
        settings->password = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(
            agent_info, "mqttBroker.hostname", &settings->hostname))
    {
        Log_Info("MQTT Broker hostname is not specified");
    }

    // Common MQTT connection data fields.
    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(
            agent_info, "mqttBroker.mqttVersion", &settings->mqttVersion))
    {
        Log_Info("Using default MQTT protocol version: %d", DEFAULT_MQTT_BROKER_PROTOCOL_VERSION);
        settings->mqttVersion = DEFAULT_MQTT_BROKER_PROTOCOL_VERSION;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(
            agent_info, "mqttBroker.tcpPort", &settings->tcpPort))
    {
        Log_Info("Using default TCP port: %d", DEFAULT_TCP_PORT);
        settings->tcpPort = DEFAULT_TCP_PORT;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetBooleanField(agent_info, "mqttBroker.useTLS", &settings->useTLS))
    {
        Log_Info("UseTLS: %d", DEFAULT_USE_TLS);
        settings->useTLS = DEFAULT_USE_TLS;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetBooleanField(
            agent_info, "mqttBroker.cleanSession", &settings->cleanSession))
    {
        Log_Info("CleanSession: %d", DEFAULT_ADPS_CLEAN_SESSION);
        settings->cleanSession = DEFAULT_ADPS_CLEAN_SESSION;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(
            agent_info, "mqttBroker.keepAliveInSeconds", &settings->keepAliveInSeconds))
    {
        Log_Info("Keep alive: %d sec.", DEFAULT_KEEP_ALIVE_IN_SECONDS);
        settings->keepAliveInSeconds = DEFAULT_KEEP_ALIVE_IN_SECONDS;
    }

    result = true;

done:
    ADUC_ConfigInfo_ReleaseInstance(config);
    if (!result)
    {
        FreeMqttBrokerSettings(settings);
    }
    return result;
}

void FreeMqttBrokerSettings(ADUC_MQTT_SETTINGS* settings)
{
    if (settings == NULL)
    {
        return;
    }

    free(settings->hostname);
    free(settings->clientId);
    free(settings->username);
    free(settings->password);
    free(settings->caFile);
    free(settings->certFile);
    free(settings->keyFile);
    free(settings->keyFilePassword);
    memset(settings, 0, sizeof(*settings));
}
