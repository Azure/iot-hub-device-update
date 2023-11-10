/**
 * @file crypto_lib.h
 * @brief Provides an implementation of cryptogrpahic functions for hashing, encrypting, and verifiying.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "crypto_key.h"
#include <aduc/c_utils.h>
#include <azure_c_shared_utility/constbuffer.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef CRYPTO_LIB_H
#    define CRYPTO_LIB_H

EXTERN_C_BEGIN

//
// Signature Algorithms
//

#    define CRYPTO_UTILS_SIGNATURE_VALIDATION_ALG_RS256 "rs256"
//
// Signature Verification
//

bool CryptoUtils_IsValidSignature(
    const char* alg,
    const uint8_t* expectedSignature,
    size_t sigLength,
    const uint8_t* blob,
    size_t blobLength,
    CryptoKeyHandle keyToSign);

//
// Key Helper Functions
//

CryptoKeyHandle RSAKey_ObjFromBytes(const uint8_t* N, size_t N_len, const uint8_t* e, size_t e_len);

CryptoKeyHandle RSAKey_ObjFromB64Strings(const char* encodedN, const char* encodedE);

CryptoKeyHandle RSAKey_ObjFromModulusBytesExponentInt(const uint8_t* N, size_t N_len, const unsigned int e);

CryptoKeyHandle RSAKey_ObjFromStrings(const char* N, const char* e);

CryptoKeyHandle GetRootKeyForKeyID(const char* kid);

void CryptoUtils_FreeCryptoKeyHandle(CryptoKeyHandle key);

CONSTBUFFER_HANDLE CryptoUtils_CreateSha256Hash(const CONSTBUFFER_HANDLE buf);

CONSTBUFFER_HANDLE CryptoUtils_GenerateRsaPublicKey(const char* modulus_b64url, const char* exponent_b64url);

EXTERN_C_END

#endif // CRYPTO_LIB_H
