/**
 * @file crypto_lib.h
 * @brief Provides an implementation of cryptogrpahic functions for hashing, encrypting, and verifiying.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "crypto_key.h"
#include <aduc/c_utils.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef CRYPTO_LIB_H
#    define CRYPTO_LIB_H

EXTERN_C_BEGIN

//
// Signature Verification
//

bool IsValidSignature(
    const char* alg,
    const uint8_t* expectedSignature,
    size_t sigLength,
    const uint8_t* blob,
    size_t blobLength,
    CryptoKeyHandle keyToSign);

//
// Key Helper Functions
//

CryptoKeyHandle RSAKey_ObjFromBytes(uint8_t* N, size_t N_len, uint8_t* e, size_t e_len);

CryptoKeyHandle RSAKey_ObjFromB64Strings(const char* encodedN, const char* encodedE);

CryptoKeyHandle RSAKey_ObjFromStrings(const char* N, const char* e);

CryptoKeyHandle GetRootKeyForKeyID(const char* kid);

void FreeCryptoKeyHandle(CryptoKeyHandle key);

EXTERN_C_END

#endif // CRYPTO_LIB_H
