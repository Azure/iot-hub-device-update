/**
 * @file rootkeypackage_types.h
 * @brief rootkeypackage types.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ROOTKEYPACKAGE_TYPES_H
#define ROOTKEYPACKAGE_TYPES_H

#include <aduc/hash_utils.h>
#include <aducpal/time.h> // for struct timespec
#include <azure_c_shared_utility/constbuffer.h>
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>

/**
 * @brief The root key keyType.
 */
typedef enum tagADUC_RootKey_KeyType
{
    ADUC_RootKey_KeyType_INVALID = 0,
    ADUC_RootKey_KeyType_RSA, /**< RSA keyType. */
} ADUC_RootKey_KeyType;

/**
 * @brief The root key Signature algorithms.
 */
typedef enum tagADUC_RootKeySigningAlgorithm
{
    ADUC_RootKeySigningAlgorithm_INVALID = 0,
    ADUC_RootKeySigningAlgorithm_RS256, /**< The RS256 algorithm. */
    ADUC_RootKeySigningAlgorithm_RS384, /**< The RS384 algorithm. */
    ADUC_RootKeySigningAlgorithm_RS512, /**< The RS512 algorithm. */
} ADUC_RootKeySigningAlgorithm;

/**
 * @brief The Root key package hash.
 */
typedef struct tagADUC_RootKeyPackage_Hash
{
    SHAversion alg; /**< The hash algorithm. */
    CONSTBUFFER_HANDLE hash; /**< The hash const buffer handle. */
} ADUC_RootKeyPackage_Hash;

/**
 * @brief The Root key package signature.
 */
typedef struct tagADUC_RootKeyPackage_Signature
{
    ADUC_RootKeySigningAlgorithm alg; /**< The signing algorithm. */
    CONSTBUFFER_HANDLE signature; /**< The signature const buffer handle. */
} ADUC_RootKeyPackage_Signature;

/**
 * @brief The RSA root key parameters.
 */
typedef struct tagADUC_RSA_RootKeyParameters
{
    CONSTBUFFER_HANDLE n; /**< The RSA modulus parameter. */
    unsigned int e; /**< The RSA exponent parameter. */
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
    bool isTest; /**< Whether the rootkey package is a test package. */
    unsigned long version; /**< The monotonic increasing version of the package. */
    time_t publishedTime; /**< The unix time of the root key. */
    VECTOR_HANDLE disabledRootKeys; /**< handle to vector of STRING_HANDLE KIDS(KeyIds) of disabled root keys. */
    VECTOR_HANDLE
    disabledSigningKeys; /**< handle to vector of ADUC_RootKeyPackage_Hash hashes of public key of disabled signing keys. */
    VECTOR_HANDLE rootKeys; /**< handle to vector of ADUC_RootKey root keys. */
} ADUC_RootKeyPackage_ProtectedProperties;

/**
 * @brief The root key package.
 */
typedef struct tagADUC_RootKeyPackage
{
    ADUC_RootKeyPackage_ProtectedProperties protectedProperties; /**< The parsed protected properties. */
    STRING_HANDLE protectedPropertiesJsonString; /**< The serialized json string for which to verify the signatures. */
    VECTOR_HANDLE
    signatures; /**< handle to vector of ADUC_RootKeyPackage_Signature signatures used to verify the protectedProperties using the provenance public root keys. */
} ADUC_RootKeyPackage;

#endif //ROOTKEYPACKAGE_TYPES_H
