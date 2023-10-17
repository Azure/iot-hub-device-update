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
#define DEFAULT_QOS 0
#define DEFAULT_CLEAN_SESSION true
#define DEFAULT_ADPS_CLEAN_SESSION false
#define DEFAULT_MQTT_BROKER_CLEAN_SESSION true
#define DEFAULT_MQTT_BROKER_PROTOCOL_VERSION 5
#define MIN_BROKER_MQTT_VERSION 4

/**
 * @brief Enumeration for specifying the source of the MQTT hostname in the configuration.
 *
 * This enumeration defines different sources for obtaining the MQTT broker hostname, which can be used
 * to establish an MQTT connection. The hostname source can be explicitly set in the configuration file
 * or come from other sources, such as DPS (Device Provisioning Service) settings.
 *
 * - `ADUC_MQTT_HOSTNAME_SOURCE_NONE`: No specific hostname source is provided.
 * - `ADUC_MQTT_HOSTNAME_SOURCE_DPS`: The hostname is obtained from the DPS settings.
 * - `ADUC_MQTT_HOSTNAME_SOURCE_CONFIG_FILE`: The hostname is sourced from the "du-config.json" configuration file.
 *
 * If the source type is not explicitly specified in the configuration, it defaults to `ADUC_MQTT_HOSTNAME_SOURCE_NONE`.
 *
 * @note When the source is DPS, the application or other modules must ensure that the correct 'hostname' value is set
 *       before attempting to create a connection to the MQTT broker.
 */
typedef enum ADUC_MQTT_HOSTNAME_SOURCE_TAG
{
    ADUC_MQTT_HOSTNAME_SOURCE_NONE = 0,
    ADUC_MQTT_HOSTNAME_SOURCE_DPS = 1,
    ADUC_MQTT_HOSTNAME_SOURCE_CONFIG_FILE = 2,
} ADUC_MQTT_HOSTNAME_SOURCE;

/**
 * @brief Structure for storing MQTT settings, including hostname and its source.
 *
 * This structure holds various settings required for configuring an MQTT connection. It includes parameters such as
 * certificate file paths, client ID, hostname, hostname source, key file path, username, keep-alive interval,
 * TCP port, clean session flag, TLS usage, Quality of Service (QoS), and MQTT protocol version.
 *
 * When using this structure, consider the source of the hostname, which can be set explicitly in the configuration
 * or obtained from other sources such as DPS settings. If the source is DPS, ensure that the correct 'hostname' value
 * is set externally before creating an MQTT connection.
 *
 * @note The `hostnameSource` field specifies the source of the MQTT hostname, and it can be one of the values defined
 *       in the `ADUC_MQTT_HOSTNAME_SOURCE` enumeration.
 */
typedef struct ADUC_MQTT_SETTINGS_TAG
{
    char* caFile; /*< Path to a PEM file with the chain required to trust the TLS endpoint certificate */
    char* certFile; /*< Path to a PEM file to establish X509 client authentication */
    char* clientId; /*< MQTT Client Id */
    char* hostname; /*< FQDN to the MQTT Broker endpoint, e.g., mybroker.mydomain.com */
    ADUC_MQTT_HOSTNAME_SOURCE hostnameSource; /*< Source of the hostname (see ADUC_MQTT_HOSTNAME_SOURCE) */
    char* keyFile; /*< Path to a KEY file to establish X509 client authentication */
    char* username; /*< MQTT Username to authenticate the connection */
    unsigned int keepAliveInSeconds; /*< Seconds to send the ping to keep the connection open */
    unsigned int tcpPort; /*< TCP port to access the endpoint, e.g., 8883 */
    bool
        cleanSession; /*< MQTT Clean Session, might require setting the ClientId (existing sessions not supported now) */
    bool useTLS; /*< Disable TLS negotiation (not recommended for production) */
    unsigned int qos; /*< MQTT QoS */
    int mqttVersion; /*< MQTT protocol version (3 = v3, 4 = v3.1.1, 5 = v5) */
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
