/**
 * @file root_key_store.h
 * @brief Defines the function for returning the path to the local store of the root key package
 * @details Functions are mockable for testing purposes
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/c_utils.h"
#include <umock_c/umock_c_prod.h>
#ifndef ROOT_KEY_STORE_H
#    define ROOT_KEY_STORE_H

EXTERN_C_BEGIN

MOCKABLE_FUNCTION(, const char*, RootKeyStore_GetRootKeyStorePath, );

EXTERN_C_END
#endif // ROOT_KEY_STORE_H
