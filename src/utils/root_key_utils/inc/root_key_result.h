/**
 * @file root_key_result.h
 * @brief Defines results for the RootKeyUtility
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ROOT_KEY_RESULT_H
#define ROOT_KEY_RESULT_H

typedef enum _RootKeyUtility_ValidationResult
{
    RootKeyUtility_ValidationResult_Success = 0,
    /**
     * Specialized results for validation should go here
     *
     */
    RootKeyUtility_ValidationResult_Failure = 1,
    RootKeyUtility_ValidationResult_HardcodedRootKeyLoadFailed = 2,
    RootKeyUtility_ValidationResult_SignatureForKeyNotFound = 3,
    RootKeyUtility_ValidationResult_KeyIdNotFound = 4,
    RootKeyUtility_ValidationResult_SignatureValidationFailed = 5,
} RootKeyUtility_ValidationResult;

#endif // ROOT_KEY_RESULT_H
