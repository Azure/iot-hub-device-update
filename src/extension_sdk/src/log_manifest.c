/**
 * @file log_manifest.c
 * @brief Implementation of the log manifest module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/log_manifest.h"

#include <parson.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Internal manifest structure.
 */
struct ADUC_LogManifest
{
    ADUC_LogManifestEntry* entries;
    size_t count;
};

static char* dup_str(const char* s)
{
    if (s == NULL)
    {
        return NULL;
    }
    size_t len = strlen(s);
    char* d = (char*)malloc(len + 1);
    if (d != NULL)
    {
        memcpy(d, s, len + 1);
    }
    return d;
}

static void free_entry(ADUC_LogManifestEntry* entry)
{
    free((void*)entry->component);
    free((void*)entry->formatString);
    free((void*)entry->description);
}

static ADUC_LogLevel level_from_int(int val) __attribute__((unused));
static ADUC_LogLevel level_from_int(int val)
{
    if (val >= ADUC_LOG_FATAL && val <= ADUC_LOG_TRACE)
    {
        return (ADUC_LogLevel)val;
    }
    return ADUC_LOG_INFO;
}

static const char* level_to_string(ADUC_LogLevel level)
{
    switch (level)
    {
        case ADUC_LOG_FATAL: return "fatal";
        case ADUC_LOG_ERROR: return "error";
        case ADUC_LOG_WARN:  return "warn";
        case ADUC_LOG_INFO:  return "info";
        case ADUC_LOG_DEBUG: return "debug";
        case ADUC_LOG_TRACE: return "trace";
        default: return "info";
    }
}

static ADUC_LogLevel level_from_string(const char* s)
{
    if (s == NULL) return ADUC_LOG_INFO;
    if (strcmp(s, "fatal") == 0) return ADUC_LOG_FATAL;
    if (strcmp(s, "error") == 0) return ADUC_LOG_ERROR;
    if (strcmp(s, "warn") == 0)  return ADUC_LOG_WARN;
    if (strcmp(s, "info") == 0)  return ADUC_LOG_INFO;
    if (strcmp(s, "debug") == 0) return ADUC_LOG_DEBUG;
    if (strcmp(s, "trace") == 0) return ADUC_LOG_TRACE;
    return ADUC_LOG_INFO;
}

ADUC_Result2 ADUC_LogManifest_Create(
    const ADUC_LogManifestEntry* entries,
    size_t count,
    ADUC_LogManifestHandle* outHandle)
{
    if (outHandle == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 1);
    }
    *outHandle = NULL;

    if (entries == NULL && count > 0)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 2);
    }

    struct ADUC_LogManifest* manifest = (struct ADUC_LogManifest*)calloc(1, sizeof(struct ADUC_LogManifest));
    if (manifest == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 1);
    }

    if (count > 0)
    {
        manifest->entries = (ADUC_LogManifestEntry*)calloc(count, sizeof(ADUC_LogManifestEntry));
        if (manifest->entries == NULL)
        {
            free(manifest);
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 1);
        }

        for (size_t i = 0; i < count; i++)
        {
            manifest->entries[i].eventId = entries[i].eventId;
            manifest->entries[i].component = dup_str(entries[i].component);
            manifest->entries[i].level = entries[i].level;
            manifest->entries[i].formatString = dup_str(entries[i].formatString);
            manifest->entries[i].description = dup_str(entries[i].description);
        }
    }

    manifest->count = count;
    *outHandle = manifest;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_LogManifest_LoadFromFile(const char* path, ADUC_LogManifestHandle* outHandle)
{
    if (path == NULL || outHandle == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 1);
    }
    *outHandle = NULL;

    JSON_Value* rootValue = json_parse_file(path);
    if (rootValue == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1);
    }

    JSON_Array* arr = json_value_get_array(rootValue);
    if (arr == NULL)
    {
        json_value_free(rootValue);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 3);
    }

    size_t count = json_array_get_count(arr);

    struct ADUC_LogManifest* manifest = (struct ADUC_LogManifest*)calloc(1, sizeof(struct ADUC_LogManifest));
    if (manifest == NULL)
    {
        json_value_free(rootValue);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 1);
    }

    if (count > 0)
    {
        manifest->entries = (ADUC_LogManifestEntry*)calloc(count, sizeof(ADUC_LogManifestEntry));
        if (manifest->entries == NULL)
        {
            free(manifest);
            json_value_free(rootValue);
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 1);
        }

        for (size_t i = 0; i < count; i++)
        {
            JSON_Object* obj = json_array_get_object(arr, i);
            if (obj == NULL)
            {
                continue;
            }
            manifest->entries[i].eventId = (uint16_t)json_object_get_number(obj, "eventId");
            manifest->entries[i].component = dup_str(json_object_get_string(obj, "component"));
            manifest->entries[i].level = level_from_string(json_object_get_string(obj, "level"));
            manifest->entries[i].formatString = dup_str(json_object_get_string(obj, "formatString"));
            manifest->entries[i].description = dup_str(json_object_get_string(obj, "description"));
        }
    }

    manifest->count = count;
    *outHandle = manifest;
    json_value_free(rootValue);
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_LogManifest_SaveToFile(ADUC_LogManifestHandle handle, const char* path)
{
    if (handle == NULL || path == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 1);
    }

    JSON_Value* rootValue = json_value_init_array();
    JSON_Array* arr = json_value_get_array(rootValue);
    if (arr == NULL)
    {
        json_value_free(rootValue);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 1);
    }

    for (size_t i = 0; i < handle->count; i++)
    {
        JSON_Value* entryVal = json_value_init_object();
        JSON_Object* entryObj = json_value_get_object(entryVal);

        json_object_set_number(entryObj, "eventId", handle->entries[i].eventId);
        json_object_set_string(entryObj, "component", handle->entries[i].component ? handle->entries[i].component : "");
        json_object_set_string(entryObj, "level", level_to_string(handle->entries[i].level));
        json_object_set_string(entryObj, "formatString", handle->entries[i].formatString ? handle->entries[i].formatString : "");
        json_object_set_string(entryObj, "description", handle->entries[i].description ? handle->entries[i].description : "");

        json_array_append_value(arr, entryVal);
    }

    JSON_Status status = json_serialize_to_file_pretty(rootValue, path);
    json_value_free(rootValue);

    if (status != JSONSuccess)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 2);
    }

    return ADUC_RESULT2_SUCCESS;
}

const char* ADUC_LogManifest_GetFormat(ADUC_LogManifestHandle handle, uint16_t eventId)
{
    if (handle == NULL)
    {
        return NULL;
    }

    for (size_t i = 0; i < handle->count; i++)
    {
        if (handle->entries[i].eventId == eventId)
        {
            return handle->entries[i].formatString;
        }
    }
    return NULL;
}

size_t ADUC_LogManifest_GetCount(ADUC_LogManifestHandle handle)
{
    if (handle == NULL)
    {
        return 0;
    }
    return handle->count;
}

void ADUC_LogManifest_Destroy(ADUC_LogManifestHandle handle)
{
    if (handle == NULL)
    {
        return;
    }

    for (size_t i = 0; i < handle->count; i++)
    {
        free_entry(&handle->entries[i]);
    }
    free(handle->entries);
    free(handle);
}
