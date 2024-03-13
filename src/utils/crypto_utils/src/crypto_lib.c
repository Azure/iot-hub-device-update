/**
 * @file crypto_lib.c
 * @brief Provides an implementation of cryptogrpahic functions for hashing, encrypting, and verifiying.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "crypto_lib.h"
#include "base64_utils.h"
#include "root_key_util.h"
#include <azure_c_shared_utility/azure_base64.h>
#include <ctype.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#    include <openssl/encoder.h>
#    include <openssl/param_build.h>
#endif

#include <openssl/rsa.h>
#include <stdio.h>
#include <string.h>

#include <aducpal/strings.h> // strcasecmp

#define SHA256NUMBYTES 32
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
 * @brief Get the Evp Md (message digest) from Sha Alg string.
 *
 * @param alg The sha alg str, e.g. "sha256"
 * @return const EVP_MD* The corresponding sha alg EVP_MD, or NULL if not supported.
 */
static const EVP_MD* GetEvpMdFromShaAlg(const char* alg)
{
    if (strcmp(alg, "sha256") == 0)
    {
        return EVP_sha256();
    }

    // unsupported
    return NULL;
}

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

    if (ADUCPAL_strcasecmp(alg, "rs256") == 0)
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

    uint8_t digest[32];
    const size_t digest_len = ARRAY_SIZE(digest);

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
bool CryptoUtils_IsValidSignature(
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

CryptoKeyHandle RSAKey_ObjFromModulusBytesExponentInt(const uint8_t* N, size_t N_len, const unsigned int e)
{
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    int status = 0;
    EVP_PKEY* result = NULL;
    EVP_PKEY_CTX* ctx = NULL;
    OSSL_PARAM_BLD* param_bld = NULL;
    OSSL_PARAM* params = NULL;
    BIGNUM* bn_N = NULL;
    BIGNUM* bn_e = NULL;

    ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);

    if (ctx == NULL)
    {
        goto done;
    }

    bn_N = BN_new();

    if (bn_N == NULL)
    {
        goto done;
    }

    bn_e = BN_new();

    if (bn_e == NULL)
    {
        goto done;
    }

    if (BN_bin2bn(N, (int)N_len, bn_N) == 0)
    {
        goto done;
    }

    if (BN_set_word(bn_e, e) == 0)
    {
        goto done;
    }

    param_bld = OSSL_PARAM_BLD_new();

    if (param_bld == NULL)
    {
        goto done;
    }

    status = OSSL_PARAM_BLD_push_BN(param_bld, "n", bn_N);
    if (status != 1)
    {
        goto done;
    }

    status = OSSL_PARAM_BLD_push_BN(param_bld, "e", bn_e);
    if (status != 1)
    {
        goto done;
    }

    status = OSSL_PARAM_BLD_push_BN(param_bld, "d", NULL);
    if (status != 1)
    {
        goto done;
    }

    params = OSSL_PARAM_BLD_to_param(param_bld);
    if (params == NULL)
    {
        goto done;
    }

    status = EVP_PKEY_fromdata_init(ctx);
    if (status != 1)
    {
        goto done;
    }

    status = EVP_PKEY_fromdata(ctx, &result, EVP_PKEY_PUBLIC_KEY, params);
    if (status != 1)
    {
        goto done;
    }
done:

    if (ctx != NULL)
    {
        EVP_PKEY_CTX_free(ctx);
    }

    if (param_bld != NULL)
    {
        OSSL_PARAM_BLD_free(param_bld);
    }

    if (params != NULL)
    {
        OSSL_PARAM_free(params);
    }

    if (bn_N != NULL)
    {
        BN_free(bn_N);
    }

    if (bn_e != NULL)
    {
        BN_free(bn_e);
    }

    if (status == 0 && result != NULL)
    {
        EVP_PKEY_free(result);
        result = NULL;
    }

    return CryptoKeyHandleToEVP_PKEY(result);

#else
    _Bool success = false;
    EVP_PKEY* pkey = NULL;

    BIGNUM* rsa_N = NULL;
    BIGNUM* rsa_e = NULL;

    RSA* rsa = RSA_new();

    if (rsa == NULL)
    {
        goto done;
    }

    rsa_N = BN_bin2bn(N, (int)N_len, NULL);

    if (rsa_N == NULL)
    {
        goto done;
    }

    rsa_e = BN_new();

    if (rsa_e == NULL)
    {
        goto done;
    }

    if (BN_set_word(rsa_e, e) == 0)
    {
        goto done;
    }

    if (RSA_set0_key(rsa, rsa_N, rsa_e, NULL) == 0)
    {
        goto done;
    }

    pkey = EVP_PKEY_new();

    if (pkey == NULL)
    {
        goto done;
    }

    if (EVP_PKEY_assign_RSA(pkey, rsa) == 0)
    {
        goto done;
    }

    success = true;

done:

    if (!success)
    {
        if (pkey != NULL)
        {
            EVP_PKEY_free(pkey);
        }
        else if (rsa != NULL)
        {
            RSA_free(rsa);
        }
        else
        {
            BN_free(rsa_N);
            BN_free(rsa_e);
        }
    }

    return CryptoKeyHandleToEVP_PKEY(pkey);
#endif
}

/**
 * @brief Makes an RSA Key from the modulus (N) and exponent (e) provided in their byte format
 * @param N a buffer containing the bytes for the modulus
 * @param N_len the length of the byte buffer
 * @param e a buffer containing the bytes for the exponent
 * @param e_len the length of the exponent buffer
 * @returns NULL on failure and a key on success
 */
CryptoKeyHandle RSAKey_ObjFromBytes(const uint8_t* N, size_t N_len, const uint8_t* e, size_t e_len)
{
#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
    int status = 0;
    EVP_PKEY* result = NULL;
    EVP_PKEY_CTX* ctx = NULL;
    OSSL_PARAM_BLD* param_bld = NULL;
    OSSL_PARAM* params = NULL;
    BIGNUM* bn_N = NULL;
    BIGNUM* bn_e = NULL;

    ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);

    if (ctx == NULL)
    {
        goto done;
    }

    bn_N = BN_new();

    if (bn_N == NULL)
    {
        goto done;
    }

    bn_e = BN_new();

    if (bn_e == NULL)
    {
        goto done;
    }

    if (BN_bin2bn(N, (int)N_len, bn_N) == 0)
    {
        goto done;
    }

    if (BN_bin2bn(e, (int)e_len, bn_e) == 0)
    {
        goto done;
    }

    param_bld = OSSL_PARAM_BLD_new();

    if (param_bld == NULL)
    {
        goto done;
    }

    status = OSSL_PARAM_BLD_push_BN(param_bld, "n", bn_N);
    if (status != 1)
    {
        goto done;
    }

    status = OSSL_PARAM_BLD_push_BN(param_bld, "e", bn_e);
    if (status != 1)
    {
        goto done;
    }

    status = OSSL_PARAM_BLD_push_BN(param_bld, "d", NULL);
    if (status != 1)
    {
        goto done;
    }

    params = OSSL_PARAM_BLD_to_param(param_bld);
    if (params == NULL)
    {
        goto done;
    }

    status = EVP_PKEY_fromdata_init(ctx);
    if (status != 1)
    {
        goto done;
    }

    status = EVP_PKEY_fromdata(ctx, &result, EVP_PKEY_PUBLIC_KEY, params);
    if (status != 1)
    {
        goto done;
    }

done:

    if (ctx != NULL)
    {
        EVP_PKEY_CTX_free(ctx);
    }

    if (param_bld != NULL)
    {
        OSSL_PARAM_BLD_free(param_bld);
    }

    if (params != NULL)
    {
        OSSL_PARAM_free(params);
    }

    if (bn_N != NULL)
    {
        BN_free(bn_N);
    }

    if (bn_e != NULL)
    {
        BN_free(bn_e);
    }

    if (status == 0 && result != NULL)
    {
        EVP_PKEY_free(result);
        result = NULL;
    }

    return CryptoKeyHandleToEVP_PKEY(result);
#else
    bool success = false;
    EVP_PKEY* pkey = NULL;

    BIGNUM* rsa_N = NULL;
    BIGNUM* rsa_e = NULL;

    RSA* rsa = RSA_new();

    if (rsa == NULL)
    {
        goto done;
    }

    rsa_N = BN_bin2bn(N, (int)N_len, NULL);

    if (rsa_N == NULL)
    {
        goto done;
    }

    rsa_e = BN_bin2bn(e, (int)e_len, NULL);

    if (rsa_e == NULL)
    {
        goto done;
    }

    if (RSA_set0_key(rsa, rsa_N, rsa_e, NULL) == 0)
    {
        goto done;
    }

    pkey = EVP_PKEY_new();

    if (pkey == NULL)
    {
        goto done;
    }

    if (EVP_PKEY_assign_RSA(pkey, rsa) == 0)
    {
        goto done;
    }

    success = true;
done:

    if (!success)
    {
        if (pkey != NULL)
        {
            EVP_PKEY_free(pkey);
        }
        else if (rsa != NULL)
        {
            RSA_free(rsa);
        }
        else
        {
            BN_free(rsa_N);
            BN_free(rsa_e);
        }

        pkey = NULL;
    }

    return CryptoKeyHandleToEVP_PKEY(pkey);
#endif // #if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
}

/**
 *@brief Makes an RSA Key from the base64 encoded strings of the modulus and exponent
 *@param encodedN a string of the modulus encoded in base64
 *@param encodede a string of the exponent encoded in base64
 *@return NULL on failure and a pointer to a key on success
 */
CryptoKeyHandle RSAKey_ObjFromStrings(const char* N, const char* e)
{
#if (OPENSSL_VERSION_NUMBER >= 0x30000000L)
    int status = 0;
    EVP_PKEY* result = NULL;
    EVP_PKEY_CTX* ctx = NULL;
    OSSL_PARAM_BLD* param_bld = NULL;
    OSSL_PARAM* params = NULL;
    BIGNUM* bn_N = NULL;
    BIGNUM* bn_e = NULL;

    ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);

    if (ctx == NULL)
    {
        goto done;
    }

    bn_N = BN_new();
    if (bn_N == NULL)
    {
        goto done;
    }

    bn_e = BN_new();
    if (bn_e == NULL)
    {
        goto done;
    }

    if (BN_hex2bn(&bn_N, N) == 0)
    {
        goto done;
    }

    if (BN_hex2bn(&bn_e, e) == 0)
    {
        goto done;
    }

    param_bld = OSSL_PARAM_BLD_new();
    if (param_bld == NULL)
    {
        goto done;
    }

    status = OSSL_PARAM_BLD_push_BN(param_bld, "n", bn_N);
    if (status != 1)
    {
        goto done;
    }

    status = OSSL_PARAM_BLD_push_BN(param_bld, "e", bn_e);
    if (status != 1)
    {
        goto done;
    }

    status = OSSL_PARAM_BLD_push_BN(param_bld, "d", NULL);
    if (status != 1)
    {
        goto done;
    }

    params = OSSL_PARAM_BLD_to_param(param_bld);
    if (params == NULL)
    {
        goto done;
    }

    status = EVP_PKEY_fromdata_init(ctx);
    if (status != 1)
    {
        goto done;
    }

    status = EVP_PKEY_fromdata(ctx, &result, EVP_PKEY_PUBLIC_KEY, params);
    if (status != 1)
    {
        goto done;
    }

done:

    if (ctx != NULL)
    {
        EVP_PKEY_CTX_free(ctx);
    }

    if (param_bld != NULL)
    {
        OSSL_PARAM_BLD_free(param_bld);
    }

    if (params != NULL)
    {
        OSSL_PARAM_free(params);
    }

    if (bn_N != NULL)
    {
        BN_free(bn_N);
    }

    if (bn_e != NULL)
    {
        BN_free(bn_e);
    }

    if (status == 0 && result != NULL)
    {
        EVP_PKEY_free(result);
        result = NULL;
    }

    return CryptoKeyHandleToEVP_PKEY(result);
#else
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
        if (pkey != NULL)
        {
            EVP_PKEY_free(pkey);
        }
        else if (rsa != NULL)
        {
            RSA_free(rsa);
        }
        else
        {
            BN_free(M);
            BN_free(E);
        }
    }

    return CryptoKeyHandleToEVP_PKEY(result);
#endif
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
void CryptoUtils_FreeCryptoKeyHandle(CryptoKeyHandle key)
{
    EVP_PKEY_free(CryptoKeyHandleToEVP_PKEY(key));
}

/**
 * @brief Computes a SHA256 hash from bytes.
 *
 * @param buf The handle to the input bytes to be hashed.
 * @return CONSTBUFFER_HANDLE The digest buffer handle. Caller will own it and need to call CONSTBUFFER_DecRef() to free.
 */
CONSTBUFFER_HANDLE CryptoUtils_CreateSha256Hash(const CONSTBUFFER_HANDLE buf)
{
    unsigned char hashBuf[SHA256NUMBYTES];
    memset(hashBuf, 0, sizeof(hashBuf));
    const CONSTBUFFER* constbuf = CONSTBUFFER_GetContent(buf);

    CONSTBUFFER_HANDLE out_handle = NULL;

    const EVP_MD* msg_digest = NULL;
    EVP_MD_CTX* mdctx;

    if (buf == NULL)
    {
        return NULL;
    }

    // Initialize OpenSSL digest context
    if ((mdctx = EVP_MD_CTX_new()) == NULL)
    {
        goto done;
    }

    msg_digest = GetEvpMdFromShaAlg("sha256");
    if (msg_digest == NULL)
    {
        goto done;
    }

    // Initialize OpenSSL digest operation
    if (1 != EVP_DigestInit_ex(mdctx, msg_digest, NULL))
    {
        goto done;
    }

    // Hash the input data
    if (1 != EVP_DigestUpdate(mdctx, constbuf->buffer, constbuf->size))
    {
        goto done;
    }

    // Finalize the digest and get the output
    if (1 != EVP_DigestFinal_ex(mdctx, &hashBuf[0], NULL))
    {
        goto done;
    }

    out_handle = CONSTBUFFER_Create(hashBuf, sizeof(hashBuf));

done:
    EVP_MD_CTX_free(mdctx);

    return out_handle;
}

CONSTBUFFER_HANDLE CryptoUtils_GenerateRsaPublicKey(const char* modulus_b64url, const char* exponent_b64url)
{
    CONSTBUFFER_HANDLE publicKeyData = NULL;

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    unsigned char *modulus_bytes, *exponent_bytes;
    int modulus_length, exponent_length;
    int status = 0;
    EVP_PKEY* pkey = NULL;
    EVP_PKEY_CTX* ctx = NULL;
    OSSL_ENCODER_CTX* pkey_encoder_ctx = NULL;
    OSSL_PARAM_BLD* param_bld = NULL;
    OSSL_PARAM* params = NULL;

    BIGNUM* bn_modulus = NULL;
    BIGNUM* bn_exponent = NULL;
    unsigned char* der_encoded_bytes = NULL;
    size_t der_length = 0;

    ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);

    modulus_length = (int)Base64URLDecode(modulus_b64url, &modulus_bytes);
    if (modulus_length == 0)
    {
        goto done;
    }

    exponent_length = (int)Base64URLDecode(exponent_b64url, &exponent_bytes);
    if (exponent_length == 0)
    {
        goto done;
    }

    bn_modulus = BN_bin2bn(modulus_bytes, modulus_length, NULL);
    if (bn_modulus == NULL)
    {
        goto done;
    }

    bn_exponent = BN_bin2bn(exponent_bytes, exponent_length, NULL);
    if (bn_exponent == NULL)
    {
        goto done;
    }

    param_bld = OSSL_PARAM_BLD_new();

    if (param_bld == NULL)
    {
        goto done;
    }

    status = OSSL_PARAM_BLD_push_BN(param_bld, "n", bn_modulus);
    if (status != 1)
    {
        goto done;
    }

    status = OSSL_PARAM_BLD_push_BN(param_bld, "e", bn_exponent);
    if (status != 1)
    {
        goto done;
    }

    status = OSSL_PARAM_BLD_push_BN(param_bld, "d", NULL);
    if (status != 1)
    {
        goto done;
    }

    params = OSSL_PARAM_BLD_to_param(param_bld);
    if (params == NULL)
    {
        goto done;
    }

    status = EVP_PKEY_fromdata_init(ctx);
    if (status != 1)
    {
        goto done;
    }

    status = EVP_PKEY_fromdata(ctx, &pkey, EVP_PKEY_PUBLIC_KEY, params);
    if (status != 1)
    {
        goto done;
    }

    pkey_encoder_ctx = OSSL_ENCODER_CTX_new_for_pkey(pkey, OSSL_KEYMGMT_SELECT_PUBLIC_KEY, "DER", NULL, NULL);

    if (pkey_encoder_ctx == NULL)
    {
        goto done;
    }

    if (OSSL_ENCODER_to_data(pkey_encoder_ctx, &der_encoded_bytes, &der_length) != 1)
    {
        goto done;
    }

    // copies bytes into new buffer, so let it free after done:
    publicKeyData = CONSTBUFFER_Create(der_encoded_bytes, der_length);

done:

    if (ctx != NULL)
    {
        EVP_PKEY_CTX_free(ctx);
    }

    if (param_bld != NULL)
    {
        OSSL_PARAM_BLD_free(param_bld);
    }

    if (params != NULL)
    {
        OSSL_PARAM_free(params);
    }

    if (pkey_encoder_ctx != NULL)
    {
        OSSL_ENCODER_CTX_free(pkey_encoder_ctx);
    }

    free(der_encoded_bytes);
    free(modulus_bytes);
    free(exponent_bytes);

    BN_free(bn_modulus);
    BN_free(bn_exponent);

    // OpenSSL 3.0 took the guess work out of ownership so we can free the pkey in addition to the bn_modulus and bn_exponent
    EVP_PKEY_free(pkey);
#else
    unsigned char* modulus_bytes = NULL;
    unsigned char* exponent_bytes = NULL;
    int modulus_length, exponent_length;
    BIGNUM* bn_modulus = NULL;
    BIGNUM* bn_exponent = NULL;
    RSA* rsa = NULL;
    unsigned char* der_encoded_bytes = NULL;
    int der_length = 0;

    modulus_length = (int)Base64URLDecode(modulus_b64url, &modulus_bytes);
    if (modulus_length == 0)
    {
        goto done;
    }

    exponent_length = (int)Base64URLDecode(exponent_b64url, &exponent_bytes);
    if (exponent_length == 0)
    {
        goto done;
    }

    bn_modulus = BN_bin2bn(modulus_bytes, modulus_length, NULL);
    if (bn_modulus == NULL)
    {
        goto done;
    }

    bn_exponent = BN_bin2bn(exponent_bytes, exponent_length, NULL);
    if (bn_exponent == NULL)
    {
        goto done;
    }

    rsa = RSA_new();
    if (rsa == NULL)
    {
        goto done;
    }

    if (RSA_set0_key(rsa, bn_modulus, bn_exponent, NULL) == 0)
    {
        goto done;
    }

    der_encoded_bytes = NULL;
    der_length = i2d_RSAPublicKey(rsa, &der_encoded_bytes); // DER PKCS#1
    if (der_length == 0)
    {
        goto done;
    }

    // copies bytes into new buffer, so let it free after done:
    publicKeyData = CONSTBUFFER_Create(der_encoded_bytes, (size_t)der_length);

done:

    free(der_encoded_bytes);
    free(modulus_bytes);
    free(exponent_bytes);

    // Do not explicitly free bn_modulus and bn_exponent
    RSA_free(rsa);

#endif

    return publicKeyData;
}
