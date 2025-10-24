/**
 * @file aduc_string_utils.c
 * @brief String utility functions for Azure Device Update Core SDK
 * 
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#include "aduc/string_utils.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/**
 * @brief Safe string duplication.
 */
char* ADUC_String_Clone(const char* source)
{
    if (source == NULL)
    {
        return NULL;
    }
    
    return strdup(source);
}

/**
 * @brief Safe string comparison.
 */
int ADUC_String_Compare(const char* str1, const char* str2)
{
    if (str1 == NULL && str2 == NULL)
    {
        return 0;
    }
    
    if (str1 == NULL)
    {
        return -1;
    }
    
    if (str2 == NULL)
    {
        return 1;
    }
    
    return strcmp(str1, str2);
}

/**
 * @brief Case-insensitive string comparison.
 */
int ADUC_String_CompareIgnoreCase(const char* str1, const char* str2)
{
    if (str1 == NULL && str2 == NULL)
    {
        return 0;
    }
    
    if (str1 == NULL)
    {
        return -1;
    }
    
    if (str2 == NULL)
    {
        return 1;
    }
    
    return strcasecmp(str1, str2);
}

/**
 * @brief Check if string is null or empty.
 */
bool ADUC_String_IsNullOrEmpty(const char* str)
{
    return (str == NULL) || (str[0] == '\0');
}

/**
 * @brief Free string memory safely.
 */
void ADUC_String_Free(char* str)
{
    if (str != NULL)
    {
        free(str);
    }
}