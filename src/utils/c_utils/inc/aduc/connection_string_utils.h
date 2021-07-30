/**
 * @file connection_string_utils.h
 * @brief connection string utilities.
 *
 * @copyright Copyright (c) 2021, Microsoft Corp.
 */
#ifndef CONNECTION_STRING_UTILS_H
#define CONNECTION_STRING_UTILS_H

#include "aduc/c_utils.h"
#include <stdbool.h>

EXTERN_C_BEGIN

/**
 * @brief Determines if the given key exists in the connection string.
 * @param[in] connectionString The connection string from the connection info.
 * @param[in] key The key in question.
 * @return Whether the key was found in the connection string.
 */
_Bool ConnectionStringUtils_DoesKeyExist(const char* connectionString, const char* key);

/**
 * @brief Gets the value for the given key out of the connection string.
 * @param[in] connectionString The connection string from the connection info.
 * @param[in] key The key for the value in question.
 * @param[out] A copy of the value for the key allocated with malloc, or NULL if not found.
 * @return true on success; false otherwise.
 */
_Bool ConnectionStringUtils_GetValue(const char* connectionString, const char* key, char** value);

/**
 * @brief Determines if the connection string indicates nested edge connectivity.
 * @param[in] connectionString the connection string from the connection info.
 */
_Bool ConnectionStringUtils_IsNestedEdge(const char* connectionString);

EXTERN_C_END

#endif // CONNECTION_STRING_UTILS_H
