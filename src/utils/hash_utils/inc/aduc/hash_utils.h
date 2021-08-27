/**
 * @file hash_utils.h
 * @brief Utilities for working with hashes.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#ifndef ADUC_HASH_UTILS_H
#define ADUC_HASH_UTILS_H

#include "aduc/c_utils.h"
#include "aduc/types/hash.h"

#include "azure_c_shared_utility/sha.h" // for SHAversion

#include <stdbool.h> // for _Bool
#include <stddef.h> // for size_t

EXTERN_C_BEGIN

_Bool ADUC_HashUtils_IsValidFileHash(const char* path, const char* hashBase64, SHAversion algorithm);

_Bool ADUC_HashUtils_IsValidBufferHash(
    const uint8_t* buffer, size_t bufferLen, const char* hashBase64, SHAversion algorithm);

_Bool ADUC_HashUtils_GetShaVersionForTypeString(const char* hashTypeStr, SHAversion* algorithm);

_Bool ADUC_HashUtils_GetFileHash(const char* path, SHAversion algorithm, char** hash);

/**
 * @brief Allocates the memory for the ADUC_Hash struct member values
 * @param hash A pointer to an ADUC_Hash struct whose member values will be allocated
 * @param hashValue The value of the hash
 * @param hashType The type of the hash
 * @returns True if successfully allocated, False if failure
 */
_Bool ADUC_Hash_Init(ADUC_Hash* hash, const char* hashValue, const char* hashType);

/**
 * @brief Free the ADUC_Hash struct members
 * @param hash a pointer to an ADUC_Hash
 */
void ADUC_Hash_UnInit(ADUC_Hash* hash);

/**
 * @brief Frees an array of ADUC_Hashes of size @p hashCount
 * @param hashCount the size of @p hashArray
 * @param hashArray a pointer to an array of ADUC_Hash structs
 */
void ADUC_Hash_FreeArray(size_t hashCount, ADUC_Hash* hashArray);

EXTERN_C_END

#endif // ADUC_HASH_UTILS_H
