/**
 * @file rootkeypackage_utils.c
 * @brief rootkeypackage_utils implementation.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/rootkeypackage_utils.h"
#include <aduc/c_utils.h> // for EXTERN_C_BEGIN, EXTERN_C_END
#include <parson.h>

EXTERN_C_BEGIN

//static ParseProtectedProperties()

/**
 * @brief Parses JSON string into an ADUC_RootKeyPackage struct.
 *
 * @param jsonString The root key package JSON string to parse.
 * @param outRootKeyPackage parameter for the resultant ADUC_RootKeyPackage.
 *
 * @return ADUC_Result The result of parsing.
 * @details Caller must call ADUC_RootKeyPackageUtils_Cleanup() on the resultant ADUC_RootKeyPackage.
 */
ADUC_Result ADUC_RootKeyPackageUtils_Parse(const char* jsonString, ADUC_RootKeyPackage* outRootKeyPackage)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    JSON_Value* rootValue = NULL;

    rootValue = json_parse_string(jsonString);
    if (rootValue == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_JSON;
        goto done;
    }

done:

    return result;
}

/**
 * @brief Cleans up an ADUC_RootKeyPackage object.
 *
 * @param rootKeyPackage The root key package object to cleanup.
 */
void ADUC_RootKeyPackageUtils_Cleanup(ADUC_RootKeyPackage* rootKeyPackage)
{
}

EXTERN_C_END
