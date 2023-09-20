/**
 * @file rootkey_store.h
 * @brief The header interface for the rootkey store.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ROOTKEY_STORE_H
#define ROOTKEY_STORE_H

#include <aduc/c_utils.h>
#include <aduc/result.h>
#include <aduc/rootkeypackage_types.h>
#include <azure_c_shared_utility/strings.h>
#include <rootkey_store_types.h>

EXTERN_C_BEGIN

RootKeyStoreHandle RootKeyStore_CreateInstance();
void RootKeyStore_DestroyInstance(RootKeyStoreHandle handle);

bool RootKeyStore_SetConfig(RootKeyStoreHandle handle, RootKeyStoreConfigProperty configProperty, STRING_HANDLE propertyValue);
STRING_HANDLE RootKeyStore_GetConfig(RootKeyStoreHandle handle, RootKeyStoreConfigProperty configProperty);

bool RootKeyStore_GetRootKeyPackage(RootKeyStoreHandle handle, ADUC_RootKeyPackage** outPackage);
bool RootKeyStore_SetRootKeyPackage(RootKeyStoreHandle handle, const ADUC_RootKeyPackage* package);

bool RootKeyStore_Load(RootKeyStoreHandle handle);
ADUC_Result RootKeyStore_Persist(RootKeyStoreHandle handle);

EXTERN_C_END

#endif // #define ROOTKEY_STORE_H
