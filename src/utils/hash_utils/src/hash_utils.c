/**
 * @file hash_utils.c
 * @brief Implements utilities for working with hashes
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#include "aduc/hash_utils.h"

#include <stdio.h> // for FILE
#include <stdlib.h> // for calloc
#include <strings.h> // for strcasecmp

#include <azure_c_shared_utility/azure_base64.h>
#include <azure_c_shared_utility/buffer_.h>
#include <azure_c_shared_utility/sha.h>

#include <aduc/logging.h>

/**
 * @brief Helper function gets the calculated hash from the @p context, compares it to @p hashBase64, and returns the appropraite value
 * @param context Context in which the hash was calculated and stored
 * @param hashBase64 The expected hash from the context
 * @param algorithm the algorithm used to calculate the hash
 * @returns bool True if the hash is valid and equals @p hashBase64
 */
static bool GetResultAndCompareHashes(USHAContext* context, const char* hashBase64, SHAversion algorithm)
{
    // "USHAHashSize(algorithm)" is more precise, but requires a variable length array, or heap allocation.
    uint8_t buffer_hash[USHAMaxHashSize];

    if (USHAResult(context, (uint8_t*)buffer_hash) != 0)
    {
        Log_Error("Error in SHA Result, SHAversion: %d", algorithm);
        return false;
    }

    STRING_HANDLE encoded_file_hash = Azure_Base64_Encode_Bytes((unsigned char*)buffer_hash, USHAHashSize(algorithm));
    if (encoded_file_hash == NULL)
    {
        Log_Error("Error in Base64 Encoding");
        return false;
    }

    const bool hashMatches = strcmp(hashBase64, STRING_c_str(encoded_file_hash)) == 0;

    STRING_delete(encoded_file_hash);
    encoded_file_hash = NULL;

    if (!hashMatches)
    {
        Log_Error(
            "Invalid Hash, Expect: %s, Result: %s, SHAversion: %d",
            hashBase64,
            STRING_c_str(encoded_file_hash),
            algorithm);
        return false;
    }

    return true;
}

/**
 * @brief Checks if the hash of the file at @p path matches @p hashBase64
 *
 * @param path The path to the file to check
 * @param hashBase64 The expected hash of the file at @p path
 * @param algorithm The hashing algorithm to use to calculate the hash.
 * @return bool True if the hash is valid and matches @p hashBase64
 */
_Bool ADUC_HashUtils_IsValidFileHash(const char* path, const char* hashBase64, SHAversion algorithm)
{
    _Bool success = false;

    FILE* file = fopen(path, "rb");
    if (file == NULL)
    {
        Log_Error("No such file or directory: %s", path);
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
            break;
        }

        if (USHAInput(&context, buffer, readSize) != 0)
        {
            Log_Error("Error in SHA Input, SHAversion: %d", algorithm);
            goto done;
        };
    }

    success = GetResultAndCompareHashes(&context, hashBase64, algorithm);
    if (!success)
    {
        goto done;
    }

done:
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

    return GetResultAndCompareHashes(&context, hashBase64, algorithm);
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
