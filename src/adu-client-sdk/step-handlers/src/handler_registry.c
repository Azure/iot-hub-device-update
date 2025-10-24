/**
 * @file handler_registry.c
 * @brief Handler registry for managing extension handlers
 * 
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#include "aduc/handler_registry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#define MAX_HANDLERS 32

/**
 * @brief Handler registry entry
 */
typedef struct
{
    char* name;
    char* path;
    void* handle;
    ADUC_ContentHandler* (*createFunction)(ADUC_LogLevel);
    bool loaded;
} HandlerEntry;

/**
 * @brief Global handler registry
 */
static struct
{
    HandlerEntry handlers[MAX_HANDLERS];
    size_t count;
    bool initialized;
} g_registry = { .initialized = false };

/**
 * @brief Initialize the handler registry
 */
ADUC_Result_t ADUC_HandlerRegistry_Initialize(void)
{
    if (g_registry.initialized)
    {
        return ADUC_Result_Success;
    }
    
    memset(&g_registry, 0, sizeof(g_registry));
    g_registry.initialized = true;
    
    return ADUC_Result_Success;
}

/**
 * @brief Cleanup the handler registry
 */
void ADUC_HandlerRegistry_Cleanup(void)
{
    if (!g_registry.initialized)
    {
        return;
    }
    
    for (size_t i = 0; i < g_registry.count; i++)
    {
        HandlerEntry* entry = &g_registry.handlers[i];
        
        if (entry->handle)
        {
            dlclose(entry->handle);
        }
        
        free(entry->name);
        free(entry->path);
    }
    
    memset(&g_registry, 0, sizeof(g_registry));
}

/**
 * @brief Register a handler
 */
ADUC_Result_t ADUC_HandlerRegistry_Register(const char* name, const char* libraryPath)
{
    if (!name || !libraryPath || g_registry.count >= MAX_HANDLERS)
    {
        return ADUC_Result_Failure_InvalidArgument;
    }
    
    HandlerEntry* entry = &g_registry.handlers[g_registry.count];
    
    entry->name = strdup(name);
    entry->path = strdup(libraryPath);
    entry->handle = NULL;
    entry->createFunction = NULL;
    entry->loaded = false;
    
    if (!entry->name || !entry->path)
    {
        free(entry->name);
        free(entry->path);
        return ADUC_Result_Failure_OutOfMemory;
    }
    
    g_registry.count++;
    return ADUC_Result_Success;
}

/**
 * @brief Load a handler by name
 */
ADUC_ContentHandler* ADUC_HandlerRegistry_LoadHandler(const char* name, ADUC_LogLevel logLevel)
{
    if (!name)
    {
        return NULL;
    }
    
    // Find the handler entry
    HandlerEntry* entry = NULL;
    for (size_t i = 0; i < g_registry.count; i++)
    {
        if (strcmp(g_registry.handlers[i].name, name) == 0)
        {
            entry = &g_registry.handlers[i];
            break;
        }
    }
    
    if (!entry)
    {
        return NULL;
    }
    
    // Load the library if not already loaded
    if (!entry->loaded)
    {
        entry->handle = dlopen(entry->path, RTLD_LAZY);
        if (!entry->handle)
        {
            printf("Failed to load handler %s: %s\n", name, dlerror());
            return NULL;
        }
        
        // Get the create function
        entry->createFunction = (ADUC_ContentHandler* (*)(ADUC_LogLevel))dlsym(
            entry->handle, 
            "CreateUpdateContentHandlerExtension"
        );
        
        if (!entry->createFunction)
        {
            printf("Failed to find CreateUpdateContentHandlerExtension in %s: %s\n", name, dlerror());
            dlclose(entry->handle);
            entry->handle = NULL;
            return NULL;
        }
        
        entry->loaded = true;
    }
    
    // Create handler instance
    return entry->createFunction(logLevel);
}