/**
 * @file static_enumerator.c
 * @brief Static file-based component enumerator implementation.
 *
 * Reads component definitions from a TOML file with the format:
 *   [[components]]
 *   id = "firmware-main"
 *   name = "Main Firmware"
 *   group = "firmware"
 *   manufacturer = "Contoso"
 *   model = "Widget-1000"
 *   version = "1.2.3"
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "static_enumerator.h"
#include "aduc/component_enumerator_vtable.h"
#include "aduc/extension_descriptor.h"
#include "aduc/extension_types.h"

#include <toml.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_CONFIG_PATH "/etc/adu/components.toml"
#define MAX_STATIC_COMPONENTS 64

static char s_configPath[512] = DEFAULT_CONFIG_PATH;

void StaticEnumerator_SetConfigPath(const char* path)
{
    if (path != NULL)
    {
        strncpy(s_configPath, path, sizeof(s_configPath) - 1);
        s_configPath[sizeof(s_configPath) - 1] = '\0';
    }
}

static void copy_toml_string(toml_table_t* tbl, const char* key, char* dest, size_t destLen)
{
    toml_datum_t val = toml_string_in(tbl, key);
    if (val.ok)
    {
        strncpy(dest, val.u.s, destLen - 1);
        dest[destLen - 1] = '\0';
        free(val.u.s);
    }
}

static ADUC_Result2 StaticEnumerator_Enumerate(
    ADUC_ComponentInfo** outComponents,
    size_t* outCount)
{
    if (outComponents == NULL || outCount == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 1);
    }

    *outComponents = NULL;
    *outCount = 0;

    FILE* fp = fopen(s_configPath, "r");
    if (fp == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 2);
    }

    char errbuf[256];
    toml_table_t* root = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (root == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 3);
    }

    toml_array_t* components = toml_array_in(root, "components");
    if (components == NULL)
    {
        toml_free(root);
        return ADUC_RESULT2_SUCCESS;
    }

    int count = toml_array_nelem(components);
    if (count <= 0)
    {
        toml_free(root);
        return ADUC_RESULT2_SUCCESS;
    }

    if ((size_t)count > MAX_STATIC_COMPONENTS)
    {
        count = MAX_STATIC_COMPONENTS;
    }

    ADUC_ComponentInfo* result = calloc((size_t)count, sizeof(ADUC_ComponentInfo));
    if (result == NULL)
    {
        toml_free(root);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 2);
    }

    for (int i = 0; i < count; i++)
    {
        toml_table_t* comp = toml_table_at(components, i);
        if (comp == NULL)
        {
            continue;
        }

        copy_toml_string(comp, "id", result[i].id, sizeof(result[i].id));
        copy_toml_string(comp, "name", result[i].name, sizeof(result[i].name));
        copy_toml_string(comp, "group", result[i].group, sizeof(result[i].group));
        copy_toml_string(comp, "manufacturer", result[i].manufacturer, sizeof(result[i].manufacturer));
        copy_toml_string(comp, "model", result[i].model, sizeof(result[i].model));
        copy_toml_string(comp, "version", result[i].version, sizeof(result[i].version));

        /* Parse [components.properties] sub-table */
        toml_table_t* propsTbl = toml_table_in(comp, "properties");
        if (propsTbl != NULL)
        {
            int nkeys = toml_table_nkval(propsTbl);
            if (nkeys > 0)
            {
                ADUC_PropertyEntry* entries = calloc((size_t)nkeys, sizeof(ADUC_PropertyEntry));
                if (entries != NULL)
                {
                    int propIdx = 0;
                    for (int k = 0; ; k++)
                    {
                        const char* pkey = toml_key_in(propsTbl, k);
                        if (pkey == NULL)
                        {
                            break;
                        }
                        toml_datum_t pval = toml_string_in(propsTbl, pkey);
                        if (pval.ok)
                        {
                            entries[propIdx].key = strdup(pkey);
                            entries[propIdx].value = pval.u.s; /* already malloc'd by tomlc99 */
                            propIdx++;
                        }
                    }
                    result[i].properties.entries = entries;
                    result[i].properties.count = (size_t)propIdx;
                }
                else
                {
                    result[i].properties.entries = NULL;
                    result[i].properties.count = 0;
                }
            }
            else
            {
                result[i].properties.entries = NULL;
                result[i].properties.count = 0;
            }
        }
        else
        {
            result[i].properties.entries = NULL;
            result[i].properties.count = 0;
        }
    }

    *outComponents = result;
    *outCount = (size_t)count;

    toml_free(root);
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 StaticEnumerator_GetById(
    const char* componentId,
    ADUC_ComponentInfo* outComponent)
{
    if (componentId == NULL || outComponent == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 1);
    }

    ADUC_ComponentInfo* components = NULL;
    size_t count = 0;
    ADUC_Result2 res = StaticEnumerator_Enumerate(&components, &count);
    if (ADUC_RESULT2_IS_FAILURE(res))
    {
        return res;
    }

    for (size_t i = 0; i < count; i++)
    {
        if (strcmp(components[i].id, componentId) == 0)
        {
            *outComponent = components[i];
            free(components);
            return ADUC_RESULT2_SUCCESS;
        }
    }

    free(components);
    return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_STATE, 1);
}

static bool StaticEnumerator_MatchesCriteria(
    const ADUC_ComponentInfo* component,
    const char* criteria)
{
    if (component == NULL || criteria == NULL)
    {
        return false;
    }
    /* Simple group match */
    return (strcmp(component->group, criteria) == 0);
}

static ADUC_Result2 StaticEnumerator_GetInstalledVersion(
    const char* componentId,
    char* versionBuf,
    size_t bufLen)
{
    if (componentId == NULL || versionBuf == NULL || bufLen == 0)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 1);
    }

    ADUC_ComponentInfo comp;
    memset(&comp, 0, sizeof(comp));
    ADUC_Result2 res = StaticEnumerator_GetById(componentId, &comp);
    if (ADUC_RESULT2_IS_FAILURE(res))
    {
        return res;
    }

    strncpy(versionBuf, comp.version, bufLen - 1);
    versionBuf[bufLen - 1] = '\0';
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 StaticEnumerator_Refresh(void)
{
    /* Static file enumerator re-reads the file on each Enumerate call. */
    return ADUC_RESULT2_SUCCESS;
}

static void StaticEnumerator_FreeComponents(ADUC_ComponentInfo* components, size_t count)
{
    if (components == NULL)
    {
        return;
    }
    for (size_t i = 0; i < count; i++)
    {
        if (components[i].properties.entries != NULL)
        {
            for (size_t j = 0; j < components[i].properties.count; j++)
            {
                free((void*)components[i].properties.entries[j].key);
                free((void*)components[i].properties.entries[j].value);
            }
            free(components[i].properties.entries);
        }
    }
    free(components);
}

static const ADUC_ComponentEnumeratorVtable s_vtable = {
    .Enumerate = StaticEnumerator_Enumerate,
    .GetById = StaticEnumerator_GetById,
    .MatchesCriteria = StaticEnumerator_MatchesCriteria,
    .GetInstalledVersion = StaticEnumerator_GetInstalledVersion,
    .Refresh = StaticEnumerator_Refresh,
    .FreeComponents = StaticEnumerator_FreeComponents,
};

const ADUC_ComponentEnumeratorVtable* StaticEnumerator_GetVtable(void)
{
    return &s_vtable;
}

/* Extension descriptor */
static ADUC_Result2 StaticEnumerator_Initialize(const ADUC_ExtensionContext* ctx)
{
    (void)ctx;
    return ADUC_RESULT2_SUCCESS;
}

static void StaticEnumerator_Uninitialize(void)
{
}

static const char* s_capabilities[] = { "static-file-enumerator", NULL };

static const ADUC_ExtensionDescriptor s_descriptor = {
    .structVersion = ADUC_EXTENSION_DESCRIPTOR_VERSION,
    .id = "com.microsoft.adu.component-enumerator.static-file",
    .name = "Static File Component Enumerator",
    .version = "1.0.0",
    .type = ADUC_EXT_TYPE_COMPONENT_ENUMERATOR,
    .minHostApiVersion = ADUC_HOST_API_VERSION,
    .Initialize = StaticEnumerator_Initialize,
    .Uninitialize = StaticEnumerator_Uninitialize,
    .vtable = &s_vtable,
    .capabilities = s_capabilities,
};

ADUC_DECLARE_EXTENSION(s_descriptor)
