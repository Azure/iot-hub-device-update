/* content_processor_template.c
 *
 * Starter template for an ADU content processor extension.
 * Build this as a shared library and install to /usr/lib/adu/extensions/.
 *
 * Copyright (c) Microsoft Corporation. Licensed under the MIT License.
 */

#include <aduc/content_processor_vtable.h>
#include <stdbool.h>
#include <stdio.h>

static bool my_processor_init(void* context)
{
    (void)context;
    printf("my_content_processor: initialized\n");
    return true;
}

static void my_processor_deinit(void* context)
{
    (void)context;
}

/* Populate the vtable that the agent will call into. */
ADUC_ContentProcessorVTable* GetContentProcessorVTable(void)
{
    static ADUC_ContentProcessorVTable vtable = {
        .Init = my_processor_init,
        .Deinit = my_processor_deinit,
        /* Fill in remaining function pointers. */
    };
    return &vtable;
}
