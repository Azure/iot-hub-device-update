
/**
 * @file rootkey_store_types.h
 * @brief The header interface for the rootkey store types.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ROOTKEY_STORE_TYPES_H
#define ROOTKEY_STORE_TYPES_H

#include <azure_c_shared_utility/strings.h>

typedef void* RootKeyStoreHandle;

typedef enum tagRootKeyStoreConfigProperty
{
    RootKeyStoreConfig_StorePath = 1,
} RootKeyStoreConfigProperty;

typedef struct tagRootKeyStoreConfig
{
    STRING_HANDLE store_path;
} RootKeyStoreConfig;

#endif // #define ROOTKEY_STORE_TYPES_H
