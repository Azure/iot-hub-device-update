/**
 * @file hash_algorithm.h
 * @brief Defines the supported SHA algorithm identifiers used throughout the
 *        Device Update agent.
 *
 * This header intentionally does NOT depend on @c azure_c_shared_utility/sha.h.
 * The reference SHA implementation bundled with @c azure_c_shared_utility is
 * not on the SDL Approved Cryptographic Libraries list, so production code
 * must not link against it. The agent uses OpenSSL EVP (an SDL-approved,
 * platform-provided library) for all hashing operations. See
 * @c docs/security/cryptography.md for the policy and rationale.
 *
 * The enumeration tag and value names below are kept identical to the
 * historical @c azure_c_shared_utility/sha.h definition so existing call
 * sites compile unchanged.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_HASH_ALGORITHM_H
#define ADUC_HASH_ALGORITHM_H

#include "aduc/c_utils.h"

EXTERN_C_BEGIN

/**
 * @brief Identifiers for SHA-family digest algorithms supported by the agent.
 *
 * Numeric values are stable and match the legacy enumeration that callers
 * historically obtained from @c azure_c_shared_utility/sha.h, so this is a
 * drop-in replacement.
 *
 * @note Not every value is valid for production payload integrity checks.
 *       Use @c ADUC_HashUtils_IsValidHashAlgorithm to gate caller-supplied
 *       algorithms (SHA-1 and SHA-224 are rejected as too weak).
 */
typedef enum SHAversion
{
    SHA1,
    SHA224,
    SHA256,
    SHA384,
    SHA512
} SHAversion;

EXTERN_C_END

#endif // ADUC_HASH_ALGORITHM_H
