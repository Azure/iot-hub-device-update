/**
 * @file extension_loader.h
 * @brief Extension loader and registry public API.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_EXTENSION_LOADER_H
#define ADUC_EXTENSION_LOADER_H

#include "aduc/extension_context.h"
#include "aduc/extension_descriptor.h"
#include "aduc/extension_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Opaque extension registry handle.
 */
typedef struct ADUC_ExtensionRegistry* ADUC_ExtensionRegistryHandle;

/**
 * @brief Create an empty extension registry.
 */
ADUC_Result2 ADUC_ExtensionRegistry_Create(ADUC_ExtensionRegistryHandle* outHandle);

/**
 * @brief Load a single extension from a shared library file.
 *
 * @param registry   Registry to add the extension to.
 * @param libraryPath  Path to .so/.dll file.
 * @param ctx        Host context to pass to extension's Initialize().
 * @return ADUC_Result2 Success or failure.
 */
ADUC_Result2 ADUC_ExtensionRegistry_LoadFromFile(
    ADUC_ExtensionRegistryHandle registry,
    const char* libraryPath,
    const ADUC_ExtensionContext* ctx);

/**
 * @brief Scan a directory for extension libraries and load them all.
 *
 * Looks for .so files (Linux) or .dll files (Windows) in the given directory.
 * Each must export ADUC_GetExtensionDescriptor.
 *
 * @param registry   Registry to add extensions to.
 * @param dirPath    Directory to scan.
 * @param ctx        Host context.
 * @return ADUC_Result2 Success if at least one loaded, failure if none.
 */
ADUC_Result2 ADUC_ExtensionRegistry_ScanDirectory(
    ADUC_ExtensionRegistryHandle registry,
    const char* dirPath,
    const ADUC_ExtensionContext* ctx);

/**
 * @brief Load extension from a directory with manifest-based verification.
 *
 * Reads the signing policy from agent configuration (security.extension_signing),
 * calls ADUC_Extension_Verify() on the extension directory, then loads the
 * library specified in the manifest.
 *
 * @param registry        Registry to add the extension to.
 * @param extensionDir    Directory containing manifest.json and library.
 * @param trustedCertPath Path to trusted signing certificate (may be NULL).
 * @param policy          Signing verification policy.
 * @param ctx             Host context.
 * @return ADUC_Result2 Success or failure.
 */
ADUC_Result2 ADUC_ExtensionRegistry_LoadVerified(
    ADUC_ExtensionRegistryHandle registry,
    const char* extensionDir,
    const char* trustedCertPath,
    int policy,
    const ADUC_ExtensionContext* ctx);

/**
 * @brief Find an extension by its unique ID.
 * @return Descriptor pointer or NULL if not found.
 */
const ADUC_ExtensionDescriptor* ADUC_ExtensionRegistry_FindById(
    ADUC_ExtensionRegistryHandle registry,
    const char* extensionId);

/**
 * @brief Find extensions by type (indexed, for iteration).
 * @param index  0-based index among extensions of this type.
 * @return Descriptor pointer or NULL if index out of range.
 */
const ADUC_ExtensionDescriptor* ADUC_ExtensionRegistry_FindByType(
    ADUC_ExtensionRegistryHandle registry,
    ADUC_ExtensionType type,
    size_t index);

/**
 * @brief Find an extension by capability string.
 * @param capability  Capability to match (e.g., "microsoft/swupdate:2").
 * @return First matching descriptor, or NULL.
 */
const ADUC_ExtensionDescriptor* ADUC_ExtensionRegistry_FindByCapability(
    ADUC_ExtensionRegistryHandle registry,
    const char* capability);

/**
 * @brief Find all extensions of a given type.
 * @param registry  Registry handle.
 * @param type      Extension type to filter by.
 * @param outCount  Output: number of matching descriptors.
 * @return Caller-owned array of descriptor pointers (free with free()), or NULL if none.
 */
const ADUC_ExtensionDescriptor** ADUC_ExtensionRegistry_FindAllByType(
    ADUC_ExtensionRegistryHandle registry,
    ADUC_ExtensionType type,
    size_t* outCount);

/**
 * @brief Get total loaded extension count.
 */
size_t ADUC_ExtensionRegistry_GetCount(ADUC_ExtensionRegistryHandle registry);

/**
 * @brief Get count of extensions of a specific type.
 */
size_t ADUC_ExtensionRegistry_GetCountByType(ADUC_ExtensionRegistryHandle registry, ADUC_ExtensionType type);

/**
 * @brief Destroy registry, uninitialize all extensions, unload libraries.
 */
void ADUC_ExtensionRegistry_Destroy(ADUC_ExtensionRegistryHandle registry);

#ifdef __cplusplus
}
#endif

#endif // ADUC_EXTENSION_LOADER_H
