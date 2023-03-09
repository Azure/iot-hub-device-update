/**
 * @file rootkeypackage_utils.h
 * @brief rootkeypackage_utils interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ROOTKEYPACKAGE_UTILS_H
#define ROOTKEYPACKAGE_UTILS_H

#include "aduc/rootkeypackage_download.h"
#include "aduc/rootkeypackage_types.h"
#include <aduc/c_utils.h>
#include <aduc/result.h>
#include <azure_c_shared_utility/strings.h>

EXTERN_C_BEGIN

ADUC_Result ADUC_RootKeyPackageUtils_DownloadPackage(
    const char* rootKeyPkgUrl,
    const char* workflowId,
    ADUC_RootKeyPkgDownloaderInfo* downloaderInfo,
    STRING_HANDLE* outRootKeyPackageDownloadedFile);

ADUC_Result ADUC_RootKeyPackageUtils_Parse(const char* jsonString, ADUC_RootKeyPackage* outRootKeyPackage);

char* ADUC_RootKeyPackageUtils_SerializePackageToJsonString(const ADUC_RootKeyPackage* rootKeyPackage);

void ADUC_RootKeyPackageUtils_DisabledRootKeys_Destroy(ADUC_RootKeyPackage* rootKeyPackage);
void ADUC_RootKeyPackageUtils_DisabledSigningKeys_Destroy(ADUC_RootKeyPackage* rootKeyPackage);
void ADUC_RootKeyPackageUtils_RootKeys_Destroy(ADUC_RootKeyPackage* rootKeyPackage);
void ADUC_RootKeyPackageUtils_Signatures_Destroy(ADUC_RootKeyPackage* rootKeyPackage);
void ADUC_RootKeyPackageUtils_Destroy(ADUC_RootKeyPackage* rootKeyPackage);

EXTERN_C_END

#endif // ROOTKEYPACKAGE_UTILS_H
