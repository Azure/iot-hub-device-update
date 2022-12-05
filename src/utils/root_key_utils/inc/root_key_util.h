/**
 * @file root_key_util.h
 * @brief Defines the functions for getting, validating, and dealing with encoded and locally stored root keys
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "crypto_key.h"
#include "aduc/rootkeypackage_types.h"
#include "aduc/c_utils.h"
#include "aduc/result.h"

#ifndef ROOT_KEY_UTIL_H
#    define ROOT_KEY_UTIL_H

EXTERN_C_BEGIN

ADUC_Result RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(const ADUC_RootKeyPackage* rootKeyPackage );

ADUC_Result RootKeyUtil_WriteRootKeyPackageToFileAtomically(const ADUC_RootKeyPackage* rootKeyPackage, const STRING_HANDLE fileDest);

/**
 * @brief Get the Key For Kid object
 *
 * @param kid
 * @return CryptoKeyHandle
 */
CryptoKeyHandle RootKeyUtility_GetKeyForKid(const char* kid);

EXTERN_C_END
#endif // ROOT_KEY_UTIL_H
