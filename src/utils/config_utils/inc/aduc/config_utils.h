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

    char* connectionType; /**< It can be either AIS or string. */

    char* connectionData; /**< the name in AIS principal (AIS); or the connectionString (connectionType string). */

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

// clang-format off
// NOLINTNEXTLINE: clang-tidy doesn't like UMock macro expansions
MOCKABLE_FUNCTION(, JSON_Value*, Parse_JSON_File, const char*, configFilePath)
// clang-format on

EXTERN_C_END

#endif
