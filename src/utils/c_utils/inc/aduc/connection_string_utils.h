/**
 * @file connection_string_utils.h
 * @brief connection string utilities.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
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
 * @brief Parses the connection string for the ModuleId and allocates it into the provided buffer
 * @param[in] connectionString the connection string to scan for the module-id
 * @param[out] moduleIdHandle the handle to be allocated with the module-id value
 * @return true on success; false otherwise.
 */
_Bool ConnectionStringUtils_GetModuleIdFromConnectionString(const char* connectionString, char** moduleIdHandle);

/**
 * @brief Parses the connection string for the deviceId and allocates it into the provided buffer
 * @param[in] connectionString the connection string to scan for the device-id
 * @param[out] deviceIdHandle the handle to be allocated with the device-id value
 * @return true on success; false otherwise
 */
_Bool ConnectionStringUtils_GetDeviceIdFromConnectionString(const char* connectionString, char** deviceIdHandle);

/**
 * @brief Determines if the connection string indicates nested edge connectivity.
 * @param[in] connectionString the connection string from the connection info.
 */
_Bool ConnectionStringUtils_IsNestedEdge(const char* connectionString);

EXTERN_C_END

#endif // CONNECTION_STRING_UTILS_H
