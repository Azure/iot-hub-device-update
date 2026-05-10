/**
 * @file extension_loader.c
 * @brief Extension loader — discovers, validates, and loads extensions.
 *
 * Scans extension directories for manifest files, verifies signatures/hashes,
 * loads shared libraries via dlopen, and calls ADUC_GetExtensionDescriptor().
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/extension_loader.h"
#include "aduc/extension_descriptor.h"
#include "aduc/extension_signing.h"

#include "aduc/platform.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <dirent.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Cross-platform library helpers */
#ifdef _WIN32
#define adu_dlclose(h) FreeLibrary((HMODULE)(h))
#define ADU_LIB_EXT ".dll"
#else
#define adu_dlclose(h) dlclose(h)
#define ADU_LIB_EXT ".so"
#endif

#define MAX_EXTENSIONS 64

typedef struct LoadedExtension
{
    void* libHandle;                   // dlopen handle
    const ADUC_ExtensionDescriptor* descriptor;
    bool initialized;
} LoadedExtension;

struct ADUC_ExtensionRegistry
{
    LoadedExtension extensions[MAX_EXTENSIONS];
    size_t count;
};

ADUC_Result2 ADUC_ExtensionRegistry_Create(ADUC_ExtensionRegistryHandle* outHandle)
{
    if (!outHandle)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_CONFIG, 1);
    }

    struct ADUC_ExtensionRegistry* registry = calloc(1, sizeof(struct ADUC_ExtensionRegistry));
    if (!registry)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 1);
    }

    *outHandle = registry;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_ExtensionRegistry_LoadFromFile(
    ADUC_ExtensionRegistryHandle registry,
    const char* libraryPath,
    const ADUC_ExtensionContext* ctx)
{
    if (!registry || !libraryPath)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_CONFIG, 1);
    }

    if (registry->count >= MAX_EXTENSIONS)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 2);
    }

    // Load shared library
#ifdef _WIN32
    void* lib = (void*)LoadLibraryA(libraryPath);
    if (!lib)
    {
        fprintf(stderr, "ExtensionLoader: LoadLibrary failed for %s (error=%lu)\n",
                libraryPath, GetLastError());
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 1);
    }

    // Find export symbol
    ADUC_GetExtensionDescriptorFn getDescriptor =
        (ADUC_GetExtensionDescriptorFn)GetProcAddress((HMODULE)lib, ADUC_EXTENSION_EXPORT_SYMBOL);
    if (!getDescriptor)
    {
        fprintf(stderr, "ExtensionLoader: symbol '%s' not found in %s\n",
                ADUC_EXTENSION_EXPORT_SYMBOL, libraryPath);
        FreeLibrary((HMODULE)lib);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_PROTOCOL, 1);
    }
#else
    void* lib = dlopen(libraryPath, RTLD_NOW | RTLD_LOCAL);
    if (!lib)
    {
        fprintf(stderr, "ExtensionLoader: dlopen failed for %s: %s\n", libraryPath, dlerror());
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 1);
    }

    // Find export symbol
    ADUC_GetExtensionDescriptorFn getDescriptor =
        (ADUC_GetExtensionDescriptorFn)dlsym(lib, ADUC_EXTENSION_EXPORT_SYMBOL);
    if (!getDescriptor)
    {
        fprintf(stderr, "ExtensionLoader: symbol '%s' not found in %s\n",
                ADUC_EXTENSION_EXPORT_SYMBOL, libraryPath);
        adu_dlclose(lib);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_PROTOCOL, 1);
    }
#endif

    // Get descriptor
    const ADUC_ExtensionDescriptor* desc = getDescriptor();
    if (!desc)
    {
        fprintf(stderr, "ExtensionLoader: GetDescriptor returned NULL for %s\n", libraryPath);
        adu_dlclose(lib);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_PROTOCOL, 2);
    }

    // Validate descriptor
    if (desc->structVersion < 1 || !desc->id || !desc->name || !desc->version)
    {
        fprintf(stderr, "ExtensionLoader: Invalid descriptor in %s\n", libraryPath);
        adu_dlclose(lib);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_PROTOCOL, 3);
    }

    // Check host API version compatibility
    if (desc->minHostApiVersion > ADUC_HOST_API_VERSION)
    {
        fprintf(stderr, "ExtensionLoader: %s requires host API v%u, we have v%u\n",
                desc->id, desc->minHostApiVersion, ADUC_HOST_API_VERSION);
        adu_dlclose(lib);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_PROTOCOL, 4);
    }

    // Initialize extension
    ADUC_Result2 initResult = ADUC_RESULT2_SUCCESS;
    if (desc->Initialize)
    {
        initResult = desc->Initialize(ctx);
        if (ADUC_RESULT2_IS_FAILURE(initResult))
        {
            fprintf(stderr, "ExtensionLoader: Initialize failed for %s (code: 0x%08x)\n",
                    desc->id, initResult.code);
            adu_dlclose(lib);
            return initResult;
        }
    }

    // Store in registry
    LoadedExtension* entry = &registry->extensions[registry->count];
    entry->libHandle = lib;
    entry->descriptor = desc;
    entry->initialized = true;
    registry->count++;

    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_ExtensionRegistry_ScanDirectory(
    ADUC_ExtensionRegistryHandle registry,
    const char* dirPath,
    const ADUC_ExtensionContext* ctx)
{
    if (!registry || !dirPath)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_STATE, 1);
    }

    size_t loaded = 0;
    const char* libExt = ADU_LIB_EXT;
    size_t extLen = strlen(libExt);

#ifdef _WIN32
    /* Use FindFirstFileA/FindNextFileA on Windows */
    char searchPath[1024];
    snprintf(searchPath, sizeof(searchPath), "%s\\*%s", dirPath, libExt);

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "ExtensionLoader: Cannot open directory %s\n", dirPath);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_CONFIG, 1);
    }

    do
    {
        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s\\%s", dirPath, findData.cFileName);

        ADUC_Result2 loadResult = ADUC_ExtensionRegistry_LoadFromFile(registry, fullPath, ctx);
        if (ADUC_RESULT2_IS_SUCCESS(loadResult))
        {
            loaded++;
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
#else
    DIR* dir = opendir(dirPath);
    if (dir == NULL)
    {
        fprintf(stderr, "ExtensionLoader: Cannot open directory %s\n", dirPath);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_CONFIG, 1);
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        const char* name = entry->d_name;
        size_t nameLen = strlen(name);
        if (nameLen < extLen + 1 || strcmp(name + nameLen - extLen, libExt) != 0)
        {
            continue;
        }

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", dirPath, name);

        ADUC_Result2 loadResult = ADUC_ExtensionRegistry_LoadFromFile(registry, fullPath, ctx);
        if (ADUC_RESULT2_IS_SUCCESS(loadResult))
        {
            loaded++;
        }
    }

    closedir(dir);
#endif

    if (loaded == 0)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_CONFIG, 2);
    }

    return ADUC_RESULT2_SUCCESS;
}

const ADUC_ExtensionDescriptor* ADUC_ExtensionRegistry_FindById(
    ADUC_ExtensionRegistryHandle registry,
    const char* extensionId)
{
    if (!registry || !extensionId)
    {
        return NULL;
    }

    for (size_t i = 0; i < registry->count; i++)
    {
        if (registry->extensions[i].descriptor &&
            strcmp(registry->extensions[i].descriptor->id, extensionId) == 0)
        {
            return registry->extensions[i].descriptor;
        }
    }
    return NULL;
}

const ADUC_ExtensionDescriptor* ADUC_ExtensionRegistry_FindByType(
    ADUC_ExtensionRegistryHandle registry,
    ADUC_ExtensionType type,
    size_t index)
{
    if (!registry)
    {
        return NULL;
    }

    size_t found = 0;
    for (size_t i = 0; i < registry->count; i++)
    {
        if (registry->extensions[i].descriptor &&
            registry->extensions[i].descriptor->type == type)
        {
            if (found == index)
            {
                return registry->extensions[i].descriptor;
            }
            found++;
        }
    }
    return NULL;
}

const ADUC_ExtensionDescriptor* ADUC_ExtensionRegistry_FindByCapability(
    ADUC_ExtensionRegistryHandle registry,
    const char* capability)
{
    if (!registry || !capability)
    {
        return NULL;
    }

    for (size_t i = 0; i < registry->count; i++)
    {
        const ADUC_ExtensionDescriptor* desc = registry->extensions[i].descriptor;
        if (desc && desc->capabilities)
        {
            for (const char** cap = desc->capabilities; *cap != NULL; cap++)
            {
                if (strcmp(*cap, capability) == 0)
                {
                    return desc;
                }
            }
        }
    }
    return NULL;
}

size_t ADUC_ExtensionRegistry_GetCount(ADUC_ExtensionRegistryHandle registry)
{
    return registry ? registry->count : 0;
}

size_t ADUC_ExtensionRegistry_GetCountByType(ADUC_ExtensionRegistryHandle registry, ADUC_ExtensionType type)
{
    if (!registry)
    {
        return 0;
    }

    size_t count = 0;
    for (size_t i = 0; i < registry->count; i++)
    {
        if (registry->extensions[i].descriptor &&
            registry->extensions[i].descriptor->type == type)
        {
            count++;
        }
    }
    return count;
}

const ADUC_ExtensionDescriptor** ADUC_ExtensionRegistry_FindAllByType(
    ADUC_ExtensionRegistryHandle registry,
    ADUC_ExtensionType type,
    size_t* outCount)
{
    if (!registry || !outCount)
    {
        if (outCount) *outCount = 0;
        return NULL;
    }

    // Count matching extensions
    size_t matchCount = ADUC_ExtensionRegistry_GetCountByType(registry, type);
    if (matchCount == 0)
    {
        *outCount = 0;
        return NULL;
    }

    // Allocate result array
    const ADUC_ExtensionDescriptor** result = calloc(matchCount, sizeof(const ADUC_ExtensionDescriptor*));
    if (!result)
    {
        *outCount = 0;
        return NULL;
    }

    size_t idx = 0;
    for (size_t i = 0; i < registry->count && idx < matchCount; i++)
    {
        if (registry->extensions[i].descriptor &&
            registry->extensions[i].descriptor->type == type)
        {
            result[idx] = registry->extensions[i].descriptor;
            idx++;
        }
    }

    *outCount = idx;
    return result;
}

void ADUC_ExtensionRegistry_Destroy(ADUC_ExtensionRegistryHandle registry)
{
    if (!registry)
    {
        return;
    }

    // Uninitialize in reverse order
    for (size_t i = registry->count; i > 0; i--)
    {
        LoadedExtension* entry = &registry->extensions[i - 1];
        if (entry->initialized && entry->descriptor && entry->descriptor->Uninitialize)
        {
            entry->descriptor->Uninitialize();
        }
        if (entry->libHandle)
        {
            adu_dlclose(entry->libHandle);
        }
    }

    free(registry);
}

ADUC_Result2 ADUC_ExtensionRegistry_LoadVerified(
    ADUC_ExtensionRegistryHandle registry,
    const char* extensionDir,
    const char* trustedCertPath,
    int policy,
    const ADUC_ExtensionContext* ctx)
{
    if (!registry || !extensionDir)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_CONFIG, 1);
    }

    // Verify extension before loading
    ADUC_Result2 verifyResult = ADUC_Extension_Verify(
        extensionDir, trustedCertPath, (ADUC_SignPolicy)policy);

    if (ADUC_RESULT2_IS_FAILURE(verifyResult))
    {
        fprintf(stderr, "ExtensionLoader: Verification failed for %s (code: 0x%08x)\n",
                extensionDir, verifyResult.code);
        return verifyResult;
    }

    // Parse manifest to get library name
    char manifestPath[1024];
    snprintf(manifestPath, sizeof(manifestPath), "%s/manifest.json", extensionDir);

    ADUC_ExtensionManifest manifest;
    ADUC_Result2 parseResult = ADUC_ExtensionManifest_Parse(manifestPath, &manifest);

    char libraryPath[1024];
    if (ADUC_RESULT2_IS_SUCCESS(parseResult) && manifest.library[0] != '\0')
    {
        snprintf(libraryPath, sizeof(libraryPath), "%s/%s", extensionDir, manifest.library);
    }
    else
    {
        // Fallback: if manifest parsing fails (e.g., policy is NONE), scan for library
        fprintf(stderr, "ExtensionLoader: No manifest library found, cannot load from %s\n", extensionDir);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_CONFIG, 3);
    }

    return ADUC_ExtensionRegistry_LoadFromFile(registry, libraryPath, ctx);
}
