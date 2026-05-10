/**
 * @file extension_signing_ut.cpp
 * @brief Unit tests for extension manifest signing and verification.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

extern "C"
{
#include "aduc/extension_signing.h"
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

static const char* TEST_EXT_DIR = "test_ext_sign_dir";

class TestDir
{
public:
    explicit TestDir(const char* dir) : m_dir(dir)
    {
        std::string cmd = std::string("mkdir -p ") + dir;
        (void)system(cmd.c_str());
    }
    ~TestDir()
    {
        std::string cmd = std::string("rm -rf ") + m_dir;
        (void)system(cmd.c_str());
    }
    std::string path() const { return m_dir; }
    std::string filePath(const char* name) const { return m_dir + "/" + name; }

private:
    std::string m_dir;
};

static void writeFile(const std::string& path, const std::string& content)
{
    std::ofstream f(path, std::ios::binary);
    f << content;
}

static void writeFile(const std::string& path, const void* data, size_t len)
{
    std::ofstream f(path, std::ios::binary);
    f.write(static_cast<const char*>(data), static_cast<std::streamsize>(len));
}

static std::string makeValidManifestJson(
    const char* name,
    const char* version,
    const char* type,
    const char* library,
    const char* sha256)
{
    std::string json = "{\n";
    json += "  \"name\": \"" + std::string(name) + "\",\n";
    json += "  \"version\": \"" + std::string(version) + "\",\n";
    json += "  \"type\": \"" + std::string(type) + "\",\n";
    json += "  \"library\": \"" + std::string(library) + "\",\n";
    json += "  \"sha256\": \"" + std::string(sha256) + "\",\n";
    json += "  \"minAgentVersion\": \"2.0.0\",\n";
    json += "  \"author\": \"Microsoft Corporation\",\n";
    json += "  \"signature\": \"\"\n";
    json += "}\n";
    return json;
}

// ─── Manifest Parsing ────────────────────────────────────────────────────────

TEST_CASE("ExtensionManifest: Parse valid manifest.json", "[extension_signing]")
{
    TestDir dir(TEST_EXT_DIR);
    std::string manifestPath = dir.filePath("manifest.json");

    std::string json = makeValidManifestJson(
        "adu-direct-comm", "2.0.0", "communication_provider",
        "libadu_direct_comm.so",
        "abc123def456abc123def456abc123def456abc123def456abc123def456abcd");

    writeFile(manifestPath, json);

    ADUC_ExtensionManifest manifest = {};
    ADUC_Result2 result = ADUC_ExtensionManifest_Parse(manifestPath.c_str(), &manifest);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(std::string(manifest.name) == "adu-direct-comm");
    CHECK(std::string(manifest.version) == "2.0.0");
    CHECK(std::string(manifest.type) == "communication_provider");
    CHECK(std::string(manifest.library) == "libadu_direct_comm.so");
    CHECK(std::string(manifest.minAgentVersion) == "2.0.0");
    CHECK(std::string(manifest.author) == "Microsoft Corporation");
}

TEST_CASE("ExtensionManifest: Parse invalid manifest — missing required fields", "[extension_signing]")
{
    TestDir dir(TEST_EXT_DIR);
    std::string manifestPath = dir.filePath("manifest.json");

    // Missing "library" and "sha256"
    std::string json = "{ \"name\": \"test\", \"version\": \"1.0.0\", \"type\": \"handler\" }";
    writeFile(manifestPath, json);

    ADUC_ExtensionManifest manifest = {};
    ADUC_Result2 result = ADUC_ExtensionManifest_Parse(manifestPath.c_str(), &manifest);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

TEST_CASE("ExtensionManifest: Parse invalid manifest — not JSON", "[extension_signing]")
{
    TestDir dir(TEST_EXT_DIR);
    std::string manifestPath = dir.filePath("manifest.json");
    writeFile(manifestPath, "this is not json");

    ADUC_ExtensionManifest manifest = {};
    ADUC_Result2 result = ADUC_ExtensionManifest_Parse(manifestPath.c_str(), &manifest);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

TEST_CASE("ExtensionManifest: Parse NULL path fails", "[extension_signing]")
{
    ADUC_ExtensionManifest manifest = {};
    ADUC_Result2 result = ADUC_ExtensionManifest_Parse(nullptr, &manifest);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

// ─── Hash Verification ──────────────────────────────────────────────────────

TEST_CASE("ExtensionManifest: VerifyHash succeeds for matching file", "[extension_signing]")
{
    TestDir dir(TEST_EXT_DIR);

    // Write a known file
    const char* content = "hello\n";
    std::string libPath = dir.filePath("test.so");
    writeFile(libPath, content, strlen(content));

    // SHA-256 of "hello\n"
    const char* expectedHash = "5891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be03";

    // Build manifest
    ADUC_ExtensionManifest manifest = {};
    strncpy(manifest.library, "test.so", sizeof(manifest.library) - 1);
    strncpy(manifest.sha256, expectedHash, sizeof(manifest.sha256) - 1);

    ADUC_Result2 result = ADUC_ExtensionManifest_VerifyHash(&manifest, dir.path().c_str());
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
}

TEST_CASE("ExtensionManifest: VerifyHash fails for tampered file", "[extension_signing]")
{
    TestDir dir(TEST_EXT_DIR);

    const char* content = "tampered content";
    std::string libPath = dir.filePath("test.so");
    writeFile(libPath, content, strlen(content));

    // Use the hash of different content
    const char* wrongHash = "5891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be03";

    ADUC_ExtensionManifest manifest = {};
    strncpy(manifest.library, "test.so", sizeof(manifest.library) - 1);
    strncpy(manifest.sha256, wrongHash, sizeof(manifest.sha256) - 1);

    ADUC_Result2 result = ADUC_ExtensionManifest_VerifyHash(&manifest, dir.path().c_str());
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

TEST_CASE("ExtensionManifest: VerifyHash fails for missing file", "[extension_signing]")
{
    TestDir dir(TEST_EXT_DIR);

    ADUC_ExtensionManifest manifest = {};
    strncpy(manifest.library, "nonexistent.so", sizeof(manifest.library) - 1);
    strncpy(manifest.sha256, "abcd", sizeof(manifest.sha256) - 1);

    ADUC_Result2 result = ADUC_ExtensionManifest_VerifyHash(&manifest, dir.path().c_str());
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

// ─── Sign Policy ─────────────────────────────────────────────────────────────

TEST_CASE("SignPolicy: NONE always succeeds", "[extension_signing]")
{
    // Even with a non-existent directory, NONE should succeed
    ADUC_Result2 result = ADUC_Extension_Verify("/nonexistent/dir", nullptr, ADUC_SIGN_POLICY_NONE);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
}

TEST_CASE("SignPolicy: WARN logs but succeeds on missing manifest", "[extension_signing]")
{
    TestDir dir(TEST_EXT_DIR);
    // No manifest.json — should warn but succeed
    ADUC_Result2 result = ADUC_Extension_Verify(dir.path().c_str(), nullptr, ADUC_SIGN_POLICY_WARN);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
}

TEST_CASE("SignPolicy: ENFORCE rejects missing manifest", "[extension_signing]")
{
    TestDir dir(TEST_EXT_DIR);
    // No manifest.json — should fail
    ADUC_Result2 result = ADUC_Extension_Verify(dir.path().c_str(), nullptr, ADUC_SIGN_POLICY_ENFORCE);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

// ─── Sign and Verify Round-Trip ─────────────────────────────────────────────

TEST_CASE("ExtensionManifest: Sign and verify round-trip", "[extension_signing]")
{
    TestDir dir(TEST_EXT_DIR);

    // Generate test ECDSA keypair
    std::string keyPath = dir.filePath("test-key.pem");
    std::string certPath = dir.filePath("test-cert.pem");

    std::string genKeyCmd = "openssl ecparam -genkey -name prime256v1 -out " + keyPath + " 2>/dev/null";
    std::string genCertCmd = "openssl req -new -x509 -key " + keyPath + " -out " + certPath +
                             " -days 1 -subj '/CN=ADU Test' 2>/dev/null";

    int rc = system(genKeyCmd.c_str());
    if (rc != 0)
    {
        SKIP("OpenSSL not available for key generation");
    }
    rc = system(genCertCmd.c_str());
    REQUIRE(rc == 0);

    // Create a library file
    const char* libContent = "fake library content for signing test";
    writeFile(dir.filePath("libtest.so"), libContent, strlen(libContent));

    // Compute hash
    char hashBuf[65] = {};
    ADUC_Result2 result = ADUC_ExtensionSigning_HashFile(
        dir.filePath("libtest.so").c_str(), hashBuf, sizeof(hashBuf));
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    // Build manifest
    ADUC_ExtensionManifest manifest = {};
    strncpy(manifest.name, "test-ext", sizeof(manifest.name) - 1);
    strncpy(manifest.version, "1.0.0", sizeof(manifest.version) - 1);
    strncpy(manifest.type, "step_handler", sizeof(manifest.type) - 1);
    strncpy(manifest.library, "libtest.so", sizeof(manifest.library) - 1);
    strncpy(manifest.sha256, hashBuf, sizeof(manifest.sha256) - 1);
    strncpy(manifest.minAgentVersion, "2.0.0", sizeof(manifest.minAgentVersion) - 1);
    strncpy(manifest.author, "Test Author", sizeof(manifest.author) - 1);

    // Sign
    result = ADUC_ExtensionManifest_Sign(&manifest, keyPath.c_str());
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(manifest.signature[0] != '\0');

    // Verify signature
    result = ADUC_ExtensionManifest_VerifySignature(&manifest, certPath.c_str());
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));

    // Verify hash
    result = ADUC_ExtensionManifest_VerifyHash(&manifest, dir.path().c_str());
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
}

TEST_CASE("ExtensionManifest: Full verify round-trip with manifest.json on disk", "[extension_signing]")
{
    TestDir dir(TEST_EXT_DIR);

    // Generate test ECDSA keypair
    std::string keyPath = dir.filePath("test-key.pem");
    std::string certPath = dir.filePath("test-cert.pem");

    std::string genKeyCmd = "openssl ecparam -genkey -name prime256v1 -out " + keyPath + " 2>/dev/null";
    std::string genCertCmd = "openssl req -new -x509 -key " + keyPath + " -out " + certPath +
                             " -days 1 -subj '/CN=ADU Test' 2>/dev/null";

    int rc = system(genKeyCmd.c_str());
    if (rc != 0)
    {
        SKIP("OpenSSL not available for key generation");
    }
    rc = system(genCertCmd.c_str());
    REQUIRE(rc == 0);

    // Create library file
    const char* libContent = "library content for full verify test";
    writeFile(dir.filePath("libfull.so"), libContent, strlen(libContent));

    // Compute hash
    char hashBuf[65] = {};
    ADUC_Result2 result = ADUC_ExtensionSigning_HashFile(
        dir.filePath("libfull.so").c_str(), hashBuf, sizeof(hashBuf));
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    // Build, sign, and write manifest
    ADUC_ExtensionManifest manifest = {};
    strncpy(manifest.name, "full-test", sizeof(manifest.name) - 1);
    strncpy(manifest.version, "1.0.0", sizeof(manifest.version) - 1);
    strncpy(manifest.type, "downloader", sizeof(manifest.type) - 1);
    strncpy(manifest.library, "libfull.so", sizeof(manifest.library) - 1);
    strncpy(manifest.sha256, hashBuf, sizeof(manifest.sha256) - 1);
    strncpy(manifest.minAgentVersion, "2.0.0", sizeof(manifest.minAgentVersion) - 1);
    strncpy(manifest.author, "Test", sizeof(manifest.author) - 1);

    result = ADUC_ExtensionManifest_Sign(&manifest, keyPath.c_str());
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    // Write manifest.json
    std::string manifestJson;
    manifestJson += "{\n";
    manifestJson += "  \"name\": \"" + std::string(manifest.name) + "\",\n";
    manifestJson += "  \"version\": \"" + std::string(manifest.version) + "\",\n";
    manifestJson += "  \"type\": \"" + std::string(manifest.type) + "\",\n";
    manifestJson += "  \"library\": \"" + std::string(manifest.library) + "\",\n";
    manifestJson += "  \"sha256\": \"" + std::string(manifest.sha256) + "\",\n";
    manifestJson += "  \"minAgentVersion\": \"" + std::string(manifest.minAgentVersion) + "\",\n";
    manifestJson += "  \"author\": \"" + std::string(manifest.author) + "\",\n";
    manifestJson += "  \"signature\": \"" + std::string(manifest.signature) + "\"\n";
    manifestJson += "}\n";
    writeFile(dir.filePath("manifest.json"), manifestJson);

    // Full verify with ENFORCE policy
    result = ADUC_Extension_Verify(dir.path().c_str(), certPath.c_str(), ADUC_SIGN_POLICY_ENFORCE);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
}

// ─── Policy string parsing ──────────────────────────────────────────────────

TEST_CASE("SignPolicy: FromString parses correctly", "[extension_signing]")
{
    CHECK(ADUC_SignPolicy_FromString("none") == ADUC_SIGN_POLICY_NONE);
    CHECK(ADUC_SignPolicy_FromString("NONE") == ADUC_SIGN_POLICY_NONE);
    CHECK(ADUC_SignPolicy_FromString("warn") == ADUC_SIGN_POLICY_WARN);
    CHECK(ADUC_SignPolicy_FromString("Warn") == ADUC_SIGN_POLICY_WARN);
    CHECK(ADUC_SignPolicy_FromString("enforce") == ADUC_SIGN_POLICY_ENFORCE);
    CHECK(ADUC_SignPolicy_FromString("ENFORCE") == ADUC_SIGN_POLICY_ENFORCE);
    CHECK(ADUC_SignPolicy_FromString("unknown") == ADUC_SIGN_POLICY_ENFORCE);
    CHECK(ADUC_SignPolicy_FromString(nullptr) == ADUC_SIGN_POLICY_ENFORCE);
}
