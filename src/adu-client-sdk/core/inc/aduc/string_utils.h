/**
 * @file string_utils.h
 * @brief String utility functions for Azure Device Update Core SDK
 * 
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#ifndef ADUC_STRING_UTILS_H
#define ADUC_STRING_UTILS_H

#include <stdbool.h>
#include "aduc/exports.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Safe string duplication.
 * 
 * @param source Source string to duplicate
 * @return Duplicated string or NULL if source is NULL. Caller must free.
 */
ADUC_SDK_EXPORT char* ADUC_String_Clone(const char* source);

/**
 * @brief Safe string comparison.
 * 
 * @param str1 First string
 * @param str2 Second string
 * @return 0 if equal, < 0 if str1 < str2, > 0 if str1 > str2
 */
ADUC_SDK_EXPORT int ADUC_String_Compare(const char* str1, const char* str2);

/**
 * @brief Case-insensitive string comparison.
 * 
 * @param str1 First string
 * @param str2 Second string
 * @return 0 if equal, < 0 if str1 < str2, > 0 if str1 > str2
 */
ADUC_SDK_EXPORT int ADUC_String_CompareIgnoreCase(const char* str1, const char* str2);

/**
 * @brief Check if string is null or empty.
 * 
 * @param str String to check
 * @return true if string is NULL or empty, false otherwise
 */
ADUC_SDK_EXPORT bool ADUC_String_IsNullOrEmpty(const char* str);

/**
 * @brief Free string memory safely.
 * 
 * @param str String to free (can be NULL)
 */
ADUC_SDK_EXPORT void ADUC_String_Free(char* str);

#ifdef __cplusplus
}
#endif

#endif // ADUC_STRING_UTILS_H