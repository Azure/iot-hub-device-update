/**
 * @file rootkeypackage_utils.h
 * @brief rootkeypackage_utils interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ROOTKEYPACKAGE_UTILS_H
#define ROOTKEYPACKAGE_UTILS_H

#include "aduc/rootkeypackage_types.h"
#include <aduc/c_utils.h>
#include <aduc/result.h>

EXTERN_C_BEGIN

_Bool ADUC_RootKeyPackageUtils_RootKey_Init(ADUC_RootKey** outKey, const char* kid, const ADUC_RootKey_KeyType keyType, const BUFFER_HANDLE n , const BUFFER_HANDLE e);

void ADUC_RootKeyPackageUtils_RootKeyDeInit(ADUC_RootKey* key);

ADUC_Result ADUC_RootKeyPackageUtils_Parse(const char* jsonString, ADUC_RootKeyPackage* outRootKeyPackage);

void ADUC_RootKeyPackageUtils_DisabledRootKeys_Cleanup(ADUC_RootKeyPackage* rootKeyPackage);
void ADUC_RootKeyPackageUtils_DisabledSigningKeys_Cleanup(ADUC_RootKeyPackage* rootKeyPackage);
void ADUC_RootKeyPackageUtils_RootKeys_Cleanup(ADUC_RootKeyPackage* rootKeyPackage);
void ADUC_RootKeyPackageUtils_Signatures_Cleanup(ADUC_RootKeyPackage* rootKeyPackage);
void ADUC_RootKeyPackageUtils_Cleanup(ADUC_RootKeyPackage* rootKeyPackage);

EXTERN_C_END

#endif // ROOTKEYPACKAGE_UTILS_H
