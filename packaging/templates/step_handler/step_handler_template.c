/* step_handler_template.c
 *
 * Starter template for an ADU step handler extension.
 * Build this as a shared library and install to /usr/lib/adu/extensions/.
 *
 * Copyright (c) Microsoft Corporation. Licensed under the MIT License.
 */

#include <aduc/step_handler_vtable.h>
#include <stdbool.h>
#include <stdio.h>

static bool my_handler_init(void* context)
{
    (void)context;
    printf("my_step_handler: initialized\n");
    return true;
}

static void my_handler_deinit(void* context)
{
    (void)context;
}

/* Populate the vtable that the agent will call into. */
ADUC_StepHandlerVTable* GetStepHandlerVTable(void)
{
    static ADUC_StepHandlerVTable vtable = {
        .Init = my_handler_init,
        .Deinit = my_handler_deinit,
        /* Fill in remaining function pointers:
         *   .Prepare, .Execute, .Verify, .Rollback, etc.
         */
    };
    return &vtable;
}
