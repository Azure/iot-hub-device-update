/**
 * @file content_handler.c
 * @brief Implementation of ADUC Content Handler SDK
 * 
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#include "aduc/content_handler.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a new content handler instance
 */
ADUC_ContentHandler* ADUC_ContentHandler_Create(void)
{
    ADUC_ContentHandler* handler = (ADUC_ContentHandler*)calloc(1, sizeof(ADUC_ContentHandler));
    if (!handler)
    {
        return NULL;
    }
    
    // Initialize all function pointers to NULL
    handler->Download = NULL;
    handler->Backup = NULL;
    handler->Install = NULL;
    handler->Apply = NULL;
    handler->Restore = NULL;
    handler->Cancel = NULL;
    handler->IsInstalled = NULL;
    handler->Free = NULL;
    handler->context = NULL;
    
    return handler;
}

/**
 * @brief Free a content handler instance
 */
void ADUC_ContentHandler_Free(ADUC_ContentHandler* handler)
{
    if (handler)
    {
        // Call the handler's own free function if available
        if (handler->Free)
        {
            handler->Free(handler);
        }
        else
        {
            // Default cleanup
            free(handler->context);
            free(handler);
        }
    }
}

/**
 * @brief Get the content handler's context
 */
void* ADUC_ContentHandler_GetContext(ADUC_ContentHandler* handler)
{
    return handler ? handler->context : NULL;
}

/**
 * @brief Set the content handler's context
 */
void ADUC_ContentHandler_SetContext(ADUC_ContentHandler* handler, void* context)
{
    if (handler)
    {
        handler->context = context;
    }
}