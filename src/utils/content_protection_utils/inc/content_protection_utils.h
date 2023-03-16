/**
 * @file content_protection_utils.h
 * @brief Provides an implementation for interacting with the content protection section of a deployment's json
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/c_utils.h>
#include <aduc/decryption_alg_types.h>
#include <stddef.h>
#include <stdint.h>

#ifndef CONTENT_PROTECTION_UTILS_H
#    define CONTENT_PROTECTION_UTILS_H

EXTERN_C_BEGIN

bool ContentProtectionUtils_DecryptDek(const STRING_HANDLE dek, const STRING_HANDLE dekCryptAlg);

DecryptionAlg ContentProtectionUtils_GetDecryptAlgFromDecryptionInfo(const JSON_Object* decryptInfo);

EXTERN_C_END

#endif // CONTENT_PROTECTION_UTILS_H
