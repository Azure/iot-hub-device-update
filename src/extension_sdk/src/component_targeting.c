/**
 * @file component_targeting.c
 * @brief Evaluate property-based targeting expressions against components.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/component_targeting.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/** Case-insensitive substring search for " AND " delimiter. */
static const char* find_and_delimiter(const char* s)
{
    while (*s != '\0')
    {
        if (s[0] == ' '
            && (s[1] == 'A' || s[1] == 'a')
            && (s[2] == 'N' || s[2] == 'n')
            && (s[3] == 'D' || s[3] == 'd')
            && s[4] == ' ')
        {
            return s;
        }
        s++;
    }
    return NULL;
}

/**
 * @brief Get a component field value by key name.
 * Returns NULL if key is not a known field or property.
 */
static const char* get_field_value(
    const ADUC_ComponentInfo* component,
    const char* key,
    size_t keyLen)
{
    if (keyLen == 2 && strncmp(key, "id", 2) == 0)
    {
        return component->id;
    }
    if (keyLen == 4 && strncmp(key, "name", 4) == 0)
    {
        return component->name;
    }
    if (keyLen == 5 && strncmp(key, "group", 5) == 0)
    {
        return component->group;
    }
    if (keyLen == 12 && strncmp(key, "manufacturer", 12) == 0)
    {
        return component->manufacturer;
    }
    if (keyLen == 5 && strncmp(key, "model", 5) == 0)
    {
        return component->model;
    }
    if (keyLen == 7 && strncmp(key, "version", 7) == 0)
    {
        return component->version;
    }

    /* Check properties map */
    if (component->properties.entries != NULL)
    {
        for (size_t i = 0; i < component->properties.count; i++)
        {
            const char* propKey = component->properties.entries[i].key;
            if (propKey != NULL && strlen(propKey) == keyLen && strncmp(propKey, key, keyLen) == 0)
            {
                return component->properties.entries[i].value;
            }
        }
    }

    return NULL;
}

/**
 * @brief Evaluate a single clause "key=value" against a component.
 */
static bool eval_clause(
    const ADUC_ComponentInfo* component,
    const char* clause,
    size_t clauseLen)
{
    /* Find '=' */
    const char* eq = memchr(clause, '=', clauseLen);
    if (eq == NULL)
    {
        return false;
    }

    size_t keyLen = (size_t)(eq - clause);
    /* Trim trailing spaces from key */
    while (keyLen > 0 && clause[keyLen - 1] == ' ')
    {
        keyLen--;
    }
    if (keyLen == 0)
    {
        return false;
    }

    const char* valStart = eq + 1;
    size_t remaining = clauseLen - (size_t)(valStart - clause);
    /* Trim leading spaces from value */
    while (remaining > 0 && *valStart == ' ')
    {
        valStart++;
        remaining--;
    }
    /* Trim trailing spaces from value */
    while (remaining > 0 && valStart[remaining - 1] == ' ')
    {
        remaining--;
    }
    if (remaining == 0)
    {
        return false;
    }

    const char* fieldVal = get_field_value(component, clause, keyLen);
    if (fieldVal == NULL)
    {
        return false;
    }

    /* Wildcard: "key=*" matches any non-empty value */
    if (remaining == 1 && valStart[0] == '*')
    {
        return (fieldVal[0] != '\0');
    }

    return (strlen(fieldVal) == remaining && strncmp(fieldVal, valStart, remaining) == 0);
}

/**
 * @brief Validate a single clause has "key=value" form.
 */
static bool validate_clause(const char* clause, size_t clauseLen)
{
    const char* eq = memchr(clause, '=', clauseLen);
    if (eq == NULL)
    {
        return false;
    }

    size_t keyLen = (size_t)(eq - clause);
    while (keyLen > 0 && clause[keyLen - 1] == ' ')
    {
        keyLen--;
    }
    if (keyLen == 0)
    {
        return false;
    }

    const char* valStart = eq + 1;
    size_t remaining = clauseLen - (size_t)(valStart - clause);
    while (remaining > 0 && *valStart == ' ')
    {
        valStart++;
        remaining--;
    }
    while (remaining > 0 && valStart[remaining - 1] == ' ')
    {
        remaining--;
    }

    return remaining > 0;
}

bool ADUC_Targeting_Matches(const ADUC_ComponentInfo* component, const char* expression)
{
    if (component == NULL || expression == NULL || *expression == '\0')
    {
        return false;
    }

    const char* cursor = expression;

    while (*cursor != '\0')
    {
        const char* delim = find_and_delimiter(cursor);
        size_t clauseLen;
        if (delim != NULL)
        {
            clauseLen = (size_t)(delim - cursor);
        }
        else
        {
            clauseLen = strlen(cursor);
        }

        if (!eval_clause(component, cursor, clauseLen))
        {
            return false;
        }

        if (delim != NULL)
        {
            cursor = delim + 5; /* skip " AND " */
        }
        else
        {
            break;
        }
    }

    return true;
}

bool ADUC_Targeting_IsValid(const char* expression)
{
    if (expression == NULL || *expression == '\0')
    {
        return false;
    }

    const char* cursor = expression;

    while (*cursor != '\0')
    {
        const char* delim = find_and_delimiter(cursor);
        size_t clauseLen;
        if (delim != NULL)
        {
            clauseLen = (size_t)(delim - cursor);
        }
        else
        {
            clauseLen = strlen(cursor);
        }

        if (!validate_clause(cursor, clauseLen))
        {
            return false;
        }

        if (delim != NULL)
        {
            cursor = delim + 5;
        }
        else
        {
            break;
        }
    }

    return true;
}
