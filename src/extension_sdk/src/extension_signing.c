/**
 * @file extension_signing.c
 * @brief Extension manifest signing and verification implementation.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/extension_signing.h"

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <parson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

#define SHA256_HEX_LEN 64
#define SHA256_BUF_MIN (SHA256_HEX_LEN + 1)
#define READ_BUF_SIZE 8192
#define MAX_CANONICAL_SIZE 4096
#define MANIFEST_FILENAME "manifest.json"

#define SIGN_ERR_INVALID_ARG    ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_AUTH, 0x0001)
#define SIGN_ERR_FILE_OPEN      ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_IO, 0x0001)
#define SIGN_ERR_ALLOC          ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_RESOURCE, 0x0001)
#define SIGN_ERR_HASH_COMPUTE   ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_IO, 0x0002)
#define SIGN_ERR_HASH_MISMATCH  ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_AUTH, 0x0002)
#define SIGN_ERR_PARSE_FAIL     ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_IO, 0x0003)
#define SIGN_ERR_KEY_LOAD       ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_AUTH, 0x0003)
#define SIGN_ERR_SIG_INVALID    ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_AUTH, 0x0004)
#define SIGN_ERR_SIG_VERIFY     ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_AUTH, 0x0005)
#define SIGN_ERR_SIG_MISSING    ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_AUTH, 0x0006)
#define SIGN_ERR_SIGN_FAIL      ADUC_RESULT2_MAKE(ADUC_FACILITY_SECURITY, ADUC_CATEGORY_AUTH, 0x0007)

/* ── Helper: safe string copy ────────────────────────────────────────────── */

static void safe_strcpy(char* dst, size_t dstSize, const char* src)
{
    if (src == NULL)
    {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
}

/* ── Canonical manifest content for signing/verification ─────────────────── */

/**
 * @brief Build canonical string from manifest fields (all except signature).
 *
 * Fields are sorted alphabetically by key, formatted as "key=value\n".
 */
static int build_canonical_content(const ADUC_ExtensionManifest* manifest, char* buf, size_t bufLen)
{
    int written = snprintf(
        buf, bufLen,
        "author=%s\n"
        "library=%s\n"
        "minAgentVersion=%s\n"
        "name=%s\n"
        "sha256=%s\n"
        "type=%s\n"
        "version=%s\n",
        manifest->author,
        manifest->library,
        manifest->minAgentVersion,
        manifest->name,
        manifest->sha256,
        manifest->type,
        manifest->version);

    return written;
}

/* ── SHA-256 file hashing ────────────────────────────────────────────────── */

ADUC_Result2 ADUC_ExtensionSigning_HashFile(
    const char* filePath,
    char* hashBuf,
    size_t hashBufLen)
{
    if (filePath == NULL || hashBuf == NULL || hashBufLen < SHA256_BUF_MIN)
    {
        return SIGN_ERR_INVALID_ARG;
    }

    FILE* f = fopen(filePath, "rb");
    if (f == NULL)
    {
        return SIGN_ERR_FILE_OPEN;
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx == NULL)
    {
        fclose(f);
        return SIGN_ERR_ALLOC;
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1)
    {
        EVP_MD_CTX_free(ctx);
        fclose(f);
        return SIGN_ERR_HASH_COMPUTE;
    }

    unsigned char buf[READ_BUF_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buf, 1, sizeof(buf), f)) > 0)
    {
        if (EVP_DigestUpdate(ctx, buf, bytesRead) != 1)
        {
            EVP_MD_CTX_free(ctx);
            fclose(f);
            return SIGN_ERR_HASH_COMPUTE;
        }
    }

    fclose(f);

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;
    if (EVP_DigestFinal_ex(ctx, digest, &digestLen) != 1)
    {
        EVP_MD_CTX_free(ctx);
        return SIGN_ERR_HASH_COMPUTE;
    }
    EVP_MD_CTX_free(ctx);

    // Hex-encode
    static const char hexChars[] = "0123456789abcdef";
    for (unsigned int i = 0; i < digestLen; i++)
    {
        hashBuf[i * 2] = hexChars[(digest[i] >> 4) & 0x0F];
        hashBuf[i * 2 + 1] = hexChars[digest[i] & 0x0F];
    }
    hashBuf[digestLen * 2] = '\0';

    return ADUC_RESULT2_SUCCESS;
}

/* ── Manifest parsing (new format) ───────────────────────────────────────── */

ADUC_Result2 ADUC_ExtensionManifest_Parse(
    const char* manifestPath,
    ADUC_ExtensionManifest* outManifest)
{
    if (manifestPath == NULL || outManifest == NULL)
    {
        return SIGN_ERR_INVALID_ARG;
    }

    memset(outManifest, 0, sizeof(*outManifest));

    JSON_Value* rootValue = json_parse_file(manifestPath);
    if (rootValue == NULL)
    {
        return SIGN_ERR_PARSE_FAIL;
    }

    JSON_Object* root = json_value_get_object(rootValue);
    if (root == NULL)
    {
        json_value_free(rootValue);
        return SIGN_ERR_PARSE_FAIL;
    }

    const char* name = json_object_get_string(root, "name");
    const char* version = json_object_get_string(root, "version");
    const char* type = json_object_get_string(root, "type");
    const char* library = json_object_get_string(root, "library");
    const char* sha256 = json_object_get_string(root, "sha256");

    // name, version, type, library, sha256 are required
    if (name == NULL || version == NULL || type == NULL || library == NULL || sha256 == NULL)
    {
        json_value_free(rootValue);
        return SIGN_ERR_PARSE_FAIL;
    }

    safe_strcpy(outManifest->name, sizeof(outManifest->name), name);
    safe_strcpy(outManifest->version, sizeof(outManifest->version), version);
    safe_strcpy(outManifest->type, sizeof(outManifest->type), type);
    safe_strcpy(outManifest->library, sizeof(outManifest->library), library);
    safe_strcpy(outManifest->sha256, sizeof(outManifest->sha256), sha256);

    // Optional fields
    const char* minAgentVersion = json_object_get_string(root, "minAgentVersion");
    const char* author = json_object_get_string(root, "author");
    const char* signature = json_object_get_string(root, "signature");

    safe_strcpy(outManifest->minAgentVersion, sizeof(outManifest->minAgentVersion),
                minAgentVersion ? minAgentVersion : "");
    safe_strcpy(outManifest->author, sizeof(outManifest->author),
                author ? author : "");
    safe_strcpy(outManifest->signature, sizeof(outManifest->signature),
                signature ? signature : "");

    json_value_free(rootValue);
    return ADUC_RESULT2_SUCCESS;
}

/* ── Legacy manifest parsing ─────────────────────────────────────────────── */

ADUC_Result2 ADUC_ExtensionSigning_ParseManifest(
    const char* manifestPath,
    ADUC_ExtensionManifest* outManifest)
{
    // Delegate to the new parser — the new struct is a superset
    return ADUC_ExtensionManifest_Parse(manifestPath, outManifest);
}

/* ── Hash verification ───────────────────────────────────────────────────── */

ADUC_Result2 ADUC_ExtensionManifest_VerifyHash(
    const ADUC_ExtensionManifest* manifest,
    const char* extensionDir)
{
    if (manifest == NULL || extensionDir == NULL)
    {
        return SIGN_ERR_INVALID_ARG;
    }

    if (manifest->library[0] == '\0' || manifest->sha256[0] == '\0')
    {
        return SIGN_ERR_PARSE_FAIL;
    }

    char libraryPath[1024];
    snprintf(libraryPath, sizeof(libraryPath), "%s/%s", extensionDir, manifest->library);

    char actualHash[SHA256_BUF_MIN];
    ADUC_Result2 result = ADUC_ExtensionSigning_HashFile(libraryPath, actualHash, sizeof(actualHash));
    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        return result;
    }

    if (strcmp(actualHash, manifest->sha256) != 0)
    {
        fprintf(stderr, "ExtensionSigning: Hash mismatch for %s\n  expected: %s\n  actual:   %s\n",
                manifest->library, manifest->sha256, actualHash);
        return SIGN_ERR_HASH_MISMATCH;
    }

    return ADUC_RESULT2_SUCCESS;
}

/* ── Signature verification ──────────────────────────────────────────────── */

/**
 * @brief Decode base64 string to binary.
 * @return Number of decoded bytes, or -1 on error.
 */
static int base64_decode(const char* input, unsigned char* output, int outputLen)
{
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new_mem_buf(input, -1);
    if (b64 == NULL || bmem == NULL)
    {
        BIO_free(b64);
        BIO_free(bmem);
        return -1;
    }

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_push(b64, bmem);

    int decoded = BIO_read(bmem, output, outputLen);
    BIO_free_all(bmem);
    return decoded;
}

/**
 * @brief Encode binary data to base64.
 * @return Allocated base64 string (caller must free), or NULL on error.
 */
static char* base64_encode(const unsigned char* input, int inputLen)
{
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    if (b64 == NULL || bmem == NULL)
    {
        BIO_free(b64);
        BIO_free(bmem);
        return NULL;
    }

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, input, inputLen);
    (void)BIO_flush(b64);

    BUF_MEM* bptr = NULL;
    BIO_get_mem_ptr(b64, &bptr);

    char* result = (char*)malloc(bptr->length + 1);
    if (result != NULL)
    {
        memcpy(result, bptr->data, bptr->length);
        result[bptr->length] = '\0';
    }

    BIO_free_all(b64);
    return result;
}

ADUC_Result2 ADUC_ExtensionManifest_VerifySignature(
    const ADUC_ExtensionManifest* manifest,
    const char* trustedCertPath)
{
    if (manifest == NULL || trustedCertPath == NULL)
    {
        return SIGN_ERR_INVALID_ARG;
    }

    if (manifest->signature[0] == '\0')
    {
        return SIGN_ERR_SIG_MISSING;
    }

    // Build canonical content
    char canonical[MAX_CANONICAL_SIZE];
    int canonLen = build_canonical_content(manifest, canonical, sizeof(canonical));
    if (canonLen <= 0 || (size_t)canonLen >= sizeof(canonical))
    {
        return SIGN_ERR_ALLOC;
    }

    // Decode signature from base64
    unsigned char sigBuf[512];
    int sigLen = base64_decode(manifest->signature, sigBuf, (int)sizeof(sigBuf));
    if (sigLen <= 0)
    {
        return SIGN_ERR_SIG_INVALID;
    }

    // Load trusted certificate and extract public key
    FILE* certFile = fopen(trustedCertPath, "r");
    if (certFile == NULL)
    {
        return SIGN_ERR_KEY_LOAD;
    }

    X509* cert = PEM_read_X509(certFile, NULL, NULL, NULL);
    fclose(certFile);
    if (cert == NULL)
    {
        return SIGN_ERR_KEY_LOAD;
    }

    EVP_PKEY* pubkey = X509_get_pubkey(cert);
    X509_free(cert);
    if (pubkey == NULL)
    {
        return SIGN_ERR_KEY_LOAD;
    }

    // Verify signature
    EVP_MD_CTX* mdCtx = EVP_MD_CTX_new();
    if (mdCtx == NULL)
    {
        EVP_PKEY_free(pubkey);
        return SIGN_ERR_ALLOC;
    }

    ADUC_Result2 result = ADUC_RESULT2_SUCCESS;

    if (EVP_DigestVerifyInit(mdCtx, NULL, EVP_sha256(), NULL, pubkey) != 1)
    {
        result = SIGN_ERR_SIG_VERIFY;
    }
    else if (EVP_DigestVerifyUpdate(mdCtx, canonical, (size_t)canonLen) != 1)
    {
        result = SIGN_ERR_SIG_VERIFY;
    }
    else if (EVP_DigestVerifyFinal(mdCtx, sigBuf, (size_t)sigLen) != 1)
    {
        result = SIGN_ERR_SIG_INVALID;
    }

    EVP_MD_CTX_free(mdCtx);
    EVP_PKEY_free(pubkey);

    return result;
}

/* ── Manifest signing ────────────────────────────────────────────────────── */

ADUC_Result2 ADUC_ExtensionManifest_Sign(
    ADUC_ExtensionManifest* manifest,
    const char* privateKeyPath)
{
    if (manifest == NULL || privateKeyPath == NULL)
    {
        return SIGN_ERR_INVALID_ARG;
    }

    // Clear existing signature before computing canonical content
    manifest->signature[0] = '\0';

    // Build canonical content
    char canonical[MAX_CANONICAL_SIZE];
    int canonLen = build_canonical_content(manifest, canonical, sizeof(canonical));
    if (canonLen <= 0 || (size_t)canonLen >= sizeof(canonical))
    {
        return SIGN_ERR_ALLOC;
    }

    // Load private key
    FILE* keyFile = fopen(privateKeyPath, "r");
    if (keyFile == NULL)
    {
        return SIGN_ERR_KEY_LOAD;
    }

    EVP_PKEY* privkey = PEM_read_PrivateKey(keyFile, NULL, NULL, NULL);
    fclose(keyFile);
    if (privkey == NULL)
    {
        return SIGN_ERR_KEY_LOAD;
    }

    // Sign
    EVP_MD_CTX* mdCtx = EVP_MD_CTX_new();
    if (mdCtx == NULL)
    {
        EVP_PKEY_free(privkey);
        return SIGN_ERR_ALLOC;
    }

    ADUC_Result2 result = ADUC_RESULT2_SUCCESS;
    unsigned char sigBuf[512];
    size_t sigLen = sizeof(sigBuf);

    if (EVP_DigestSignInit(mdCtx, NULL, EVP_sha256(), NULL, privkey) != 1)
    {
        result = SIGN_ERR_SIGN_FAIL;
        goto cleanup;
    }

    if (EVP_DigestSignUpdate(mdCtx, canonical, (size_t)canonLen) != 1)
    {
        result = SIGN_ERR_SIGN_FAIL;
        goto cleanup;
    }

    if (EVP_DigestSignFinal(mdCtx, sigBuf, &sigLen) != 1)
    {
        result = SIGN_ERR_SIGN_FAIL;
        goto cleanup;
    }

    // Base64-encode signature into manifest
    {
        char* b64 = base64_encode(sigBuf, (int)sigLen);
        if (b64 == NULL)
        {
            result = SIGN_ERR_ALLOC;
            goto cleanup;
        }
        safe_strcpy(manifest->signature, sizeof(manifest->signature), b64);
        free(b64);
    }

cleanup:
    EVP_MD_CTX_free(mdCtx);
    EVP_PKEY_free(privkey);
    return result;
}

/* ── Full verification with policy ───────────────────────────────────────── */

ADUC_Result2 ADUC_Extension_Verify(
    const char* extensionDir,
    const char* trustedCertPath,
    ADUC_SignPolicy policy)
{
    if (extensionDir == NULL)
    {
        return SIGN_ERR_INVALID_ARG;
    }

    if (policy == ADUC_SIGN_POLICY_NONE)
    {
        fprintf(stderr, "ExtensionSigning: Policy is NONE — skipping verification for %s\n", extensionDir);
        return ADUC_RESULT2_SUCCESS;
    }

    // Parse manifest
    char manifestPath[1024];
    snprintf(manifestPath, sizeof(manifestPath), "%s/%s", extensionDir, MANIFEST_FILENAME);

    ADUC_ExtensionManifest manifest;
    ADUC_Result2 result = ADUC_ExtensionManifest_Parse(manifestPath, &manifest);
    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        fprintf(stderr, "ExtensionSigning: Failed to parse manifest at %s\n", manifestPath);
        if (policy == ADUC_SIGN_POLICY_WARN)
        {
            fprintf(stderr, "ExtensionSigning: Policy is WARN — allowing unsigned extension\n");
            return ADUC_RESULT2_SUCCESS;
        }
        return result;
    }

    // Verify hash
    result = ADUC_ExtensionManifest_VerifyHash(&manifest, extensionDir);
    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        fprintf(stderr, "ExtensionSigning: Hash verification failed for %s\n", manifest.library);
        if (policy == ADUC_SIGN_POLICY_WARN)
        {
            fprintf(stderr, "ExtensionSigning: Policy is WARN — allowing despite hash mismatch\n");
            return ADUC_RESULT2_SUCCESS;
        }
        return result;
    }

    // Verify signature
    if (trustedCertPath == NULL || trustedCertPath[0] == '\0')
    {
        if (manifest.signature[0] != '\0')
        {
            fprintf(stderr, "ExtensionSigning: Manifest is signed but no trusted cert provided\n");
        }
        if (policy == ADUC_SIGN_POLICY_ENFORCE)
        {
            return SIGN_ERR_KEY_LOAD;
        }
        fprintf(stderr, "ExtensionSigning: Policy is WARN — skipping signature check (no cert)\n");
        return ADUC_RESULT2_SUCCESS;
    }

    result = ADUC_ExtensionManifest_VerifySignature(&manifest, trustedCertPath);
    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        fprintf(stderr, "ExtensionSigning: Signature verification failed for %s\n", manifest.name);
        if (policy == ADUC_SIGN_POLICY_WARN)
        {
            fprintf(stderr, "ExtensionSigning: Policy is WARN — allowing despite invalid signature\n");
            return ADUC_RESULT2_SUCCESS;
        }
        return result;
    }

    return ADUC_RESULT2_SUCCESS;
}

/* ── Legacy Verify (unchanged behavior) ──────────────────────────────────── */

ADUC_Result2 ADUC_ExtensionSigning_Verify(
    const char* soPath,
    const char* manifestPath,
    const char* trustedKeyPath)
{
    if (soPath == NULL || manifestPath == NULL || trustedKeyPath == NULL)
    {
        return SIGN_ERR_INVALID_ARG;
    }

    ADUC_ExtensionManifest manifest;
    ADUC_Result2 result = ADUC_ExtensionManifest_Parse(manifestPath, &manifest);
    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        return result;
    }

    // Compute hash of the .so file
    char actualHash[SHA256_BUF_MIN];
    result = ADUC_ExtensionSigning_HashFile(soPath, actualHash, sizeof(actualHash));
    if (ADUC_RESULT2_IS_FAILURE(result))
    {
        return result;
    }

    if (strcmp(actualHash, manifest.sha256) != 0)
    {
        return SIGN_ERR_HASH_MISMATCH;
    }

    return ADUC_ExtensionManifest_VerifySignature(&manifest, trustedKeyPath);
}

/* ── Policy string parsing ───────────────────────────────────────────────── */

ADUC_SignPolicy ADUC_SignPolicy_FromString(const char* policyStr)
{
    if (policyStr == NULL)
    {
        return ADUC_SIGN_POLICY_ENFORCE;
    }

    if (strcasecmp(policyStr, "none") == 0)
    {
        return ADUC_SIGN_POLICY_NONE;
    }
    if (strcasecmp(policyStr, "warn") == 0)
    {
        return ADUC_SIGN_POLICY_WARN;
    }
    if (strcasecmp(policyStr, "enforce") == 0)
    {
        return ADUC_SIGN_POLICY_ENFORCE;
    }

    return ADUC_SIGN_POLICY_ENFORCE;
}
