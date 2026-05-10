/* communication_provider_template.c
 *
 * Starter template for an ADU communication provider extension.
 * Build this as a shared library and install to /usr/lib/adu/extensions/.
 *
 * Copyright (c) Microsoft Corporation. Licensed under the MIT License.
 */

#include <aduc/communication_vtable.h>
#include <stdbool.h>
#include <stdio.h>

static bool my_comm_init(void* context)
{
    (void)context;
    printf("my_comm_provider: initialized\n");
    return true;
}

static void my_comm_deinit(void* context)
{
    (void)context;
}

/* Populate the vtable that the agent will call into. */
ADUC_CommunicationVTable* GetCommunicationVTable(void)
{
    static ADUC_CommunicationVTable vtable = {
        .Init = my_comm_init,
        .Deinit = my_comm_deinit,
        /* Fill in remaining function pointers. */
    };
    return &vtable;
}
