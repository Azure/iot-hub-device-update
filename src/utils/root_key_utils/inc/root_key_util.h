/**
 * @file root_key_util.h
 * @brief Defines the functions for getting, validating, and dealing with encoded and locally stored root keys
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/c_utils.h"
#include "aduc/result.h"
#include "aduc/rootkeypackage_types.h"
#include "crypto_key.h"
#include <stdbool.h>
#ifndef ROOT_KEY_UTIL_H
#    define ROOT_KEY_UTIL_H

EXTERN_C_BEGIN

ADUC_Result RootKeyUtility_LoadPackageFromDisk(
    ADUC_RootKeyPackage** rootKeyPackage, const char* fileLocation, bool validateSignatures);
ADUC_Result RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(const ADUC_RootKeyPackage* rootKeyPackage);
ADUC_Result RootKeyUtility_WriteRootKeyPackageToFileAtomically(
    const ADUC_RootKeyPackage* rootKeyPackage, const STRING_HANDLE fileDest);
ADUC_Result RootKeyUtility_ReloadPackageFromDisk(const char* filepath, bool validateSignatures);
ADUC_Result RootKeyUtility_LoadSerializedPackage(const char* fileLocation, char** outSerializePackage);
void RootKeyUtility_SetReportingErc(ADUC_Result_t erc);
void RootKeyUtility_ClearReportingErc();
ADUC_Result_t RootKeyUtility_GetReportingErc();
bool ADUC_RootKeyUtility_IsUpdateStoreNeeded(const STRING_HANDLE storePath, const ADUC_RootKeyPackage* packageToTest);
ADUC_Result RootKeyUtility_GetDisabledSigningKeys(VECTOR_HANDLE* outDisabledSigningKeyList);
bool RootKeyUtility_RootKeyIsDisabled(const ADUC_RootKeyPackage* rootKeyPackage, const char* keyId);

ADUC_Result RootKeyUtility_GetKeyForKidFromHardcodedKeys(CryptoKeyHandle* key, const char* kid);
ADUC_Result RootKeyUtility_GetKeyForKid(CryptoKeyHandle* key, const char* kid);
EXTERN_C_END
#endif // ROOT_KEY_UTIL_H
