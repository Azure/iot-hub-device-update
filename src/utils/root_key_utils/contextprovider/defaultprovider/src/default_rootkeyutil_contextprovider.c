/**
 * @file default_rootkeyutil_contextprovider.c
 * @brief The implentation for the default provider of rootkey utility context.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/logging.h"
#include "root_key_util.h"

/**
 * @brief Global rootkey util context.
 */
RootKeyUtilContext* g_rootkey_util_context = NULL;

RootKeyUtilContext* GetRootKeyUtilContext()
{
    return g_rootkey_util_context;
}

bool RootKeyContextProvider_CreateRootKeyUtilContext()
{
    if (g_rootkey_util_context != NULL)
    {
        RootKeyUtility_UninitContext(g_rootkey_util_context);
        g_rootkey_util_context = NULL;
    }

    g_rootkey_util_context = RootKeyUtility_InitContext(NULL /* rootkey_store_path */);
    if (g_rootkey_util_context == NULL)
    {
        Log_Error("failed to init rootkey utility context");
        return false;
    }

    return true;
}
