/**
 * @file component_enumerator_vtable.h
 * @brief Extension vtable for component enumerators.
 *
 * Component enumerators are plugins that discover what updateable
 * components exist on the device.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_COMPONENT_ENUMERATOR_VTABLE_H
#define ADUC_COMPONENT_ENUMERATOR_VTABLE_H

#include "aduc/extension_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Component info returned by an enumerator.
 */
typedef struct ADUC_ComponentInfo
{
    char id[128];           /**< Unique component ID */
    char name[128];         /**< Display name */
    char group[64];         /**< Group/category (for targeting) */
    char manufacturer[64];
    char model[64];
    char version[32];       /**< Current installed version */
    ADUC_PropertyMap properties; /**< Additional key-value properties */
} ADUC_ComponentInfo;

/**
 * @brief Vtable for component enumerator extensions.
 */
typedef struct ADUC_ComponentEnumeratorVtable
{
    /** Enumerate all components. Caller must free with FreeComponents. */
    ADUC_Result2 (*Enumerate)(ADUC_ComponentInfo** outComponents, size_t* outCount);

    /** Get a specific component by ID. */
    ADUC_Result2 (*GetById)(const char* componentId, ADUC_ComponentInfo* outComponent);

    /** Check if a component matches targeting criteria. */
    bool (*MatchesCriteria)(const ADUC_ComponentInfo* component, const char* criteria);

    /** Get installed version for a component. */
    ADUC_Result2 (*GetInstalledVersion)(const char* componentId, char* versionBuf, size_t bufLen);

    /** Refresh/re-scan (e.g., after hotplug). */
    ADUC_Result2 (*Refresh)(void);

    /** Free component array from Enumerate. */
    void (*FreeComponents)(ADUC_ComponentInfo* components, size_t count);
} ADUC_ComponentEnumeratorVtable;

#ifdef __cplusplus
}
#endif

#endif /* ADUC_COMPONENT_ENUMERATOR_VTABLE_H */
