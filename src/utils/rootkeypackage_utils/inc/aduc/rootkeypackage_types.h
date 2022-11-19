/**
 * @file rootkeypackage_types.h
 * @brief rootkeypackage types.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ROOTKEYPACKAGE_TYPES_H
#define ROOTKEYPACKAGE_TYPES_H

#include <azure_c_shared_utility/constbuffer.h>
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include <sys/time.h> // for struct timespec

/**
 * @brief The root key keytype.
 */
typedef enum tagADUC_RootKey_KeyType
{
    ADUC_RootKey_KeyType_INVALID = 0,
    ADUC_RootKey_KeyType_RSA, /**< RSA keytype. */
} ADUC_RootKey_KeyType;

/**
 * @brief The root key SHA algorithms.
 */
typedef enum tagADUC_RootKeyShaAlgorithm
{
    ADUC_RootKeyShaAlgorithm_INVALID = 0,
    ADUC_RootKeyShaAlgorithm_SHA256, /**< The SHA256 algorithm. */
    ADUC_RootKeyShaAlgorithm_SHA384, /**< The SHA384 algorithm. */
    ADUC_RootKeyShaAlgorithm_SHA512, /**< The SHA512 algorithm. */
} ADUC_RootKeyShaAlgorithm;

/**
 * @brief The Root key package hash.
 */
typedef struct tagADUC_RootKeyPackage_Hash
{
    ADUC_RootKeyShaAlgorithm alg; /**< The hash algorithm. */
    CONSTBUFFER_HANDLE hash; /**< The hash const buffer handle. */
} ADUC_RootKeyPackage_Hash;

/**
 * @brief The RSA root key parameters.
 */
typedef struct tagADUC_RSA_RootKeyParameters
{
    CONSTBUFFER_HANDLE n; /**< The RSA modulus parameter. */
    CONSTBUFFER_HANDLE e; /**< The RSA exponent parameter. */
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
    unsigned long version; /**< The monotonic increasing version of the package. */
    struct timespec publishedTime; /**< The struct timespec published unix time of the root key. */
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
        signatures; /**< handle to vector of ADUC_RootKeyPackage_Hash signatures used to verify the propertedProperties using the provenance public root keys. */
} ADUC_RootKeyPackage;

#endif //ROOTKEYPACKAGE_TYPES_H
