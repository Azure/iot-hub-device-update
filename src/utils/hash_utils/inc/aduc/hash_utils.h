/**
 * @file hash_utils.h
 * @brief Utilities for working with hashes.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_HASH_UTILS_H
#define ADUC_HASH_UTILS_H

#include "aduc/c_utils.h"
#include "aduc/types/hash.h"

#include "azure_c_shared_utility/sha.h" // for SHAversion

#include <stdbool.h> // for bool
#include <stddef.h> // for size_t

EXTERN_C_BEGIN

bool ADUC_HashUtils_IsValidFileHash(
    const char* path, const char* hashBase64, SHAversion algorithm, bool suppressErrorLog);

bool ADUC_HashUtils_IsValidBufferHash(
    const uint8_t* buffer, size_t bufferLen, const char* hashBase64, SHAversion algorithm);

bool ADUC_HashUtils_GetShaVersionForTypeString(const char* hashTypeStr, SHAversion* algorithm);

bool ADUC_HashUtils_GetFileHash(const char* path, SHAversion algorithm, char** hash);

/**
 * @brief Get file hash type at specified index.
 * @param hashArray The ADUC_Hash array.
 * @param arraySize Size of array.
 * @param hashIndex Index of the hash to return.
 *
 * @return Hash type if succeeded. Otherwise, return NULL.
 */
char* ADUC_HashUtils_GetHashType(const ADUC_Hash* hashArray, size_t arraySize, size_t index);

/**
 * @brief Get file hash value at specified index.
 * @param hashArray The ADUC_Hash array.
 * @param arraySize Size of array.
 * @param hashIndex Index of the hash to return.
 *
 * @return Hash value if succeeded. Otherwise, return NULL.
 */
char* ADUC_HashUtils_GetHashValue(const ADUC_Hash* hashArray, size_t arraySize, size_t index);

/**
 * @brief Allocates the memory for the ADUC_Hash struct member values
 * @param hash A pointer to an ADUC_Hash struct whose member values will be allocated
 * @param hashValue The value of the hash
 * @param hashType The type of the hash
 * @returns True if successfully allocated, False if failure
 */
bool ADUC_Hash_Init(ADUC_Hash* hash, const char* hashValue, const char* hashType);

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

/**
 * @brief For the given array of ADUC_Hash, it will verify that the hash of the file contents matches the strongest hash in the array.
 *
 * @param filePath The path to the file with contents to hash.
 * @param hashes The array of ADUC_Hash objects.
 * @param hashCount The length of the array.
 * @return bool true if the hash with the strongest algorithm matches the hash of the file at the given path.
 */
bool ADUC_HashUtils_VerifyWithStrongestHash(const char* filePath, const ADUC_Hash* hashes, size_t hashCount);

EXTERN_C_END

#endif // ADUC_HASH_UTILS_H
