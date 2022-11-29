/**
 * @file root_key_util.h
 * @brief Defines the functions for getting, validating, and dealing with encoded and locally stored root keys
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <azure_c_shared_utility/constbuffer.h>
#include <azure_c_shared_utility/vector.h>
#include <azure_c_shared_utility/strings.h>

#include "crypto_key.h"
#include "root_key_result.h"

#ifndef ROOT_KEY_UTIL_H
#    define ROOT_KEY_UTIL_H

RootKeyUtility_ValidationResult RootKeyUtil_ValidateRootKeyPackage(const STRING_HANDLE rootKeyPackagePath );

/**
 * @brief Get the Key For Kid object
 *
 * @param kid
 * @return CryptoKeyHandle
 */
CryptoKeyHandle RootKeyUtility_GetKeyForKid(const char* kid);

#endif // ROOT_KEY_UTIL_H
