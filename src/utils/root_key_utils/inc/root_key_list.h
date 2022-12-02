/**
 * @file root_key_list.h
 * @brief Defines functions for getting the hardcoded root keys
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <umock_c/umock_c_prod.h>
#include "aduc/c_utils.h"

#ifndef ROOT_KEY_LIST_H
#define ROOT_KEY_LIST_H

EXTERN_C_BEGIN

typedef struct tagRSARootKey
{
    const char* kid;
    const char* N;
    const unsigned int e;
} RSARootKey;

MOCKABLE_FUNCTION(, const RSARootKey*,RootKeyList_GetHardcodedRsaRootKeys,)

EXTERN_C_END

#endif // ROOT_KEY_LIST_H
