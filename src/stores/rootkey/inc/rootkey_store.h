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
#include <azure_c_shared_utility/strings.h>
#include <aduc/rootkeypackage_types.h> // for ADUC_RootKeyPackage

EXTERN_C_BEGIN

RootKeyStoreHandle RootKeyStore_CreateInstance();
void RootKeyStore_DestroyInstance(RootKeyStoreHandle handle);

bool RootKeyStore_SetConfig(RootKeyStoreHandle handle, RootKeyStoreConfigProperty configProperty, STRING_HANDLE propertyValue);
STRING_HANDLE RootKeyStore_GetConfig(RootKeyStoreHandle handle, RootKeyStoreConfigProperty configProperty);

ADUC_RootkeyPackage* RootKeyStore_GetRootKeyPackage(RootKeyStoreHandle handle);
bool RootKeyStore_SetRootKeyPackage(RootKeyStoreHandle handle, ADUC_RootKeyPackage*);

bool RootKeyStore_Load(RootKeyStoreHandle handle);
bool RootKeyStore_Persist(RootKeyStoreHandle handle);

EXTERN_C_END

#define ROOTKEY_STORE_H
