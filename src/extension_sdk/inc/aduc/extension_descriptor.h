/**
 * @file extension_descriptor.h
 * @brief Unified extension descriptor — the single export contract for all ADU extensions.
 *
 * Every ADU Gen2 extension (.so / .dll) exports exactly one symbol:
 *   const ADUC_ExtensionDescriptor* ADUC_GetExtensionDescriptor(void);
 *
 * The descriptor provides identity, capabilities, lifecycle hooks, and a type-specific
 * vtable pointer that the host casts based on the extension type.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_EXTENSION_DESCRIPTOR_H
#define ADUC_EXTENSION_DESCRIPTOR_H

#include "aduc/extension_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Forward declaration of host-provided extension context.
 * Defined in extension_context.h — extensions receive this during Initialize().
 */
typedef struct ADUC_ExtensionContext ADUC_ExtensionContext;

/**
 * @brief Extension descriptor — the contract between host and extension.
 *
 * @note structVersion MUST be checked before accessing fields.
 *       Fields are append-only; new versions add fields at the end.
 */
typedef struct ADUC_ExtensionDescriptor
{
    /** ABI version of this struct (currently 1). */
    uint32_t structVersion;

    /** Unique extension identifier (reverse-DNS style).
     *  Example: "com.microsoft.adu.iothub-comm" */
    const char* id;

    /** Human-readable display name. */
    const char* name;

    /** Semantic version string (e.g., "1.2.0"). */
    const char* version;

    /** Extension type — determines how `vtable` is cast. */
    ADUC_ExtensionType type;

    /** Minimum host API version this extension requires. */
    uint32_t minHostApiVersion;

    /**
     * @brief Initialize the extension.
     * Called once after loading. The extension should store the context for later use.
     *
     * @param ctx Host-provided services (logging, config, secrets, etc.)
     * @return ADUC_Result2 Success or failure with structured error code.
     */
    ADUC_Result2 (*Initialize)(const ADUC_ExtensionContext* ctx);

    /**
     * @brief Uninitialize the extension.
     * Called before unloading. Release all resources.
     */
    void (*Uninitialize)(void);

    /**
     * @brief Type-specific vtable.
     * Cast to the appropriate vtable type based on `type`:
     *   ADUC_EXT_TYPE_COMMUNICATION    → const ADUC_CommunicationVtable*
     *   ADUC_EXT_TYPE_STEP_HANDLER     → const ADUC_StepHandlerVtable*
     *   ADUC_EXT_TYPE_CONTENT_PROCESSOR→ const ADUC_ContentProcessorVtable*
     *   ADUC_EXT_TYPE_DOWNLOADER       → const ADUC_DownloaderVtable*
     *   ADUC_EXT_TYPE_COMPONENT_ENUMERATOR → const ADUC_ComponentEnumeratorVtable*
     */
    const void* vtable;

    /**
     * @brief Capabilities declared by this extension (NULL-terminated string array).
     * Used for matching deployments to handlers.
     * Example: {"microsoft/swupdate:2", "microsoft/apt:1", NULL}
     */
    const char** capabilities;

} ADUC_ExtensionDescriptor;

/**
 * @brief The single export symbol every extension must provide.
 *
 * The host calls this after dlopen() to obtain the extension's descriptor.
 * The returned pointer must remain valid for the lifetime of the loaded library.
 */
typedef const ADUC_ExtensionDescriptor* (*ADUC_GetExtensionDescriptorFn)(void);

/** Export symbol name (string) for dlsym/GetProcAddress. */
#define ADUC_EXTENSION_EXPORT_SYMBOL "ADUC_GetExtensionDescriptor"

/**
 * @brief Current host API version.
 * Extensions declare minHostApiVersion; host rejects if extension requires > this.
 */
#define ADUC_HOST_API_VERSION 1

/**
 * @brief Current struct version for ADUC_ExtensionDescriptor.
 */
#define ADUC_EXTENSION_DESCRIPTOR_VERSION 1

/**
 * @brief Helper macro for extensions to declare their descriptor export.
 *
 * Usage in extension source:
 *   ADUC_DECLARE_EXTENSION(myDescriptor)
 * where myDescriptor is a static const ADUC_ExtensionDescriptor.
 */
#ifdef _WIN32
#define ADUC_DECLARE_EXTENSION(desc) \
    __declspec(dllexport) const ADUC_ExtensionDescriptor* ADUC_GetExtensionDescriptor(void) { return &(desc); }
#else
#define ADUC_DECLARE_EXTENSION(desc) \
    __attribute__((visibility("default"))) const ADUC_ExtensionDescriptor* ADUC_GetExtensionDescriptor(void) { return &(desc); }
#endif

#ifdef __cplusplus
}
#endif

#endif // ADUC_EXTENSION_DESCRIPTOR_H
