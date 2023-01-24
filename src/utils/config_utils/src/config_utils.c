/**
 * @file config_utils.c
 * @brief Implements the Configuration Utility for reading, parsing the ADUC configuration file
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/config_utils.h"

#include <aduc/c_utils.h>
#include <aduc/logging.h>
#include <aduc/string_c_utils.h>
#include <azure_c_shared_utility/crt_abstractions.h>
#include <azure_c_shared_utility/strings_types.h>
#include <parson.h>
#include <parson_json_utils.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void ADUC_AgentInfo_Free(ADUC_AgentInfo* agent);

/**
 * @brief Initializes an ADUC_AgentInfo object
 * @param agent the agent to be initialized
 * @param name name for @p agent
 * @param runas runas(the user the agent runs as) for @p agent
 * @param connectionType connectionType for @p agent
 * @param connectionData connectionData for @p agent
 * @param manufacturer manufacturer for @p agent
 * @param model connectionData for @p agent
 * @returns True on success and false on failure
 */
static bool ADUC_AgentInfo_Init(ADUC_AgentInfo* agent, const JSON_Object* agent_obj)
{
    bool success = false;

    if (agent == NULL || agent_obj == NULL)
    {
        return false;
    }

    memset(agent, 0, sizeof(*agent));

    const char* name = json_object_get_string(agent_obj, "name");
    const char* runas = json_object_get_string(agent_obj, "runas");
    const char* manufacturer = json_object_get_string(agent_obj, "manufacturer");
    const char* model = json_object_get_string(agent_obj, "model");

    JSON_Object* connection_source = json_object_get_object(agent_obj, "connectionSource");
    const char* connection_type = NULL;
    const char* connection_data = NULL;

    if (connection_source == NULL)
    {
        return false;
    }

    connection_type = json_object_get_string(connection_source, "connectionType");
    connection_data = json_object_get_string(connection_source, "connectionData");

    // As these fields are mandatory, if any of the fields doesn't exist, the agent will fail to be constructed.
    if (name == NULL || runas == NULL || connection_type == NULL || connection_data == NULL || manufacturer == NULL
        || model == NULL)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(agent->name), name) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(agent->runas), runas) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(agent->connectionType), connection_type) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(agent->connectionData), connection_data) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(agent->manufacturer), manufacturer) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(agent->model), model) != 0)
    {
        goto done;
    }

    agent->additionalDeviceProperties = json_object_get_object(agent_obj, "additionalDeviceProperties");

    success = true;
done:

    if (!success)
    {
        ADUC_AgentInfo_Free(agent);
    }
    return success;
}

/**
 * @brief Free ADUC_AgentInfo object.
 *
 * Caller should assume agent object is invalid after this method returns.
 *
 * @param agent ADUC_AgentInfo object to free.
 */
static void ADUC_AgentInfo_Free(ADUC_AgentInfo* agent)
{
    free(agent->name);
    agent->name = NULL;

    free(agent->runas);
    agent->runas = NULL;

    free(agent->connectionType);
    agent->connectionType = NULL;

    free(agent->connectionData);
    agent->connectionData = NULL;

    free(agent->manufacturer);
    agent->manufacturer = NULL;

    free(agent->model);
    agent->model = NULL;

    agent->additionalDeviceProperties = NULL;
}

/**
 * @brief Free ADUC_AgentInfo Array.
 *
 * Caller should assume agents object is invalid after this method returns.
 *
 * @param agentCount Count of objects in agents.
 * @param agents Array of ADUC_AgentInfo objects to free.
 */
static void ADUC_AgentInfoArray_Free(unsigned int agentCount, ADUC_AgentInfo* agents)
{
    for (unsigned int index = 0; index < agentCount; ++index)
    {
        ADUC_AgentInfo* agent = agents + index;
        ADUC_AgentInfo_Free(agent);
    }
    free(agents);
}

/**
 * @param root_value config JSON_Value to get agents from
 * @param agentCount Returned number of agents.
 * @param agents ADUC_AgentInfo (size agentCount). Array to be freed using free(), objects must also be freed.
 * @return bool Success state.
 */
bool ADUC_Json_GetAgents(JSON_Value* root_value, unsigned int* agentCount, ADUC_AgentInfo** agents)
{
    bool succeeded = false;
    if ((agentCount == NULL) || (agents == NULL))
    {
        return false;
    }
    *agentCount = 0;
    *agents = NULL;

    const JSON_Object* root_object = json_value_get_object(root_value);

    JSON_Array* agents_array = json_object_get_array(root_object, "agents");

    if (agents_array == NULL)
    {
        Log_Error("Invalid json - '%s' missing or incorrect", "agents");
        goto done;
    }

    const size_t agents_count = json_array_get_count(agents_array);

    if (agents_count == 0)
    {
        Log_Error("Invalid json - Agents count cannot be zero");
        goto done;
    }

    *agents = calloc(agents_count, sizeof(ADUC_AgentInfo));
    if (*agents == NULL)
    {
        goto done;
    }

    *agentCount = agents_count;

    for (size_t index = 0; index < agents_count; ++index)
    {
        ADUC_AgentInfo* cur_agent = *agents + index;

        const JSON_Object* cur_agent_obj = json_array_get_object(agents_array, index);

        if (cur_agent_obj == NULL)
        {
            Log_Error("No agent @ %zu", index);
            goto done;
        }

        if (!ADUC_AgentInfo_Init(cur_agent, cur_agent_obj))
        {
            Log_Error("Invalid agent arguments");
            goto done;
        }
    }

    succeeded = true;

done:
    if (!succeeded)
    {
        ADUC_AgentInfoArray_Free(*agentCount, *agents);
        *agents = NULL;
        *agentCount = 0;
    }

    return succeeded;
}

/**
 * @brief Allocates the memory for the ADUC_ConfigInfo struct member values
 * @param config A pointer to an ADUC_ConfigInfo struct whose member values will be allocated
 * @param configFilePath The path of configuration file
 * @returns True if successfully allocated, False if failure
 */
bool ADUC_ConfigInfo_Init(ADUC_ConfigInfo* config, const char* configFilePath)
{
    bool succeeded = false;

    // Initialize out parameter.
    memset(config, 0, sizeof(*config));

    JSON_Value* root_value = Parse_JSON_File(configFilePath);

    if (root_value == NULL)
    {
        Log_Error("Failed parse of JSON file: %s", ADUC_CONF_FILE_PATH);
        goto done;
    }

    if (!ADUC_Json_GetAgents(root_value, &(config->agentCount), &(config->agents)))
    {
        goto done;
    }

    const char* schema_version = ADUC_JSON_GetStringFieldPtr(root_value, "schemaVersion");

    if (schema_version == NULL)
    {
        goto done;
    }
    else
    {
        if (mallocAndStrcpy_s(&(config->schemaVersion), schema_version) != 0)
        {
            goto done;
        }
    }

    const char* manufacturer = ADUC_JSON_GetStringFieldPtr(root_value, "manufacturer");
    const char* model = ADUC_JSON_GetStringFieldPtr(root_value, "model");

    if (manufacturer == NULL || model == NULL)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(config->manufacturer), manufacturer) != 0
        || mallocAndStrcpy_s(&(config->model), model) != 0)
    {
        goto done;
    }

    const char* edgegateway_cert_path = ADUC_JSON_GetStringFieldPtr(root_value, "edgegatewayCertPath");

    if (edgegateway_cert_path != NULL)
    {
        if (mallocAndStrcpy_s(&(config->edgegatewayCertPath), edgegateway_cert_path) != 0)
        {
            goto done;
        }
    }

    const JSON_Object* root_object = json_value_get_object(root_value);

    config->aduShellTrustedUsers = json_object_get_array(root_object, "aduShellTrustedUsers");
    if (config->aduShellTrustedUsers == NULL)
    {
        goto done;
    }

    const char* compatPropertyNames = ADUC_JSON_GetStringFieldPtr(root_value, "compatPropertyNames");

    if (compatPropertyNames != NULL)
    {
        if (mallocAndStrcpy_s(&(config->compatPropertyNames), compatPropertyNames) != 0)
        {
            goto done;
        }
    }

    const char* iotHubProtocol = ADUC_JSON_GetStringFieldPtr(root_value, "iotHubProtocol");

    if (iotHubProtocol != NULL)
    {
        if (mallocAndStrcpy_s(&(config->iotHubProtocol), iotHubProtocol) != 0)
        {
            goto done;
        }
    }

    succeeded = true;

done:
    if (!succeeded)
    {
        ADUC_ConfigInfo_UnInit(config);
    }
    return succeeded;
}

/**
 * @brief Free members of ADUC_ConfigInfo object.
 *
 * @param config Object to free.
 */
void ADUC_ConfigInfo_UnInit(ADUC_ConfigInfo* config)
{
    if (config == NULL)
    {
        return;
    }

    free(config->manufacturer);
    free(config->model);
    free(config->iotHubProtocol);
    ADUC_AgentInfoArray_Free(config->agentCount, config->agents);

    memset(config, 0, sizeof(*config));
}

/**
 * @brief Get the agent information of the desired index from the ADUC_ConfigInfo object
 *
 * @param config A pointer to an ADUC_ConfigInfo struct
 * @param index the index of the Agent array in config that we want to access
 * @return const ADUC_AgentInfo*, NULL if failure
 */
const ADUC_AgentInfo* ADUC_ConfigInfo_GetAgent(ADUC_ConfigInfo* config, unsigned int index)
{
    if (config == NULL)
    {
        return NULL;
    }
    if (index >= config->agentCount)
    {
        return NULL;
    }
    return config->agents + index;
}

/**
 * @brief Get the adu trusted user list
 *
 * @param config A pointer to a const ADUC_ConfigInfo struct
 * @return VECTOR_HANDLE
 */
VECTOR_HANDLE ADUC_ConfigInfo_GetAduShellTrustedUsers(const ADUC_ConfigInfo* config)
{
    bool success = false;

    if (config == NULL)
    {
        return NULL;
    }

    VECTOR_HANDLE userVector = VECTOR_create(sizeof(STRING_HANDLE));

    JSON_Array* trustedUsers = config->aduShellTrustedUsers;

    for (size_t i = 0; i < json_array_get_count(trustedUsers); i++)
    {
        STRING_HANDLE userString = STRING_construct(json_array_get_string(trustedUsers, i));
        if (userString == NULL)
        {
            Log_Error("Cannot read the %zu index user from adu shell trusted users. ", i);
            goto done;
        }
        // Note that VECTOR_push_back's second parameter is the pointer to the STRING_HANDLE userString
        if (VECTOR_push_back(userVector, &userString, 1) != 0)
        {
            Log_Error("Cannot add user to adu shell trusted user vector.");
            STRING_delete(userString);
            goto done;
        }
    }

    success = true;

done:
    if (!success)
    {
        Log_Error("Failed to get adu shell trusted users array.");
        ADUC_ConfigInfo_FreeAduShellTrustedUsers(userVector);
        userVector = NULL;
    }
    return userVector;
}

/**
 * @brief Free the VECTOR_HANDLE (adu shell truster users) and all the elements in it
 *
 * @param users Object to free. The vector (type VECTOR_HANDLE) containing users (type STRING_HANDLE)
 */
void ADUC_ConfigInfo_FreeAduShellTrustedUsers(VECTOR_HANDLE users)
{
    if (users == NULL)
    {
        return;
    }
    for (size_t i = 0; i < VECTOR_size(users); i++)
    {
        // VECTOR_element returns the pointer to a STRING_HANDLE
        STRING_HANDLE* userPtr = VECTOR_element(users, i);
        STRING_delete(*userPtr);
    }

    VECTOR_clear(users);
}
