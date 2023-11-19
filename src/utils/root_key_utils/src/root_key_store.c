/**
 * @file root_key_utility.c
 * @brief Implements a static list of root keys used to verify keys and messages within crypto_lib and it's callers.
 * these have exponents and parameters that have been extracted from their certs
 * to ease the computation process but remain the same as the ones issued by the Authority
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "root_key_store.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

//
// Surfaces the hardcoded package path for the root key package
//

#ifndef ADUC_ROOTKEY_STORE_PACKAGE_PATH
#    error "ADUC_ROOTKEY_STORE_PACKAGE_PATH must be defined"
#endif

const char* RootKeyStore_GetRootKeyStorePath()
{
    return ADUC_ROOTKEY_STORE_PACKAGE_PATH;
}
