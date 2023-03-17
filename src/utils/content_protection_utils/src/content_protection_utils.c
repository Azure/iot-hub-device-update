/**
 * @file content_protection_utils.c
 * @brief Provides an implementation of the utilities for interacting with the content protection blocks
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "content_protection_utils.h"
#include "content_protection_field_defs.h"
#include <azure_c_shared_utility/strings.h>
#include <parson.h>
#include <stdbool.h>
#include <strings.h>

//
// Stub function for workflow implementation for decrypting the DEK
//
bool ContentProtectionUtils_DecryptDek(const STRING_HANDLE dek, const STRING_HANDLE dekCryptAlg)
{
    UNREFERENCED_PARAMETER(dek);
    UNREFERENCED_PARAMETER(dekCryptAlg);

    return false;
}

/**
 * @brief Consumes the JSON that describes the algorithm and data that the content was encrypted with so that it can be decrypted
 *
 * @param decryptInfo JSON section that describes the encryption/decryption scheme of the content in the dpeloyment
 * @return A DecryptionAlg type
 */
DecryptionAlg ContentProtectionUtils_GetDecryptAlgFromDecryptionInfo(const JSON_Object* decryptInfo)
{
    DecryptionAlg alg = UNSUPPORTED_DECRYPTION_ALG;

    const char* alg = json_object_get_string(decryptInfo, DECRYPT_INFO_ALG_FIELD);

    const char* mode = json_object_get_string(decryptInfo, DECRYPT_INFO_MODE_FIELD);

    const char* keyLen = json_object_get_string(decryptInfo, DECRYPT_INFO_KEY_LEN_FIELD);

    if (strcasecmp(alg, "aes") == 0)
    {
        if (strcasecmp(keyLen, "128") == 0)
        {
            if (strcasecmp(mode, "cbc") == 0)
            {
                return AES_128_CBC;
            }
        }
        else if (strcasecmp(keyLen, "192") == 0)
        {
            if (strcasecmp(mode, "cbc") == 0)
            {
                return AES_192_CBC;
            }
        }
        else if (strcasecmp(keyLen, "256") == 0)
        {
            if (strcasecmp(mode, "cbc") == 0)
            {
                return AES_256_CBC;
            }
        }
    }

    return UNSUPPORTED_DECRYPTION_ALG;
}
