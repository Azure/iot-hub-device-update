/**
 * @file root_key_result.h
 * @brief Defines results for the RootKeyUtility
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */


typedef enum _RootKeyUtility_InstallResult
{
    RootKeyUtility_InstallResult_Success = 0,
    RootKeyUtility_InstallResult_Failed = 1,
}RootKeyUtility_InstallResult;


typedef enum _RootKeyUtility_ValidationResult
{
    RootKeyUtility_ValidationResult_Success = 0,
    /**
     * Specialized results for validation should go here
     *
     */
    RootKeyUtility_ValidationResult_Failure = 1,
    RootKeyUtility_ValidationResult_KeyIdNotFound = 2,
} RootKeyUtility_ValidationResult;
