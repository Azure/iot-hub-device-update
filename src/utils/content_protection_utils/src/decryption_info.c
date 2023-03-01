/**
 * @file decryption_info.h
 * @brief Decryption Info struct and related function impls as part of utilities for content protections.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "decryption_info.h"

/**
 * @brief Initializes the decryption info from the pnp message.
 *
 * @param[in] pnpMsg The pnp message json object.
 * @param[out] outDecryptionInfo The pointer to ADUC_Decryption_Info struct with fields to initialize.
 * @return ADUC_Result The result.
 */
ADUC_Result DecryptionInfo_Init(JSON_Object* pnpMsg, ADUC_Decryption_Info* outDecryptionInfo)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure, 0 };
    // TODO
    return result;
}

/**
 * @brief Uninitialize decryption info.
 * @details It will securely zero-out decrypted field.
 *
 * @param decryptionInfo The ADUC_Decryption_Info struct to uninitialize.
 */
void DecryptionInfo_Uninit(ADUC_Decryption_Info* decryptionInfo)
{
    if (decryptionInfo->decryptedDEK != NULL)
    {
        STRING_delete(decryptionInfo->decryptedDEK);
        decryptionInfo->decryptedDEK = NULL;
    }
}
