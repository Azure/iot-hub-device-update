/**
 * @file adps_gen2.c
 * @brief Implements the Azure DPS Gen2 communication utility.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/adps_gen2.h"
#include "aduc/config_utils.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h" // ADUC_StringFormat
#include "stdlib.h" // memset

#define DEFAULT_DPS_API_VERSION "2023-02-01-preview"
#define DEFAULT_DPS_GLOBAL_ENDPOINT "global.azure-devices-provisioning.net"
// Default MQTT protocol version for Azure DPS Gen2 is v3.1.1 (4)
#define DEFAULT_DPS_MQTT_PROTOCOL_VERSION 4
#define MIN_DPS_MQTT_VERSION 4

/**
 * @brief Read the Azure DPS Gen2 connection data from the config file.
 *        This follow MQTT protocol describe at https://learn.microsoft.com/azure/iot/iot-mqtt-connect-to-iot-dps
 * @param settings The Azure DPS Gen2 MQTT settings to populate. The caller is responsible for freeing the returned settings by calling FreeAzureDPS2MqttSettings function.
 * @return true if the settings were read successfully, false otherwise.
 */
bool ReadAzureDPS2MqttSettings(AZURE_DPS_2_MQTT_SETTINGS* settings)
{
    bool result = false;
    const ADUC_ConfigInfo* config = NULL;
    const ADUC_AgentInfo* agent_info = NULL;
    int tmp = -1;

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
    if (strcmp(agent_info->connectionType, ADUC_CONNECTION_TYPE_ADPS2_MQTT) != 0)
    {
        goto done;
    }

    // Read the x.506 certificate authentication data.
    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.caFile", &settings->mqttSettings.caFile))
    {
        settings->mqttSettings.caFile = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.certFile", &settings->mqttSettings.certFile))
    {
        settings->mqttSettings.certFile = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.keyFile", &settings->mqttSettings.keyFile))
    {
        settings->mqttSettings.keyFile = NULL;
    }

    // NOTE: If you use X.509 certificate authentication, the registration ID is provided by
    // the subject common name (CN) of the device leaf (end-entity) certificate.
    // {registration_id} in the Username field must match the common name.
    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.registrationId", &settings->registrationId))
    {
        // TODO (nox-msft) : read common name from certificate.
        Log_Error("get registrationId");
        goto done;
    }

    // For DPS connection, the clientId and registrationId fields are the same.
    if (mallocAndStrcpy_s(&settings->mqttSettings.clientId, settings->registrationId) != 0)
    {
        Log_Error("alloc clientId");
        goto done;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.idScope", &settings->idScope))
    {
        Log_Error("get idScope");
        goto done;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.apiVersion", &settings->dpsApiVersion))
    {
        Log_Info("default apiVersion: %s", DEFAULT_DPS_API_VERSION);
        goto done;
    }

    // 'username' field is generated from the idScope, clientId and dps.apiVersion fields.
    // Format: <idScope>/registrations/<registrationId>/api-version=<apiVersion>
    // NOTE: registrationId is the same as clientId and must match the common name in the certificate.
    settings->mqttSettings.username = ADUC_StringFormat(
        "%s/registrations/%s/api-version=%s", settings->idScope, settings->registrationId, settings->dpsApiVersion);

    if (settings->mqttSettings.username == NULL)
    {
        Log_Error("fail gen username(idScope:%s, registrationId:%s, apiVersion:%s)");
        goto done;
    }

    // NOTE: This is the 'globalDeviceEndpoint' field in the config file.
    if (!ADUC_AgentInfo_ConnectionData_GetStringField(
            agent_info, "dps.globalDeviceEndpoint", &settings->mqttSettings.hostname))
    {
        Log_Info("default hostname: %s", DEFAULT_DPS_GLOBAL_ENDPOINT);
        settings->mqttSettings.hostname = strdup(DEFAULT_DPS_GLOBAL_ENDPOINT);
        goto done;
    }

    // Common MQTT connection data fields.
    if (!ADUC_AgentInfo_ConnectionData_GetIntegerField(
            agent_info, "dps.mqttVersion", &tmp) || tmp < MIN_DPS_MQTT_VERSION)
    {
        Log_Info("default mqttVersion: %d", DEFAULT_DPS_MQTT_PROTOCOL_VERSION);
        settings->mqttSettings.mqttVersion = DEFAULT_DPS_MQTT_PROTOCOL_VERSION;
    }
    else
    {
        settings->mqttSettings.mqttVersion = tmp;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(
            agent_info, "dps.tcpPort", &settings->mqttSettings.tcpPort))
    {
        Log_Info("default tcpPort: %d", DEFAULT_TCP_PORT);
        settings->mqttSettings.tcpPort = DEFAULT_TCP_PORT;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetBooleanField(agent_info, "dps.useTLS", &settings->mqttSettings.useTLS))
    {
        Log_Info("default useTLS: %d", DEFAULT_USE_TLS);
        settings->mqttSettings.useTLS = DEFAULT_USE_TLS;
    }

    tmp = -1;
    if (!ADUC_AgentInfo_ConnectionData_GetIntegerField(agent_info, "dps.qos", &tmp) || tmp < 0 || tmp > 2)
    {
        Log_Info("default qos: %d", DEFAULT_QOS);
        settings->mqttSettings.qos = DEFAULT_QOS;
    }
    else
    {
        settings->mqttSettings.qos = tmp;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetBooleanField(
            agent_info, "dps.cleanSession", &settings->mqttSettings.cleanSession))
    {
        Log_Info("default CleanSession: %d", DEFAULT_ADPS_CLEAN_SESSION);
        settings->mqttSettings.cleanSession = DEFAULT_ADPS_CLEAN_SESSION;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(
            agent_info, "dps.keepAliveInSeconds", &settings->mqttSettings.keepAliveInSeconds))
    {
        Log_Info("default KeepAliveInSeconds: %d", DEFAULT_KEEP_ALIVE_IN_SECONDS);
        settings->mqttSettings.keepAliveInSeconds = DEFAULT_KEEP_ALIVE_IN_SECONDS;
    }

    result = true;

done:
    ADUC_ConfigInfo_ReleaseInstance(config);
    if (!result)
    {
        FreeAzureDPS2MqttSettings(settings);
    }
    return result;
}

/**
 * @brief Free resources allocated for the Azure DPS Gen2 MQTT settings.
 * @param settings The Azure DPS Gen2 MQTT settings to free.
 */
void FreeAzureDPS2MqttSettings(AZURE_DPS_2_MQTT_SETTINGS* settings)
{
    if (settings == NULL)
    {
        return;
    }

    free(settings->idScope);
    free(settings->registrationId);
    free(settings->dpsApiVersion);

    free(settings->mqttSettings.hostname);
    free(settings->mqttSettings.clientId);
    free(settings->mqttSettings.username);
    free(settings->mqttSettings.caFile);
    free(settings->mqttSettings.certFile);
    free(settings->mqttSettings.keyFile);
    memset(settings, 0, sizeof(*settings));
}
