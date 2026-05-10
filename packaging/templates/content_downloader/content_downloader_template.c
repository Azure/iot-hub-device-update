/* content_downloader_template.c
 *
 * Starter template for an ADU content downloader extension.
 * Build this as a shared library and install to /usr/lib/adu/extensions/.
 *
 * Copyright (c) Microsoft Corporation. Licensed under the MIT License.
 */

#include <aduc/content_downloader_vtable.h>
#include <stdbool.h>
#include <stdio.h>

static bool my_downloader_init(void* context)
{
    (void)context;
    printf("my_downloader: initialized\n");
    return true;
}

static void my_downloader_deinit(void* context)
{
    (void)context;
}

/* Populate the vtable that the agent will call into. */
ADUC_ContentDownloaderVTable* GetContentDownloaderVTable(void)
{
    static ADUC_ContentDownloaderVTable vtable = {
        .Init = my_downloader_init,
        .Deinit = my_downloader_deinit,
        /* Fill in remaining function pointers. */
    };
    return &vtable;
}
