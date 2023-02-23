/**
 * @file content_protection_utils.c
 * @brief Provides an implementation of the utilities for interacting with the content protection blocks
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <azure_c_shared_utility/strings.h>
#include "content_protection_utils.h"
#include "content_protection_field_defs.h"
#include <parson.h>
#include <stdbool.h>
#include <strings.h>

bool ContentProtectionUtils_DecryptDek(const STRING_HANDLE dek, const STRING_HANDLE dekCryptAlg);

DecryptionAlg ContentProtectionUtils_GetDecryptAlgFromDecryptionInfo(const JSON_Object* decryptInfo)
{
    DecryptionAlg alg = UNSUPPORTED_DECRYPTION_ALG;

    const char* alg = json_object_get_string(decryptInfo,DECRYPT_INFO_ALG_FIELD);

    const char* mode = json_object_get_string(decryptInfo,DECRYPT_INFO_MODE_FIELD);

    const char* keyLen = json_object_get_string(decryptInfo,DECRYPT_INFO_KEY_LENGTH_FIELD);

    if (strcasecmp(alg,"aes") == 0)
    {
        if (strcasecmp(keyLen,"128") == 0)
        {
            if (strcasecmp(mode, "cbc") == 0 )
            {
                return AES_128_CBC;
            }
        }else if (strcasecmp(keyLen,"192") == 0)
        {
            if (strcasecmp(mode, "cbc") == 0 )
            {
                return AES_192_CBC;
            }
        }else if (strcasecmp(keyLen,"256") == 0)
        {
            if (strcasecmp(mode, "cbc") == 0 )
            {

                return AES_256_CBC;
            }
        }
    }
}
