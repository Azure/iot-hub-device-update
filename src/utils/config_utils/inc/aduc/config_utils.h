/**
 * @file config_utils.h
 * @brief Header file for the Configuration Utility for reading, parsing the ADUC configuration file
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#ifdef ENABLE_MOCKS

#    undef ENABLE_MOCKS
#    include "azure_c_shared_utility/strings.h"
#    include "azure_c_shared_utility/vector.h"
#    define ENABLE_MOCKS

#else
#    include "azure_c_shared_utility/strings.h"
#    include "azure_c_shared_utility/vector.h"

#endif

#include <aduc/c_utils.h>
#include <parson.h>
#include <stdbool.h>
#include <umock_c/umock_c_prod.h>

#define ADUC_CONFIG_FOLDER_ENV "ADUC_CONF_FOLDER"

EXTERN_C_BEGIN

#define ADUC_CONNECTION_TYPE_AIS "AIS"
#define ADUC_CONNECTION_TYPE_MQTTBROKER "MQTTBroker"
#define ADUC_CONNECTION_TYPE_ADPS2_MQTT "ADPS2/MQTT"
#define ADUC_CONNECTION_TYPE_STRING "string"

/**
 * @brief ADUC_AgentInfo that stores all the agent information from configuration file.
 *
 * Fields:
 *  'name' : The user-defined friendly name of the agent.
 *  'runas' : The user name to run the agent as.
 *
 *  'connectionType' can be 'string', 'AIS' (Azure Identity Service), 'MQTTBroker' or 'ADPS2/MQTT' (Azure Device Provisioning Service v2 over MQTT).
 *
 *   For version 1.x, the supported connection types are 'AIS' and 'string'.
 *           If it is 'AIS', the 'connectionData' is the name of the principal in AIS.
 *           If it is 'string', the 'connectionData' is used as a string. For Device Update Agent V1, this is the connection string from the IoT Hub.
 *
 *   For version 2.x, the supported connection types are 'AIS', 'MQTTBroker', and 'ADPS2/MQTT'.
 *           If it is 'MQTTBroker', the 'connectionData' is not used. The 'connectionDataJson' is used instead. It contains
 *           the following fields:
 *             'hostName' : The fully qualified domain name of the MQTT broker.
 *             'tcpPort' : The port number of the MQTT broker.
 *             'useTLS' : Whether to use TLS for the MQTT connection.
 *             'cleanSession' : Whether to use a clean session for the MQTT connection.
 *             'keepAliveInSeconds' : The keep alive interval in seconds for the MQTT connection.
 *             'clientId' : The client ID for the MQTT connection.
 *             'userName' : The user name for the MQTT connection.
 *             'password' : [NOT RECOMMEND FOR PRODUCTION] The password for the MQTT connection.
 *             'caFile' : The path to the CA certificate file for the MQTT connection.
 *             'certFile' : The path to the client certificate file for the MQTT connection.
 *             'keyFile' : The path to the client key file for the MQTT connection.
 *
 *           If it is 'ADPS2/MQTT', the 'connectionData' is not used. The 'connectionDataJson' is used instead. It contains fields:
 *             'idScope': The ID scope of the device provisioning service.
 *             'registrationId': The registration ID of the device.
 *             'globalDeviceEndpoint': The global device endpoint of the device provisioning service.
 *
 *             'tcpPort' : The port number of the MQTT broker.
 *             'useTLS' : Whether to use TLS for the MQTT connection.
 *             'cleanSession' : Whether to use a clean session for the MQTT connection.
 *             'keepAliveInSeconds' : The keep alive interval in seconds for the MQTT connection.
 *             'clientId' : The client ID for the MQTT connection.
 *             'userName' : The user name for the MQTT connection.
 *             'password' : [NOT RECOMMENDED FOR PRODUCTION] The password for the MQTT connection.
 *             'caFile' : The path to the CA certificate file for the MQTT connection.
 *             'certFile' : The path to the client certificate file for the MQTT connection.
 *             'keyFile' : The path to the client key file for the MQTT connection.
 *
 *  'manufacturer' : The manufacturer of the device.
 *  'model' : The model of the device.
 *  'additionalDeviceProperties' : Additional device properties.
 *
 */
typedef struct tagADUC_AgentInfo
{
    char* name; /**< The name of the agent. */

    char* runas; /**< Run as a trusted user. */

    char* connectionType; /**< It can be 'AIS', 'MQTTBroker', 'ADPS2/MQTT', or 'string'. */

    char* connectionData; /**< the name in AIS principal (AIS); the connectionString (connectionType string). */

    JSON_Value* connectionDataJson; /**< The connection data as a JSON object. (for MQTTBroker and ADPS2/MQTT)*/

    char* manufacturer; /**< Device property manufacturer. */

    char* model; /**< Device property model. */

    JSON_Object* additionalDeviceProperties; /**< Additional device properties. */

} ADUC_AgentInfo;

/**
 * @brief  ADUC_ConfigInfo that stores all the configuration info from configuration file
 */

typedef struct tagADUC_ConfigInfo
{
    int refCount; /**< A reference count for this object. */

    JSON_Value* rootJsonValue; /**< The root value of the configuration. */

    const char* schemaVersion; /**< The version of the schema for the configuration. */

    JSON_Array* aduShellTrustedUsers; /**< All the trusted users for ADU shell. */

    const char* manufacturer; /**< Device info manufacturer. */

    const char* model; /**< Device info model. */

    const char* edgegatewayCertPath; /**< Edge gateway certificate path */

    ADUC_AgentInfo* agents; /**< Array of agents that are configured. */

    size_t agentCount; /**< Total number of agents configured. */

    const char* compatPropertyNames; /**< Compat property names. */

    const char* iotHubProtocol; /**< The IotHub transport protocol to use. */

    unsigned int
        downloadTimeoutInMinutes; /**< The timeout for downloading an update payload. A value of zero means to use the default. */

    const char* aduShellFolder; /**< The folder where ADU shell is installed. */

    char* aduShellFilePath; /**< The full path to ADU shell binary. */

    char* configFolder; /**< The folder where ADU stores its configuration. */

    const char* dataFolder; /**< The folder where ADU stores its data. */

    const char* downloadsFolder; /**< The folder where ADU stores downloaded payloads. */

    const char* extensionsFolder; /**< The folder where ADU stores its extensions. */

    char* extensionsComponentEnumeratorFolder; /**< The folder where ADU stores its component enumerator extensions. */

    char* extensionsContentDownloaderFolder; /**< The folder where ADU stores its content downloader extensions. */

    char* extensionsStepHandlerFolder; /**< The folder where ADU stores its step handler extensions. */

    char* extensionsDownloadHandlerFolder; /**< The folder where ADU stores its downloader handler extensions. */
} ADUC_ConfigInfo;

/**
 * @brief Create the ADUC_ConfigInfo object.
 *
 * @return const ADUC_ConfigInfo* a pointer to ADUC_ConfigInfo object. NULL if failure.
 * Caller must call ADUC_ConfigInfo_Release to free the object.
 */
const ADUC_ConfigInfo* ADUC_ConfigInfo_GetInstance();

/**
 * @brief Release the ADUC_ConfigInfo object.
 *
 * @param configInfo a pointer to ADUC_ConfigInfo object
 * @return int The reference count of the ADUC_ConfigInfo object after released.
 */
int ADUC_ConfigInfo_ReleaseInstance(const ADUC_ConfigInfo* configInfo);

/**
 * @brief Allocates the memory for the ADUC_ConfigInfo struct member values
 * @param config A pointer to an ADUC_ConfigInfo struct whose member values will be allocated
 * @param configFilePath The path of configuration file
 * @returns True if successfully allocated, False if failure
 */
bool ADUC_ConfigInfo_Init(ADUC_ConfigInfo* config, const char* configFilePath);

/**
 * @brief Free members of ADUC_ConfigInfo object.
 *
 * @param config Object to free.
 */
void ADUC_ConfigInfo_UnInit(ADUC_ConfigInfo* config);

/**
 * @brief Get the agent information of the desired index from the ADUC_ConfigInfo object
 *
 * @param config
 * @param index
 * @return const ADUC_AgentInfo*, NULL if failure
 */
const ADUC_AgentInfo* ADUC_ConfigInfo_GetAgent(const ADUC_ConfigInfo* config, unsigned int index);

/**
 * @brief Get the adu trusted user list
 *
 * @param config A pointer to a const ADUC_ConfigInfo struct
 * @return VECTOR_HANDLE
 */
VECTOR_HANDLE ADUC_ConfigInfo_GetAduShellTrustedUsers(const ADUC_ConfigInfo* config);

/**
 * @brief Free the VECTOR_HANDLE (adu shell truster users) and all the elements in it
 *
 * @param users Object to free. The vector (type VECTOR_HANDLE) containing users (type STRING_HANDLE)
 */
void ADUC_ConfigInfo_FreeAduShellTrustedUsers(VECTOR_HANDLE users);

/**
 * @brief Get DU Agent's connection data field of type string.
 *
 * @param agent Pointer to ADUC_AgentInfo object.
 * @param fieldName The field name to get. This can be null or a nested field name, e.g. "name" or "updateId.name".
 *                  If fieldName not null, this function will return a connectionDataJson
 * @param value Pointer to a char* to receive the value. Caller must free.
 * @return bool True if successful.
 */
bool ADUC_AgentInfo_ConnectionData_GetStringField(const ADUC_AgentInfo* agent, const char* fieldName, char** value);

/**
 * @brief Get DU Agent's connection data field of type boolean.
 *
 * @param agent Pointer to ADUC_AgentInfo object.
 * @param fieldName The field name to get. This can be a nested field name, e.g. "useSTL" or "options.keepAlive".
 * @param value Pointer to a bool to receive the value.
 * @return bool True if successful.
 */
bool ADUC_AgentInfo_ConnectionData_GetBooleanField(const ADUC_AgentInfo* agent, const char* fieldName, bool* value);

/**
 * @brief Get DU Agent's connection data field of type unsigned int.
 *
 * @param agent Pointer to ADUC_AgentInfo object.
 * @param fieldName The field name to get. This can be a nested field name, e.g. "port" or "options.maxRetry".
 * @param value Pointer to an unsigned int to receive the value.
 * @return bool True if successful.
 */
bool ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(
    const ADUC_AgentInfo* agent, const char* fieldName, unsigned int* value);

// clang-format off
// NOLINTNEXTLINE: clang-tidy doesn't like UMock macro expansions
MOCKABLE_FUNCTION(, JSON_Value*, Parse_JSON_File, const char*, configFilePath)
// clang-format on

EXTERN_C_END

#endif
