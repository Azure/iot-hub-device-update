/**
 * @file base64_utils.h
 * @brief Provides an implementation for Base64 encoding and decoding
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/c_utils.h>
#include <aduc/decryption_alg_types.h>
#include <stddef.h>
#include <stdint.h>

#ifndef BASE64_UTILS_H
#    define BASE64_UTILS_H

EXTERN_C_BEGIN

bool ContentProtectionUtils_DecryptDek(const STRING_HANDLE dek, const STRING_HANDLE dekCryptAlg);

bool ContentProtectionUtils_GetDecryptAlgFromDecryptionInfo(const JSON_Object* decryptInfo, DecryptionAlg* alg);

EXTERN_C_END

#endif // BASE64_UTILS_H
