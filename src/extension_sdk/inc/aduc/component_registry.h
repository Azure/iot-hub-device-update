/**
 * @file component_registry.h
 * @brief Aggregates multiple component enumerators into a unified inventory.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_COMPONENT_REGISTRY_H
#define ADUC_COMPONENT_REGISTRY_H

#include "aduc/component_enumerator_vtable.h"
#include "aduc/extension_types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Opaque handle to a component registry. */
typedef struct ADUC_ComponentRegistry* ADUC_ComponentRegistryHandle;

/** Create a new component registry. */
ADUC_Result2 ADUC_ComponentRegistry_Create(ADUC_ComponentRegistryHandle* outHandle);

/** Add an enumerator extension to the registry. */
ADUC_Result2 ADUC_ComponentRegistry_AddEnumerator(
    ADUC_ComponentRegistryHandle handle,
    const ADUC_ComponentEnumeratorVtable* vtable);

/** Refresh all enumerators and rebuild the inventory. */
ADUC_Result2 ADUC_ComponentRegistry_Refresh(ADUC_ComponentRegistryHandle handle);

/** Get all components. Caller must free with FreeComponents. */
ADUC_Result2 ADUC_ComponentRegistry_GetAll(
    ADUC_ComponentRegistryHandle handle,
    ADUC_ComponentInfo** outComponents,
    size_t* outCount);

/** Find components matching a group. Caller must free with FreeComponents. */
ADUC_Result2 ADUC_ComponentRegistry_FindByGroup(
    ADUC_ComponentRegistryHandle handle,
    const char* group,
    ADUC_ComponentInfo** outComponents,
    size_t* outCount);

/** Find a single component by ID. */
ADUC_Result2 ADUC_ComponentRegistry_FindById(
    ADUC_ComponentRegistryHandle handle,
    const char* componentId,
    ADUC_ComponentInfo* outComponent);

/** Get current component count. */
size_t ADUC_ComponentRegistry_GetCount(ADUC_ComponentRegistryHandle handle);

/** Free a component array returned by GetAll/FindByGroup. */
void ADUC_ComponentRegistry_FreeComponents(ADUC_ComponentInfo* components, size_t count);

/** Destroy the registry and free all resources. */
void ADUC_ComponentRegistry_Destroy(ADUC_ComponentRegistryHandle handle);

#ifdef __cplusplus
}
#endif

#endif /* ADUC_COMPONENT_REGISTRY_H */
