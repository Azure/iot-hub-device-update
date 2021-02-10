/**
 * @file hash_utils.h
 * @brief Utilities for working with hashes.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#ifndef ADUC_HASH_UTILS_H
#define ADUC_HASH_UTILS_H

#include <aduc/c_utils.h>

#include <azure_c_shared_utility/sha.h> // for SHAversion

#include <stdbool.h> // for _Bool
#include <stddef.h> // for size_t

EXTERN_C_BEGIN

_Bool ADUC_HashUtils_IsValidFileHash(const char* path, const char* hashBase64, SHAversion algorithm);

_Bool ADUC_HashUtils_IsValidBufferHash(
    const uint8_t* buffer, size_t bufferLen, const char* hashBase64, SHAversion algorithm);

_Bool ADUC_HashUtils_GetShaVersionForTypeString(const char* hashTypeStr, SHAversion* algorithm);

EXTERN_C_END

#endif // ADUC_HASH_UTILS_H
