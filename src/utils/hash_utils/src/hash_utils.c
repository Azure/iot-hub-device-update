/**
 * @file hash_utils.c
 * @brief Implements utilities for working with hashes
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/hash_utils.h"

#include <stdio.h> // for FILE
#include <stdlib.h> // for calloc
#include <strings.h> // for strcasecmp

#include <azure_c_shared_utility/azure_base64.h>
#include <azure_c_shared_utility/buffer_.h>
#include <azure_c_shared_utility/crt_abstractions.h> // for mallocAndStrcpy_s
#include <azure_c_shared_utility/sha.h>

#include <aduc/logging.h>

/**
 * @brief Helper function gets the calculated hash from the @p context, compares it to @p hashBase64, and returns the appropriate value
 * @param context Context in which the hash was calculated and stored
 * @param hashBase64 The expected hash from the context. If NULL, skip hashes comparison.
 * @param algorithm the algorithm used to calculate the hash
 * @param outputHash an optional output buffer for computed hash. Caller must call free() to deallocate the buffer when done.
 * @returns bool True if the hash is valid and equals @p hashBase64
 */
static bool GetResultAndCompareHashes(
    USHAContext* context, const char* hashBase64, SHAversion algorithm, bool suppressErrorLog, char** outputHash)
{
    bool success = false;
    // "USHAHashSize(algorithm)" is more precise, but requires a variable length array, or heap allocation.
    uint8_t buffer_hash[USHAMaxHashSize];
    STRING_HANDLE encoded_file_hash = NULL;

    if (USHAResult(context, (uint8_t*)buffer_hash) != 0)
    {
        if (!suppressErrorLog)
        {
            Log_Error("Error in SHA Result, SHAversion: %d", algorithm);
        }
        goto done;
    }

    encoded_file_hash = Azure_Base64_Encode_Bytes((unsigned char*)buffer_hash, USHAHashSize(algorithm));
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

/**
 * @brief Checks if the hash of the file at @p path matches @p hashBase64
 *
 * @param path The path to the file to check
 * @param algorithm The hashing algorithm to use to calculate the hash.
 * @param hash [out] The pointer to output buffer. Caller must call free() when done with the returned buffer.
 * @return bool True if the hash data is successfully generated.
 */
_Bool ADUC_HashUtils_GetFileHash(const char* path, SHAversion algorithm, char** hash)
{
    _Bool success = false;
    FILE* file = NULL;

    if (hash == NULL)
    {
        Log_Error("Invalid input. 'hash' is NULL.");
        goto done;
    }

    *hash = NULL;

    file = fopen(path, "rb");
    if (file == NULL)
    {
        // Sometime we call this function to check whether the file is already exist.
        // So, log info here instead of error.
        Log_Info("No such file or directory: %s", path);
        goto done;
    }

    USHAContext context;

    if (USHAReset(&context, algorithm) != 0)
    {
        Log_Error("Error in SHA Reset, SHAversion: %d", algorithm);
        goto done;
    };

    // Repeatedly read and hash chunks of the file
    while (!feof(file))
    {
        uint8_t buffer[USHA_Max_Message_Block_Size];
        const size_t readSize = fread(buffer, sizeof(buffer[0]), ARRAY_SIZE(buffer), file);
        if (readSize == 0)
        {
            if (ferror(file))
            {
                Log_Error("Error reading file content.");
                goto done;
            }

            // At the end of file. We're done here.
            break;
        }

        if (USHAInput(&context, buffer, readSize) != 0)
        {
            Log_Error("Error in SHA Input, SHAversion: %d", algorithm);
            goto done;
        };
    }

    success = GetResultAndCompareHashes(&context, NULL, algorithm, true, hash);

done:

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
_Bool ADUC_HashUtils_IsValidFileHash(
    const char* path, const char* hashBase64, SHAversion algorithm, bool suppressErrorLog)
{
    _Bool success = false;

    FILE* file = fopen(path, "rb");
    if (file == NULL)
    {
        if (!suppressErrorLog)
        {
            Log_Error("Cannot open file: %s", path);
        }
        goto done;
    }

    USHAContext context;

    if (USHAReset(&context, algorithm) != 0)
    {
        if (!suppressErrorLog)
        {
            Log_Error("Error in SHA Reset, SHAversion: %d", algorithm);
        }
        goto done;
    };

    // Repeatedly read and hash chunks of the file
    while (!feof(file))
    {
        uint8_t buffer[USHA_Max_Message_Block_Size];
        const size_t readSize = fread(buffer, sizeof(buffer[0]), ARRAY_SIZE(buffer), file);
        if (readSize == 0)
        {
            if (ferror(file))
            {
                if (!suppressErrorLog)
                {
                    Log_Error("Error reading file content.");
                }
                goto done;
            }

            // At the end of file. We're done here.
            break;
        }

        if (USHAInput(&context, buffer, readSize) != 0)
        {
            if (!suppressErrorLog)
            {
                Log_Error("Error in SHA Input, SHAversion: %d", algorithm);
            }
            goto done;
        };
    }

    success = GetResultAndCompareHashes(&context, hashBase64, algorithm, suppressErrorLog, NULL /* outputHash */);
    if (!success)
    {
        goto done;
    }

done:
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
_Bool ADUC_HashUtils_IsValidBufferHash(
    const uint8_t* buffer, size_t bufferLen, const char* hashBase64, SHAversion algorithm)
{
    USHAContext context;

    if (USHAReset(&context, algorithm) != 0)
    {
        Log_Error("Error in SHA Reset, SHAversion: %d", algorithm);
        return false;
    }

    if (USHAInput(&context, buffer, bufferLen) != 0)
    {
        Log_Error("Error in SHA Input, SHAversion: %d", algorithm);
        return false;
    }

    return GetResultAndCompareHashes(&context, hashBase64, algorithm, true, NULL);
}

/**
 * @brief Helper functions returns the SHAversion associated with the @p hashTypeStr
 * @param hashTypeStr the hash type to be used
 * @param algorithm the destination to store the SHAversion
 * @returns True if a hash type was found, false if it was not
 */
_Bool ADUC_HashUtils_GetShaVersionForTypeString(const char* hashTypeStr, SHAversion* algorithm)
{
    bool success = true;

    if (strcasecmp(hashTypeStr, "sha1") == 0)
    {
        *algorithm = SHA1;
    }
    else if (strcasecmp(hashTypeStr, "sha224") == 0)
    {
        *algorithm = SHA224;
    }
    else if (strcasecmp(hashTypeStr, "sha256") == 0)
    {
        *algorithm = SHA256;
    }
    else if (strcasecmp(hashTypeStr, "sha384") == 0)
    {
        *algorithm = SHA384;
    }
    else if (strcasecmp(hashTypeStr, "sha512") == 0)
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
_Bool ADUC_Hash_Init(ADUC_Hash* hash, const char* hashValue, const char* hashType)
{
    _Bool success = false;

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
    for (size_t hash_index = 0; hash_index < hashCount; ++hash_index)
    {
        ADUC_Hash* hashEntity = hashArray + hash_index;
        ADUC_Hash_UnInit(hashEntity);
    }
    free(hashArray);
}
