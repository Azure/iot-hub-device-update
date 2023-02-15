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
#include <azure_c_shared_utility/constbuffer.h>
#include <ctype.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/aes.h>


//
// Note: This is not generalized, for now we assume the same AES key for the entire time
//
typedef struct _DecryptionKey
{
    CONSTBUFFER_HANDLE keyData;
    CONSTBUFFER_HANDLE iv;
} DecryptionKey;
//
// Initialize/ De-Initialize
//

const EVP_CIPHER* DecryptionUtils_GetCipherType(const char* alg)
{
    if (strcasecmp(alg, "aes256") == 0)
    {
        return EVP_aes_256_cbc();
    }

    return NULL;
}

bool InitializeDecryptionCtx(EVP_CIPHER_CTX** ctx, const char* alg, const DecryptionKey* dKey)
{
    bool success = false;

    if (ctx == NULL || dKey == NULL || alg == NULL)
    {
        goto done;
    }

    *ctx = EVP_CIPHER_CTX_new();

    if (*ctx == NULL)
    {
        goto done;
    }

    const EVP_CIPHER* cipherType = DecryptionUtils_GetCipherType(alg);

    if (cipherType ==  NULL)
    {
        goto done;
    }

    const CONSTBUFFER* keyData = CONSTBUFFER_GetContent(dKey->keyData);

    const CONSTBUFFER* iv = CONSTBUFFER_GetContent(dKey->iv);

    if (EVP_DecryptInit(*ctx,cipherType,keyData->buffer,iv->buffer) != 1)
    {
        goto done;
    }

    success = true;
done:

    if (! success )
    {
        EVP_CIPHER_CTX_free(*ctx);
    }

    return success;
}

void DeInitializeDecryptionCtx(EVP_CIPHER_CTX** ctx)
{
    EVP_CIPHER_CTX_free(*ctx);
    *ctx = NULL;
}

//
// Decryption Algorithms
//

size_t CryptoUtils_DecryptChunk(EVP_CIPHER_CTX* ctx, const unsigned char* chunk, const size_t chunk_size, char* plaintext, const size_t plaintext_len)
{
    if (ctx == NULL || chunk == NULL || plaintext == NULL)
    {
        return 0;
    }


}
