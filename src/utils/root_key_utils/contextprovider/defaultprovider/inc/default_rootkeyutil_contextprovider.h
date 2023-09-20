/**
 * @file default_rootkeyutil_contextprovider.h
 * @brief The header for the default provider of rootkey utility context.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef DEFAULT_ROOTKEYUTIL_CONTEXTPROVIDER
#define DEFAULT_ROOTKEYUTIL_CONTEXTPROVIDER

#include "rootkeyutil_contextprovider.h"

extern RootKeyUtilContext* g_rootkey_util_context;

bool RootKeyContextProvider_CreateRootKeyUtilContext();

#endif // #define DEFAULT_ROOTKEYUTIL_CONTEXTPROVIDER
