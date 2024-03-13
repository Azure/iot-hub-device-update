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
#include <aducpal/stdlib.h> // setenv
#include <azure_c_shared_utility/crt_abstractions.h>
#include <azure_c_shared_utility/strings_types.h>
#include <parson.h>
#include <parson_json_utils.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

static const char* CONFIG_ADU_SHELL_FOLDER = "aduShellFolder";
static const char* CONFIG_ADU_DATA_FOLDER = "dataFolder";
static const char* CONFIG_ADU_EXTENSIONS_FOLDER = "extensionsFolder";
static const char* CONFIG_ADU_DOWNLOADS_FOLDER = "downloadsFolder";

static const char* DOWNLOADS_PATH_SEGMENT = "downloads";
static const char* EXTENSIONS_PATH_SEGMENT = "extensions";

static const char* CONFIG_IOT_HUB_PROTOCOL = "iotHubProtocol";
static const char* CONFIG_COMPAT_PROPERTY_NAMES = "compatPropertyNames";
static const char* CONFIG_ADU_SHELL_TRUSTED_USERS = "aduShellTrustedUsers";
static const char* CONFIG_EDGE_GATEWAY_CERT_PATH = "edgegatewayCertPath";
static const char* CONFIG_MANUFACTURER = "manufacturer";
static const char* CONFIG_MODEL = "model";
static const char* CONFIG_SCHEMA_VERSION = "schemaVersion";
static const char* CONFIG_DOWNLOAD_TIMEOUT_IN_MINUTES = "downloadTimeoutInMinutes";

static const char* CONFIG_NAME = "name";
static const char* CONFIG_RUN_AS = "runas";
static const char* CONFIG_CONNECTION_SOURCE = "connectionSource";
static const char* CONFIG_CONNECTION_TYPE = "connectionType";
static const char* CONFIG_CONNECTION_DATA = "connectionData";
static const char* CONFIG_ADDITIONAL_DEVICE_PROPERTIES = "additionalDeviceProperties";
static const char* CONFIG_AGENTS = "agents";

static const char* INVALID_OR_MISSING_FIELD_ERROR_FMT = "Invalid json - '%s' missing or incorrect";

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
static void ADUC_AgentInfoArray_Free(size_t agentCount, ADUC_AgentInfo* agents)
{
    for (size_t index = 0; index < agentCount; ++index)
    {
        ADUC_AgentInfo* agent = (ADUC_AgentInfo*)(agents + index);
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
bool ADUC_Json_GetAgents(JSON_Value* root_value, size_t* agentCount, ADUC_AgentInfo** agents)
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
        Log_Error(INVALID_OR_MISSING_FIELD_ERROR_FMT, CONFIG_AGENTS);
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

    *agentCount = (unsigned int)agents_count;

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

bool EnsureDataSubFolderSpecifiedOrSetDefaultValue(
    const JSON_Value* rootValue,
    const char* fieldName,
    const char** folder,
    const char* dataFolder,
    const char* defaultSubFolderValue)
{
    if (rootValue == NULL || fieldName == NULL || folder == NULL || dataFolder == NULL)
    {
        return false;
    }

    // If the folder is already specified, we're done.
    *folder = ADUC_JSON_GetStringFieldPtr(rootValue, fieldName);
    if (*folder != NULL)
    {
        return true;
    }

    char* subFolder = ADUC_StringFormat("%s/%s", dataFolder, defaultSubFolderValue);
    if (subFolder == NULL)
    {
        Log_Error("Failed to allocate memory for %s folder", defaultSubFolderValue);
        return false;
    }

    bool setOk = ADUC_JSON_SetStringField(rootValue, fieldName, subFolder);
    free(subFolder);
    if (!setOk)
    {
        Log_Error("Failed to set %s field.", fieldName);
        return false;
    }

    *folder = ADUC_JSON_GetStringFieldPtr(rootValue, fieldName);
    if (*folder == NULL)
    {
        Log_Error("Failed to get %s field.", fieldName);
        return false;
    }
    return true;
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

    // Initialize out parameter.
    memset(config, 0, sizeof(*config));

    if (mallocAndStrcpy_s(&(config->configFolder), configFolder) != 0)
    {
        Log_Error("Failed to allocate memory for config file folder");
        return false;
    }

    char* configFilePath = NULL;

    if (IsNullOrEmpty(configFolder))
    {
        configFilePath = ADUC_StringFormat("%s/%s", ADUC_CONF_FOLDER, ADUC_CONF_FILE);
    }
    else
    {
        configFilePath = ADUC_StringFormat("%s/%s", configFolder, ADUC_CONF_FILE);
    }

    if (IsNullOrEmpty(configFilePath))
    {
        Log_Error("Failed to allocate memory for config file path");
        goto done;
    }

    config->rootJsonValue = Parse_JSON_File(configFilePath);

    if (config->rootJsonValue == NULL)
    {
        Log_Error("Failed parse of JSON file: %s", configFilePath);
        goto done;
    }

    if (!ADUC_Json_GetAgents(config->rootJsonValue, &(config->agentCount), &(config->agents)))
    {
        goto done;
    }

    config->schemaVersion = ADUC_JSON_GetStringFieldPtr(config->rootJsonValue, CONFIG_SCHEMA_VERSION);

    if (IsNullOrEmpty(config->schemaVersion))
    {
        Log_Error(INVALID_OR_MISSING_FIELD_ERROR_FMT, CONFIG_SCHEMA_VERSION);
        goto done;
    }

    config->manufacturer = ADUC_JSON_GetStringFieldPtr(config->rootJsonValue, CONFIG_MANUFACTURER);

    if (IsNullOrEmpty(config->manufacturer))
    {
        Log_Error(INVALID_OR_MISSING_FIELD_ERROR_FMT, CONFIG_MANUFACTURER);
        goto done;
    }

    config->model = ADUC_JSON_GetStringFieldPtr(config->rootJsonValue, CONFIG_MODEL);

    if (IsNullOrEmpty(config->model))
    {
        Log_Error(INVALID_OR_MISSING_FIELD_ERROR_FMT, CONFIG_MODEL);
        goto done;
    }

    // Edge gateway certification path is optional.
    config->edgegatewayCertPath = ADUC_JSON_GetStringFieldPtr(config->rootJsonValue, CONFIG_EDGE_GATEWAY_CERT_PATH);

    const JSON_Object* root_object = json_value_get_object(config->rootJsonValue);

    config->aduShellTrustedUsers = json_object_get_array(root_object, CONFIG_ADU_SHELL_TRUSTED_USERS);

    if (config->aduShellTrustedUsers == NULL)
    {
        Log_Error(INVALID_OR_MISSING_FIELD_ERROR_FMT, CONFIG_ADU_SHELL_TRUSTED_USERS);
        goto done;
    }

    // Note: compat property names are optional.
    config->compatPropertyNames = ADUC_JSON_GetStringFieldPtr(config->rootJsonValue, CONFIG_COMPAT_PROPERTY_NAMES);

    // Note: IoT Hub protocol is is optional.
    config->iotHubProtocol = ADUC_JSON_GetStringFieldPtr(config->rootJsonValue, CONFIG_IOT_HUB_PROTOCOL);

    // Note: download timeout is optional.
    ADUC_JSON_GetUnsignedIntegerField(
        config->rootJsonValue, CONFIG_DOWNLOAD_TIMEOUT_IN_MINUTES, &(config->downloadTimeoutInMinutes));

    // Ensure that adu-shell folder is valid.
    config->aduShellFolder = ADUC_JSON_GetStringFieldPtr(config->rootJsonValue, CONFIG_ADU_SHELL_FOLDER);

    if (config->aduShellFolder == NULL)
    {
        if (!ADUC_JSON_SetStringField(config->rootJsonValue, CONFIG_ADU_SHELL_FOLDER, ADUSHELL_FOLDER))
        {
            Log_Error("Failed to set adu-shell folder.");
            goto done;
        }
        config->aduShellFolder = ADUC_JSON_GetStringFieldPtr(config->rootJsonValue, CONFIG_ADU_SHELL_FOLDER);
    }

    config->aduShellFilePath = ADUC_StringFormat("%s/%s", config->aduShellFolder, ADUSHELL_FILENAME);
    if (config->aduShellFilePath == NULL)
    {
        Log_Error("Failed to allocate memory for adu-shell file path");
        goto done;
    }

    config->dataFolder = ADUC_JSON_GetStringFieldPtr(config->rootJsonValue, CONFIG_ADU_DATA_FOLDER);

    if (config->dataFolder == NULL)
    {
        if (!ADUC_JSON_SetStringField(config->rootJsonValue, CONFIG_ADU_DATA_FOLDER, ADUC_DATA_FOLDER))
        {
            Log_Error("Failed to set data folder.");
            goto done;
        }
        config->dataFolder = ADUC_JSON_GetStringFieldPtr(config->rootJsonValue, CONFIG_ADU_DATA_FOLDER);
    }

    if (!EnsureDataSubFolderSpecifiedOrSetDefaultValue(
            config->rootJsonValue,
            CONFIG_ADU_DOWNLOADS_FOLDER,
            &config->downloadsFolder,
            config->dataFolder,
            DOWNLOADS_PATH_SEGMENT))
    {
        goto done;
    }

    if (!EnsureDataSubFolderSpecifiedOrSetDefaultValue(
            config->rootJsonValue,
            CONFIG_ADU_EXTENSIONS_FOLDER,
            &config->extensionsFolder,
            config->dataFolder,
            EXTENSIONS_PATH_SEGMENT))
    {
        goto done;
    }

    // Since we're only allow overriding of 'extensions' folder, let's populate all extensions sub-folders.
    config->extensionsComponentEnumeratorFolder =
        ADUC_StringFormat("%s/%s", config->extensionsFolder, ADUC_EXTENSIONS_SUBDIR_COMPONENT_ENUMERATOR);

    if (config->extensionsComponentEnumeratorFolder == NULL)
    {
        Log_Error("Failed to allocate memory for component enumerator extension folder");
        goto done;
    }

    config->extensionsContentDownloaderFolder =
        ADUC_StringFormat("%s/%s", config->extensionsFolder, ADUC_EXTENSIONS_SUBDIR_CONTENT_DOWNLOADER);
    if (config->extensionsContentDownloaderFolder == NULL)
    {
        Log_Error("Failed to allocate memory for content downloader extension folder");
        goto done;
    }

    config->extensionsStepHandlerFolder =
        ADUC_StringFormat("%s/%s", config->extensionsFolder, ADUC_EXTENSIONS_SUBDIR_UPDATE_CONTENT_HANDLERS);
    if (config->extensionsStepHandlerFolder == NULL)
    {
        Log_Error("Failed to allocate memory for step handler extension folder");
        goto done;
    }

    config->extensionsDownloadHandlerFolder =
        ADUC_StringFormat("%s/%s", config->extensionsFolder, ADUC_EXTENSIONS_SUBDIR_DOWNLOAD_HANDLERS);
    if (config->extensionsDownloadHandlerFolder == NULL)
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

    // Free all computed value those cached outside of the config->rootJsonValue.
    free(config->configFolder);
    free(config->aduShellFilePath);
    free(config->extensionsComponentEnumeratorFolder);
    free(config->extensionsContentDownloaderFolder);
    free(config->extensionsStepHandlerFolder);
    free(config->extensionsDownloadHandlerFolder);

    json_value_free(config->rootJsonValue);
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
 * @brief Get the existing ADUC_ConfigInfo object, or create one if it doesn't exist.
 *
 * @return const ADUC_ConfigInfo* a pointer to ADUC_ConfigInfo object. NULL if failure.
 * Caller must call ADUC_ConfigInfo_Release to free the object.
 */
const ADUC_ConfigInfo* ADUC_ConfigInfo_GetInstance()
{
    const ADUC_ConfigInfo* config = NULL;

    s_config_lock();
    if (s_configInfo.refCount == 0)
    {
        char* configFolder = getenv(ADUC_CONFIG_FOLDER_ENV);
        if (configFolder == NULL)
        {
            Log_Info(
                "%s environment variable not set, fallback to the default value %s.",
                ADUC_CONFIG_FOLDER_ENV,
                ADUC_CONF_FOLDER);
            ADUCPAL_setenv(ADUC_CONFIG_FOLDER_ENV, configFolder = ADUC_CONF_FOLDER, 1);
        }
        if (!ADUC_ConfigInfo_Init(&s_configInfo, configFolder))
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
 * @brief Release the ADUC_ConfigInfo object.
 *
 * @param configInfo a pointer to ADUC_ConfigInfo object
 * @return int The reference count of the ADUC_ConfigInfo object after released. -1 if failure.
 */
int ADUC_ConfigInfo_ReleaseInstance(const ADUC_ConfigInfo* configInfo)
{
    int ret = -1;
    if (configInfo != &s_configInfo)
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
