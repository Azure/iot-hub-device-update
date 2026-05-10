/**
 * @file secure_storage.h
 * @brief Abstraction for secure credential/secret storage.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_SECURE_STORAGE_H
#define ADUC_SECURE_STORAGE_H

#include "aduc/extension_types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Secure storage backend type.
 */
typedef enum ADUC_SecureStorageBackend
{
    ADUC_SECURE_STORAGE_FILE = 0,   // Encrypted file (default fallback)
    ADUC_SECURE_STORAGE_KEYRING,    // Linux kernel keyring
    ADUC_SECURE_STORAGE_TPM,        // TPM 2.0 (stub for now)
} ADUC_SecureStorageBackend;

typedef struct ADUC_SecureStorage* ADUC_SecureStorageHandle;

/**
 * @brief Create a secure storage instance.
 */
ADUC_Result2 ADUC_SecureStorage_Create(
    ADUC_SecureStorageBackend backend,
    const char* storePath,
    ADUC_SecureStorageHandle* outHandle);

/**
 * @brief Store a secret.
 */
ADUC_Result2 ADUC_SecureStorage_Put(
    ADUC_SecureStorageHandle handle,
    const char* key,
    const void* value,
    size_t valueLen);

/**
 * @brief Retrieve a secret (caller must call Release after use).
 */
ADUC_Result2 ADUC_SecureStorage_Get(
    ADUC_SecureStorageHandle handle,
    const char* key,
    void* valueBuf,
    size_t bufLen,
    size_t* outLen);

/**
 * @brief Release/zero a secret buffer.
 */
void ADUC_SecureStorage_Release(void* buf, size_t len);

/**
 * @brief Delete a secret.
 */
ADUC_Result2 ADUC_SecureStorage_Delete(ADUC_SecureStorageHandle handle, const char* key);

/**
 * @brief List all keys.
 */
ADUC_Result2 ADUC_SecureStorage_ListKeys(
    ADUC_SecureStorageHandle handle,
    char** outKeys,
    size_t* outCount);

/**
 * @brief Free keys returned by ListKeys.
 */
void ADUC_SecureStorage_FreeKeys(char** keys, size_t count);

/**
 * @brief Destroy a secure storage instance and free resources.
 */
void ADUC_SecureStorage_Destroy(ADUC_SecureStorageHandle handle);

#ifdef __cplusplus
}
#endif

#endif // ADUC_SECURE_STORAGE_H
