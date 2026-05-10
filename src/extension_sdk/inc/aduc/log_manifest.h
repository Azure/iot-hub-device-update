/**
 * @file log_manifest.h
 * @brief Log manifest schema mapping event IDs to format strings.
 *
 * Enables compact binary logs to be reconstructed by mapping event IDs
 * to printf-style format strings, component names, and severity levels.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_LOG_MANIFEST_H
#define ADUC_LOG_MANIFEST_H

#include "aduc/extension_types.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief A single entry in the log manifest.
 */
typedef struct ADUC_LogManifestEntry
{
    uint16_t eventId;
    const char* component;
    ADUC_LogLevel level;
    const char* formatString;   /**< printf-style format */
    const char* description;    /**< Human-readable description */
} ADUC_LogManifestEntry;

/**
 * @brief Opaque log manifest handle.
 */
typedef struct ADUC_LogManifest* ADUC_LogManifestHandle;

/**
 * @brief Create a manifest from a static array of entries.
 *
 * Entries are copied internally.
 *
 * @param entries  Array of manifest entries.
 * @param count    Number of entries in the array.
 * @param outHandle  Receives the created manifest handle.
 * @return ADUC_Result2 Success or error code.
 */
ADUC_Result2 ADUC_LogManifest_Create(
    const ADUC_LogManifestEntry* entries,
    size_t count,
    ADUC_LogManifestHandle* outHandle);

/**
 * @brief Load a manifest from a JSON file.
 *
 * The JSON file should contain an array of objects, each with:
 * "eventId", "component", "level", "formatString", "description".
 *
 * @param path       Path to JSON file.
 * @param outHandle  Receives the created manifest handle.
 * @return ADUC_Result2 Success or error code.
 */
ADUC_Result2 ADUC_LogManifest_LoadFromFile(const char* path, ADUC_LogManifestHandle* outHandle);

/**
 * @brief Save a manifest to a JSON file.
 *
 * @param handle  Manifest handle.
 * @param path    Output file path.
 * @return ADUC_Result2 Success or error code.
 */
ADUC_Result2 ADUC_LogManifest_SaveToFile(ADUC_LogManifestHandle handle, const char* path);

/**
 * @brief Look up the format string for a given event ID.
 *
 * @param handle   Manifest handle.
 * @param eventId  Event ID to look up.
 * @return Format string, or NULL if not found.
 */
const char* ADUC_LogManifest_GetFormat(ADUC_LogManifestHandle handle, uint16_t eventId);

/**
 * @brief Get the number of entries in the manifest.
 *
 * @param handle  Manifest handle.
 * @return Entry count, or 0 if handle is NULL.
 */
size_t ADUC_LogManifest_GetCount(ADUC_LogManifestHandle handle);

/**
 * @brief Destroy a manifest and free all resources.
 *
 * @param handle  Manifest handle (may be NULL).
 */
void ADUC_LogManifest_Destroy(ADUC_LogManifestHandle handle);

#ifdef __cplusplus
}
#endif

#endif // ADUC_LOG_MANIFEST_H
