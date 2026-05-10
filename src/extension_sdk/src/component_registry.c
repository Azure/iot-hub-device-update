/**
 * @file component_registry.c
 * @brief Component registry implementation — aggregates enumerators.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/component_registry.h"

#include <stdlib.h>
#include <string.h>

#define MAX_ENUMERATORS 8
#define MAX_COMPONENTS 256

struct ADUC_ComponentRegistry
{
    const ADUC_ComponentEnumeratorVtable* enumerators[MAX_ENUMERATORS];
    size_t enumeratorCount;

    ADUC_ComponentInfo* components;
    size_t componentCount;
};

ADUC_Result2 ADUC_ComponentRegistry_Create(ADUC_ComponentRegistryHandle* outHandle)
{
    if (outHandle == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 1);
    }

    struct ADUC_ComponentRegistry* reg = calloc(1, sizeof(*reg));
    if (reg == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 2);
    }

    *outHandle = reg;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_ComponentRegistry_AddEnumerator(
    ADUC_ComponentRegistryHandle handle,
    const ADUC_ComponentEnumeratorVtable* vtable)
{
    if (handle == NULL || vtable == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 1);
    }
    if (handle->enumeratorCount >= MAX_ENUMERATORS)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 3);
    }

    handle->enumerators[handle->enumeratorCount++] = vtable;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_ComponentRegistry_Refresh(ADUC_ComponentRegistryHandle handle)
{
    if (handle == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 1);
    }

    /* Free previous inventory */
    free(handle->components);
    handle->components = NULL;
    handle->componentCount = 0;

    /* Temporary buffer to accumulate results */
    ADUC_ComponentInfo* merged = calloc(MAX_COMPONENTS, sizeof(ADUC_ComponentInfo));
    if (merged == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 2);
    }

    size_t total = 0;

    for (size_t i = 0; i < handle->enumeratorCount; i++)
    {
        const ADUC_ComponentEnumeratorVtable* vt = handle->enumerators[i];

        /* Call Refresh on each enumerator first */
        if (vt->Refresh != NULL)
        {
            vt->Refresh();
        }

        ADUC_ComponentInfo* enumComponents = NULL;
        size_t enumCount = 0;
        ADUC_Result2 res = vt->Enumerate(&enumComponents, &enumCount);
        if (ADUC_RESULT2_IS_FAILURE(res))
        {
            continue;
        }

        for (size_t j = 0; j < enumCount && total < MAX_COMPONENTS; j++)
        {
            merged[total++] = enumComponents[j];
        }

        if (vt->FreeComponents != NULL)
        {
            vt->FreeComponents(enumComponents, enumCount);
        }
    }

    handle->components = merged;
    handle->componentCount = total;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_ComponentRegistry_GetAll(
    ADUC_ComponentRegistryHandle handle,
    ADUC_ComponentInfo** outComponents,
    size_t* outCount)
{
    if (handle == NULL || outComponents == NULL || outCount == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 1);
    }

    if (handle->componentCount == 0)
    {
        *outComponents = NULL;
        *outCount = 0;
        return ADUC_RESULT2_SUCCESS;
    }

    size_t sz = handle->componentCount * sizeof(ADUC_ComponentInfo);
    ADUC_ComponentInfo* copy = malloc(sz);
    if (copy == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 2);
    }

    memcpy(copy, handle->components, sz);
    *outComponents = copy;
    *outCount = handle->componentCount;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_ComponentRegistry_FindByGroup(
    ADUC_ComponentRegistryHandle handle,
    const char* group,
    ADUC_ComponentInfo** outComponents,
    size_t* outCount)
{
    if (handle == NULL || group == NULL || outComponents == NULL || outCount == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 1);
    }

    /* Count matches */
    size_t matchCount = 0;
    for (size_t i = 0; i < handle->componentCount; i++)
    {
        if (strcmp(handle->components[i].group, group) == 0)
        {
            matchCount++;
        }
    }

    if (matchCount == 0)
    {
        *outComponents = NULL;
        *outCount = 0;
        return ADUC_RESULT2_SUCCESS;
    }

    ADUC_ComponentInfo* matches = malloc(matchCount * sizeof(ADUC_ComponentInfo));
    if (matches == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 2);
    }

    size_t idx = 0;
    for (size_t i = 0; i < handle->componentCount; i++)
    {
        if (strcmp(handle->components[i].group, group) == 0)
        {
            matches[idx++] = handle->components[i];
        }
    }

    *outComponents = matches;
    *outCount = matchCount;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_ComponentRegistry_FindById(
    ADUC_ComponentRegistryHandle handle,
    const char* componentId,
    ADUC_ComponentInfo* outComponent)
{
    if (handle == NULL || componentId == NULL || outComponent == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 1);
    }

    for (size_t i = 0; i < handle->componentCount; i++)
    {
        if (strcmp(handle->components[i].id, componentId) == 0)
        {
            *outComponent = handle->components[i];
            return ADUC_RESULT2_SUCCESS;
        }
    }

    return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_STATE, 1);
}

size_t ADUC_ComponentRegistry_GetCount(ADUC_ComponentRegistryHandle handle)
{
    if (handle == NULL)
    {
        return 0;
    }
    return handle->componentCount;
}

void ADUC_ComponentRegistry_FreeComponents(ADUC_ComponentInfo* components, size_t count)
{
    (void)count;
    free(components);
}

void ADUC_ComponentRegistry_Destroy(ADUC_ComponentRegistryHandle handle)
{
    if (handle == NULL)
    {
        return;
    }
    free(handle->components);
    free(handle);
}
