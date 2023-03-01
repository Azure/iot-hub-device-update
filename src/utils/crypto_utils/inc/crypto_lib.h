/**
 * @file crypto_lib.h
 * @brief Provides an implementation of cryptogrpahic functions for hashing, encrypting, and verifiying.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "crypto_key.h"
#include "key_data.h"
#include <aduc/c_utils.h>
#include <aduc/result.h>
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
// CryptoKey Helper Functions
//

CryptoKeyHandle RSAKey_ObjFromBytes(uint8_t* N, size_t N_len, uint8_t* e, size_t e_len);

CryptoKeyHandle RSAKey_ObjFromB64Strings(const char* encodedN, const char* encodedE);

CryptoKeyHandle RSAKey_ObjFromStrings(const char* N, const char* e);

CryptoKeyHandle GetRootKeyForKeyID(const char* kid);

void FreeCryptoKeyHandle(CryptoKeyHandle key);

//
// Encryption/Decryption Functions
//

ADUC_Result CryptoUtils_DecryptBuffer(
    const char* alg,
    const KeyData* dKey,
    const char* iv,
    const unsigned char* encryptedBuff,
    const size_t encryptedBuffSize,
    unsigned char** destBuff,
    size_t* destBuffSize);

ADUC_Result CryptoUtils_EncryptBuffer(
    const char* alg,
    const KeyData* eKey,
    const char* iv,
    const unsigned char* plainTextBuff,
    const size_t plainTextBuffSize,
    unsigned char** destBuff,
    size_t* destBuffSize);

ADUC_Result CryptoUtils_CalculateIV(
    unsigned char** destIvBuffer,
    size_t destIvBufferSize,
    const char* alg,
    const KeyData* eKey,
    const unsigned char* inputForIv,
    const size_t sizeOfInputIv);

//
// Helper Functions for KeyData
//

bool CryptoUtils_InitializeKeyData(const char* keyBytes, const size_t keySize, KeyData** dKey);

bool CryptoUtils_IsKeyNullOrEmpty(const KeyData* key);

void CryptoUtils_DeInitializeKeyData(KeyData** dKey);

EXTERN_C_END

#endif // CRYPTO_LIB_H
