/**
 * @file mqtt_client.h
 * @brief Header file for the MQTT client.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef __ADUC_MQTT_CLIENT_H__
#define __ADUC_MQTT_CLIENT_H__

#include "du_agent_sdk/agent_common.h"
#include "stdbool.h"

EXTERN_C_BEGIN

#define DEFAULT_TCP_PORT 8883
#define DEFAULT_KEEP_ALIVE_IN_SECONDS 30
#define DEFAULT_USE_TLS true
#define DEFAULT_ADPS_CLEAN_SESSION false
#define DEFAULT_MQTT_BROKER_CLEAN_SESSION true
#define DEFAULT_MQTT_BROKER_PROTOCOL_VERSION 5

/**
 * @brief Struct containing the connection settings for the MQTT client.
 */
typedef struct ADUC_MQTT_SETTINGS_TAG
{
    char* caFile; /*< Path to a PEM file with the chain required to trust the TLS endpoint certificate */
    char* certFile; /*< Path to a PEM file to establish X509 client authentication */
    char* clientId; /*< MQTT Client Id */
    char* hostname; /*< FQDM to the MQTT Broker endpoint, eg: mybroker.mydomain.com */
    char* keyFile; /*< Path to a KEY file to establish X509 client authentication */
    char* keyFilePassword; /*< Password (aka pass-phrase) to protect the key file */
    char* username; /*< MQTT Username to authenticate the connection */
    char* password; /*< MQTT Password to authenticate the connection */
    unsigned int keepAliveInSeconds; /*< Seconds to send the ping to keep the connection open */
    unsigned int tcpPort; /*< TCP port to access the endpoint eg: 8883 */
    bool
        cleanSession; /*< MQTT Clean Session, might require to set the ClientId [existing sessions are not supported now] */
    bool useTLS; /*< Disable TLS negotiation (not recommended for production)*/
    unsigned int mqttVersion; /*< MQTT protocol version. 3 = v3, 4 = v3.1.1, 5 = v5 */
} ADUC_MQTT_SETTINGS;

/**
 * @brief Struct containing the connection settings for the Azure DPS .
 * This is a data structure that is used for unit testing only.
 * The actual MQTT broker connection settings are defined in src/extensions/agent_modules/communication_modules/inc.
 * This data structure is used to compare the values read from the config file with the expected values.
 */
typedef struct AZURE_DPS_MQTT_SETTINGS_TAG
{
    char* idScope; /*< Device Provisioning Service Id Scope */
    char* registrationId; /*< Device Provisioning Service Registration Id */
    char* dpsApiVersion; /*< Device Provisioning Service API Version */
    ADUC_MQTT_SETTINGS mqttSettings; /*< MQTT Settings */
} AZURE_DPS_MQTT_SETTINGS;

EXTERN_C_END

#endif // __ADUC_MQTT_CLIENT_H__
