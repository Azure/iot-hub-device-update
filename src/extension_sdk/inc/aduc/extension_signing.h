/**
 * @file extension_signing.h
 * @brief Extension manifest signing and verification.
 *
 * Provides manifest parsing, SHA-256 hash verification of extension libraries,
 * cryptographic signature verification (ECDSA / RSA-PSS), and a configurable
 * signing policy (none / warn / enforce).
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_EXTENSION_SIGNING_H
#define ADUC_EXTENSION_SIGNING_H

#include "aduc/extension_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Parsed extension manifest (manifest.json).
 */
typedef struct ADUC_ExtensionManifest
{
    char name[128];
    char version[64];
    char type[64];
    char library[256];
    char sha256[65];          /**< Hex-encoded SHA-256 of the library file. */
    char minAgentVersion[64];
    char author[256];
    char signature[1024];     /**< Base64-encoded signature. */
} ADUC_ExtensionManifest;

/**
 * @brief Extension signing verification policy.
 */
typedef enum ADUC_SignPolicy
{
    ADUC_SIGN_POLICY_NONE    = 0,  /**< No verification (dev mode). */
    ADUC_SIGN_POLICY_WARN    = 1,  /**< Warn but allow unsigned extensions. */
    ADUC_SIGN_POLICY_ENFORCE = 2,  /**< Reject unsigned / invalid extensions. */
} ADUC_SignPolicy;

/**
 * @brief Parse manifest.json from a file path.
 *
 * @param manifestPath  Path to the manifest.json file.
 * @param outManifest   Output manifest structure.
 * @return ADUC_Result2 Success or error code.
 */
ADUC_Result2 ADUC_ExtensionManifest_Parse(
    const char* manifestPath,
    ADUC_ExtensionManifest* outManifest);

/**
 * @brief Verify the SHA-256 hash of the library file matches the manifest.
 *
 * @param manifest      Parsed manifest.
 * @param extensionDir  Directory containing the library file.
 * @return ADUC_Result2 Success if hash matches; failure otherwise.
 */
ADUC_Result2 ADUC_ExtensionManifest_VerifyHash(
    const ADUC_ExtensionManifest* manifest,
    const char* extensionDir);

/**
 * @brief Verify the cryptographic signature of the manifest.
 *
 * Computes a canonical representation of the manifest (all fields except
 * signature, sorted by key, newline-separated) and verifies the signature
 * using the public key from the trusted certificate.
 *
 * @param manifest        Parsed manifest (including base64 signature).
 * @param trustedCertPath Path to PEM-encoded trusted certificate.
 * @return ADUC_Result2   Success if signature is valid.
 */
ADUC_Result2 ADUC_ExtensionManifest_VerifySignature(
    const ADUC_ExtensionManifest* manifest,
    const char* trustedCertPath);

/**
 * @brief Full verification: parse manifest, check hash, check signature.
 *
 * @param extensionDir   Directory containing manifest.json and the library.
 * @param trustedCertPath Path to trusted certificate (may be NULL if policy is NONE).
 * @param policy         Signing verification policy.
 * @return ADUC_Result2  Success or structured error code.
 */
ADUC_Result2 ADUC_Extension_Verify(
    const char* extensionDir,
    const char* trustedCertPath,
    ADUC_SignPolicy policy);

/**
 * @brief Sign a manifest using a private key.
 *
 * Computes the canonical representation and signs it with the given
 * private key. The base64-encoded signature is written into manifest->signature.
 *
 * @param manifest       Manifest to sign (signature field is overwritten).
 * @param privateKeyPath Path to PEM-encoded private key.
 * @return ADUC_Result2  Success or error code.
 */
ADUC_Result2 ADUC_ExtensionManifest_Sign(
    ADUC_ExtensionManifest* manifest,
    const char* privateKeyPath);

/* ── Legacy API (kept for backward compatibility) ─────────────────────────── */

/**
 * @brief Compute SHA-256 of a file (hex output).
 */
ADUC_Result2 ADUC_ExtensionSigning_HashFile(
    const char* filePath,
    char* hashBuf,
    size_t hashBufLen);

/**
 * @brief Parse extension manifest file (legacy format).
 */
ADUC_Result2 ADUC_ExtensionSigning_ParseManifest(
    const char* manifestPath,
    ADUC_ExtensionManifest* outManifest);

/**
 * @brief Verify extension .so against its manifest and signing key (legacy).
 */
ADUC_Result2 ADUC_ExtensionSigning_Verify(
    const char* soPath,
    const char* manifestPath,
    const char* trustedKeyPath);

/**
 * @brief Parse a signing policy string ("none", "warn", "enforce").
 *
 * @param policyStr  Policy string from configuration.
 * @return Corresponding ADUC_SignPolicy value (defaults to ENFORCE for unknown).
 */
ADUC_SignPolicy ADUC_SignPolicy_FromString(const char* policyStr);

#ifdef __cplusplus
}
#endif

#endif // ADUC_EXTENSION_SIGNING_H
