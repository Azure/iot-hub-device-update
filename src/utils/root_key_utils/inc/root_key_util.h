/**
 * @file root_key_util.h
 * @brief Defines the functions for getting, validating, and dealing with encoded and locally stored root keys
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ROOT_KEY_UTIL_H
#    define ROOT_KEY_UTIL_H

#include "aduc/c_utils.h"
#include "aduc/result.h"
#include "aduc/rootkeypackage_types.h"
#include "rootkey_store_types.h"
#include "crypto_lib.h"

typedef struct tagRootKeyUtilContext
{
    RootKeyStoreHandle rootKeyStoreHandle;
    ADUC_Result_t rootKeyExtendedResult;
} RootKeyUtilContext;

EXTERN_C_BEGIN

RootKeyUtilContext* RootKeyUtility_InitContext();
void RootKeyUtility_UninitContext(RootKeyUtilContext* context);

ADUC_Result RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(RootKeyUtilContext* rootKeyUtilContext, const ADUC_RootKeyPackage* rootKeyPackage);

ADUC_Result RootKeyUtility_SaveRootKeyPackageToStore(const RootKeyUtilContext* rootKeyUtilContext, const ADUC_RootKeyPackage* rootKeyPackage);

ADUC_Result RootKeyUtility_GetKeyForKid(const RootKeyUtilContext* context, CryptoKeyHandle* key, const char* kid);

void RootKeyUtility_SetReportingErc(RootKeyUtilContext* context, ADUC_Result_t erc);
void RootKeyUtility_ClearReportingErc(RootKeyUtilContext* context);
ADUC_Result_t RootKeyUtility_GetReportingErc(const RootKeyUtilContext* context);
ADUC_Result RootKeyUtility_GetDisabledSigningKeys(const RootKeyUtilContext* context, VECTOR_HANDLE* outDisabledSigningKeyList);

EXTERN_C_END

#endif // ROOT_KEY_UTIL_H
