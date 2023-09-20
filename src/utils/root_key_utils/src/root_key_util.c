/**
 * @file root_key_utility.c
 * @brief Implements a static list of root keys used to verify keys and messages within crypto_lib and it's callers.
 * these have exponents and parameters that have been extracted from their certs
 * to ease the computation process but remain the same as the ones issued by the Authority
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "root_key_util.h"
#include "root_key_util_helper.h"

#include "aduc/rootkeypackage_parse.h"
#include "aduc/rootkeypackage_types.h"
#include "aduc/rootkeypackage_utils.h"
#include "base64_utils.h"
#include "crypto_lib.h"
#include "root_key_list.h"
#include "rootkey_store.h"
#include <aduc/result.h>
#include <aduc/string_c_utils.h>
#include <aduc/system_utils.h>

#ifdef ENABLE_MOCKS
#    undef ENABLE_MOCKS
#    include <azure_c_shared_utility/constbuffer.h>
#    include <azure_c_shared_utility/strings.h>
#    include <azure_c_shared_utility/vector.h>
#    define ENABLE_MOCKS
#else
#    include <azure_c_shared_utility/constbuffer.h>
#    include <azure_c_shared_utility/strings.h>
#    include <azure_c_shared_utility/vector.h>
#endif // ENABLE_MOCKS

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifdef ENABLE_MOCKS
#    include <umock_c/umock_c_prod.h>
#endif

//
// Root Key Validation Helper Functions
//
/**
 * @brief Helper function for making a CryptoKeyHandle from an ADUC_RootKey
 *
 * @param rootKey ADUC_RootKey to make into a crypto key handle
 * @return NULL on failure, a CryptoKeyHandle on success
 */
CryptoKeyHandle MakeCryptoKeyHandleFromADUC_RootKey(const ADUC_RootKey* rootKey)
{
    if (rootKey == NULL)
    {
        return NULL;
    }

    CryptoKeyHandle key = NULL;

    const CONSTBUFFER* modulus;
    switch (rootKey->keyType)
    {
    case ADUC_RootKey_KeyType_RSA:
        modulus = CONSTBUFFER_GetContent(rootKey->rsaParameters.n);

        key = RSAKey_ObjFromModulusBytesExponentInt(modulus->buffer, modulus->size, rootKey->rsaParameters.e);
        break;

    case ADUC_RootKey_KeyType_INVALID:
        break;

    default:
        break;
    }

    return key;
}

/**
 * @brief Makes a CryptoKeyHandle key from a rootkey of type RSARootKey
 *
 * @param rootKey one of the hardcoded RSA Rootkeys
 * @return a pointer to a CryptoKeyHandle on success, null on failure
 */
CryptoKeyHandle MakeCryptoKeyHandleFromRSARootkey(const RSARootKey rootKey)
{
    CryptoKeyHandle key = NULL;
    uint8_t* modulus = NULL;

    const size_t modulusSize = Base64URLDecode(rootKey.N, &modulus);

    if (modulusSize == 0)
    {
        goto done;
    }

    key = RSAKey_ObjFromModulusBytesExponentInt(modulus, modulusSize, rootKey.e);

done:

    free(modulus);

    return key;
}

/**
 * @brief Initializes an aDUC_RootKey with the parameters in an RSARootKey
 * @details Caller must take care to de-init the rootKey using ADUC_RootKey_DeInit()
 * @param rootKey rootKey pointer to initialize with the data
 * @param rsaKey rsaKey to use for intiialization
 * @return True on success; false on failure
 */
static bool InitializeADUC_RootKey_From_RSARootKey(ADUC_RootKey* rootKey, const RSARootKey* rsaKey)
{
    bool success = false;
    STRING_HANDLE kid = NULL;
    uint8_t* modulus = 0;

    if (rootKey == NULL || rsaKey == NULL || rsaKey->N == NULL || IsNullOrEmpty(rsaKey->kid))
    {
        goto done;
    }

    rootKey->keyType = ADUC_RootKey_KeyType_RSA;

    kid = STRING_construct(rsaKey->kid);

    if (kid == NULL)
    {
        goto done;
    }

    rootKey->kid = kid;

    const size_t modulusSize = Base64URLDecode(rsaKey->N, &modulus);

    if (modulusSize == 0)
    {
        goto done;
    }

    rootKey->rsaParameters.n = CONSTBUFFER_Create(modulus, modulusSize);

    if (rootKey->rsaParameters.n == NULL)
    {
        goto done;
    }

    rootKey->rsaParameters.e = rsaKey->e;

    success = true;
done:

    if (!success)
    {
        ADUC_RootKey_DeInit(rootKey);
    }

    return success;
}

/**
 * @brief Initialize RootKeyUtility Context for use with RootKeyUtility API that interact with RootKey store.
 *
 * @param rootkey_store_path The path to the rootkey store, or NULL for default.
 * @return RootKeyUtilContext* The rootkey utility context.
 */
RootKeyUtilContext* RootKeyUtility_InitContext(const char* rootkey_store_path)
{
    bool succeeded = false;
    RootKeyStoreHandle store_handle = NULL;
    RootKeyUtilContext* context = (RootKeyUtilContext*)malloc(sizeof(*context));
    if (context == NULL)
    {
        return NULL;
    }
    memset(context, 0, sizeof(*context));

    if (rootkey_store_path == NULL)
    {
        rootkey_store_path = ADUC_ROOTKEY_STORE_PACKAGE_PATH;
    }

    store_handle = RootKeyStore_CreateInstance();
    if (store_handle == NULL)
    {
        goto done;
    }

    if (!RootKeyStore_SetConfig(store_handle, RootKeyStoreConfig_StorePath, STRING_construct(rootkey_store_path)))
    {
        goto done;
    }

    if (!RootKeyStore_Load(store_handle))
    {
        goto done;
    }

    context->rootKeyStoreHandle = store_handle;
    store_handle = NULL;

done:
    if (!succeeded)
    {
        RootKeyStore_DestroyInstance(store_handle);
        store_handle = NULL;

        RootKeyUtility_UninitContext(context);
        context = NULL;
    }

    return context;
}

/**
 * @brief Uninitializes the rootkey utility context.
 *
 * @param context The context to uninitialize.
 */
void RootKeyUtility_UninitContext(RootKeyUtilContext* context)
{
    if (context != NULL && context->rootKeyStoreHandle != NULL)
    {
        RootKeyStore_DestroyInstance(context->rootKeyStoreHandle);
        memset(context, 0, sizeof(*context));
    }
}

/**
 * @brief Returns the index for the signature associated with the key
 *
 * @param foundIndex index that will be set with the index for the signature
 * @param rootKeyPackage the root key package to search
 * @param seekKid the kid to search for within the rootKeyPackage
 * @return True on success; false on failure
 */
bool RootKeyUtility_GetSignatureForKey(
    size_t* foundIndex, const ADUC_RootKeyPackage* rootKeyPackage, const char* seekKid)
{
    if (foundIndex == NULL || rootKeyPackage == NULL || seekKid == NULL)
    {
        return false;
    }

    size_t numKeys = VECTOR_size(rootKeyPackage->protectedProperties.rootKeys);

    for (int i = 0; i < numKeys; ++i)
    {
        ADUC_RootKey* root_key = (ADUC_RootKey*)VECTOR_element(rootKeyPackage->protectedProperties.rootKeys, i);

        if (root_key == NULL)
        {
            return false;
        }

        if (strcmp(STRING_c_str(root_key->kid), seekKid) == 0)
        {
            *foundIndex = i;
            return true;
        }
    }

    return false;
}

/**
 * @brief Fills a VECTOR_HANDLE with the hardcoded root keys in ADUC_RootKey format
 * @details Caller must free @p aducRootKeyVector using VECTOR_delete()
 * @param aducRootKeyVector vector handle to initialize with the hard coded root keys
 * @return True on sucess; false on failure
 */
bool RootKeyUtility_GetHardcodedKeysAsAducRootKeys(VECTOR_HANDLE* aducRootKeyVector)
{
    bool success = false;

    VECTOR_HANDLE tempHandle = VECTOR_create(sizeof(ADUC_RootKey));

    if (tempHandle == NULL)
    {
        goto done;
    }

    const RSARootKey* rsaKeyList = RootKeyList_GetHardcodedRsaRootKeys();

    const size_t rsaKeyListSize = RootKeyList_numHardcodedKeys();

    for (int i = 0; i < rsaKeyListSize; ++i)
    {
        RSARootKey key = rsaKeyList[i];
        ADUC_RootKey rootKey = {};
        if (!InitializeADUC_RootKey_From_RSARootKey(&rootKey, &key))
        {
            goto done;
        }
        VECTOR_push_back(tempHandle, &rootKey, 1);
    }

    if (VECTOR_size(tempHandle) == 0)
    {
        goto done;
    }

    //
    // Other key types can be added here
    //
    success = true;

done:

    if (!success)
    {
        if (tempHandle != NULL)
        {
            const size_t tempHandleSize = VECTOR_size(tempHandle);
            for (size_t i = 0; i < tempHandleSize; ++i)
            {
                ADUC_RootKey* rootKey = VECTOR_element(tempHandle, i);

                ADUC_RootKey_DeInit(rootKey);
            }
            VECTOR_destroy(tempHandle);
            tempHandle = NULL;
        }
    }

    *aducRootKeyVector = tempHandle;

    return success;
}

/**
 * @brief Validates the @p rootKeyPackage using a hardcoded root key
 * @details Helper function for RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys()
 * @param rootKeyUtilContext The rootkey utility context
 * @param rootKeyPackage The package to be validated
 * @param rootKey The key to use to validate the package
 * @return ADUC_Result The result
 */
static ADUC_Result RootKeyUtility_ValidatePackageWithKey(RootKeyUtilContext* rootKeyUtilContext, const ADUC_RootKeyPackage* rootKeyPackage, const RSARootKey rootKey)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    CryptoKeyHandle rootKeyCryptoKey = NULL;

    if (rootKeyPackage == NULL)
    {
        goto done;
    }

    size_t signatureIndex = 0;
    if (!RootKeyUtility_GetSignatureForKey(&signatureIndex, rootKeyPackage, rootKey.kid))
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNATURE_FOR_KEY_NOT_FOUND;
        goto done;
    }

    // Get the signature
    ADUC_RootKeyPackage_Hash* signature =
        (ADUC_RootKeyPackage_Hash*)VECTOR_element(rootKeyPackage->signatures, signatureIndex);

    if (signature == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_UNEXPECTED;
        goto done;
    }

    ADUC_Result kidResult = RootKeyUtility_GetKeyForKid(rootKeyUtilContext, &rootKeyCryptoKey, rootKey.kid);

    if (IsAducResultCodeFailure(kidResult.ResultCode))
    {
        result = kidResult;
        goto done;
    }

    const char* protectedProperties = STRING_c_str(rootKeyPackage->protectedPropertiesJsonString);
    const size_t protectedPropertiesLength = STRING_length(rootKeyPackage->protectedPropertiesJsonString);

    const CONSTBUFFER* signatureHash = CONSTBUFFER_GetContent(signature->hash);

    if (!CryptoUtils_IsValidSignature(
            CRYPTO_UTILS_SIGNATURE_VALIDATION_ALG_RS256,
            signatureHash->buffer,
            signatureHash->size,
            (const uint8_t*)protectedProperties,
            protectedPropertiesLength,
            rootKeyCryptoKey))
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNATURE_VALIDATION_FAILED;
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;

done:

    if (rootKeyCryptoKey != NULL)
    {
        CryptoUtils_FreeCryptoKeyHandle(rootKeyCryptoKey);
    }

    return result;
}

/**
 * @brief Validates the @p rootKeyPackage using the hard coded root keys from the RootKeyList within the agent binary
 *
 * @param rootKeyPackage root key package containing the signatures to validate with the hardcoded root keys
 * @return a value of ADUC_Result
 */
ADUC_Result RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(RootKeyUtilContext* rootKeyUtilContext, const ADUC_RootKeyPackage* rootKeyPackage)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    VECTOR_HANDLE hardcodedRootKeys = NULL;

    const RSARootKey* hardcodedRsaKeys = RootKeyList_GetHardcodedRsaRootKeys();
    const size_t numHardcodedKeys = RootKeyList_numHardcodedKeys();
    if (hardcodedRsaKeys == NULL || numHardcodedKeys == 0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_HARDCODED_ROOTKEY_LOAD_FAILED;
        goto done;
    }

    for (size_t i = 0; i < numHardcodedKeys; ++i)
    {
        const RSARootKey rootKey = hardcodedRsaKeys[i];

        ADUC_Result validationResult = RootKeyUtility_ValidatePackageWithKey(rootKeyUtilContext, rootKeyPackage, rootKey);

        if (IsAducResultCodeFailure(validationResult.ResultCode))
        {
            result = validationResult;
            goto done;
        }
    }

    result.ResultCode = ADUC_GeneralResult_Success;

done:

    if (hardcodedRootKeys != NULL)
    {
        const size_t hardcodedRootKeysLength = VECTOR_size(hardcodedRootKeys);
        for (size_t i = 0; i < hardcodedRootKeysLength; ++i)
        {
            ADUC_RootKey* rootKey = (ADUC_RootKey*)VECTOR_element(hardcodedRootKeys, i);

            ADUC_RootKey_DeInit(rootKey);
        }
        VECTOR_destroy(hardcodedRootKeys);
    }

    return result;
}


ADUC_Result RootKeyUtility_SaveRootKeyPackageToStore(const RootKeyUtilContext* rootKeyUtilContext, const ADUC_RootKeyPackage* rootKeyPackage)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, ADUC_ERC_INVALIDARG };

    RootKeyStoreHandle store_handle = NULL;

    if (rootKeyPackage == NULL || rootKeyUtilContext == NULL)
    {
        return result;
    }

    store_handle = rootKeyUtilContext->rootKeyStoreHandle;
    if (store_handle == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_STORE_HANDLE_MISSING_IN_CONTEXT;
        return result;
    }

    if (!RootKeyStore_SetRootKeyPackage(store_handle, rootKeyPackage))
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_FAILED_SERIALIZE_TO_STRING;
        return result;
    }

    return RootKeyStore_Persist(store_handle);
}

/**
 * @brief Helper function that returns a CryptoKeyHandle associated with the kid
 * @details The caller must free the returned Key with the CryptoUtils_FreeCryptoKeyHandle() function
 * @param context The root key util context.
 * @param key the handle to allocate the CryptoKeyHandle for @p kid
 * @param kid the key identifier associated with the key
 * @returns a value of ADUC_Result
 */
ADUC_Result RootKeyUtility_GetKeyForKid(const RootKeyUtilContext* context, CryptoKeyHandle* key, const char* kid)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_NO_ROOTKEY_FOUND_FOR_KEYID };

    CryptoKeyHandle tempKey = NULL;

    ADUC_RootKeyPackage* store_package = NULL;
    if (!RootKeyStore_GetRootKeyPackage(context->rootKeyStoreHandle, &store_package))
    {
        goto done;
    }

    if (RootKeyUtility_RootKeyIsDisabled(store_package, kid))
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNING_ROOTKEY_IS_DISABLED;
        goto done;
    }

    const RSARootKey* hardcodedRsaRootKeys = RootKeyList_GetHardcodedRsaRootKeys();

    const unsigned numberKeys = RootKeyList_numHardcodedKeys();
    for (unsigned i = 0; i < numberKeys; ++i)
    {
        if (strcmp(hardcodedRsaRootKeys[i].kid, kid) == 0)
        {
            tempKey = MakeCryptoKeyHandleFromRSARootkey(hardcodedRsaRootKeys[i]);
            break;
        }
    }

    if (tempKey == NULL)
    {
        for (size_t i = 0; i < VECTOR_size(store_package->protectedProperties.rootKeys); ++i)
        {
            ADUC_RootKey* rootKey = VECTOR_element(store_package->protectedProperties.rootKeys, i);

            if (strcmp(STRING_c_str(rootKey->kid), kid) == 0)
            {
                if (RootKeyUtility_RootKeyIsDisabled(store_package, kid))
                {
                    tempKey = NULL;
                    result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEY_KID_FOUND_BUT_DISABLED;
                }
                else
                {
                    tempKey = MakeCryptoKeyHandleFromADUC_RootKey(rootKey);
                }

                break;
            }
        }
    }

    if (tempKey != NULL)
    {
        result.ResultCode = ADUC_GeneralResult_Success;
        result.ExtendedResultCode = 0;
        *key = tempKey;
        tempKey = NULL;
    }

done:
    ADUC_RootKeyPackageUtils_Destroy(store_package);

    return result;
}

void RootKeyUtility_SetReportingErc(RootKeyUtilContext* context, ADUC_Result_t erc)
{
    if (context != NULL)
    {
        context->rootKeyExtendedResult = erc;
    }
}

void RootKeyUtility_ClearReportingErc(RootKeyUtilContext* context)
{
    if (context != NULL)
    {
        context->rootKeyExtendedResult = 0;
    }
}

ADUC_Result_t RootKeyUtility_GetReportingErc(const RootKeyUtilContext* context)
{
    return context != NULL
        ? context->rootKeyExtendedResult
        : 0;
}

ADUC_Result RootKeyUtility_GetDisabledSigningKeys(const RootKeyUtilContext* context, VECTOR_HANDLE* outDisabledSigningKeyList)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    VECTOR_HANDLE disabledSigningKeyList = NULL;
    ADUC_RootKeyPackage* store_package = NULL;

    if (context == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_INVALIDARG;
        return result;
    }

    disabledSigningKeyList = VECTOR_create(sizeof(ADUC_RootKeyPackage_Signature));
    if (disabledSigningKeyList == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    if (!RootKeyStore_GetRootKeyPackage(context->rootKeyStoreHandle, &store_package))
    {
        goto done;
    }

    for (size_t i = 0; i < VECTOR_size(store_package->protectedProperties.disabledSigningKeys); ++i)
    {
        if (VECTOR_push_back(
                disabledSigningKeyList, VECTOR_element(store_package->protectedProperties.disabledSigningKeys, i), 1)
            != 0)
        {
            result.ExtendedResultCode = ADUC_ERC_NOMEM;
            goto done;
        }
    }

    *outDisabledSigningKeyList = disabledSigningKeyList;
    disabledSigningKeyList = NULL;
    result.ResultCode = ADUC_GeneralResult_Success;

done:
    if (disabledSigningKeyList != NULL)
    {
        VECTOR_destroy(disabledSigningKeyList);
        disabledSigningKeyList = NULL;
    }

    if (store_package != NULL)
    {
        ADUC_RootKeyPackageUtils_Destroy(store_package);
    }

    return result;
}
