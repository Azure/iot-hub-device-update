/**
 * @file crypto_lib.c
 * @brief Provides an implementation of cryptogrpahic functions for hashing, encrypting, and verifiying.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "base64_utils.h"
#include "crypto_lib.h"
#include "decryption_alg_types.h"
#include "root_key_util.h"

#include <aduc/result.h>
#include <azure_c_shared_utility/constbuffer.h>
#include <ctype.h>
#include <openssl/aes.h>
#include <openssl/bn.h>
#include <openssl/evp.h>

//
// Note: This is not generalized, for now we assume the same AES key for the entire time
//
typedef struct KeyData
{
    CONSTBUFFER_HANDLE keyData;
} KeyData;

//
// Initialize/ De-Initialize
//

const EVP_CIPHER* CryptoUtils_GetCipherType(const DecryptionAlg alg)
{
    if (alg == AES_128_CBC)
    {
        return EVP_aes_128_cbc();
    }
    else if (alg == AES_192_CBC)
    {
        return EVP_aes_192_cbc();
    }
    else if (alg == AES_256_CBC)
    {
        return EVP_aes_256_cbc();
    }

    return NULL;
}

void CryptoUtils_DeInitializeKeyData(KeyData** dKey)
{
    CONSTBUFFER_DecRef((*dKey)->keyData);
    free(*dKey);
    *dKey = NULL;
}

bool CryptoUtils_IsKeyNullOrEmpty(const KeyData* key)
{
    if (key == NULL || key->keyData == NULL)
    {
        return true;
    }

    return false;
}

bool CryptoUtils_InitializeKeyData(const char* keyBytes, const size_t keySize, KeyData** dKey)
{
    bool success = false;
    KeyData* tempKey = NULL;

    if (keyBytes == NULL || dKey == NULL)
    {
        goto done;
    }

    tempKey = malloc(sizeof(KeyData));

    if (tempKey == NULL)
    {
        goto done;
    }

    tempKey->keyData = CONSTBUFFER_CreateWithMoveMemory(keyBytes, keySize);

    if (tempKey->keyData == NULL)
    {
        goto done;
    }

    success = true;
done:

    if (!success)
    {
        CryptoUtils_DeInitializeDecryptionKey(dKey);
    }
    *dKey = tempKey;
    return success;
}

//
// Decryption Algorithms
//

ADUC_Result CryptoUtils_EncryptBuffer(
    const char* alg,
    const KeyData* eKey,
    const char* iv,
    const unsigned char* plainTextBuff,
    const size_t plainTextBuffSize,
    unsigned char** destBuff,
    size_t* destBuffSize)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    unsigned char* encryptedBuff = NULL;
    if (alg == NULL || CryptoUtils_IsKeyNullOrEmpty(eKey) || plainTextBuff == NULL || destBuff == NULL)
    {
        goto done;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    if (ctx == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    const EVP_CIPHER* cipherType = CryptoUtils_GetCipherType(alg);

    if (cipherType == UNSUPPORTED_DECRYPTION_ALG)
    {
        result.ExtendedResultCode = ADUC_ERC_CRYPTO_ALG_NOTSUPPORTED;
        goto done;
    }

    const CONSTBUFFER* keyData = CONSTBUFFER_GetContent(eKey->keyData);
    if (EVP_EncryptInit(ctx, cipherType, keyData->buffer, iv) != 1)
    {
        result.ExtendedResultCode = ADUC_ERC_ENCRYPTION_INIT_FAILED;
        goto done;
    }

    const size_t maxEncryptedBuffSize = plainTextBuffSize + 16 - (plainTextBuffSize % 128);
    encryptedBuff = (unsigned char*)malloc(maxEncryptedBuffSize);

    size_t outSize = 0;

    size_t encryptedLength = 0;
    if (EVP_EncryptUpdate(ctx, encryptedBuff, &encryptedLength, plainTextBuff, plainTextBuffSize) != 1)
    {
        result.ExtendedResultCode = ADUC_ERC_ENCRYPTION_UPDATE_FAILED;
        goto done;
    }

    outSize += encryptedLength;

    if (EVP_EncryptFinal(ctx, encryptedBuff + encryptedLength, &encryptedLength) != 1)
    {
        result.ExtendedResultCode = ADUC_ERC_ENCRYPTION_FINAL_FAILED;
        goto done;
    }

    outSize += encryptedLength;

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        free(encryptedBuff);
        encryptedBuff = NULL;
        outSize = 0;
    }

    *destBuff = encryptedBuff;
    *destBuffSize = outSize;

    return result;
}

ADUC_Result CryptoUtils_CalculateIV(
    unsigned char** destIvBuffer,
    size_t destIvBufferSize,
    const char* alg,
    const KeyData* eKey,
    const unsigned char* inputForIv,
    const size_t sizeOfInputIv)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    unsigned char* destIvBuff = NULL;

    if (CryptoUtils_IsKeyNullOrEmpty(eKey) || inputForIv == NULL)
    {
        goto done;
    }

    result = CryptoUtils_EncryptBuffer(alg, eKey, NULL, inputForIv, sizeOfInputIv, destIvBuffer, destIvBufferSize);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

done:

    return result;
}

ADUC_Result CryptoUtils_DecryptBuffer(
    const char* alg,
    const KeyData* dKey,
    const char* iv,
    const unsigned char* encryptedBuff,
    const size_t encryptedBuffSize,
    unsigned char** destBuff,
    size_t* destBuffSize)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    unsigned char* decryptedBuffer = NULL;

    if (CryptoUtils_IsKeyNullOrEmpty(dKey) || encryptedBuff == NULL || destBuff == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    if (ctx == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    const EVP_CIPHER* cipherType = CryptoUtils_GetCipherType(alg);

    if (cipherType == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    const CONSTBUFFER* keyData = CONSTBUFFER_GetContent(dKey->keyData);

    if (EVP_DecryptInit(ctx, cipherType, keyData->buffer, iv) != 1)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    //
    // The following is the chunking of each of the output sets as we go through decrypting each block of the cipher
    //
    const int blockSize = EVP_CIPHER_CTX_block_size(ctx);

    //
    // Create the overall buffer at the length of the encrypted content
    //

    size_t outSize = 0;
    const size_t maxOutputBuffSize = encryptedBuffSize + (16 - (encryptedBuffSize % 16));
    decryptedBuffer = (unsigned char*)malloc(maxOutputBuffSize);

    if (decryptedBuffer == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    size_t decryptedLength = 0;

    //
    // Need to check the in length vs the out length
    //
    if (EVP_DecryptUpdate(ctx, decryptedBuffer, &decryptedLength, encryptedBuff, blockSize) != 1)
    {
        result.ExtendedResultCode = ADUC_ERC_DECRYPTION_UPDATE_FAILED;
        goto done;
    }

    outSize += decryptedLength;

    if (EVP_DecryptFinal(ctx, decryptedBuffer + decryptedLength, &decryptedLength) != 1)
    {
        result.ExtendedResultCode = ADUC_ERC_DECRYPTION_FINAL_FAILED;
        goto done;
    }
    outSize += decryptedLength;

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        free(destBuff);
        decryptedBuffer = NULL;
        outSize = 0;
    }

    *destBuff = decryptedBuffer;
    *destBuffSize = outSize;

    EVP_CIPHER_CTX_free(ctx);
    return result;
}
