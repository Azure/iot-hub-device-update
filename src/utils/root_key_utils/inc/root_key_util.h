/**
 * @file root_key_util.h
 * @brief Defines the functions for getting, validating, and dealing with encoded and locally stored root keys
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <azure_c_shared_utility/constbuffer.h>
#include <azure_c_shared_utiltiy/strings.h>
#include <azure_c_shared_utility/vector.h>

#include "crypto_key.h"
#include "root_key_result.h"

#ifndef ROOT_KEY_UTIL_H
#    define ROOT_KEY_UTIL_H

RootKeyUtility_ValidationResult RootKeyUtil_ValidateRootKeyPackage(const STRING_HANDLE rootKeyPackagePath );

RootKeyUtility_InstallResult RootKeyUtil_StoreNewRootKeyPackage(const STRING_HANDLE srcRootKeyPackagePath );

RootKeyUtility_LoadResult RootKeyUtil_LoadLocalRootKeyPackage(const STRING_HANDLE rootKeyPackagePath);


/**
 * @brief Creates and cleans the workspace pointed to by @p root_key_workspace_path
 * @details
 *
 **/
RootKeyUtility_Result RootKeyUtility_CreateAndCleanLocalRootKeyArchiveWorkspace(const char* root_key_workspace_path);

/**
 * @brief Cleans and destroys the workspace directory, Performs any other local cleaning operations
 * @param[in] path_to_rootkey_workspace the path to the root key workspace to be cleaned
*/
void RootKeyUtility_CleanUpWorkspace(const char* path_to_rootkey_workspace);

/**
 * @brief Get the Key For Kid object
 *
 * @param kid
 * @return CryptoKeyHandle
 */
CryptoKeyHandle RootKeyUtility_GerKeyForKid(const char* kid);

#endif // ROOT_KEY_UTIL_H
