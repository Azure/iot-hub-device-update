/**
 * @file component.h
 * @brief Component model types for multi-component update targeting.
 *
 * Defines the flat component descriptor used by the property-based
 * targeting engine.  The static-array layout avoids heap allocations
 * for the common case where a device has a small number of components.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_COMPONENT_H
#define ADUC_COMPONENT_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define ADUC_MAX_COMPONENT_ID_LEN   128
#define ADUC_MAX_COMPONENT_NAME_LEN 256
#define ADUC_MAX_COMPONENT_GROUP_LEN 128
#define ADUC_MAX_COMPONENT_PROPS    32
#define ADUC_MAX_COMPONENTS         64

/**
 * @brief Component property (key-value pair).
 */
typedef struct ADUC_ComponentProperty
{
    char key[64];
    char value[256];
} ADUC_ComponentProperty;

/**
 * @brief Component descriptor.
 */
typedef struct ADUC_Component
{
    char id[ADUC_MAX_COMPONENT_ID_LEN];
    char name[ADUC_MAX_COMPONENT_NAME_LEN];
    char group[ADUC_MAX_COMPONENT_GROUP_LEN];
    char manufacturer[128];
    char model[128];
    char installedVersion[64];
    ADUC_ComponentProperty properties[ADUC_MAX_COMPONENT_PROPS];
    int propertyCount;
    bool isUpdatable;
} ADUC_Component;

/**
 * @brief List of components (fixed-capacity).
 */
typedef struct ADUC_ComponentList
{
    ADUC_Component components[ADUC_MAX_COMPONENTS];
    int count;
} ADUC_ComponentList;

/**
 * @brief Check if a component matches a set of targeting properties.
 *
 * Returns true if ALL properties in the query match the component.
 * A query value of "*" matches any value (the property must exist).
 *
 * The search considers both built-in fields (id, name, group,
 * manufacturer, model, installedVersion) and the properties array.
 *
 * @param component      Component to test.
 * @param queryProps     Array of key-value properties to match.
 * @param queryPropCount Number of elements in queryProps.
 * @return true if the component matches all query properties.
 */
bool ADUC_Component_MatchesProperties(
    const ADUC_Component* component,
    const ADUC_ComponentProperty* queryProps,
    int queryPropCount);

/**
 * @brief Find components in a list that match a set of properties.
 *
 * @param list           Component list to search.
 * @param queryProps     Array of targeting properties.
 * @param queryPropCount Number of targeting properties.
 * @param outMatches     Array of pointers filled with matching components.
 * @param maxMatches     Capacity of outMatches.
 * @return Number of matching components written to outMatches.
 */
int ADUC_Component_FindMatching(
    const ADUC_ComponentList* list,
    const ADUC_ComponentProperty* queryProps,
    int queryPropCount,
    const ADUC_Component** outMatches,
    int maxMatches);

#ifdef __cplusplus
}
#endif

#endif /* ADUC_COMPONENT_H */
