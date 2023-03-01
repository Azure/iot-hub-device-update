/**
 * @file crypto_lib.c
 * @brief Provides an implementation of cryptogrpahic functions for hashing, encrypting, and verifiying.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "crypto_lib.h"
#include "base64_utils.h"
#include "decryption_alg_types.h"
#include "root_key_util.h"
#include <azure_c_shared_utility/azure_base64.h>
#include <ctype.h>
#include <openssl/aes.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

/**
 * @brief Algorithm_Id values and supported algorithms
 */
typedef enum tagAlgorithm_Id
{
    Alg_NotSupported = 0,
    Alg_RSA256 = 1
} Algorithm_Id;

//
// Helper Functions
//

/**
 * @brief Helper function that casts the CryptoKeyHandle to the EVP_PKEY struct
 * @details The CryptoKey pointed to by handle MUST have been created using one of the crypto_lib function
 * @param key the CryptoKeyHandle to be cast
 * @returns an EVP_PKEY pointer
 */
EVP_PKEY* CryptoKeyHandleToEVP_PKEY(CryptoKeyHandle key)
{
    return (EVP_PKEY*)key;
}

/**
 * @brief Helper function that returns the Algoprithm_Id corresponding to the string @p alg
 * @details The comparison does not care about case
 * @param alg the string to be compared to find the algorithm.
 * @returns Alg_NotSupported if there is no algorithm related to the string @p alg, otherwise the Algorithm_Id
 */
Algorithm_Id AlgorithmIdFromString(const char* alg)
{
    Algorithm_Id algorithmId = Alg_NotSupported;
    if (strcasecmp(alg, "rs256") == 0)
    {
        algorithmId = Alg_RSA256;
    }
    return algorithmId;
}

/**
 * @brief Verifies the @p signature using RS256 on the @p blob and the @p key.
 * @details This RS256 implementation uses the RSA_PKCS1_PADDING type.
 *
 * @param signature the expected signature to compare against the one computed from @p blob using RS256 and the @p key
 * @param sigLength the total length of the signature
 * @param blob the data for which the RS256 encoded hash will be computed from
 * @param blobLength the size of buffer @p blob
 * @param keyToSign the public key for the RS256 validation of the expected signature against blob
 * @returns True if @p signature equals the one computer from the blob and key using RS256, False otherwise
 */
bool VerifyRS256Signature(
    const uint8_t* signature,
    const size_t sigLength,
    const uint8_t* blob,
    const size_t blobLength,
    CryptoKeyHandle keyToSign)
{
    bool success = false;
    EVP_MD_CTX* mdctx = NULL;
    EVP_PKEY_CTX* ctx = NULL;

    const size_t digest_len = 32;
    uint8_t digest[digest_len];

    if ((mdctx = EVP_MD_CTX_new()) == NULL)
    {
        goto done;
    }

    const EVP_MD* hash_alg = EVP_sha256();
    if (EVP_DigestInit_ex(mdctx, hash_alg, NULL) != 1)
    {
        goto done;
    }

    if (EVP_DigestUpdate(mdctx, blob, blobLength) != 1)
    {
        goto done;
    }

    unsigned int digest_len_temp = (unsigned int)digest_len;
    if (EVP_DigestFinal_ex(mdctx, digest, &digest_len_temp) != 1)
    {
        goto done;
    }

    EVP_PKEY* pu_key = CryptoKeyHandleToEVP_PKEY(keyToSign);
    ctx = EVP_PKEY_CTX_new(pu_key, NULL);

    if (ctx == NULL)
    {
        goto done;
    }

    if (EVP_PKEY_verify_init(ctx) <= 0)
    {
        goto done;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0)
    {
        goto done;
    }

    if (EVP_PKEY_CTX_set_signature_md(ctx, hash_alg) <= 0)
    {
        goto done;
    }

    if (EVP_PKEY_verify(ctx, signature, sigLength, digest, digest_len) == 1)
    {
        success = true;
    }

done:

    if (mdctx != NULL)
    {
        EVP_MD_CTX_free(mdctx);
    }

    if (ctx != NULL)
    {
        EVP_PKEY_CTX_free(ctx);
    }

    return success;
}

//
// Signature Verification
//

/**
 * @brief Checks if the provided signature is valid using the associated algorithm and provided key.
 * @details the alg provided must be one of the currently supported ones.
 * @param alg the algorithm to use for signature verification
 * @param expectedSignature the expected signature to validate
 * @param blob buffer that contains the data for computing a signature to be checked against @p expectedSignature should be an array of bytes
 * @param blobLength the size of buffer @p blob
 * @param keyToSign key that should be used for generating the computed signature. May be NULL depending on the algorithm
 * @returns true if the signature is valid, false if it is invalid
 */
bool IsValidSignature(
    const char* alg,
    const uint8_t* expectedSignature,
    size_t sigLength,
    const uint8_t* blob,
    size_t blobLength,
    CryptoKeyHandle keyToSign)
{
    if (alg == NULL || expectedSignature == NULL || sigLength == 0 || blob == NULL || blobLength == 0)
    {
        return false;
    }
    bool result = false;

    Algorithm_Id algId = AlgorithmIdFromString(alg);

    switch (algId)
    {
    case Alg_RSA256:
        result = VerifyRS256Signature(expectedSignature, sigLength, blob, blobLength, keyToSign);
        break;

    default:
    case Alg_NotSupported:
        result = false;
    }

    return result;
}

/**
 * @brief Makes an RSA Key from the modulus (N) and exponent (e) provided in their byte format
 * @param N a buffer containing the bytes for the modulus
 * @param N_len the length of the byte buffer
 * @param e a buffer containing the bytes for the exponent
 * @param e_len the length of the exponent buffer
 * @returns NULL on failure and a key on success
 */
CryptoKeyHandle RSAKey_ObjFromBytes(uint8_t* N, size_t N_len, uint8_t* e, size_t e_len)
{
    EVP_PKEY* result = NULL;

    BIGNUM* rsa_N = NULL;
    BIGNUM* rsa_e = NULL;

    RSA* rsa = RSA_new();

    if (rsa == NULL)
    {
        goto done;
    }

    rsa_N = BN_bin2bn(N, N_len, NULL);

    if (rsa_N == NULL)
    {
        goto done;
    }

    rsa_e = BN_bin2bn(e, e_len, NULL);

    if (rsa_e == NULL)
    {
        goto done;
    }

    if (RSA_set0_key(rsa, rsa_N, rsa_e, NULL) == 0)
    {
        goto done;
    }

    EVP_PKEY* pkey = EVP_PKEY_new();

    if (pkey == NULL)
    {
        goto done;
    }

    if (EVP_PKEY_assign_RSA(pkey, rsa) == 0)
    {
        goto done;
    }

    result = pkey;
done:

    if (result == NULL)
    {
        BN_free(rsa_N);
        BN_free(rsa_e);
    }

    return CryptoKeyHandleToEVP_PKEY(result);
}

/**
 *@brief Makes an RSA Key from the base64 encoded strings of the modulus and exponent
 *@param encodedN a string of the modulus encoded in base64
 *@param encodede a string of the exponent encoded in base64
 *@return NULL on failure and a pointer to a key on success
 */
CryptoKeyHandle RSAKey_ObjFromStrings(const char* N, const char* e)
{
    EVP_PKEY* result = NULL;
    EVP_PKEY* pkey = NULL;
    BIGNUM* M = NULL;
    BIGNUM* E = NULL;

    RSA* rsa = RSA_new();
    if (rsa == NULL)
    {
        goto done;
    }

    M = BN_new();
    if (M == NULL)
    {
        goto done;
    }

    E = BN_new();
    if (E == NULL)
    {
        goto done;
    }

    if (BN_hex2bn(&M, N) == 0)
    {
        goto done;
    }

    if (BN_hex2bn(&E, e) == 0)
    {
        goto done;
    }

    if (RSA_set0_key(rsa, M, E, NULL) == 0)
    {
        goto done;
    }

    pkey = EVP_PKEY_new();
    if (EVP_PKEY_assign_RSA(pkey, rsa) == 0)
    {
        goto done;
    }

    result = pkey;

done:
    if (result == NULL)
    {
        BN_free(M);
        BN_free(E);
        EVP_PKEY_free(pkey);
    }

    return CryptoKeyHandleToEVP_PKEY(result);
}

/**
 * @brief Makes an RSA key from pure strings.
 * @param N the modulus in string format
 * @param e the exponent in string format
 * @return NULL on failure and a pointer to a key on success
 */
CryptoKeyHandle RSAKey_ObjFromB64Strings(const char* encodedN, const char* encodedE)
{
    CryptoKeyHandle result = NULL;
    BUFFER_HANDLE eBuff = NULL;

    BUFFER_HANDLE nBuff = Azure_Base64_Decode(encodedN);
    if (nBuff == NULL)
    {
        goto done;
    }

    eBuff = Azure_Base64_Decode(encodedE);
    if (eBuff == NULL)
    {
        goto done;
    }

    result =
        RSAKey_ObjFromBytes(BUFFER_u_char(nBuff), BUFFER_length(nBuff), BUFFER_u_char(eBuff), BUFFER_length(eBuff));

done:
    BUFFER_delete(nBuff);
    BUFFER_delete(eBuff);

    return result;
}

/**
 * @brief Frees the key structure
 * @details Caller should assume the key is invalid after this call
 * @param key the key to free
 */
void FreeCryptoKeyHandle(CryptoKeyHandle key)
{
    EVP_PKEY_free(CryptoKeyHandleToEVP_PKEY(key));
}

/**
 * @brief Returns the master key for the provided kid
 * @details this cals into the master_key_utility to get the key
 * @param kid the key identifier
 * @returns NULL on failure and a pointer to a key on success.
 */
CryptoKeyHandle GetRootKeyForKeyID(const char* kid)
{
    return GetKeyForKid(kid);
}

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

bool CryptoUtils_InitializeKeyData(KeyData** dKey, const char* keyBytes, const size_t keySize)
{
    bool success = false;
    KeyData* tempKey = NULL;

    unsigned char* keyBytesBuff = NULL;

    if (keyBytes == NULL || dKey == NULL)
    {
        goto done;
    }

    tempKey = malloc(sizeof(KeyData));

    if (tempKey == NULL)
    {
        goto done;
    }

    keyBytesBuff = (unsigned char*)malloc(sizeof(char) * keySize);

    if (keyBytesBuff == NULL)
    {
        goto done;
    }

    memcpy(keyBytesBuff, keyBytes, keySize);

    tempKey->keyData = CONSTBUFFER_CreateWithMoveMemory(keyBytesBuff, keySize);

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
