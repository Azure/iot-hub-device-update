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

#   undef ENABLE_MOCKS
#   include "azure_c_shared_utility/strings.h"
#   include "azure_c_shared_utility/vector.h"
#   define ENABLE_MOCKS

#else
#   include "azure_c_shared_utility/strings.h"
#   include "azure_c_shared_utility/vector.h"

#endif

#include <aduc/c_utils.h>
#include <parson.h>
#include <stdbool.h>
#include <umock_c/umock_c_prod.h>

EXTERN_C_BEGIN

typedef struct tagADUC_AgentInfo
{
    char* name; /**< The name of the agent. */

    char* runas; /**< Run as a trusted user. */

    char* connectionType; /**< It can be either AIS or string. */

    char* connectionData; /**< the name in AIS principal (AIS); or the connectionString (conncetionType string). */

    char* manufacturer; /**< Device property manufacturer. */

    char* model; /**< Device property model. */

} ADUC_AgentInfo;

/**
 * @brief  ADUC_ConfigInfo that stores all the configuration info from configuration file
 */

typedef struct tagADUC_ConfigInfo
{
    char* schemaVersion;

    JSON_Array* aduShellTrustedUsers; /**< All the trusted users for ADU shell. */

    char* manufacturer; /**< Device info manufacturer. */

    char* model; /**< Device info model. */

    bool simulateUnhealthyState; /**< A configuration for simulator. */

    char* edgegatewayCertPath; /**< Edge gateway certificate path */

    ADUC_AgentInfo* agents; /**< Array of agents that are configured. */

    unsigned int agentCount; /**< Total number of agents configured. */

    char* compatPropertyNames; /**< Compat property names. */
} ADUC_ConfigInfo;

/**
 * @brief Allocates the memory for the ADUC_ConfigInfo struct member values
 * @param config A pointer to an ADUC_ConfigInfo struct whose member values will be allocated
 * @param configFilePath The path of configuration file
 * @returns True if successfully allocated, False if failure
 */
_Bool ADUC_ConfigInfo_Init(ADUC_ConfigInfo* config, const char* configFilePath);

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
const ADUC_AgentInfo* ADUC_ConfigInfo_GetAgent(ADUC_ConfigInfo* config, unsigned int index);

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
