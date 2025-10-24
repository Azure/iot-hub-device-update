/**
 * @file step_handler_interface.c
 * @brief Implementation of Step Handler Interface SDK
 *
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#include "aduc/step_handler_interface.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a new step handler interface
 */
ADUC_StepHandler* ADUC_StepHandler_Create(void)
{
    ADUC_StepHandler* handler = (ADUC_StepHandler*)calloc(1, sizeof(ADUC_StepHandler));
    if (!handler)
    {
        return NULL;
    }

    // Initialize all function pointers to NULL
    handler->Initialize = NULL;
    handler->Download = NULL;
    handler->Backup = NULL;
    handler->Install = NULL;
    handler->Apply = NULL;
    handler->Restore = NULL;
    handler->Cancel = NULL;
    handler->IsInstalled = NULL;
    handler->Cleanup = NULL;
    handler->context = NULL;

    return handler;
}

/**
 * @brief Free a step handler interface
 */
void ADUC_StepHandler_Free(ADUC_StepHandler* handler)
{
    if (handler)
    {
        // Call cleanup if available
        if (handler->Cleanup)
        {
            handler->Cleanup(handler);
        }

        free(handler->context);
        free(handler);
    }
}
