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
    if (alg == UNSUPPORTED_DECRYPTION_ALG || expectedSignature == NULL || sigLength == 0 || blob == NULL
        || blobLength == 0)
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

/**
 * @brief Helper function for getting the block size of a DecryptionAlg
 *
 * @param alg alg to get the block size for
 * @return the size of the block
 */
const size_t CryptoUtils_GetBlockSizeForAlg(const DecryptionAlg alg)
{
    const EVP_CIPHER* cipher = CryptoUtils_GetCipherType(alg);

    return (EVP_CIPHER_block_size(cipher) / sizeof(char));
}

/**
 * @brief DeInit function for a KeyData structure
 *
 * @param dKey the key to be de-intialized
 */
void CryptoUtils_DeInitializeKeyData(KeyData** dKey)
{
    if (*dKey == NULL)
    {
        return;
    }

    if ((*dKey)->keyData != NULL)
    {
        CONSTBUFFER_DecRef((*dKey)->keyData);
    }

    free(*dKey);
    *dKey = NULL;
}

/**
 * @brief Checks if a KeyData strucure is null or empty
 *
 * @param key the key to check
 * @return true if it is null or empty; false otherwise
 */
bool CryptoUtils_IsKeyNullOrEmpty(const KeyData* key)
{
    if (key == NULL || key->keyData == NULL)
    {
        return true;
    }

    return false;
}

/**
 * @brief Initializes a KeyData struct with base 64 url decoded value of @p b64UrlEncodedKeyBytes
 *
 * @param dKey the key data structure to allocate with the decoded bytes of b64UrlEncodedKeyBytes
 * @param b64UrlEncodedKeyBytes the base 64 url encoded key bytes to be turned into a KeyData structure
 * @return true on success; false on failure
 */
bool CryptoUtils_InitializeKeyDataFromUrlEncodedB64String(KeyData** dKey, const char* b64UrlEncodedKeyBytes)
{
    bool success = false;
    KeyData* tempKey = NULL;

    unsigned char* keyBytesBuff = NULL;

    if (b64UrlEncodedKeyBytes == NULL || dKey == NULL)
    {
        goto done;
    }

    tempKey = malloc(sizeof(KeyData));

    if (tempKey == NULL)
    {
        goto done;
    }

    size_t decodedKeyBytesLength = Base64URLDecode(b64UrlEncodedKeyBytes, &keyBytesBuff);

    if (keyBytesBuff == NULL)
    {
        goto done;
    }

    tempKey->keyData = CONSTBUFFER_CreateWithMoveMemory(keyBytesBuff, decodedKeyBytesLength);

    if (tempKey->keyData == NULL)
    {
        goto done;
    }

    success = true;
done:

    if (!success)
    {
        CryptoUtils_DeInitializeKeyData(&tempKey);
    }

    *dKey = tempKey;
    return success;
}

/**
 * @brief Encrypts a buffer block by block and stores the result in @p destBuff
 * @param destBuff the destination buffer to be allocated with the encrypted contents of @p plainTextBuffer
 * @param destBuffSize the size of @p destBuff after encryption
 * @param alg the algorithm to use for encryption
 * @param eKey the encryption key to use for the encryption
 * @param plainTextBuffer the plain text to be encrypted into @p destbuff
 * @param plainTextBufferSize the size of @p plainTexTBuffer
 * @return a value of ADUC_Result
 */
ADUC_Result CryptoUtils_EncryptBufferBlockByBlock(
    unsigned char** destBuff,
    size_t* destBuffSize,
    const DecryptionAlg alg,
    const KeyData* eKey,
    const char* plainTextBuffer,
    const size_t plainTextBufferSize)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    unsigned char* encryptedBuffer = NULL;
    size_t encryptedBufferSize = 0;

    unsigned char* iv = NULL;
    unsigned char* blockBuffer = NULL;
    unsigned char* paddedPlainTextBuffer = NULL;

    if (CryptoUtils_IsKeyNullOrEmpty(eKey) || plainTextBuffer == NULL || destBuff == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    const size_t hangingBytes = plainTextBufferSize % 16;
    const size_t maxEncryptedBuffSize = plainTextBufferSize + 16 - hangingBytes;

    encryptedBuffer = (unsigned char*)malloc(maxEncryptedBuffSize);

    if (encryptedBuffer == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    const size_t blockSize = CryptoUtils_GetBlockSizeForAlg(alg);

    if (blockSize == 0)
    {
        result.ExtendedResultCode = ADUC_ERC_ZERO_BLOCK_SIZE;
        goto done;
    }

    size_t blockOffSet = 0;

    do
    {
        result = CryptoUtils_CalculateIV(&iv, blockSize, alg, eKey, blockOffSet);

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }

        if (hangingBytes != 0 && blockOffSet == (plainTextBufferSize - hangingBytes))
        {
            paddedPlainTextBuffer = (unsigned char*)malloc(blockSize);

            if (paddedPlainTextBuffer == NULL)
            {
                result.ExtendedResultCode = ADUC_ERC_NOMEM;
                goto done;
            }

            size_t bytesToPad = blockSize - hangingBytes;

            memcpy(paddedPlainTextBuffer, (plainTextBuffer + blockOffSet), hangingBytes);

            memset((unsigned char*)(paddedPlainTextBuffer + hangingBytes), bytesToPad, bytesToPad);

            result = CryptoUtils_EncryptBlock(&blockBuffer, alg, eKey, iv, paddedPlainTextBuffer, blockSize);

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                goto done;
            }

            memcpy((encryptedBuffer + blockOffSet), blockBuffer, blockSize);

            free(paddedPlainTextBuffer);
            paddedPlainTextBuffer = NULL;
        }
        else
        {
            result = CryptoUtils_EncryptBlock(
                &blockBuffer, alg, eKey, iv, (unsigned char*)(plainTextBuffer + blockOffSet), blockSize);

            if (IsAducResultCodeFailure(result.ResultCode))
            {
                goto done;
            }

            memcpy((encryptedBuffer + blockOffSet), blockBuffer, blockSize);
        }

        encryptedBufferSize += blockSize;

        free(blockBuffer);
        blockBuffer = NULL;

        free(iv);
        iv = NULL;

        blockOffSet += blockSize;
    } while (blockOffSet < plainTextBufferSize);

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        free(encryptedBuffer);
        encryptedBuffer = NULL;

        encryptedBufferSize = 0;
    }

    free(iv);
    free(blockBuffer);
    free(paddedPlainTextBuffer);

    *destBuff = encryptedBuffer;
    *destBuffSize = encryptedBufferSize;
    return result;
}

/**
 * @brief Encrypts a singular block from @p block into @p destBuff of size @p plainTextblockSize using the @p alg and @p eKey
 * @details if the block size of the algorithm does not match @p plainTextBlockSize the function will fail
 * @param destBuff the destination buffer allocated of size @p plainTextBlockSize that will contain the encrypted block
 * @param alg the algorithm to use for encryption
 * @param eKey the encryption key to be used for the encryption
 * @param iv the initialization vector for this block
 * @param block the block to be encrypted
 * @param plainTextBlockSize the size of @p block
 * @return a value of ADUC_Result
 */
ADUC_Result CryptoUtils_EncryptBlock(
    unsigned char** destBuff,
    const DecryptionAlg alg,
    const KeyData* eKey,
    const unsigned char* iv,
    const unsigned char* block,
    const int plainTextBlockSize)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    unsigned char* encryptedBuffer = NULL;

    if (alg == UNSUPPORTED_DECRYPTION_ALG || CryptoUtils_IsKeyNullOrEmpty(eKey) || block == NULL || destBuff == NULL)
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

    const size_t blockSize = EVP_CIPHER_block_size(cipherType);

    if (blockSize != plainTextBlockSize)
    {
        result.ExtendedResultCode = ADUC_ERC_INCORRECT_BLOCK_SIZE;
        goto done;
    }

    const CONSTBUFFER* keyData = CONSTBUFFER_GetContent(eKey->keyData);
    if (EVP_EncryptInit_ex(ctx, cipherType, NULL, keyData->buffer, iv) != 1)
    {
        result.ExtendedResultCode = ADUC_ERC_ENCRYPTION_INIT_FAILED;
        goto done;
    }

    if (EVP_CIPHER_CTX_set_padding(ctx, 0) != 1)
    {
        goto done;
    }

    size_t outSize = 0;

    encryptedBuffer = (unsigned char*)malloc(blockSize);

    if (encryptedBuffer == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    int encryptedLength = 0;
    if (EVP_EncryptUpdate(ctx, encryptedBuffer, &encryptedLength, block, plainTextBlockSize) != 1)
    {
        result.ExtendedResultCode = ADUC_ERC_ENCRYPTION_UPDATE_FAILED;
        goto done;
    }

    outSize += encryptedLength;

    if (outSize > plainTextBlockSize)
    {
        result.ExtendedResultCode = ADUC_ERC_INCORRECT_BLOCK_SIZE;
        goto done;
    }

    int finalEncryptedLength = 0;

    if (EVP_EncryptFinal_ex(ctx, encryptedBuffer + encryptedLength, &finalEncryptedLength) != 1)
    {
        result.ExtendedResultCode = ADUC_ERC_ENCRYPTION_FINAL_FAILED;
        goto done;
    }

    EVP_CIPHER_CTX_cleanup(ctx);

    outSize += finalEncryptedLength;

    if (outSize != plainTextBlockSize)
    {
        result.ExtendedResultCode = ADUC_ERC_INCORRECT_BLOCK_SIZE;
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        free(encryptedBuffer);
        encryptedBuffer = NULL;
        outSize = 0;
    }

    *destBuff = encryptedBuffer;

    return result;
}

/**
 * @brief Helper function that loads the @p buffToFill with the offset bytes
 *
 * @param buffToFill the buffer to allocated with the bytes of offset
 * @param blockSize the blockSize for the block who's offset is being loading into the buffer
 * @param offset the offset to be loaded into @p buffToFill
 */
void LoadIvBufferWithOffsetHelper(unsigned char* buffToFill, size_t blockSize, unsigned long long offset)
{
    unsigned long long mask = 0xFF;
    for (int i = blockSize - 1; i >= 0; --i)
    {
        buffToFill[i] = mask & offset;
        mask <<= 8; // shift left 8 bytes to select the next value
    }
}

/**
 * @brief Helper fucntion for calculating the intialization vector for a certain block
 * @details the IV is calculated as the Encrypt(<offSet-in-a-byte-buffer-of-block-size>, eKey)
 * @param destIvBuffer the buffer to store the bytes of the iv in
 * @param blockSize the size of the block for the algorithm, indicates the size that @p destIvBuffer must be
 * @param alg the algorithm the IV is being generated for
 * @param eKey the key to use to calculate the intialization vector
 * @param offSet the offSet for the block who's IV is being calculated
 * @return a value of ADUC_Result
 */
ADUC_Result CryptoUtils_CalculateIV(
    unsigned char** destIvBuffer, size_t blockSize, const DecryptionAlg alg, const KeyData* eKey, const size_t offSet)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    unsigned char* tempDestIvBuff = NULL;
    unsigned char* ivByteBuff = NULL;

    if (CryptoUtils_IsKeyNullOrEmpty(eKey) || destIvBuffer == NULL)
    {
        goto done;
    }

    ivByteBuff = (unsigned char*)malloc(blockSize);

    memset(ivByteBuff, 0, blockSize);

    LoadIvBufferWithOffsetHelper(ivByteBuff, blockSize, offSet);

    result = CryptoUtils_EncryptBlock(&tempDestIvBuff, alg, eKey, NULL, ivByteBuff, blockSize);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        free(tempDestIvBuff);
        tempDestIvBuff = NULL;
    }

    free(ivByteBuff);

    *destIvBuffer = tempDestIvBuff;

    return result;
}

/**
 * @brief Helper function for finding the number of padded bytes in the searchBlock according to PKCS#7
 *
 * @param searchBlock the block to search for the padded size
 * @param blockSize the size of the block
 * @return the number of bytes that are padding in @p searchBlock
 */
size_t FindPKCS7BytesPadded(char* searchBlock, const size_t blockSize)
{
    size_t paddingCandidate = searchBlock[blockSize - 1];

    if (paddingCandidate > 0 && paddingCandidate <= 0x10)
    {
        size_t index;
        for (index = blockSize - 1; index >= 0; --index)
        {
            if (searchBlock[index] != paddingCandidate)
            {
                break;
            }
        }

        size_t numberSame = blockSize - 1 - index;

        if (numberSame == paddingCandidate)
        {
            return paddingCandidate;
        }
    }
    return 0;
}

/**
 * @brief Helper function that trims @p decryptedBuffer according to PKCS#7 padding and stores the result in @p trimmedBufferHandle of size @p trimmedSize
 *
 * @param trimmedBufferHandle the handle for storing the trimmed content
 * @param trimmedSize the size of @p trimmedBufferHandle
 * @param decryptedBuffer the buffer holding the data to be trimmed
 * @param bufferSize the size of @p decryptedBuffer
 * @param blockSize the block size of the algorithm being used
 * @return true on success; false on failure
 */
bool CryptoUtils_TrimPKCS7Padding(
    char** trimmedBufferHandle,
    size_t* trimmedSize,
    const char* decryptedBuffer,
    const size_t bufferSize,
    const size_t blockSize)
{
    bool success = false;

    char* tempBuff = NULL;
    size_t tempTrimmedSize = 0;

    char* possiblePaddedBlock = (char*)(decryptedBuffer + bufferSize - blockSize);

    size_t numBytesPadded = FindPKCS7BytesPadded(possiblePaddedBlock, blockSize);

    if (numBytesPadded == 0)
    {
        goto done;
    }

    tempTrimmedSize = bufferSize - numBytesPadded;

    tempBuff = (char*)malloc(tempTrimmedSize);

    if (tempBuff == NULL)
    {
        goto done;
    }

    memcpy(tempBuff, decryptedBuffer, tempTrimmedSize);

    success = true;
done:

    if (!success)
    {
        free(tempBuff);
        tempBuff = NULL;
        tempTrimmedSize = 0;
    }
    *trimmedBufferHandle = tempBuff;
    *trimmedSize = tempTrimmedSize;
    return success;
}

/**
 * @brief Decrypts a singular @p block of data to @p destBuff using the specified algorithm, key, and iv
 * @details function will fail if @p encryptedBlockSize does not match the expected block size of the @p alg
 *
 * @param destBuff A buffer that will be allocated with the decrypted block (size will be the same as the encrypted block)
 * @param alg A value of DecryptionAlg that indicates the algorithm to be used for the decryption
 * @param dKey the key data for the decryption key
 * @param iv the initialization vector to be used for the decryption, should be the size of one block
 * @param block the block to decrypt into @p destBuff
 * @param encryptedBlockSize the size of the block
 * @return a value of ADUC_Result
 */
ADUC_Result CryptoUtils_DecryptBlock(
    unsigned char** destBuff,
    const DecryptionAlg alg,
    const KeyData* dKey,
    const unsigned char* iv,
    const unsigned char* block,
    const int encryptedBlockSize)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    unsigned char* decryptedBuffer = NULL;

    if (CryptoUtils_IsKeyNullOrEmpty(dKey) || block == NULL || destBuff == NULL)
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

    if (EVP_DecryptInit_ex(ctx, cipherType, NULL, keyData->buffer, iv) != 1)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    if (EVP_CIPHER_CTX_set_padding(ctx, 0) != 1)
    {
        goto done;
    }

    const int blockSize = EVP_CIPHER_CTX_block_size(ctx);

    if (blockSize != encryptedBlockSize)
    {
        result.ExtendedResultCode = ADUC_ERC_INCORRECT_BLOCK_SIZE;
        goto done;
    }

    size_t outSize = 0;

    decryptedBuffer = (unsigned char*)malloc(blockSize);

    if (decryptedBuffer == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    int decryptedLength = 0;

    if (EVP_DecryptUpdate(ctx, decryptedBuffer, &decryptedLength, block, encryptedBlockSize) != 1)
    {
        result.ExtendedResultCode = ADUC_ERC_DECRYPTION_UPDATE_FAILED;
        goto done;
    }

    outSize += decryptedLength;

    //
    // Can be less than here if there is residual data in the context
    //
    if (outSize > encryptedBlockSize)
    {
        result.ExtendedResultCode = ADUC_ERC_INCORRECT_BLOCK_SIZE;
        goto done;
    }

    if (EVP_DecryptFinal_ex(ctx, decryptedBuffer + decryptedLength, &decryptedLength) != 1)
    {
        result.ExtendedResultCode = ADUC_ERC_DECRYPTION_FINAL_FAILED;
        goto done;
    }

    EVP_CIPHER_CTX_cleanup(ctx);

    outSize += decryptedLength;

    if (outSize != encryptedBlockSize)
    {
        result.ExtendedResultCode = ADUC_ERC_INCORRECT_BLOCK_SIZE;
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        free(decryptedBuffer);
        decryptedBuffer = NULL;
    }

    *destBuff = decryptedBuffer;

    EVP_CIPHER_CTX_free(ctx);
    return result;
}

/**
 * @brief Decrypts @p encryptedBuffer and allocates the result to @p destbuff with size @p destBuffSize
 * @details During the decryption process any padding is trimmed and removed before the result is returned to the caller.
 * @param destBuff the destination for the decrypted data
 * @param destBuffSize the size of the data allocated to @p destBuff
 * @param alg the algorithm to use for decryption
 * @param dKey the decryption key
 * @param encryptedBuffer the buffer with the encrypted data
 * @param encryptedBufferSize the size of @p encryptedBuffer
 * @return a value of ADUC_Result
 */
ADUC_Result CryptoUtils_DecryptBufferBlockByBlock(
    char** destBuff,
    size_t* destBuffSize,
    const DecryptionAlg alg,
    const KeyData* dKey,
    const unsigned char* encryptedBuffer,
    const size_t encryptedBufferSize)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    char* decryptedBuffer = NULL;
    size_t decryptedBufferSize = 0;
    char* trimmedBuffer = NULL;
    size_t trimmedBufferSize = 0;

    unsigned char* iv = NULL;
    unsigned char* blockBuffer = NULL;

    if (CryptoUtils_IsKeyNullOrEmpty(dKey) || encryptedBuffer == NULL || destBuff == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    decryptedBuffer = (char*)malloc(encryptedBufferSize);

    if (decryptedBuffer == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    const size_t blockSize = CryptoUtils_GetBlockSizeForAlg(alg);

    if (blockSize == 0)
    {
        result.ExtendedResultCode = ADUC_ERC_ZERO_BLOCK_SIZE;
        goto done;
    }

    size_t blockOffSet = 0;

    do
    {
        result = CryptoUtils_CalculateIV(&iv, blockSize, alg, dKey, blockOffSet);

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }

        result = CryptoUtils_DecryptBlock(&blockBuffer, alg, dKey, iv, (encryptedBuffer + blockOffSet), blockSize);

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }

        memcpy((decryptedBuffer + blockOffSet), blockBuffer, blockSize);

        decryptedBufferSize += blockSize;

        free(blockBuffer);
        blockBuffer = NULL;

        free(iv);
        iv = NULL;

        blockOffSet += blockSize;

    } while (blockOffSet < encryptedBufferSize);

    if (!CryptoUtils_TrimPKCS7Padding(
            &trimmedBuffer, &trimmedBufferSize, decryptedBuffer, decryptedBufferSize, blockSize))
    {
        goto done;
    }

    if (trimmedBuffer == NULL || trimmedBufferSize == 0)
    {
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        free(decryptedBuffer);
        decryptedBuffer = NULL;
        decryptedBufferSize = 0;
    }

    free(iv);
    free(blockBuffer);
    free(decryptedBuffer);

    *destBuff = trimmedBuffer;
    *destBuffSize = trimmedBufferSize;

    return result;
}
