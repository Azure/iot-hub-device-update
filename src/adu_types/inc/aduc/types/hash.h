/**
 * @file hash.h
 * @brief Defines types related to hashing functionality.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_TYPES_HASH_H
#define ADUC_TYPES_HASH_H

#include <stdbool.h>    // for bool
#include <stddef.h>     // for size_T

#include "aduc/c_utils.h"


EXTERN_C_BEGIN

/**
 * @brief Encapsulates the hash and the hash type
 */
typedef struct tagADUC_Hash
{
    char* value; /** The value of the actual hash */
    char* type; /** The type of hash held in the entry*/
} ADUC_Hash;

EXTERN_C_END

#endif // ADUC_TYPES_HASH_H
