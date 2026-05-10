/**
 * @file main.c
 * @brief Command-line tool for signing and verifying ADU extension manifests.
 *
 * Usage:
 *   adu-extension-sign --extension-dir /path/to/extension --key /path/to/private.pem
 *   adu-extension-sign --verify --extension-dir /path/to/extension --cert /path/to/cert.pem
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/extension_signing.h"

#include <parson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char* progName)
{
    fprintf(stderr,
        "Usage:\n"
        "  %s --extension-dir DIR --key PRIVATE_KEY_PEM\n"
        "      Sign the manifest.json in DIR using the private key.\n"
        "\n"
        "  %s --verify --extension-dir DIR --cert CERT_PEM\n"
        "      Verify the manifest.json in DIR using the trusted certificate.\n",
        progName, progName);
}

/**
 * @brief Write the signed manifest back to manifest.json.
 */
static int write_manifest(const char* manifestPath, const ADUC_ExtensionManifest* manifest)
{
    JSON_Value* rootValue = json_value_init_object();
    JSON_Object* root = json_value_get_object(rootValue);
    if (root == NULL)
    {
        return -1;
    }

    json_object_set_string(root, "name", manifest->name);
    json_object_set_string(root, "version", manifest->version);
    json_object_set_string(root, "type", manifest->type);
    json_object_set_string(root, "library", manifest->library);
    json_object_set_string(root, "sha256", manifest->sha256);
    json_object_set_string(root, "minAgentVersion", manifest->minAgentVersion);
    json_object_set_string(root, "author", manifest->author);
    json_object_set_string(root, "signature", manifest->signature);

    JSON_Status status = json_serialize_to_file_pretty(rootValue, manifestPath);
    json_value_free(rootValue);

    return (status == JSONSuccess) ? 0 : -1;
}

/**
 * @brief Compute the SHA-256 of the library and update manifest->sha256.
 */
static ADUC_Result2 update_manifest_hash(ADUC_ExtensionManifest* manifest, const char* extensionDir)
{
    char libraryPath[1024];
    snprintf(libraryPath, sizeof(libraryPath), "%s/%s", extensionDir, manifest->library);

    return ADUC_ExtensionSigning_HashFile(libraryPath, manifest->sha256, sizeof(manifest->sha256));
}

int main(int argc, char* argv[])
{
    const char* extensionDir = NULL;
    const char* keyPath = NULL;
    const char* certPath = NULL;
    int verifyMode = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--extension-dir") == 0 && i + 1 < argc)
        {
            extensionDir = argv[++i];
        }
        else if (strcmp(argv[i], "--key") == 0 && i + 1 < argc)
        {
            keyPath = argv[++i];
        }
        else if (strcmp(argv[i], "--cert") == 0 && i + 1 < argc)
        {
            certPath = argv[++i];
        }
        else if (strcmp(argv[i], "--verify") == 0)
        {
            verifyMode = 1;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        else
        {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (extensionDir == NULL)
    {
        fprintf(stderr, "Error: --extension-dir is required\n");
        print_usage(argv[0]);
        return 1;
    }

    if (verifyMode)
    {
        // Verification mode
        if (certPath == NULL)
        {
            fprintf(stderr, "Error: --cert is required for verification\n");
            return 1;
        }

        ADUC_Result2 result = ADUC_Extension_Verify(extensionDir, certPath, ADUC_SIGN_POLICY_ENFORCE);
        if (ADUC_RESULT2_IS_SUCCESS(result))
        {
            printf("Verification PASSED for %s\n", extensionDir);
            return 0;
        }
        else
        {
            fprintf(stderr, "Verification FAILED for %s (error: 0x%08x)\n", extensionDir, result.code);
            return 1;
        }
    }
    else
    {
        // Signing mode
        if (keyPath == NULL)
        {
            fprintf(stderr, "Error: --key is required for signing\n");
            return 1;
        }

        char manifestPath[1024];
        snprintf(manifestPath, sizeof(manifestPath), "%s/manifest.json", extensionDir);

        // Parse existing manifest
        ADUC_ExtensionManifest manifest;
        ADUC_Result2 result = ADUC_ExtensionManifest_Parse(manifestPath, &manifest);
        if (ADUC_RESULT2_IS_FAILURE(result))
        {
            fprintf(stderr, "Failed to parse manifest at %s (error: 0x%08x)\n", manifestPath, result.code);
            return 1;
        }

        // Recompute SHA-256 of the library
        result = update_manifest_hash(&manifest, extensionDir);
        if (ADUC_RESULT2_IS_FAILURE(result))
        {
            fprintf(stderr, "Failed to hash library %s (error: 0x%08x)\n", manifest.library, result.code);
            return 1;
        }

        // Sign the manifest
        result = ADUC_ExtensionManifest_Sign(&manifest, keyPath);
        if (ADUC_RESULT2_IS_FAILURE(result))
        {
            fprintf(stderr, "Failed to sign manifest (error: 0x%08x)\n", result.code);
            return 1;
        }

        // Write back
        if (write_manifest(manifestPath, &manifest) != 0)
        {
            fprintf(stderr, "Failed to write manifest to %s\n", manifestPath);
            return 1;
        }

        printf("Signed manifest written to %s\n", manifestPath);
        printf("  SHA-256: %s\n", manifest.sha256);
        return 0;
    }
}
