
/**
 * @file content_protection_utils.h
 * @brief Utilities for content protections.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "content_protection_utils.h"

ADUC_Result
ContentProtectionUtils_DecryptFile(ADUC_Decryption_Info* decryptionInfo, const char* downloadedSandboxFilePath)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure, 0 };

    UNREFERENCED_PARAMETER(decryptionInfo);
    UNREFERENCED_PARAMETER(downloadedSandboxFilePath);

    // Currently, a no-op as service does not implement encrypted payloads yet.
    // TODO: Call decryption utils and replace encrypted file with decrypted one.
    result.ResultCode = ADUC_GeneralResult_Success;

    return result;
}
