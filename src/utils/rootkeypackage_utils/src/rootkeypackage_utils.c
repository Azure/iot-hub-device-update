/**
 * @file rootkeypackage_utils.c
 * @brief rootkeypackage_utils implementation.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/rootkeypackage_utils.h"
#include "aduc/rootkeypackage_parse.h"
#include <aduc/c_utils.h> // for EXTERN_C_BEGIN, EXTERN_C_END
#include <aduc/string_c_utils.h> // for IsNullOrEmpty
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include <parson.h>

EXTERN_C_BEGIN

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

    ADUC_RootKeyPackage pkg;
    memset(&pkg, 0, sizeof(pkg));

    JSON_Value* rootValue = NULL;
    JSON_Object* rootObj = NULL;

    if (IsNullOrEmpty(jsonString) || outRootKeyPackage == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG;
        return result;
    }

    rootValue = json_parse_string(jsonString);
    if (rootValue == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_JSON;
        goto done;
    }

    rootObj = json_object(rootValue);
    if (rootObj == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_JSON;
        goto done;
    }

    result = RootKeyPackage_ParseProtectedProperties(rootObj, &pkg);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = RootKeyPackage_ParseSignatures(rootObj, &pkg);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    *outRootKeyPackage = pkg;
    memset(&pkg, 0, sizeof(pkg));

    result.ResultCode = ADUC_GeneralResult_Success;

done:
    json_value_free(rootValue);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        ADUC_RootKeyPackageUtils_Destroy(&pkg);
    }

    return result;
}

ADUC_Result ADUC_RootKeyPackageUtil_DownloadPackage(char** outPathToRootKeyPackage)
{
    ADUC_Result result = { .ResultCode = ADUC_Result_Failure, .ExtendedResultCode = 0 };
    return result;
}

/**
 * @brief Cleans up the disabled root keys in the rootkey package.
 *
 * @param rootKeyPackage The root key package.
 */
void ADUC_RootKeyPackageUtils_DisabledRootKeys_Destroy(ADUC_RootKeyPackage* rootKeyPackage)
{
    if (rootKeyPackage == NULL)
    {
        return;
    }

    VECTOR_HANDLE vec = rootKeyPackage->protectedProperties.disabledRootKeys;
    if (vec != NULL)
    {
        size_t cnt = VECTOR_size(vec);
        for (size_t i = 0; i < cnt; ++i)
        {
            STRING_HANDLE* h = (STRING_HANDLE*)VECTOR_element(vec, i);
            if (h != NULL)
            {
                STRING_delete(*h);
            }
        }
        VECTOR_destroy(rootKeyPackage->protectedProperties.disabledRootKeys);
        rootKeyPackage->protectedProperties.disabledRootKeys = NULL;
    }
}

/**
 * @brief Cleans up the disabled signing keys in the rootkey package.
 *
 * @param rootKeyPackage The root key package.
 */
void ADUC_RootKeyPackageUtils_DisabledSigningKeys_Destroy(ADUC_RootKeyPackage* rootKeyPackage)
{
    if (rootKeyPackage == NULL)
    {
        return;
    }

    VECTOR_HANDLE vec = rootKeyPackage->protectedProperties.disabledSigningKeys;
    if (vec != NULL)
    {
        size_t cnt = VECTOR_size(vec);
        for (size_t i = 0; i < cnt; ++i)
        {
            ADUC_RootKeyPackage_Hash* node = (ADUC_RootKeyPackage_Hash*)VECTOR_element(vec, i);
            ADUC_RootKeyPackage_Hash_DeInit(node);
        }

        VECTOR_destroy(rootKeyPackage->protectedProperties.disabledSigningKeys);
        rootKeyPackage->protectedProperties.disabledSigningKeys = NULL;
    }
}

/**
 * @brief Cleans up the root keys in the rootkey package.
 *
 * @param rootKeyPackage The root key package.
 */
void ADUC_RootKeyPackageUtils_RootKeys_Destroy(ADUC_RootKeyPackage* rootKeyPackage)
{
    if (rootKeyPackage == NULL)
    {
        return;
    }

    VECTOR_HANDLE vec = rootKeyPackage->protectedProperties.rootKeys;
    if (vec != NULL)
    {
        size_t cnt = VECTOR_size(vec);
        for (size_t i = 0; i < cnt; ++i)
        {
            ADUC_RootKey* rootKeyEntry = (ADUC_RootKey*)VECTOR_element(vec, i);
            ADUC_RootKey_DeInit(rootKeyEntry);
        }
        VECTOR_destroy(rootKeyPackage->protectedProperties.rootKeys);
        rootKeyPackage->protectedProperties.rootKeys = NULL;
    }
}

/**
 * @brief Cleans up the signatures in the rootkey package.
 *
 * @param rootKeyPackage The root key package.
 */
void ADUC_RootKeyPackageUtils_Signatures_Destroy(ADUC_RootKeyPackage* rootKeyPackage)
{
    if (rootKeyPackage == NULL)
    {
        return;
    }

    VECTOR_HANDLE vec = rootKeyPackage->signatures;
    if (vec != NULL)
    {
        size_t cnt = VECTOR_size(vec);
        for (size_t i = 0; i < cnt; ++i)
        {
            ADUC_RootKeyPackage_Signature* node = (ADUC_RootKeyPackage_Signature*)VECTOR_element(vec, i);
            ADUC_RootKeyPackage_Signature_DeInit(node);
        }

        VECTOR_destroy(rootKeyPackage->signatures);
        rootKeyPackage->signatures = NULL;
    }
}

/**
 * @brief Cleans up an ADUC_RootKeyPackage object.
 *
 * @param rootKeyPackage The root key package object to cleanup.
 */
void ADUC_RootKeyPackageUtils_Destroy(ADUC_RootKeyPackage* rootKeyPackage)
{
    if (rootKeyPackage == NULL)
    {
        return;
    }

    ADUC_RootKeyPackageUtils_DisabledRootKeys_Destroy(rootKeyPackage);
    ADUC_RootKeyPackageUtils_DisabledSigningKeys_Destroy(rootKeyPackage);
    ADUC_RootKeyPackageUtils_RootKeys_Destroy(rootKeyPackage);

    ADUC_RootKeyPackageUtils_Signatures_Destroy(rootKeyPackage);

    memset(rootKeyPackage, 0, sizeof(*rootKeyPackage));
}

EXTERN_C_END
