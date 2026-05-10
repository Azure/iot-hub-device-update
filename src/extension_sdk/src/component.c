/**
 * @file component.c
 * @brief Property-based component matching implementation.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/component.h"

#include <string.h>

/**
 * @brief Get the value of a built-in or custom component field by key.
 *
 * Built-in fields: id, name, group, manufacturer, model, installedVersion.
 * Falls back to the component's properties array.
 *
 * @return Pointer to the value string, or NULL if not found.
 */
static const char* get_component_field(const ADUC_Component* component, const char* key)
{
    if (strcmp(key, "id") == 0)
    {
        return component->id;
    }
    if (strcmp(key, "name") == 0)
    {
        return component->name;
    }
    if (strcmp(key, "group") == 0)
    {
        return component->group;
    }
    if (strcmp(key, "manufacturer") == 0)
    {
        return component->manufacturer;
    }
    if (strcmp(key, "model") == 0)
    {
        return component->model;
    }
    if (strcmp(key, "installedVersion") == 0)
    {
        return component->installedVersion;
    }

    for (int i = 0; i < component->propertyCount; i++)
    {
        if (strcmp(component->properties[i].key, key) == 0)
        {
            return component->properties[i].value;
        }
    }

    return NULL;
}

bool ADUC_Component_MatchesProperties(
    const ADUC_Component* component,
    const ADUC_ComponentProperty* queryProps,
    int queryPropCount)
{
    if (component == NULL || queryProps == NULL || queryPropCount <= 0)
    {
        return false;
    }

    for (int i = 0; i < queryPropCount; i++)
    {
        const char* fieldVal = get_component_field(component, queryProps[i].key);
        if (fieldVal == NULL)
        {
            return false;
        }

        /* Wildcard: property must exist but value is don't-care */
        if (strcmp(queryProps[i].value, "*") == 0)
        {
            if (fieldVal[0] == '\0')
            {
                return false;
            }
            continue;
        }

        /* Exact match */
        if (strcmp(fieldVal, queryProps[i].value) != 0)
        {
            return false;
        }
    }

    return true;
}

int ADUC_Component_FindMatching(
    const ADUC_ComponentList* list,
    const ADUC_ComponentProperty* queryProps,
    int queryPropCount,
    const ADUC_Component** outMatches,
    int maxMatches)
{
    if (list == NULL || outMatches == NULL || maxMatches <= 0)
    {
        return 0;
    }

    int found = 0;

    for (int i = 0; i < list->count && found < maxMatches; i++)
    {
        if (ADUC_Component_MatchesProperties(&list->components[i], queryProps, queryPropCount))
        {
            outMatches[found++] = &list->components[i];
        }
    }

    return found;
}
