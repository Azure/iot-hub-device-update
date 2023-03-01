/**
 * @file decryption_info.h
 * @brief Decryption Info struct and related prototypes as part of utilities for content protections.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef DECRYPTION_INFO_H
#define DECRYPTION_INFO_H

#include "azure_c_shared_utility/strings.h"
#include <aduc/c_utils.h>
#include <aduc/result.h>
#include <parson.h>

EXTERN_C_BEGIN
typedef struct tagADUC_Decryption_Info
{
    STRING_HANDLE decryptedDEK; /**< The decrypted DEK octets. */
    char* alg; /**< The symmetric algorithm used to encrypt the update content, e.g. aes */
    char* mode; /*< The mode of operation used, e.g. cbc for chaining, or gcm for galois counter mode. */
    char* keyLen; /*< The key length, e.g. for aes it can be 128, 192, or 256. */
    char* props; /*< Additional properties as a json string. Typically will be NULL. */
} ADUC_Decryption_Info;

/**
 * @brief Initializes the decryption info from the pnp message.
 *
 * @param[in] pnpMsg The pnp message json object.
 * @param[out] outDecryptionInfo The pointer to ADUC_Decryption_Info struct with fields to initialize.
 * @return ADUC_Result The result.
 */
ADUC_Result DecryptionInfo_Init(JSON_Object* pnpMsg, ADUC_Decryption_Info* outDecryptionInfo);

/**
 * @brief Uninitialize decryption info.
 * @details It will securely zero-out decrypted field.
 *
 * @param decryptionInfo The ADUC_Decryption_Info struct to uninitialize.
 * @return ADUC_Result The result.
 */
void DecryptionInfo_Uninit(ADUC_Decryption_Info* decryptionInfo);

EXTERN_C_END

#endif // DECRYPTION_INFO_H
