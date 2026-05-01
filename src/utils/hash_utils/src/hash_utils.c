/**
 * @file hash_utils.c
 * @brief Implements utilities for working with hashes.
 *
 * Hashing is performed via the OpenSSL EVP message-digest API, which is on
 * the SDL Approved Cryptographic Libraries list and is dynamically linked.
 * See @c docs/security/cryptography.md for the policy and rationale.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/hash_utils.h"

#include <stdio.h> // for FILE
#include <stdlib.h> // for calloc

#include <aducpal/strings.h> // strcasecmp

#include <azure_c_shared_utility/azure_base64.h>
#include <azure_c_shared_utility/buffer_.h>
#include <azure_c_shared_utility/crt_abstractions.h> // for mallocAndStrcpy_s

#include <openssl/err.h>
#include <openssl/evp.h>

#include <aduc/logging.h>

// File-read buffer used while streaming content into the digest. EVP imposes
// no chunk-size constraint, so a larger buffer reduces fread/EVP loop overhead
// without affecting correctness.
#define ADUC_HASH_FILE_READ_CHUNK_SIZE (64 * 1024)

/**
 * @brief Helper that logs every pending OpenSSL error from the thread-local
 *        error queue. Mirrors the pattern used in @c crypto_lib.c.
 *
 * @param context Free-form string describing where the error occurred.
 */
static void LogOpenSSLErrors(const char* context)
{
    unsigned long err;
    char buf[256];

    Log_Error("OpenSSL error context: %s", context);
    while ((err = ERR_get_error()) != 0)
    {
        ERR_error_string_n(err, buf, sizeof(buf));
        Log_Error("  OpenSSL error: %s", buf);
    }
}

/**
 * @brief Map a SHAversion to the corresponding OpenSSL EVP_MD.
 *
 * @param algorithm The requested SHA algorithm.
 * @return const EVP_MD* OpenSSL message-digest pointer, or NULL if unsupported.
 */
static const EVP_MD* GetEvpMdForSha(SHAversion algorithm)
{
    switch (algorithm)
    {
    case SHA1:
        return EVP_sha1();
    case SHA224:
        return EVP_sha224();
    case SHA256:
        return EVP_sha256();
    case SHA384:
        return EVP_sha384();
    case SHA512:
        return EVP_sha512();
    default:
        return NULL;
    }
}

/**
 * @brief Helper function that finalizes the hash from @p ctx, compares it to
 *        @p hashBase64, and optionally returns the computed hash to the caller.
 *
 * @param ctx Initialized digest context whose Update calls have completed.
 * @param hashBase64 The expected hash. If NULL, hash comparison is skipped.
 * @param algorithm The algorithm used (logged on errors).
 * @param suppressErrorLog When true, errors are not logged at error level.
 * @param outputHash Optional output. Caller must @c free() the returned buffer.
 * @return bool True if the digest finalized cleanly and (when supplied) matches @p hashBase64.
 */
static bool FinalizeAndCompareHashes(
    EVP_MD_CTX* ctx, const char* hashBase64, SHAversion algorithm, bool suppressErrorLog, char** outputHash)
{
    bool success = false;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;
    STRING_HANDLE encoded_file_hash = NULL;

    if (EVP_DigestFinal_ex(ctx, digest, &digestLen) != 1)
    {
        if (!suppressErrorLog)
        {
            LogOpenSSLErrors("EVP_DigestFinal_ex");
            Log_Error("Error finalizing digest, SHAversion: %d", algorithm);
        }
        goto done;
    }

    if (digestLen == 0 || digestLen > EVP_MAX_MD_SIZE)
    {
        if (!suppressErrorLog)
        {
            Log_Error("Unexpected digest length %u for SHAversion: %d", digestLen, algorithm);
        }
        goto done;
    }

    encoded_file_hash = Azure_Base64_Encode_Bytes(digest, (size_t)digestLen);
    if (encoded_file_hash == NULL)
    {
        if (!suppressErrorLog)
        {
            Log_Error("Error in Base64 Encoding");
        }
        goto done;
    }

    const bool hashMatches = ((hashBase64 == NULL) || (strcmp(hashBase64, STRING_c_str(encoded_file_hash)) == 0));

    if (!hashMatches)
    {
        if (!suppressErrorLog)
        {
            Log_Error(
                "Invalid Hash, Expect: %s, Result: %s, SHAversion: %d",
                hashBase64,
                STRING_c_str(encoded_file_hash),
                algorithm);
        }
        goto done;
    }

    if (outputHash != NULL)
    {
        if (mallocAndStrcpy_s(outputHash, STRING_c_str(encoded_file_hash)) != 0)
        {
            if (!suppressErrorLog)
            {
                Log_Error("Cannot allocate output buffer and copy hash.");
            }
            goto done;
        }
    }

    success = true;

done:
    STRING_delete(encoded_file_hash);
    return success;
}

bool ADUC_HashUtils_IsValidHashAlgorithm(SHAversion sha)
{
    // SHA-1 and SHA-224 are not strong enough for payload integrity. Only
    // accept SHA-256/384/512. Bound the upper end explicitly so out-of-range
    // values produced by future enum additions are rejected here rather than
    // silently passed to OpenSSL.
    return sha >= SHA256 && sha <= SHA512;
}

bool ADUC_HashUtils_GetIndexStrongestValidHash(
    const ADUC_Hash* hashes, size_t hashCount, size_t* outIndexStrongestAlgorithm, SHAversion* outBestShaVersion)
{
    if (outIndexStrongestAlgorithm == NULL || outBestShaVersion == NULL)
    {
        return false;
    }

    bool foundStrongest = false;
    size_t strongestIndex; // Assume hashes array is not sorted by strength ordering.
    SHAversion curBestAlg = SHA1;

    for (size_t i = 0; i < hashCount; ++i)
    {
        SHAversion algVersion = SHA1;
        char* hashType = ADUC_HashUtils_GetHashType(hashes, hashCount, i);
        if (!ADUC_HashUtils_GetShaVersionForTypeString(hashType, &algVersion))
        {
            Log_Error("Unsupported algorithm: %s", hashType);
            return false;
        }

        // Just because it's supported by the underlying library does not mean
        // it's valid for file digests (e.g. SHA1 is not valid).
        if (!ADUC_HashUtils_IsValidHashAlgorithm(algVersion))
        {
            Log_Warn("Invalid hash alg: %s", hashType);
            continue;
        }

        if (algVersion > curBestAlg)
        {
            foundStrongest = true;
            strongestIndex = i;
            curBestAlg = algVersion;
        }
    }

    if (foundStrongest)
    {
        *outIndexStrongestAlgorithm = strongestIndex;
        *outBestShaVersion = curBestAlg;
        return true;
    }

    return false;
}

/**
 * @brief For the given array of ADUC_Hash, it will verify that the hash of the file contents matches the strongest hash in the array.
 *
 * @param filePath The path to the file with contents to hash.
 * @param hashes The array of ADUC_Hash objects.
 * @param hashCount The length of the array.
 * @return bool true if the hash with the strongest algorithm matches the hash of the file at the given path.
 */
bool ADUC_HashUtils_VerifyWithStrongestHash(const char* filePath, const ADUC_Hash* hashes, size_t hashCount)
{
    size_t indexStrongestAlgorithm = 0;
    SHAversion bestShaVersion = SHA256;
    if (!ADUC_HashUtils_GetIndexStrongestValidHash(hashes, hashCount, &indexStrongestAlgorithm, &bestShaVersion))
    {
        // There is no hash with a valid algorithm.
        return false;
    }

    Log_Debug("Best hash index %d", indexStrongestAlgorithm);

    char* hashValue = ADUC_HashUtils_GetHashValue(hashes, hashCount, indexStrongestAlgorithm);
    if (!ADUC_HashUtils_IsValidFileHash(filePath, hashValue, bestShaVersion, false))
    {
        return false;
    }

    return true;
}

/**
 * @brief Streams the contents of @p file through @p ctx using @c EVP_DigestUpdate.
 *
 * @param ctx Initialized digest context.
 * @param file Open file positioned where streaming should begin.
 * @param suppressErrorLog When true, errors are not logged at error level.
 * @return true on success (including end-of-file), false on read or digest error.
 */
static bool DigestUpdateFromFile(EVP_MD_CTX* ctx, FILE* file, bool suppressErrorLog)
{
    uint8_t buffer[ADUC_HASH_FILE_READ_CHUNK_SIZE];

    while (!feof(file))
    {
        const size_t readSize = fread(buffer, sizeof(buffer[0]), ARRAY_SIZE(buffer), file);
        if (readSize == 0)
        {
            if (ferror(file))
            {
                if (!suppressErrorLog)
                {
                    Log_Error("Error reading file content.");
                }
                return false;
            }

            // At end of file. Done.
            break;
        }

        if (EVP_DigestUpdate(ctx, buffer, readSize) != 1)
        {
            if (!suppressErrorLog)
            {
                LogOpenSSLErrors("EVP_DigestUpdate");
            }
            return false;
        }
    }

    return true;
}

/**
 * @brief Computes the hash of the file at @p path and returns the base64 string.
 *
 * @param path The path to the file to check.
 * @param algorithm The hashing algorithm to use to calculate the hash.
 * @param hash [out] Pointer to output buffer. Caller must @c free() when done.
 * @return bool True if the hash data is successfully generated.
 */
bool ADUC_HashUtils_GetFileHash(const char* path, SHAversion algorithm, char** hash)
{
    bool success = false;
    FILE* file = NULL;
    EVP_MD_CTX* ctx = NULL;
    const EVP_MD* evpMd = NULL;

    if (hash == NULL)
    {
        Log_Error("Invalid input. 'hash' is NULL.");
        goto done;
    }

    *hash = NULL;

    evpMd = GetEvpMdForSha(algorithm);
    if (evpMd == NULL)
    {
        Log_Error("Unsupported SHAversion: %d", algorithm);
        goto done;
    }

    file = fopen(path, "rb");
    if (file == NULL)
    {
        // Sometime we call this function to check whether the file is already exist.
        // So, log info here instead of error.
        Log_Info("No such file or directory: %s", path);
        goto done;
    }

    ctx = EVP_MD_CTX_new();
    if (ctx == NULL)
    {
        LogOpenSSLErrors("EVP_MD_CTX_new");
        goto done;
    }

    if (EVP_DigestInit_ex(ctx, evpMd, NULL) != 1)
    {
        LogOpenSSLErrors("EVP_DigestInit_ex");
        Log_Error("Error initializing digest, SHAversion: %d", algorithm);
        goto done;
    }

    if (!DigestUpdateFromFile(ctx, file, true /* suppressErrorLog */))
    {
        goto done;
    }

    success = FinalizeAndCompareHashes(ctx, NULL, algorithm, true, hash);

    if (!success)
    {
        Log_Debug("Hash compare failed on file '%s'", path);
    }

done:

    if (ctx != NULL)
    {
        EVP_MD_CTX_free(ctx);
    }

    if (file != NULL)
    {
        fclose(file);
    }

    return success;
}

/**
 * @brief Get file hash type at specified index.
 * @param hashArray The ADUC_Hash array.
 * @param arraySize Size of array.
 * @param hashIndex Index of the hash to return.
 *
 * @return Hash type if succeeded. Otherwise, return NULL.
 */
char* ADUC_HashUtils_GetHashType(const ADUC_Hash* hashArray, size_t arraySize, size_t index)
{
    if (index >= arraySize)
    {
        return NULL;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return hashArray[index].type;
}

/**
 * @brief Get file hash value at specified index.
 * @param hashArray The ADUC_Hash array.
 * @param arraySize Size of array.
 * @param hashIndex Index of the hash to return.
 *
 * @return Hash value if succeeded. Otherwise, return NULL.
 */
char* ADUC_HashUtils_GetHashValue(const ADUC_Hash* hashArray, size_t arraySize, size_t index)
{
    if (index >= arraySize)
    {
        return NULL;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return hashArray[index].value;
}

/**
 * @brief Checks if the hash of the file at @p path matches @p hashBase64
 *
 * @param path The path to the file to check
 * @param hashBase64 The expected hash of the file at @p path
 * @param algorithm The hashing algorithm to use to calculate the hash.
 * @param suppressErrorLog A boolean indicates whether to log error message inside this function.
 * @return bool True if the hash is valid and matches @p hashBase64
 */
bool ADUC_HashUtils_IsValidFileHash(
    const char* path, const char* hashBase64, SHAversion algorithm, bool suppressErrorLog)
{
    bool success = false;
    FILE* file = NULL;
    EVP_MD_CTX* ctx = NULL;
    const EVP_MD* evpMd = NULL;

    evpMd = GetEvpMdForSha(algorithm);
    if (evpMd == NULL)
    {
        if (!suppressErrorLog)
        {
            Log_Error("Unsupported SHAversion: %d", algorithm);
        }
        goto done;
    }

    file = fopen(path, "rb");
    if (file == NULL)
    {
        if (!suppressErrorLog)
        {
            Log_Error("Cannot open file: %s", path);
        }
        goto done;
    }

    ctx = EVP_MD_CTX_new();
    if (ctx == NULL)
    {
        if (!suppressErrorLog)
        {
            LogOpenSSLErrors("EVP_MD_CTX_new");
        }
        goto done;
    }

    if (EVP_DigestInit_ex(ctx, evpMd, NULL) != 1)
    {
        if (!suppressErrorLog)
        {
            LogOpenSSLErrors("EVP_DigestInit_ex");
            Log_Error("Error initializing digest, SHAversion: %d", algorithm);
        }
        goto done;
    }

    if (!DigestUpdateFromFile(ctx, file, suppressErrorLog))
    {
        goto done;
    }

    success = FinalizeAndCompareHashes(ctx, hashBase64, algorithm, suppressErrorLog, NULL /* outputHash */);
    if (!success && !suppressErrorLog)
    {
        Log_Debug("Hash compare failed on file '%s'", path);
        goto done;
    }

done:
    if (ctx != NULL)
    {
        EVP_MD_CTX_free(ctx);
    }

    if (file != NULL)
    {
        fclose(file);
    }

    return success;
}

/**
 * @brief Checks if the hash of the @p buffer matches @p hashBase64
 *
 * @param buffer The buffer to check
 * @param bufferLen The length of the @p buffer
 * @param hashBase64 The expected hash of the buffer @p buffer
 * @return bool True if the hash is valid and matches @p hashBase64
 */
bool ADUC_HashUtils_IsValidBufferHash(
    const uint8_t* buffer, size_t bufferLen, const char* hashBase64, SHAversion algorithm)
{
    bool success = false;
    EVP_MD_CTX* ctx = NULL;
    const EVP_MD* evpMd = GetEvpMdForSha(algorithm);

    if (evpMd == NULL)
    {
        Log_Error("Unsupported SHAversion: %d", algorithm);
        goto done;
    }

    ctx = EVP_MD_CTX_new();
    if (ctx == NULL)
    {
        LogOpenSSLErrors("EVP_MD_CTX_new");
        goto done;
    }

    if (EVP_DigestInit_ex(ctx, evpMd, NULL) != 1)
    {
        LogOpenSSLErrors("EVP_DigestInit_ex");
        Log_Error("Error initializing digest, SHAversion: %d", algorithm);
        goto done;
    }

    if (EVP_DigestUpdate(ctx, buffer, bufferLen) != 1)
    {
        LogOpenSSLErrors("EVP_DigestUpdate");
        Log_Error("Error in digest update, SHAversion: %d", algorithm);
        goto done;
    }

    success = FinalizeAndCompareHashes(ctx, hashBase64, algorithm, true, NULL);

done:
    if (ctx != NULL)
    {
        EVP_MD_CTX_free(ctx);
    }

    return success;
}

/**
 * @brief Helper functions returns the SHAversion associated with the @p hashTypeStr
 * @param hashTypeStr the hash type to be used
 * @param algorithm the destination to store the SHAversion
 * @returns True if a hash type was found, false if it was not
 */
bool ADUC_HashUtils_GetShaVersionForTypeString(const char* hashTypeStr, SHAversion* algorithm)
{
    bool success = true;

    if (ADUCPAL_strcasecmp(hashTypeStr, "sha1") == 0)
    {
        *algorithm = SHA1;
    }
    else if (ADUCPAL_strcasecmp(hashTypeStr, "sha224") == 0)
    {
        *algorithm = SHA224;
    }
    else if (ADUCPAL_strcasecmp(hashTypeStr, "sha256") == 0)
    {
        *algorithm = SHA256;
    }
    else if (ADUCPAL_strcasecmp(hashTypeStr, "sha384") == 0)
    {
        *algorithm = SHA384;
    }
    else if (ADUCPAL_strcasecmp(hashTypeStr, "sha512") == 0)
    {
        *algorithm = SHA512;
    }
    else
    {
        success = false;
    }

    return success;
}

/**
 * @brief Free the ADUC_Hash struct members
 * @param hash a pointer to an ADUC_Hash
 */
void ADUC_Hash_UnInit(ADUC_Hash* hash)
{
    free(hash->value);
    hash->value = NULL;

    free(hash->type);
    hash->type = NULL;
}

/**
 * @brief Allocates the memory for the ADUC_Hash struct member values
 * @param hash A pointer to an ADUC_Hash struct whose member values will be allocated
 * @param hashValue The value of the hash
 * @param hashType The type of the hash
 * @returns True if successfully allocated, False if failure
 */
bool ADUC_Hash_Init(ADUC_Hash* hash, const char* hashValue, const char* hashType)
{
    bool success = false;

    if (hash == NULL)
    {
        return false;
    }

    if (hashValue == NULL || hashType == NULL)
    {
        Log_Error("Invalid call to ADUC_Hash_Init with hashValue %s and hashType %s", hashValue, hashType);
        return false;
    }

    hash->value = NULL;
    hash->type = NULL;

    if (mallocAndStrcpy_s(&(hash->value), hashValue) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(hash->type), hashType) != 0)
    {
        goto done;
    }

    success = true;

done:

    if (!success)
    {
        ADUC_Hash_UnInit(hash);
    }

    return success;
}

/**
 * @brief Frees an array of ADUC_Hashes of size @p hashCount
 * @param hashCount the size of @p hashArray
 * @param hashArray a pointer to an array of ADUC_Hash structs
 */
void ADUC_Hash_FreeArray(size_t hashCount, ADUC_Hash* hashArray)
{
    if (hashArray != NULL)
    {
        for (size_t hash_index = 0; hash_index < hashCount; ++hash_index)
        {
            ADUC_Hash* hashEntity = hashArray + hash_index;
            ADUC_Hash_UnInit(hashEntity);
        }
        free(hashArray);
    }
}
