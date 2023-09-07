/**
 * @file config_utils_ut.cpp
 * @brief Unit Tests for config_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "umock_c/umock_c.h"

#include <catch2/catch.hpp>

#include "aduc/c_utils.h"
#include "aduc/calloc_wrapper.hpp"
#include <azure_c_shared_utility/crt_abstractions.h>
#include <azure_c_shared_utility/strings.h>
#include <parson.h>
#include <string.h>

#define ENABLE_MOCKS
#include "aduc/config_utils.h"
#undef ENABLE_MOCKS

using Catch::Matchers::Equals;

// clang-format off
static const char* validConfigContentStr =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("compatPropertyNames": "manufacturer,model",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box")"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* validConfigContentNoCompatPropertyNames =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box")"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* validConfigContentMqttIotHubProtocol =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("iotHubProtocol": "mqtt",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box")"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* validConfigContentMqttWebSocketsIotHubProtocol =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("iotHubProtocol": "mqtt/ws",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box")"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* validConfigContentMissingIotHubProtocol =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box")"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* invalidConfigContentNoDeviceInfoStr =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box")"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* invalidConfigContentNoDevicePropertiesStr =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("compatPropertyNames": "manufacturer,model",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(})"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(})"
            R"(})"
        R"(])"
    R"(})";

static const char* invalidConfigContentStrEmpty = R"({})";

static const char* invalidConfigContentStr =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("agents": [])"
    R"(})";

static const char* validConfigContentAdditionalPropertyNames =
    R"({)"
        R"("schemaVersion": "1.0",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box",)"
            R"("additionalDeviceProperties": {)"
                R"("location": "US",)"
                R"("language": "English")"
            R"(})"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* validConfigContentDownloadTimeout =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("downloadTimeoutInMinutes": 1440,)"
        R"("compatPropertyNames": "manufacturer,model",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box")"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* validConfigWithOverrideFolder =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("downloadTimeoutInMinutes": 1440,)"
        R"("aduShellFolder": "/usr/mybin",)"
        R"("dataFolder": "/var/lib/adu/mydata",)"
        R"("extensionsFolder": "/var/lib/adu/myextensions",)"
        R"("compatPropertyNames": "manufacturer,model",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box")"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* invalidConfigContentDownloadTimeout =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("downloadTimeoutInMinutes": -1,)"
        R"("compatPropertyNames": "manufacturer,model",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box")"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* testADSP2Config =
    R"({)"
        R"("schemaVersion": "2.0",)"
        R"("aduShellTrustedUsers": [)"
        R"("adu")"
        R"(],)"
        R"("manufacturer": "contoso",)"
        R"("model": "espresso-v1",)"
        R"("agents" : [{)"
            R"("name": "main",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
            R"("connectionType": "ADPS2/MQTT",)"
            R"("connectionData": {)"
              R"("dps" : {)"
                R"("idScope" : "0ne0123456789abcdef",)"
                R"("registrationId" : "adu-dps-client-unit-test-device",)"
                R"("apiVersion" : "2021-06-01",)"
                R"("globalDeviceEndpoint" : "global.azure-devices-provisioning.net",)"
                R"("tcpPort" : 8883,)"
                R"("useTLS": true,)"
                R"("cleanSession" : false,)"
                R"("keepAliveInSeconds" : 3600,)"
                R"("clientId" : "adu-dps-client-unit-test-device",)"
                R"("userName" : "adu-dps-client-unit-test-user-1",)"
                R"("password" : "adu-dps-client-unit-test-password-1",)"
                R"("caFile" : "adu-dps-client-unit-test-ca-1",)"
                R"("certFile" : "adu-dps-client-unit-test-cert-1.pem",)"
                R"("keyFile" : "adu-dps-client-unit-test-key-1.pem",)"
                R"("keyFilePassword" : "adu-dps-client-unit-test-key-password-1")"
              R"(})"
            R"(})"
            R"(},)"
            R"("manufacturer": "contoso",)"
            R"("model": "espresso-v1")"
        R"(}])"
    R"(})";

static const char* testMQTTBrokerConfig =
    R"({)"
        R"("schemaVersion": "2.0",)"
        R"("aduShellTrustedUsers": [)"
        R"("adu")"
        R"(],)"
        R"("manufacturer": "contoso",)"
        R"("model": "espresso-v1",)"
        R"("agents" : [{)"
            R"("name": "main",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
            R"("connectionType": "MQTTBroker",)"
            R"("connectionData": {)"
              R"("mqttBroker" : {)"
                R"("hostName" : "contoso-red-devbox-wus3-eg.westus2-1.ts.eventgrid.azure.net",)"
                R"("tcpPort" : 8883,)"
                R"("useTLS": true,)"
                R"("cleanSession" : true,)"
                R"("keepAliveInSeconds" : 3600,)"
                R"("clientId" : "adu-mqtt-client-unit-test-device",)"
                R"("userName" : "adu-mqtt-client-unit-test-user-1",)"
                R"("password" : "adu-mqtt-client-unit-test-password-1",)"
                R"("caFile" : "adu-mqtt-client-unit-test-ca-1",)"
                R"("certFile" : "adu-mqtt-client-unit-test-cert-1.pem",)"
                R"("keyFile" : "adu-mqtt-client-unit-test-key-1.pem",)"
                R"("keyFilePassword" : "adu-mqtt-client-unit-test-key-password-1")"
              R"(})"
            R"(})"
            R"(},)"
            R"("manufacturer": "contoso",)"
            R"("model": "espresso-v1")"
        R"(}])"
    R"(})";

static char* g_configContentString = nullptr;

/**
 * @brief Mock for json_parse_file. This function will return a JSON_Value* based on the value of g_configContentString, if specified.
 * Otherwise, it fallback to the default implementation.
 *
 * @param configFilePath A path to a file containing JSON data.
 *
 * @return JSON_Value* A JSON_Value containing the parsed data.
 */
static JSON_Value* MockParse_JSON_File(const char* configFilePath)
{
    UNREFERENCED_PARAMETER(configFilePath);
    return json_parse_string(g_configContentString);
}

/**
 * @brief Struct containing the connection settings for the Azure DPS .
 * This is a data structure that is used for unit testing only.
 * The actual MQTT broker connection settings are defined in src/extensions/agent_modules/communication_modules/inc.
 * This data structure is used to compare the values read from the config file with the expected values.
 */
typedef struct _mock_adps_settings
{
  char* idScope;           /*< Device Provisioning Service Id Scope */
  char* registrationId;    /*< Device Provisioning Service Registration Id */
  char* dpsApiVersion;     /*< Device Provisioning Service API Version */
  char* caFile;            /*< Path to a PEM file with the chain required to trust the TLS endpoint certificate */
  char* certFile;          /*< Path to a PEM file to establish X509 client authentication */
  char* clientId;          /*< MQTT Client Id */
  char* hostname;          /*< FQDM to the MQTT Broker endpoint, eg: mybroker.mydomain.com */
  char* keyFile;           /*< Path to a KEY file to establish X509 client authentication */
  char* keyFilePassword;   /*< Password (aka pass-phrase) to protect the key file */
  char* password;          /*< MQTT Password to authenticate the connection */
  char* username;          /*< MQTT Username to authenticate the connection */
  unsigned int keepAliveInSeconds; /*< Seconds to send the ping to keep the connection open */
  unsigned int tcpPort;    /*< TCP port to access the endpoint eg: 8883 */
  bool cleanSession;       /*< MQTT Clean Session, might require to set the ClientId [existing sessions are not supported now] */
  bool useTLS;             /*< Disable TLS negotiation (not recommended for production)*/
} Mock_ADPS_Settings;

#define DEFAULT_TCP_PORT 8883
#define DEFAULT_KEEP_ALIVE_IN_SECONDS 30
#define DEFAULT_USE_TLS true
#define DEFAULT_CLEAN_SESSION false
#define DEFAULT_MQTTBROKER_CLEAN_SESSION true

static void free_mock_adps_settings(Mock_ADPS_Settings* settings)
{
    if (settings == NULL)
    {
        return;
    }

    free(settings->idScope);
    free(settings->registrationId);
    free(settings->dpsApiVersion);

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

static bool _read_mock_adps_settings(Mock_ADPS_Settings* settings)
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
    if (config == NULL) {
        goto done;
    }

    agent_info = ADUC_ConfigInfo_GetAgent(config, 0);

    // Currently only support 'ADPS2/MQTT' connection data.
    if (strcmp(agent_info->connectionType, ADUC_CONNECTION_TYPE_ADPS2_MQTT) != 0)
    {
        goto done;
    }

    // NOTE: This is the 'globalDeviceEndpoint' field in the config file.
    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.globalDeviceEndpoint", &settings->hostname))
    {
        goto done;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.idScope", &settings->idScope))
    {
        goto done;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.apiVersion", &settings->dpsApiVersion))
    {
        goto done;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.registrationId", &settings->registrationId))
    {
        goto done;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(agent_info, "dps.tcpPort", &settings->tcpPort))
    {
        settings->tcpPort = DEFAULT_TCP_PORT;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetBooleanField(agent_info, "dps.useTLS", &settings->useTLS))
    {
        settings->useTLS = DEFAULT_USE_TLS;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetBooleanField(agent_info, "dps.cleanSession", &settings->cleanSession))
    {
        settings->cleanSession = DEFAULT_CLEAN_SESSION;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(agent_info, "dps.keepAliveInSeconds", &settings->keepAliveInSeconds))
    {
        settings->keepAliveInSeconds = DEFAULT_KEEP_ALIVE_IN_SECONDS;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.clientId", &settings->clientId))
    {
        settings->clientId = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.userName", &settings->username))
    {
        settings->username = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.password", &settings->password))
    {
        settings->password = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.caFile", &settings->caFile))
    {
        settings->caFile = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.certFile", &settings->certFile))
    {
        settings->certFile = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.keyFile", &settings->keyFile))
    {
        settings->keyFile = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "dps.keyFilePassword", &settings->keyFilePassword))
    {
        settings->keyFilePassword = NULL;
    }

    result = true;

done:
    ADUC_ConfigInfo_ReleaseInstance(config);
    if (!result)
    {
        free_mock_adps_settings(settings);
    }
    return result;
}

/**
 * @brief Struct containing the connection settings for the MQTT client.
 * This is a data structure that is used for unit testing only.
 * The actual MQTT broker connection settings are defined in src/extensions/agent_modules/communication_modules/inc.
 * This data structure is used to compare the values read from the config file with the expected values.
 */
typedef struct _mock_mqtt_broker_settings
{
  char* caFile;            /*< Path to a PEM file with the chain required to trust the TLS endpoint certificate */
  char* certFile;          /*< Path to a PEM file to establish X509 client authentication */
  char* clientId;          /*< MQTT Client Id */
  char* hostname;          /*< FQDM to the MQTT Broker endpoint, eg: mybroker.mydomain.com */
  char* keyFile;           /*< Path to a KEY file to establish X509 client authentication */
  char* keyFilePassword;   /*< Password (aka pass-phrase) to protect the key file */
  char* password;          /*< MQTT Password to authenticate the connection */
  char* username;          /*< MQTT Username to authenticate the connection */
  unsigned int keepAliveInSeconds; /*< Seconds to send the ping to keep the connection open */
  unsigned int tcpPort;    /*< TCP port to access the endpoint eg: 8883 */
  bool cleanSession;       /*< MQTT Clean Session, might require to set the ClientId [existing sessions are not supported now] */
  bool useTLS;             /*< Disable TLS negotiation (not recommended for production)*/
} Mock_MQTT_Broker_Settings;

static void free_mock_mqtt_broker_settings(Mock_MQTT_Broker_Settings* settings)
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

static bool _read_mock_mqtt_broker_settings(Mock_MQTT_Broker_Settings* settings)
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
    if (config == NULL) {
        goto done;
    }

    agent_info = ADUC_ConfigInfo_GetAgent(config, 0);

    // Currently only support "MQTTBroker" connection data. (DPS Gen2 support will be available after 2023/Q4)
    if (strcmp(agent_info->connectionType, ADUC_CONNECTION_TYPE_MQTTBROKER) != 0)
    {
        goto done;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.hostName", &settings->hostname))
    {
        goto done;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(agent_info, "mqttBroker.tcpPort", &settings->tcpPort))
    {
        settings->tcpPort = DEFAULT_TCP_PORT;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetBooleanField(agent_info, "mqttBroker.useTLS", &settings->useTLS))
    {
        settings->useTLS = DEFAULT_USE_TLS;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetBooleanField(agent_info, "mqttBroker.cleanSession", &settings->cleanSession))
    {
        settings->cleanSession = DEFAULT_MQTTBROKER_CLEAN_SESSION;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(agent_info, "mqttBroker.keepAliveInSeconds", &settings->keepAliveInSeconds))
    {
        settings->keepAliveInSeconds = DEFAULT_KEEP_ALIVE_IN_SECONDS;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.clientId", &settings->clientId))
    {
        settings->clientId = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.userName", &settings->username))
    {
        settings->username = NULL;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.password", &settings->password))
    {
        settings->password = NULL;
    }

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

    if (!ADUC_AgentInfo_ConnectionData_GetStringField(agent_info, "mqttBroker.keyFilePassword", &settings->keyFilePassword))
    {
        settings->keyFilePassword = NULL;
    }

    result = true;

done:
    ADUC_ConfigInfo_ReleaseInstance(config);
    if (!result)
    {
        free_mock_mqtt_broker_settings(settings);
    }
    return result;
}

class GlobalMockHookTestCaseFixture
{
public:
    GlobalMockHookTestCaseFixture()
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast): clang-tidy can't handle the way that UMock expands macro's so we're suppressing
        REGISTER_GLOBAL_MOCK_HOOK(Parse_JSON_File, MockParse_JSON_File);

    }

    ~GlobalMockHookTestCaseFixture() = default;

    GlobalMockHookTestCaseFixture(const GlobalMockHookTestCaseFixture&) = delete;
    GlobalMockHookTestCaseFixture& operator=(const GlobalMockHookTestCaseFixture&) = delete;
    GlobalMockHookTestCaseFixture(GlobalMockHookTestCaseFixture&&) = delete;
    GlobalMockHookTestCaseFixture& operator=(GlobalMockHookTestCaseFixture&&) = delete;
};

TEST_CASE_METHOD(GlobalMockHookTestCaseFixture, "ADUC_ConfigInfo_Init Functional Tests")
{
    SECTION("Valid config content, Success Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentStr) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        CHECK_THAT(json_array_get_string(config.aduShellTrustedUsers, 0), Equals("adu"));
        CHECK_THAT(json_array_get_string(config.aduShellTrustedUsers, 1), Equals("do"));
        CHECK_THAT(config.schemaVersion, Equals("1.1"));
        CHECK_THAT(config.manufacturer, Equals("device_info_manufacturer"));
        CHECK_THAT(config.model, Equals("device_info_model"));
        CHECK_THAT(config.compatPropertyNames, Equals("manufacturer,model"));
        CHECK(config.agentCount == 2);
        const ADUC_AgentInfo* first_agent_info = ADUC_ConfigInfo_GetAgent(&config, 0);
        CHECK_THAT(first_agent_info->name, Equals("host-update"));
        CHECK_THAT(first_agent_info->runas, Equals("adu"));
        CHECK_THAT(first_agent_info->manufacturer, Equals("Contoso"));
        CHECK_THAT(first_agent_info->model, Equals("Smart-Box"));
        CHECK_THAT(first_agent_info->connectionType, Equals("AIS"));
        CHECK_THAT(first_agent_info->connectionData, Equals("iotHubDeviceUpdate"));
        const ADUC_AgentInfo* second_agent_info = ADUC_ConfigInfo_GetAgent(&config, 1);
        CHECK_THAT(second_agent_info->name, Equals("leaf-update"));
        CHECK_THAT(second_agent_info->runas, Equals("adu"));
        CHECK_THAT(second_agent_info->manufacturer, Equals("Fabrikam"));
        CHECK_THAT(second_agent_info->model, Equals("Camera"));
        CHECK_THAT(second_agent_info->connectionType, Equals("string"));
        CHECK_THAT(second_agent_info->connectionData, Equals("HOSTNAME=..."));
        CHECK(first_agent_info->additionalDeviceProperties == nullptr);

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content without compatPropertyNames, Success Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentNoCompatPropertyNames) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        CHECK(config.compatPropertyNames == nullptr);

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Config content with customized additional device properties, Successful Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentAdditionalPropertyNames) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        const ADUC_AgentInfo* first_agent_info = ADUC_ConfigInfo_GetAgent(&config, 0);
        CHECK(first_agent_info->additionalDeviceProperties != nullptr);
        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content without device info, Failure Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, invalidConfigContentNoDeviceInfoStr) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK_FALSE(ADUC_ConfigInfo_Init(&config, "/etc/adu"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content without device properties, Failure Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, invalidConfigContentNoDevicePropertiesStr) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK_FALSE(ADUC_ConfigInfo_Init(&config, "/etc/adu"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Empty config content, Failure Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, invalidConfigContentStrEmpty) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK_FALSE(ADUC_ConfigInfo_Init(&config, "/etc/adu"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Invalid config content, Failure Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, invalidConfigContentStr) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK_FALSE(ADUC_ConfigInfo_Init(&config, "/etc/adu"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content, downloadTimeoutInMinutes")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentDownloadTimeout) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        CHECK(config.downloadTimeoutInMinutes == 1440);

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Invalid config content, downloadTimeoutInMinutes")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, invalidConfigContentDownloadTimeout) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        CHECK(config.downloadTimeoutInMinutes == 0);
        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content, mqtt iotHubProtocol")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentMqttIotHubProtocol) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        CHECK_THAT(config.iotHubProtocol, Equals("mqtt"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content, mqtt/ws iotHubProtocol")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentMqttWebSocketsIotHubProtocol) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        CHECK_THAT(config.iotHubProtocol, Equals("mqtt/ws"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content, missing iotHubProtocol defaults to mqtt.")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentMissingIotHubProtocol) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        CHECK(config.iotHubProtocol == nullptr);

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Refcount Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentDownloadTimeout) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };
        const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
        CHECK(config != NULL);
        CHECK(config->refCount == 1);

        const ADUC_ConfigInfo* config2 = ADUC_ConfigInfo_GetInstance();
        CHECK(config2 != NULL);
        CHECK(config2->refCount == 2);

        ADUC_ConfigInfo_ReleaseInstance(config2);
        CHECK(config2->refCount == 1);
        CHECK(config->refCount == 1);

        ADUC_ConfigInfo_ReleaseInstance(config);
        CHECK(config->refCount == 0);
    }

    SECTION("User folders from build configs")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentDownloadTimeout) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };
        const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
        CHECK(config != NULL);
        CHECK_THAT(config->aduShellFolder, Equals("/usr/bin"));

#if defined(WIN32)
        CHECK_THAT(config->aduShellFilePath, Equals("/usr/bin/adu-shell.exe"));
#else
        CHECK_THAT(config->aduShellFilePath, Equals("/usr/bin/adu-shell"));
#endif
        CHECK_THAT(config->dataFolder, Equals("/var/lib/adu"));
        CHECK_THAT(config->extensionsFolder, Equals("/var/lib/adu/extensions"));
        CHECK_THAT(config->extensionsComponentEnumeratorFolder, Equals("/var/lib/adu/extensions/component_enumerator"));
        CHECK_THAT(config->extensionsContentDownloaderFolder, Equals("/var/lib/adu/extensions/content_downloader"));
        CHECK_THAT(config->extensionsStepHandlerFolder, Equals("/var/lib/adu/extensions/update_content_handlers"));
        CHECK_THAT(config->extensionsDownloadHandlerFolder, Equals("/var/lib/adu/extensions/download_handlers"));
        CHECK_THAT(config->downloadsFolder, Equals("/var/lib/adu/downloads"));
        ADUC_ConfigInfo_ReleaseInstance(config);
        CHECK(config->refCount == 0);
    }

    SECTION("User folders from du-config file")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigWithOverrideFolder) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };
        const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
        CHECK_THAT(config->aduShellFolder, Equals("/usr/mybin"));

#if defined(WIN32)
        CHECK_THAT(config->aduShellFilePath, Equals("/usr/mybin/adu-shell.exe"));
#else
        CHECK_THAT(config->aduShellFilePath, Equals("/usr/mybin/adu-shell"));
#endif

        CHECK_THAT(config->dataFolder, Equals("/var/lib/adu/mydata"));
        CHECK_THAT(config->extensionsFolder, Equals("/var/lib/adu/myextensions"));
        CHECK_THAT(config->extensionsComponentEnumeratorFolder, Equals("/var/lib/adu/myextensions/component_enumerator"));
        CHECK_THAT(config->extensionsContentDownloaderFolder, Equals("/var/lib/adu/myextensions/content_downloader"));
        CHECK_THAT(config->extensionsStepHandlerFolder, Equals("/var/lib/adu/myextensions/update_content_handlers"));
        CHECK_THAT(config->extensionsDownloadHandlerFolder, Equals("/var/lib/adu/myextensions/download_handlers"));
        CHECK_THAT(config->downloadsFolder, Equals("/var/lib/adu/mydata/downloads"));
        ADUC_ConfigInfo_ReleaseInstance(config);
        CHECK(config->refCount == 0);
    }

    SECTION("ADPS2/MQTT connection config test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, testADSP2Config) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        Mock_ADPS_Settings settings;

        bool success = _read_mock_adps_settings(&settings);
        CHECK(success);

        CHECK_THAT(settings.hostname, Equals("global.azure-devices-provisioning.net"));
        CHECK_THAT(settings.clientId, Equals("adu-dps-client-unit-test-device"));
        CHECK_THAT(settings.registrationId, Equals("adu-dps-client-unit-test-device"));
        CHECK_THAT(settings.idScope, Equals("0ne0123456789abcdef"));
        CHECK_THAT(settings.dpsApiVersion, Equals("2021-06-01"));
        CHECK_THAT(settings.username, Equals("adu-dps-client-unit-test-user-1"));
        CHECK_THAT(settings.password, Equals("adu-dps-client-unit-test-password-1"));
        CHECK_THAT(settings.caFile, Equals("adu-dps-client-unit-test-ca-1"));
        CHECK_THAT(settings.certFile, Equals("adu-dps-client-unit-test-cert-1.pem"));
        CHECK_THAT(settings.keyFile, Equals("adu-dps-client-unit-test-key-1.pem"));
        CHECK_THAT(settings.keyFilePassword, Equals("adu-dps-client-unit-test-key-password-1"));
        CHECK(settings.keepAliveInSeconds == 3600);
        CHECK(settings.tcpPort == 8883);
        CHECK(settings.cleanSession == false);
        CHECK(settings.useTLS == true);

        free_mock_adps_settings(&settings);
    }

    SECTION("MQTTBroker connection config test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, testMQTTBrokerConfig) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        Mock_MQTT_Broker_Settings settings;

        bool success = _read_mock_mqtt_broker_settings(&settings);
        CHECK(success);

        CHECK_THAT(settings.hostname, Equals("contoso-red-devbox-wus3-eg.westus2-1.ts.eventgrid.azure.net"));
        CHECK_THAT(settings.clientId, Equals("adu-mqtt-client-unit-test-device"));
        CHECK_THAT(settings.username, Equals("adu-mqtt-client-unit-test-user-1"));
        CHECK_THAT(settings.password, Equals("adu-mqtt-client-unit-test-password-1"));
        CHECK_THAT(settings.caFile, Equals("adu-mqtt-client-unit-test-ca-1"));
        CHECK_THAT(settings.certFile, Equals("adu-mqtt-client-unit-test-cert-1.pem"));
        CHECK_THAT(settings.keyFile, Equals("adu-mqtt-client-unit-test-key-1.pem"));
        CHECK_THAT(settings.keyFilePassword, Equals("adu-mqtt-client-unit-test-key-password-1"));
        CHECK(settings.keepAliveInSeconds == 3600);
        CHECK(settings.tcpPort == 8883);
        CHECK(settings.cleanSession == true);
        CHECK(settings.useTLS == true);

        free_mock_mqtt_broker_settings(&settings);
    }
}
