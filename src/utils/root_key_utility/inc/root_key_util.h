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

/**
 * Below is just for development and should be implemented by JeffW's PR that preceed it
 */

/**
 * @brief The root key SHA algorithms.
 */
typedef enum tagADUC_RootKeyShaAlgorithm
{
    SHA256,
    SHA384,
    SHA512,
} ADUC_RootKeyShaAlgorithm;

typedef struct tagADUC_RootKeyPackage_Hash
{
    ADUC_RootKeyShaAlgorithm alg;
    CONSTBUFFER_HANDLE hash;
} ADUC_RootKeyPackage_Hash;

typedef struct tagADUC_RSA_RootKeyParameters
{
    CONSTBUFFER_HANDLE n;
    CONSTBUFFER_HANDLE e;
} ADUC_RSA_RootKeyParameters;

/**
 * @brief The root key.
 */
typedef struct tagADUC_RootKey
{
    STRING_HANDLE kid; /**< The key id. */
    ADUC_RootKey_KeyType keyType; /**< The key type. */
    ADUC_RSA_RootKeyParameters rsaParameters; /**< The RSA key parameters */
} ADUC_RootKey;

/**
 * @brief The protected properties of the root key package.
 */
typedef struct tagADUC_RootKeyPackage_ProtectedProperties
{
    VECTOR_HANDLE disabledRootKeys; /**< handle to vector of STRING_HANDLE KIDS(KeyIds) of disabled root keys. */
    VECTOR_HANDLE disabledSigningKeys;/**< handle to vector of ADUC_RootKeyPackage_Hash hashes of public key of disabled signing keys. */
    VECTOR_HANDLE rootKeys; /**< handle to vector of ADUC_RootKey root keys. */
    unsigned long version; /**< The monotonic increasing version of the package. */
    struct timespec publishedTime; /**< The struct timespec published unix time of the root key. */
} ADUC_RootKeyPackage_ProtectedProperties;

/**
 * @brief The root key package.
 */
typedef struct tagADUC_RootKeyPackage
{
    STRING_HANDLE protectedPropertiesJsonString; /**< The serialized json string for which to verify the signatures. */
    VECTOR_HANDLE signatures; /**< handle to vector of ADUC_RootKeyPackage_Hash signatures used to verify the propertedProperties using the provenance public root keys. */
    ADUC_RootKeyPackage_ProtectedProperties protectedProperties; /**< The parsed protected properties. */
} ADUC_RootKeyPackage;



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
