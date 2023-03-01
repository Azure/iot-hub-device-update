/**
 * @file content_protection_utils.h
 * @brief Prototypes of utilities for content protections.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef CONTENT_PROTECTION_UTILS_H
#define CONTENT_PROTECTION_UTILS_H

#include <aduc/c_utils.h>
#include <aduc/result.h>
#include <decryption_info.h>

EXTERN_C_BEGIN

ADUC_Result
ContentProtectionUtils_DecryptFile(ADUC_Decryption_Info* decryptionInfo, const char* downloadedSandboxFilePath);

EXTERN_C_END

#endif // CONTENT_PROTECTION_UTILS_H
