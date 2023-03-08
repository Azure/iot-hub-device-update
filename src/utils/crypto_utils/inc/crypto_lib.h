/**
 * @file crypto_lib.h
 * @brief Provides an implementation of cryptogrpahic functions for hashing, encrypting, and verifiying.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "crypto_key.h"
#include "decryption_alg_types.h"
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

ADUC_Result CryptoUtils_DecryptBlock(
    unsigned char** destBuff,
    const DecryptionAlg alg,
    const KeyData* dKey,
    const unsigned char* iv,
    const unsigned char* block,
    const int encryptedBlockSize);

ADUC_Result CryptoUtils_DecryptBufferBlockByBlock(
    unsigned char** destBuff,
    size_t* destBuffSize,
    const DecryptionAlg alg,
    const KeyData* dKey,
    const unsigned char* encryptedBuffer,
    const size_t encryptedBufferSize);

ADUC_Result CryptoUtils_EncryptBlock(
    unsigned char** destBuff,
    const DecryptionAlg alg,
    const KeyData* eKey,
    const unsigned char* iv,
    const unsigned char* block,
    const int plainTextBlockSize);

ADUC_Result CryptoUtils_EncryptBufferBlockByBlock(
    unsigned char** destBuff,
    size_t* destBuffSize,
    const DecryptionAlg alg,
    const KeyData* eKey,
    const char* plainTextBuffer,
    const size_t plainTextBufferSize);

//
// TODO: Reorganize outputs so they are even with inputs and outputs
//
ADUC_Result CryptoUtils_CalculateIV(
    unsigned char** destIvBuffer, size_t blockSize, const DecryptionAlg alg, const KeyData* eKey, const size_t offSet);

//
// Helper Functions for KeyData
//

bool CryptoUtils_InitializeKeyDataFromUrlEncodedB64String(KeyData** dKey, const char* b64UrlEncodedKeyBytes);

bool CryptoUtils_IsKeyNullOrEmpty(const KeyData* key);

void CryptoUtils_DeInitializeKeyData(KeyData** dKey);

EXTERN_C_END

#endif // CRYPTO_LIB_H
