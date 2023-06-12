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
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static pthread_mutex_t s_config_mutex = PTHREAD_MUTEX_INITIALIZER;

static ADUC_ConfigInfo s_configInfo = { 0 };

static inline void s_config_lock(void)
{
    pthread_mutex_lock(&s_config_mutex);
}

static inline void s_config_unlock(void)
{
    pthread_mutex_unlock(&s_config_mutex);
}

//static const char* CONFIG_LOG_FOLDER = "logFolder";
static const char* CONFIG_ADU_SHELL_FOLDER = "aduShellFolder";
static const char* CONFIG_ADU_DATA_FOLDER = "dataFolder";
static const char* CONFIG_ADU_EXTENSIONS_FOLDER = "extensionsFolder";
static const char* CONFIG_ADU_DOWNLOADS_FOLDER = "downloadsFolder";

static const char* CONFIG_IOT_HUB_PROTOCOL = "iotHubProtocol";
static const char* CONFIG_COMPAT_PROPERTY_NAMES = "compatPropertyNames";
static const char* CONFIG_ADU_SHELL_TRUSTED_USERS = "aduShellTrustedUsers";
static const char* CONFIG_EDGE_GATEWAY_CERT_PATH = "edgegatewayCertPath";
static const char* CONFIG_MANUFACTURER = "manufacturer";
static const char* CONFIG_MODEL = "model";
static const char* CONFIG_SCHEMA_VERSION = "schemaVersion";

static const char* CONFIG_NAME = "name";
static const char* CONFIG_RUN_AS = "runas";
static const char* CONFIG_CONNECTION_SOURCE = "connectionSource";
static const char* CONFIG_CONNECTION_TYPE = "connectionType";
static const char* CONFIG_CONNECTION_DATA = "connectionData";
static const char* CONFIG_ADDITIONAL_DEVICE_PROPERTIES = "additionalDeviceProperties";
static const char* CONFIG_AGENTS = "agents";

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

    const char* name = json_object_get_string(agent_obj, CONFIG_NAME);
    const char* runas = json_object_get_string(agent_obj, CONFIG_RUN_AS);
    const char* manufacturer = json_object_get_string(agent_obj, CONFIG_MANUFACTURER);
    const char* model = json_object_get_string(agent_obj, CONFIG_MODEL);

    JSON_Object* connection_source = json_object_get_object(agent_obj, CONFIG_CONNECTION_SOURCE);
    const char* connection_type = NULL;
    const char* connection_data = NULL;

    if (connection_source == NULL)
    {
        return false;
    }

    connection_type = json_object_get_string(connection_source, CONFIG_CONNECTION_TYPE);
    connection_data = json_object_get_string(connection_source, CONFIG_CONNECTION_DATA);

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

    agent->additionalDeviceProperties = json_object_get_object(agent_obj, CONFIG_ADDITIONAL_DEVICE_PROPERTIES);

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

    JSON_Array* agents_array = json_object_get_array(root_object, CONFIG_AGENTS);

    if (agents_array == NULL)
    {
        Log_Error("Invalid json - '%s' missing or incorrect", CONFIG_AGENTS);
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
 * @param configFolder The folder of configuration files. If NULL, the default folder (ADUC_CONF_FOLDER) will be used.
 * @returns True if successfully allocated, False if failure
 */
bool ADUC_ConfigInfo_Init(ADUC_ConfigInfo* config, const char* configFolder)
{
    bool succeeded = false;

    char* configFilePath = NULL;

    if (IsNullOrEmpty(configFolder))
    {
        configFilePath = ADUC_StringFormat("%s/%s", ADUC_CONF_FOLDER, ADUC_CONF_FILE);
    }
    else
    {
        configFilePath = ADUC_StringFormat("%s/%s", configFolder, ADUC_CONF_FILE);
    }

    if (configFilePath == NULL)
    {
        Log_Error("Failed to allocate memory for config file path");
        goto done;
    }

    // Initialize out parameter.
    memset(config, 0, sizeof(*config));

    JSON_Value* root_value = Parse_JSON_File(configFilePath);

    if (root_value == NULL)
    {
        Log_Error("Failed parse of JSON file: %s", configFilePath);
        goto done;
    }

    if (!ADUC_Json_GetAgents(root_value, &(config->agentCount), &(config->agents)))
    {
        goto done;
    }

    const char* schema_version = ADUC_JSON_GetStringFieldPtr(root_value, CONFIG_SCHEMA_VERSION);

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

    const char* manufacturer = ADUC_JSON_GetStringFieldPtr(root_value, CONFIG_MANUFACTURER);
    const char* model = ADUC_JSON_GetStringFieldPtr(root_value, CONFIG_MODEL);

    if (manufacturer == NULL || model == NULL)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(config->manufacturer), manufacturer) != 0
        || mallocAndStrcpy_s(&(config->model), model) != 0)
    {
        goto done;
    }

    const char* edgegateway_cert_path = ADUC_JSON_GetStringFieldPtr(root_value, CONFIG_EDGE_GATEWAY_CERT_PATH);

    if (edgegateway_cert_path != NULL)
    {
        if (mallocAndStrcpy_s(&(config->edgegatewayCertPath), edgegateway_cert_path) != 0)
        {
            goto done;
        }
    }

    const JSON_Object* root_object = json_value_get_object(root_value);

    config->aduShellTrustedUsers = json_object_get_array(root_object, CONFIG_ADU_SHELL_TRUSTED_USERS);
    if (config->aduShellTrustedUsers == NULL)
    {
        goto done;
    }

    const char* compatPropertyNames = ADUC_JSON_GetStringFieldPtr(root_value, CONFIG_COMPAT_PROPERTY_NAMES);

    if (compatPropertyNames != NULL)
    {
        if (mallocAndStrcpy_s(&(config->compatPropertyNames), compatPropertyNames) != 0)
        {
            goto done;
        }
    }

    const char* iotHubProtocol = ADUC_JSON_GetStringFieldPtr(root_value, CONFIG_IOT_HUB_PROTOCOL);

    if (iotHubProtocol != NULL)
    {
        if (mallocAndStrcpy_s(&(config->iotHubProtocol), iotHubProtocol) != 0)
        {
            goto done;
        }
    }

    if (!ADUC_JSON_GetUnsignedIntegerField(
            root_value, "downloadTimeoutInMinutes", &(config->downloadTimeoutInMinutes)))
    {
        goto done;
    }

    const char* aduShellFolder = ADUC_JSON_GetStringFieldPtr(root_value, CONFIG_ADU_SHELL_FOLDER);

    if (aduShellFolder != NULL)
    {
        if (mallocAndStrcpy_s(&(config->aduShellFolder), aduShellFolder) != 0)
        {
            goto done;
        }
    }
    else
    {
        if (mallocAndStrcpy_s(&(config->aduShellFolder), ADUSHELL_FOLDER) != 0)
        {
            goto done;
        }
    }

    config->aduShellFilePath = ADUC_StringFormat("%s/%s", config->aduShellFolder, ADUSHELL_FILENAME);
    if (config->aduShellFilePath == NULL)
    {
        Log_Error("Failed to allocate memory for adu-shell file path");
        goto done;
    }

    const char* dataFolder = ADUC_JSON_GetStringFieldPtr(root_value, CONFIG_ADU_DATA_FOLDER);

    if (dataFolder != NULL)
    {
        if (mallocAndStrcpy_s(&(config->dataFolder), dataFolder) != 0)
        {
            goto done;
        }
    }
    else
    {
        if (mallocAndStrcpy_s(&(config->dataFolder), ADUC_DATA_FOLDER) != 0)
        {
            goto done;
        }
    }

    const char* downloadsFolder = ADUC_JSON_GetStringFieldPtr(root_value, CONFIG_ADU_DOWNLOADS_FOLDER);

    if (downloadsFolder != NULL)
    {
        if (mallocAndStrcpy_s(&(config->downloadsFolder), downloadsFolder) != 0)
        {
            goto done;
        }
    }
    else
    {
        config->downloadsFolder = ADUC_StringFormat("%s/%s", config->dataFolder, "downloads");
        if (config->downloadsFolder == NULL)
        {
            Log_Error("Failed to allocate memory for downloads folder");
            goto done;
        }
    }

    const char* extensionsFolder = ADUC_JSON_GetStringFieldPtr(root_value, CONFIG_ADU_EXTENSIONS_FOLDER);

    if (extensionsFolder != NULL)
    {
        if (mallocAndStrcpy_s(&(config->extensionsFolder), extensionsFolder) != 0)
        {
            goto done;
        }
    }
    else
    {
        config->extensionsFolder = ADUC_StringFormat("%s/%s", config->dataFolder, "extensions");
        if (config->extensionsFolder == NULL)
        {
            Log_Error("Failed to allocate memory for extension folder");
            goto done;
        }
    }

    if (NULL == (config->extensionsComponentEnumeratorFolder =
        ADUC_StringFormat("%s/%s", config->extensionsFolder, ADUC_EXTENSIONS_SUBDIR_COMPONENT_ENUMERATOR )))
    {
        Log_Error("Failed to allocate memory for component enumerator extension folder");
        goto done;
    }

    if (NULL == (config->extensionsContentDownloaderFolder =
        ADUC_StringFormat("%s/%s", config->extensionsFolder, ADUC_EXTENSIONS_SUBDIR_CONTENT_DOWNLOADER)))
    {
        Log_Error("Failed to allocate memory for content downloader extension folder");
        goto done;
    }

    if (NULL == (config->extensionsStepHandlerFolder =
        ADUC_StringFormat("%s/%s", config->extensionsFolder, ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS)))
    {
        Log_Error("Failed to allocate memory for step handler extension folder");
        goto done;
    }

    if (NULL == (config->extensionsDownloadHandlerFolder =
        ADUC_StringFormat("%s/%s", config->extensionsFolder, ADUC_EXTENSIONS_SUBDIR_DOWNLOAD_HANDLERS)))
    {
        Log_Error("Failed to allocate memory for download handler extension folder");
        goto done;
    }


    succeeded = true;

done:

    if (configFilePath != NULL)
    {
        free(configFilePath);
    }

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
    Log_Info("Uninitializing config info.");

    if (config == NULL)
    {
        return;
    }

    config->refCount = 0;

    free(config->schemaVersion);
    free(config->aduShellFolder);
    free(config->aduShellFilePath);
    free(config->configFolder);
    free(config->dataFolder);
    free(config->downloadsFolder);
    free(config->extensionsFolder);
    free(config->extensionsComponentEnumeratorFolder);
    free(config->extensionsContentDownloaderFolder);
    free(config->extensionsStepHandlerFolder);
    free(config->extensionsDownloadHandlerFolder);
    free(config->iotHubProtocol);
    free(config->manufacturer);
    free(config->model);

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
const ADUC_AgentInfo* ADUC_ConfigInfo_GetAgent(const ADUC_ConfigInfo* config, unsigned int index)
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

/**
 * @brief Create the ADUC_ConfigInfo object.
 *
 * @param configFilePath a pointer to the configuration file path
 * @return const ADUC_ConfigInfo* a pointer to ADUC_ConfigInfo object. NULL if failure.
 * Caller must call ADUC_ConfigInfo_Release to free the object.
 */
const ADUC_ConfigInfo* ADUC_ConfigInfo_CreateInstance(const char* configFilePath)
{
    const ADUC_ConfigInfo* config = NULL;
    s_config_lock();
    if (s_configInfo.refCount == 0)
    {
        if (!ADUC_ConfigInfo_Init(&s_configInfo, configFilePath))
        {
            goto done;
        }
    }
    s_configInfo.refCount++;
    config = &s_configInfo;
done:
    s_config_unlock();
    return config;
}

/**
 * @brief Get the existing ADUC_ConfigInfo object.
 *
 * @return const ADUC_ConfigInfo* a pointer to ADUC_ConfigInfo object. NULL if failure.
 * Caller must call ADUC_ConfigInfo_Release to free the object.
 */
const ADUC_ConfigInfo* ADUC_ConfigInfo_GetInstance()
{
    ADUC_ConfigInfo* config = NULL;
    s_config_lock();
    if (s_configInfo.refCount > 0)
    {
        s_configInfo.refCount++;
        config = &s_configInfo;
    }
    s_config_unlock();
    return config;
}

/**
 * @brief Release the ADUC_ConfigInfo object.
 *
 * @param configInfo a pointer to ADUC_ConfigInfo object
 * @return int The reference count of the ADUC_ConfigInfo object after released. -1 if failure.
 */
int ADUC_ConfigInfo_ReleaseInstance(const ADUC_ConfigInfo* configInfo)
{
    int ret = -1;
    if (configInfo == NULL)
    {
        return ret;
    }

    s_config_lock();

    if (s_configInfo.refCount == 0)
    {
        goto done;
    }

    s_configInfo.refCount--;
    if (s_configInfo.refCount == 0)
    {
        ADUC_ConfigInfo_UnInit(&s_configInfo);
    }

    ret = s_configInfo.refCount;

done:
    s_config_unlock();
    return ret;
}
