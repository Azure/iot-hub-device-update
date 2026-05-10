/**
 * @file secure_storage.c
 * @brief Abstraction for secure credential/secret storage.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef _WIN32
#define _GNU_SOURCE
#endif

#include "aduc/secure_storage.h"
#include "aduc/platform.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#include <windows.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define MAX_KEY_LEN 128
#define MAX_PATH_LEN 512
#define OBFUSCATION_KEY_LEN 32

#define SS_ERR_INVALID_ARG    ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_IO, 0x0010)
#define SS_ERR_ALLOC          ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_RESOURCE, 0x0010)
#define SS_ERR_NOT_IMPL       ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_STATE, 0x0010)
#define SS_ERR_FILE_OPEN      ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_IO, 0x0011)
#define SS_ERR_IO             ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_IO, 0x0012)
#define SS_ERR_NOT_FOUND      ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_IO, 0x0013)
#define SS_ERR_BUF_TOO_SMALL  ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_IO, 0x0014)
#define SS_ERR_DIR_CREATE     ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_IO, 0x0015)

struct ADUC_SecureStorage
{
    ADUC_SecureStorageBackend backend;
    char storePath[MAX_PATH_LEN];
    uint8_t obfuscationKey[OBFUSCATION_KEY_LEN];
};

// Simple device-derived obfuscation key (placeholder — real crypto deferred)
static void derive_obfuscation_key(uint8_t* key, size_t keyLen)
{
    // Use a fixed derivation for now based on machine-id if available
    FILE* f = fopen("/etc/machine-id", "r");
    if (f != NULL)
    {
        char machineId[64] = { 0 };
        size_t n = fread(machineId, 1, sizeof(machineId) - 1, f);
        fclose(f);

        for (size_t i = 0; i < keyLen; i++)
        {
            key[i] = (uint8_t)(machineId[i % n] ^ (uint8_t)(i * 0x5A));
        }
    }
    else
    {
        // Fallback: static key (not secure, but functional for dev)
        for (size_t i = 0; i < keyLen; i++)
        {
            key[i] = (uint8_t)(0xA5 ^ (i * 0x37));
        }
    }
}

static void xor_obfuscate(const uint8_t* key, size_t keyLen, const void* input, void* output, size_t dataLen)
{
    const uint8_t* in = (const uint8_t*)input;
    uint8_t* out = (uint8_t*)output;
    for (size_t i = 0; i < dataLen; i++)
    {
        out[i] = in[i] ^ key[i % keyLen];
    }
}

static void build_key_path(const struct ADUC_SecureStorage* store, const char* key, char* pathBuf, size_t pathBufLen)
{
    snprintf(pathBuf, pathBufLen, "%s/%s", store->storePath, key);
}

static bool is_valid_key(const char* key)
{
    if (key == NULL || key[0] == '\0')
    {
        return false;
    }
    // Only allow alphanumeric, dash, underscore, dot
    for (const char* p = key; *p != '\0'; p++)
    {
        char c = *p;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.'))
        {
            return false;
        }
    }
    return strlen(key) < MAX_KEY_LEN;
}

ADUC_Result2 ADUC_SecureStorage_Create(
    ADUC_SecureStorageBackend backend,
    const char* storePath,
    ADUC_SecureStorageHandle* outHandle)
{
    if (outHandle == NULL)
    {
        return SS_ERR_INVALID_ARG;
    }

    if (backend != ADUC_SECURE_STORAGE_FILE)
    {
        return SS_ERR_NOT_IMPL;
    }

    if (storePath == NULL)
    {
        return SS_ERR_INVALID_ARG;
    }

    struct ADUC_SecureStorage* store = (struct ADUC_SecureStorage*)calloc(1, sizeof(struct ADUC_SecureStorage));
    if (store == NULL)
    {
        return SS_ERR_ALLOC;
    }

    store->backend = backend;
    strncpy(store->storePath, storePath, MAX_PATH_LEN - 1);
    store->storePath[MAX_PATH_LEN - 1] = '\0';
    derive_obfuscation_key(store->obfuscationKey, OBFUSCATION_KEY_LEN);

    // Ensure store directory exists with restricted permissions
    if (adu_mkdir(store->storePath, 0700) != 0 && errno != EEXIST)
    {
        free(store);
        return SS_ERR_DIR_CREATE;
    }

    *outHandle = store;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_SecureStorage_Put(
    ADUC_SecureStorageHandle handle,
    const char* key,
    const void* value,
    size_t valueLen)
{
    if (handle == NULL || value == NULL || valueLen == 0)
    {
        return SS_ERR_INVALID_ARG;
    }

    if (!is_valid_key(key))
    {
        return SS_ERR_INVALID_ARG;
    }

    char path[MAX_PATH_LEN];
    build_key_path(handle, key, path, sizeof(path));

    // XOR-obfuscate the data
    uint8_t* obfuscated = (uint8_t*)malloc(valueLen);
    if (obfuscated == NULL)
    {
        return SS_ERR_ALLOC;
    }

    xor_obfuscate(handle->obfuscationKey, OBFUSCATION_KEY_LEN, value, obfuscated, valueLen);

    // Write with 0600 permissions
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
    {
        free(obfuscated);
        return SS_ERR_FILE_OPEN;
    }

    FILE* f = fdopen(fd, "wb");
    if (f == NULL)
    {
        close(fd);
        free(obfuscated);
        return SS_ERR_FILE_OPEN;
    }

    size_t written = fwrite(obfuscated, 1, valueLen, f);
    fclose(f);
    free(obfuscated);

    if (written != valueLen)
    {
        return SS_ERR_IO;
    }

    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_SecureStorage_Get(
    ADUC_SecureStorageHandle handle,
    const char* key,
    void* valueBuf,
    size_t bufLen,
    size_t* outLen)
{
    if (handle == NULL || valueBuf == NULL || outLen == NULL)
    {
        return SS_ERR_INVALID_ARG;
    }

    if (!is_valid_key(key))
    {
        return SS_ERR_INVALID_ARG;
    }

    char path[MAX_PATH_LEN];
    build_key_path(handle, key, path, sizeof(path));

    FILE* f = fopen(path, "rb");
    if (f == NULL)
    {
        return SS_ERR_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fileSize < 0)
    {
        fclose(f);
        return SS_ERR_IO;
    }

    if ((size_t)fileSize > bufLen)
    {
        fclose(f);
        *outLen = (size_t)fileSize;
        return SS_ERR_BUF_TOO_SMALL;
    }

    uint8_t* obfuscated = (uint8_t*)malloc((size_t)fileSize);
    if (obfuscated == NULL)
    {
        fclose(f);
        return SS_ERR_ALLOC;
    }

    size_t bytesRead = fread(obfuscated, 1, (size_t)fileSize, f);
    fclose(f);

    if (bytesRead != (size_t)fileSize)
    {
        free(obfuscated);
        return SS_ERR_IO;
    }

    // De-obfuscate
    xor_obfuscate(handle->obfuscationKey, OBFUSCATION_KEY_LEN, obfuscated, valueBuf, bytesRead);
    free(obfuscated);

    *outLen = bytesRead;
    return ADUC_RESULT2_SUCCESS;
}

void ADUC_SecureStorage_Release(void* buf, size_t len)
{
    if (buf != NULL && len > 0)
    {
        volatile uint8_t* p = (volatile uint8_t*)buf;
        for (size_t i = 0; i < len; i++)
        {
            p[i] = 0;
        }
    }
}

ADUC_Result2 ADUC_SecureStorage_Delete(ADUC_SecureStorageHandle handle, const char* key)
{
    if (handle == NULL)
    {
        return SS_ERR_INVALID_ARG;
    }

    if (!is_valid_key(key))
    {
        return SS_ERR_INVALID_ARG;
    }

    char path[MAX_PATH_LEN];
    build_key_path(handle, key, path, sizeof(path));

    if (unlink(path) != 0)
    {
        if (errno == ENOENT)
        {
            return SS_ERR_NOT_FOUND;
        }
        return SS_ERR_IO;
    }

    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_SecureStorage_ListKeys(
    ADUC_SecureStorageHandle handle,
    char** outKeys,
    size_t* outCount)
{
    if (handle == NULL || outKeys == NULL || outCount == NULL)
    {
        return SS_ERR_INVALID_ARG;
    }

#ifdef _WIN32
    /* Use FindFirstFileA/FindNextFileA on Windows */
    char searchPath[MAX_PATH_LEN];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", handle->storePath);

    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return SS_ERR_FILE_OPEN;
    }

    // First pass: count entries
    size_t count = 0;
    do
    {
        if (findData.cFileName[0] != '.')
        {
            count++;
        }
    } while (FindNextFileA(hFind, &findData));

    if (count == 0)
    {
        FindClose(hFind);
        *outKeys = NULL;
        *outCount = 0;
        return ADUC_RESULT2_SUCCESS;
    }

    char** keys = (char**)calloc(count, sizeof(char*));
    if (keys == NULL)
    {
        FindClose(hFind);
        return SS_ERR_ALLOC;
    }

    // Second pass: collect names
    FindClose(hFind);
    hFind = FindFirstFileA(searchPath, &findData);
    size_t idx = 0;
    do
    {
        if (findData.cFileName[0] == '.' || idx >= count)
        {
            continue;
        }
        keys[idx] = _strdup(findData.cFileName);
        if (keys[idx] == NULL)
        {
            for (size_t i = 0; i < idx; i++)
            {
                free(keys[i]);
            }
            free(keys);
            FindClose(hFind);
            return SS_ERR_ALLOC;
        }
        idx++;
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
#else
    DIR* dir = opendir(handle->storePath);
    if (dir == NULL)
    {
        return SS_ERR_FILE_OPEN;
    }

    // First pass: count entries
    size_t count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
        {
            continue;
        }
        count++;
    }

    if (count == 0)
    {
        closedir(dir);
        *outKeys = NULL;
        *outCount = 0;
        return ADUC_RESULT2_SUCCESS;
    }

    // Allocate array
    char** keys = (char**)calloc(count, sizeof(char*));
    if (keys == NULL)
    {
        closedir(dir);
        return SS_ERR_ALLOC;
    }

    // Second pass: collect names
    rewinddir(dir);
    size_t idx = 0;
    while ((entry = readdir(dir)) != NULL && idx < count)
    {
        if (entry->d_name[0] == '.')
        {
            continue;
        }
        keys[idx] = strdup(entry->d_name);
        if (keys[idx] == NULL)
        {
            // Cleanup on failure
            for (size_t i = 0; i < idx; i++)
            {
                free(keys[i]);
            }
            free(keys);
            closedir(dir);
            return SS_ERR_ALLOC;
        }
        idx++;
    }

    closedir(dir);
#endif

    // outKeys is a pointer to a char* array; reinterpret to store the allocated array
    memcpy(outKeys, &keys, sizeof(char**));
    *outCount = idx;

    return ADUC_RESULT2_SUCCESS;
}

void ADUC_SecureStorage_FreeKeys(char** keys, size_t count)
{
    if (keys == NULL)
    {
        return;
    }
    for (size_t i = 0; i < count; i++)
    {
        free(keys[i]);
    }
    free(keys);
}

void ADUC_SecureStorage_Destroy(ADUC_SecureStorageHandle handle)
{
    if (handle != NULL)
    {
        // Zero the obfuscation key before freeing
        volatile uint8_t* p = (volatile uint8_t*)handle->obfuscationKey;
        for (size_t i = 0; i < OBFUSCATION_KEY_LEN; i++)
        {
            p[i] = 0;
        }
        free(handle);
    }
}
