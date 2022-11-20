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

ADUC_Result ADUC_RootKeyPackageUtils_Parse(const char* jsonString, ADUC_RootKeyPackage* outRootKeyPackage);

void ADUC_RootKeyPackageUtils_DisabledRootKeys_Cleanup(ADUC_RootKeyPackage* rootKeyPackage);
void ADUC_RootKeyPackageUtils_DisabledSigningKeys_Cleanup(ADUC_RootKeyPackage* rootKeyPackage);
void ADUC_RootKeyPackageUtils_RootKeys_Cleanup(ADUC_RootKeyPackage* rootKeyPackage);
void ADUC_RootKeyPackageUtils_Signatures_Cleanup(ADUC_RootKeyPackage* rootKeyPackage);
void ADUC_RootKeyPackageUtils_Cleanup(ADUC_RootKeyPackage* rootKeyPackage);

EXTERN_C_END

#endif // ROOTKEYPACKAGE_UTILS_H
