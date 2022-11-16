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

/**
 * @brief The root key SHA algorithms.
 */
typedef enum tagADUC_RootKeyShaAlgorithm
{
    SHA256,
    SHA384,
    SHA512,
} ADUC_RootKeyShaAlgorithm;

/**
 * @brief The root key SHA elliptic curve algorithms.
 */
typedef enum tagADUC_EllipticCurve
{
    P_256,
    P_256K,
    P_384,
    P_512,
} ADUC_EllipticCurve;

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
 * @brief Elliptic Curve rootkey parameters.
 */
typedef struct tagADUC_EC_RootKeyParameters
{
    ADUC_EllipticCurve crv;
    CONSTBUFFER_HANDLE x;
    CONSTBUFFER_HANDLE y;
} ADUC_EC_RootKeyParameters;

/**
 * @brief The root key.
 */
typedef struct tagADUC_RootKey
{
    STRING_HANDLE kid; /**< The key id. */
    ADUC_RootKey_KeyType keyType; /**< The key type. */
    union
    {
        ADUC_RSA_RootKeyParameters rsaParameters; /**< The RSA key parameters */
        ADUC_EC_RootKeyParameters ecParameters; /**< The EC key parameters */
    }
} ADUC_RootKey;

/**
 * @brief The protected properties of the root key package.
 */
typedef struct tagADUC_RootKeyPackage_ProtectedProperties
{
    VECTOR_HANDLE untrustedRootKeys; /**< handle to vector of STRING_HANDLE KIDS(KeyIds) of untrusted root keys. */
    VECTOR_HANDLE untrustedSigningKeys;/**< handle to vector of ADUC_RootKeyPackage_Hash hashes of public key of untrusted signing keys. */
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

#endif //ROOTKEYPACKAGE_TYPES_H
