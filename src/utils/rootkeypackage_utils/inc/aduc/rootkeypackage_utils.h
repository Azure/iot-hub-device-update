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
#include <aduc/result.h>

ADUC_Result ADUC_RootKeyPackageUtils_Parse(const char* jsonString, ADUC_RootKeyPackage* outRootKeyPackage);
void ADUC_RootKeyPackageUtils_Cleanup(ADUC_RootKeyPackage* rootKeyPackage);

#endif // ROOTKEYPACKAGE_UTILS_H
