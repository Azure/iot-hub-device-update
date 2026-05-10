/**
 * @file security_ut.cpp
 * @brief Unit tests for secure storage, extension signing, and cert lifecycle.
 */

#include <catch2/catch_all.hpp>

#include <cstdio>
#include <cstring>
#include <string>

extern "C"
{
#include "aduc/secure_storage.h"
#include "aduc/extension_signing.h"
#include "aduc/cert_lifecycle.h"
}

static const char* TEST_STORE_PATH = "test_secure_store";
static const char* TEST_HASH_FILE = "test_hash_input.bin";

class TempFile
{
public:
    explicit TempFile(const char* path, const void* content = nullptr, size_t len = 0)
        : m_path(path)
    {
        if (content != nullptr && len > 0)
        {
            FILE* f = fopen(path, "wb");
            if (f != nullptr)
            {
                fwrite(content, 1, len, f);
                fclose(f);
            }
        }
    }
    ~TempFile() { remove(m_path.c_str()); }
    const char* path() const { return m_path.c_str(); }
private:
    std::string m_path;
};

// ─── Secure Storage Tests ────────────────────────────────────────────────────

TEST_CASE("SecureStorage: Create FILE backend succeeds", "[secure_storage]")
{
    TempFile tmp(TEST_STORE_PATH);
    ADUC_SecureStorageHandle handle = nullptr;

    ADUC_Result2 result = ADUC_SecureStorage_Create(
        ADUC_SECURE_STORAGE_FILE, TEST_STORE_PATH, &handle);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(handle != nullptr);

    ADUC_SecureStorage_Destroy(handle);
}

TEST_CASE("SecureStorage: Put and get round-trip", "[secure_storage]")
{
    TempFile tmp(TEST_STORE_PATH);
    ADUC_SecureStorageHandle handle = nullptr;

    ADUC_Result2 result = ADUC_SecureStorage_Create(
        ADUC_SECURE_STORAGE_FILE, TEST_STORE_PATH, &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    const char* key = "test-secret";
    const char* value = "super-secret-value";
    size_t valueLen = strlen(value);

    result = ADUC_SecureStorage_Put(handle, key, value, valueLen);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    char buf[256] = {};
    size_t outLen = 0;
    result = ADUC_SecureStorage_Get(handle, key, buf, sizeof(buf), &outLen);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(outLen == valueLen);
    CHECK(memcmp(buf, value, valueLen) == 0);

    ADUC_SecureStorage_Destroy(handle);
}

TEST_CASE("SecureStorage: Get non-existent key fails", "[secure_storage]")
{
    TempFile tmp(TEST_STORE_PATH);
    ADUC_SecureStorageHandle handle = nullptr;

    ADUC_Result2 result = ADUC_SecureStorage_Create(
        ADUC_SECURE_STORAGE_FILE, TEST_STORE_PATH, &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    char buf[256] = {};
    size_t outLen = 0;
    result = ADUC_SecureStorage_Get(handle, "nonexistent_key", buf, sizeof(buf), &outLen);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    ADUC_SecureStorage_Destroy(handle);
}

TEST_CASE("SecureStorage: Delete removes key", "[secure_storage]")
{
    TempFile tmp(TEST_STORE_PATH);
    ADUC_SecureStorageHandle handle = nullptr;

    ADUC_Result2 result = ADUC_SecureStorage_Create(
        ADUC_SECURE_STORAGE_FILE, TEST_STORE_PATH, &handle);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    const char* key = "delete-me";
    const char* value = "temporary";
    result = ADUC_SecureStorage_Put(handle, key, value, strlen(value));
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    result = ADUC_SecureStorage_Delete(handle, key);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));

    // Should no longer be retrievable
    char buf[256] = {};
    size_t outLen = 0;
    result = ADUC_SecureStorage_Get(handle, key, buf, sizeof(buf), &outLen);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));

    ADUC_SecureStorage_Destroy(handle);
}

TEST_CASE("SecureStorage: Release zeros buffer", "[secure_storage]")
{
    char buf[32];
    memset(buf, 0xAA, sizeof(buf));

    ADUC_SecureStorage_Release(buf, sizeof(buf));

    // Verify buffer is zeroed
    for (size_t i = 0; i < sizeof(buf); i++)
    {
        CHECK(buf[i] == 0);
    }
}

// ─── Extension Signing Tests ─────────────────────────────────────────────────

TEST_CASE("ExtensionSigning: Hash known file produces expected SHA-256", "[extension_signing]")
{
    // Write known content "hello\n" and verify the hash
    const char* content = "hello\n";
    TempFile tmp(TEST_HASH_FILE, content, strlen(content));

    char hashBuf[65] = {};
    ADUC_Result2 result = ADUC_ExtensionSigning_HashFile(TEST_HASH_FILE, hashBuf, sizeof(hashBuf));
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));

    // SHA-256 of "hello\n" is 5891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be03
    std::string expected = "5891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be03";
    CHECK(std::string(hashBuf) == expected);
}

TEST_CASE("ExtensionSigning: Hash non-existent file fails", "[extension_signing]")
{
    char hashBuf[65] = {};
    ADUC_Result2 result = ADUC_ExtensionSigning_HashFile(
        "/no/such/file_xyz.bin", hashBuf, sizeof(hashBuf));
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

// ─── Cert Lifecycle Tests ────────────────────────────────────────────────────

TEST_CASE("CertLifecycle: GetInfo on non-existent cert fails", "[cert_lifecycle]")
{
    ADUC_CertInfo info = {};
    ADUC_Result2 result = ADUC_Cert_GetInfo("/no/such/cert.pem", &info);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}
