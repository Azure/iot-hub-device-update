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

typedef struct tagADUC_AgentInfo
{
    char* name; /**< The name of the agent. */

    char* runas; /**< Run as a trusted user. */

    char* connectionType; /**< It can be AIS, MQTTBroker, or string. */

    char* connectionData; /**< the name in AIS principal (AIS); the connectionString (connectionType string). */

    JSON_Value* connectionDataJson; /**< The connection data as a JSON object. (MQTTBroker)*/

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

    const char* schemaVersion;

    JSON_Array* aduShellTrustedUsers; /**< All the trusted users for ADU shell. */

    const char* manufacturer; /**< Device info manufacturer. */

    const char* model; /**< Device info model. */

    const char* edgegatewayCertPath; /**< Edge gateway certificate path */

    ADUC_AgentInfo* agents; /**< Array of agents that are configured. */

    unsigned int agentCount; /**< Total number of agents configured. */

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
 * @brief Get DU AGent's connection data field of type string.
 *
 * @param agent Pointer to ADUC_AgentInfo object.
 * @param fieldName The field name to get. This can be null or a nested field name, e.g. "name" or "updateId.name".
 *                  If fieldName not null, this function will return a connectionDataJson
 * @param value Pointer to a char* to receive the value. Caller must free.
 * @return bool True if successful.
 */
bool ADUC_AgentInfo_ConnectionData_GetStringField(const ADUC_AgentInfo* agent, const char* fieldName,  char** value);

/**
 * @brief Get DU AGent's connection data field of type boolean.
 *
 * @param agent Pointer to ADUC_AgentInfo object.
 * @param fieldName The field name to get. This can be a nested field name, e.g. "useSTL" or "options.keepAlive".
 * @param value Pointer to a bool to receive the value.
 * @return bool True if successful.
 */
bool ADUC_AgentInfo_ConnectionData_GetBooleanField(const ADUC_AgentInfo* agent, const char* fieldName, bool* value);

/**
 * @brief Get DU AGent's connection data field of type unsigned int.
 *
 * @param agent Pointer to ADUC_AgentInfo object.
 * @param fieldName The field name to get. This can be a nested field name, e.g. "port" or "options.maxRetry".
 * @param value Pointer to an unsigned int to receive the value.
 * @return bool True if successful.
 */
bool ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(const ADUC_AgentInfo* agent, const char* fieldName, unsigned int* value);

// clang-format off
// NOLINTNEXTLINE: clang-tidy doesn't like UMock macro expansions
MOCKABLE_FUNCTION(, JSON_Value*, Parse_JSON_File, const char*, configFilePath)
// clang-format on

EXTERN_C_END

#endif
