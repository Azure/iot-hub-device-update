/**
 * @file config_reader.c
 * @brief TOML-based layered configuration reader implementation.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/config_reader.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <toml.h>

// Default paths
#define DEFAULT_CONFIG_DIR "/etc/adu"
#define DEFAULT_AGENT_CONF "adu-agent.conf"
#define DEFAULT_CONF_D "conf.d"
#define DEFAULT_FACTORY_CONF "/usr/lib/adu/defaults.conf"

struct ADUC_Config
{
    toml_table_t* root;  // Merged config (all layers)
    char* rawData;       // Backing buffer for current parse (simplification: last file wins)
    // TODO: For proper deep merge, store all layers and do runtime lookup with priority
    toml_table_t** layers;
    size_t layerCount;
};

/**
 * @brief Parse a single TOML file into a toml_table_t.
 */
static toml_table_t* parse_toml_file(const char* path)
{
    FILE* fp = fopen(path, "r");
    if (!fp)
    {
        return NULL;
    }

    char errbuf[256];
    toml_table_t* table = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (!table)
    {
        fprintf(stderr, "ADUC_Config: Failed to parse %s: %s\n", path, errbuf);
    }
    return table;
}

ADUC_Result2 ADUC_Config_LoadFile(const char* filePath, ADUC_ConfigHandle* outHandle)
{
    if (!filePath || !outHandle)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 1);
    }

    struct ADUC_Config* config = calloc(1, sizeof(struct ADUC_Config));
    if (!config)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 1);
    }

    config->root = parse_toml_file(filePath);
    if (!config->root)
    {
        free(config);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 2);
    }

    *outHandle = config;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_Config_Load(const char* configDir, ADUC_ConfigHandle* outHandle)
{
    if (!outHandle)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 1);
    }

    const char* dir = configDir ? configDir : DEFAULT_CONFIG_DIR;

    // Build path to main config
    char mainConfPath[512];
    snprintf(mainConfPath, sizeof(mainConfPath), "%s/%s", dir, DEFAULT_AGENT_CONF);

    // For now, just load the main config file
    // TODO: Implement proper layered merge (factory → main → conf.d/*)
    ADUC_Result2 result = ADUC_Config_LoadFile(mainConfPath, outHandle);
    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        // Try factory defaults as fallback
        return ADUC_Config_LoadFile(DEFAULT_FACTORY_CONF, outHandle);
    }

    return result;
}

/**
 * @brief Navigate dotted key path to find a value in a TOML table.
 * Handles paths like "agent.device_id", "workflow.retry.max_retries".
 */
static toml_table_t* navigate_to_parent(toml_table_t* root, const char* key, const char** leafKey)
{
    if (!root || !key)
    {
        return NULL;
    }

    // Make a mutable copy of key for tokenizing
    char* keyCopy = strdup(key);
    if (!keyCopy)
    {
        return NULL;
    }

    toml_table_t* current = root;
    char* saveptr = NULL;
    char* token = strtok_r(keyCopy, ".", &saveptr);
    while (token)
    {
        char* next = strtok_r(NULL, ".", &saveptr);
        if (!next)
        {
            // This is the leaf key
            *leafKey = key + (token - keyCopy);
            free(keyCopy);
            return current;
        }

        // Navigate into subtable
        current = toml_table_in(current, token);
        if (!current)
        {
            free(keyCopy);
            return NULL;
        }
        token = next;
    }

    free(keyCopy);
    return NULL;
}

const char* ADUC_Config_GetString(ADUC_ConfigHandle handle, const char* key)
{
    if (!handle || !handle->root || !key)
    {
        return NULL;
    }

    const char* leafKey = NULL;
    toml_table_t* parent = navigate_to_parent(handle->root, key, &leafKey);
    if (!parent || !leafKey)
    {
        return NULL;
    }

    toml_datum_t datum = toml_string_in(parent, leafKey);
    if (!datum.ok)
    {
        return NULL;
    }

    // Note: toml_string_in returns malloc'd string, we leak it here for simplicity.
    // In production, we'd cache these in the config struct.
    // TODO: Track allocated strings for proper cleanup.
    return datum.u.s;
}

ADUC_Result2 ADUC_Config_GetInt(ADUC_ConfigHandle handle, const char* key, int64_t* outValue)
{
    if (!handle || !handle->root || !key || !outValue)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 1);
    }

    const char* leafKey = NULL;
    toml_table_t* parent = navigate_to_parent(handle->root, key, &leafKey);
    if (!parent || !leafKey)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 3);
    }

    toml_datum_t datum = toml_int_in(parent, leafKey);
    if (!datum.ok)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 4);
    }

    *outValue = datum.u.i;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_Config_GetBool(ADUC_ConfigHandle handle, const char* key, bool* outValue)
{
    if (!handle || !handle->root || !key || !outValue)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 1);
    }

    const char* leafKey = NULL;
    toml_table_t* parent = navigate_to_parent(handle->root, key, &leafKey);
    if (!parent || !leafKey)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 3);
    }

    toml_datum_t datum = toml_bool_in(parent, leafKey);
    if (!datum.ok)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 4);
    }

    *outValue = (bool)datum.u.b;
    return ADUC_RESULT2_SUCCESS;
}

bool ADUC_Config_HasKey(ADUC_ConfigHandle handle, const char* key)
{
    return ADUC_Config_GetString(handle, key) != NULL;
}

void ADUC_Config_Free(ADUC_ConfigHandle handle)
{
    if (!handle)
    {
        return;
    }

    if (handle->root)
    {
        toml_free(handle->root);
    }

    // Free layers if allocated
    if (handle->layers)
    {
        for (size_t i = 0; i < handle->layerCount; i++)
        {
            if (handle->layers[i])
            {
                toml_free(handle->layers[i]);
            }
        }
        free(handle->layers);
    }

    free(handle);
}
