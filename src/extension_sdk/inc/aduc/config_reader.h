/**
 * @file config_reader.h
 * @brief TOML-based layered configuration reader for ADU Gen2.
 *
 * Reads configuration from:
 *   1. Factory defaults (/usr/lib/adu/defaults.conf)
 *   2. Agent config (/etc/adu/adu-agent.conf)
 *   3. Drop-in overrides (/etc/adu/conf.d/ *.conf) — numbered priority
 *
 * Higher-numbered files override lower. Deep merge for tables, replace for values.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_CONFIG_READER_H
#define ADUC_CONFIG_READER_H

#include "aduc/extension_types.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Opaque config handle.
 */
typedef struct ADUC_Config* ADUC_ConfigHandle;

/**
 * @brief Load configuration from the standard paths.
 *
 * @param configDir  Base config directory (default: "/etc/adu").
 *                   Pass NULL to use default.
 * @param outHandle  Output config handle.
 * @return ADUC_Result2 Success or failure.
 */
ADUC_Result2 ADUC_Config_Load(const char* configDir, ADUC_ConfigHandle* outHandle);

/**
 * @brief Load configuration from a single TOML file (for testing).
 *
 * @param filePath   Path to TOML file.
 * @param outHandle  Output config handle.
 * @return ADUC_Result2 Success or failure.
 */
ADUC_Result2 ADUC_Config_LoadFile(const char* filePath, ADUC_ConfigHandle* outHandle);

/**
 * @brief Get a string value by dotted key path.
 *
 * @param handle  Config handle.
 * @param key     Dotted key path (e.g., "agent.device_id", "logging.min_level").
 * @return Value string, or NULL if not found. Valid until config is freed.
 */
const char* ADUC_Config_GetString(ADUC_ConfigHandle handle, const char* key);

/**
 * @brief Get an integer value by dotted key path.
 *
 * @param handle    Config handle.
 * @param key       Dotted key path.
 * @param outValue  Output integer value.
 * @return ADUC_Result2 Success or failure (key not found, wrong type).
 */
ADUC_Result2 ADUC_Config_GetInt(ADUC_ConfigHandle handle, const char* key, int64_t* outValue);

/**
 * @brief Get a boolean value by dotted key path.
 *
 * @param handle    Config handle.
 * @param key       Dotted key path.
 * @param outValue  Output boolean value.
 * @return ADUC_Result2 Success or failure.
 */
ADUC_Result2 ADUC_Config_GetBool(ADUC_ConfigHandle handle, const char* key, bool* outValue);

/**
 * @brief Get a string array value by dotted key path.
 *
 * @param handle    Config handle.
 * @param key       Dotted key path.
 * @param outArray  Output array of strings (valid until config freed).
 * @param outCount  Number of elements.
 * @return ADUC_Result2 Success or failure.
 */
ADUC_Result2 ADUC_Config_GetStringArray(ADUC_ConfigHandle handle, const char* key,
                                         const char*** outArray, size_t* outCount);

/**
 * @brief Check if a key exists.
 */
bool ADUC_Config_HasKey(ADUC_ConfigHandle handle, const char* key);

/**
 * @brief Free configuration handle and all associated memory.
 */
void ADUC_Config_Free(ADUC_ConfigHandle handle);

#ifdef __cplusplus
}
#endif

#endif // ADUC_CONFIG_READER_H
