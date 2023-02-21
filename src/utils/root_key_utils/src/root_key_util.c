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

#include "aduc/rootkeypackage_parse.h"
#include "aduc/rootkeypackage_types.h"
#include "aduc/rootkeypackage_utils.h"
#include "base64_utils.h"
#include "crypto_lib.h"
#include "root_key_list.h"
#include <aduc/logging.h>
#include <aduc/result.h>
#include <aduc/string_c_utils.h>
#include <azure_c_shared_utility/constbuffer.h>
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

//
// Root Key Validation Helper Functions
//

static ADUC_RootKeyPackage* localStore = NULL;

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

    rootKey->rsaParameters.n = CONSTBUFFER_CreateWithMoveMemory(modulus, modulusSize);

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
 * @param rootKeyPackage package to be validated
 * @param rootKey the key to use to validate the package
 * @return a value of ADUC_Result
 */
ADUC_Result RootKeyUtility_ValidatePackageWithKey(const ADUC_RootKeyPackage* rootKeyPackage, const RSARootKey rootKey)
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

    ADUC_Result kidResult = RootKeyUtility_GetKeyForKid(&rootKeyCryptoKey, rootKey.kid);

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
ADUC_Result RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(const ADUC_RootKeyPackage* rootKeyPackage)
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

        ADUC_Result validationResult = RootKeyUtility_ValidatePackageWithKey(rootKeyPackage, rootKey);

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

/**
 * @brief Writes the package @p rootKeyPackage to file location @p fileDest atomically
 * @details creates a temp file with the content of @p rootKeyPackage and renames it to @p fileDest
 * @param rootKeyPackage the root key package to be written to the file
 * @param fileDest the destination for the file
 * @return a value of ADUC_Result
 */
ADUC_Result RootKeyUtility_WriteRootKeyPackageToFileAtomically(
    const ADUC_RootKeyPackage* rootKeyPackage, const STRING_HANDLE fileDest)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    JSON_Value* rootKeyPackageValue = NULL;

    char* rootKeyPackageSerializedString = NULL;
    STRING_HANDLE tempFileName = NULL;

    if (rootKeyPackage == NULL || fileDest == NULL || STRING_length(fileDest) == 0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_BAD_ARGS;
        goto done;
    }

    rootKeyPackageSerializedString = ADUC_RootKeyPackageUtils_SerializePackageToJsonString(rootKeyPackage);

    if (rootKeyPackageSerializedString == NULL)
    {
        goto done;
    }

    rootKeyPackageValue = json_parse_string(rootKeyPackageSerializedString);

    if (rootKeyPackageValue == NULL)
    {
        goto done;
    }

    tempFileName = STRING_construct_sprintf("%s-temp", STRING_c_str(fileDest));

    if (tempFileName == NULL)
    {
        goto done;
    }

    if (json_serialize_to_file(rootKeyPackageValue, STRING_c_str(tempFileName)) != JSONSuccess)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_CANNOT_WRITE_PACKAGE_TO_STORE;
        goto done;
    }

    if (rename(STRING_c_str(tempFileName), STRING_c_str(fileDest)) != 0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_CANT_RENAME_TO_STORE;
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;

done:

    if (rootKeyPackageSerializedString != NULL)
    {
        free(rootKeyPackageSerializedString);
    }

    if (rootKeyPackageValue != NULL)
    {
        json_value_free(rootKeyPackageValue);
    }

    if (tempFileName != NULL)
    {
        if (remove(STRING_c_str(tempFileName)) != 0)
        {
            Log_Info(
                "RootKeyUtility_WriteRootKeyPackageToFileAtomically failed to remove temp file at %s",
                STRING_c_str(tempFileName));
        }

        STRING_delete(tempFileName);
    }

    return result;
}

/**
 * @brief Reloads the package from disk into the local store
 *
 * @return a value of ADUC_Result
 */
ADUC_Result RootKeyUtility_ReloadPackageFromDisk()
{
    if (localStore != NULL)
    {
        ADUC_RootKeyPackageUtils_Destroy(localStore);
        free(localStore);
        localStore = NULL;
    }

    return RootKeyUtility_LoadPackageFromDisk(&localStore, ADUC_ROOTKEY_STORE_PACKAGE_PATH);
}

/**
 * @brief Loads the RootKeyPackage from disk at file location @p fileLocation
 *
 * @param rootKeyPackage the address of the pointer to the root key package to be set
 * @param fileLocation the file location to read the data from
 * @return a value of ADUC_Result
 */
ADUC_Result RootKeyUtility_LoadPackageFromDisk(ADUC_RootKeyPackage** rootKeyPackage, const char* fileLocation)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    JSON_Value* rootKeyPackageValue = NULL;

    ADUC_RootKeyPackage* tempPkg = NULL;

    char* rootKeyPackageJsonString = NULL;

    if (fileLocation == NULL || rootKeyPackage == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_BAD_ARGS;
        goto done;
    }

    rootKeyPackageValue = json_parse_file(fileLocation);

    if (rootKeyPackageValue == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_CANT_LOAD_FROM_STORE;
        goto done;
    }
    rootKeyPackageJsonString = json_serialize_to_string(rootKeyPackageValue);

    if (rootKeyPackageJsonString == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_FAILED_SERIALIZE_TO_STRING;
        goto done;
    }

    tempPkg = (ADUC_RootKeyPackage*)malloc(sizeof(ADUC_RootKeyPackage));

    if (tempPkg == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_ERRNOMEM;
        goto done;
    }

    ADUC_Result parseResult = ADUC_RootKeyPackageUtils_Parse(rootKeyPackageJsonString, tempPkg);

    if (IsAducResultCodeFailure(parseResult.ResultCode))
    {
        result = parseResult;
        goto done;
    }

    ADUC_Result validationResult = RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(tempPkg);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        result = validationResult;
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;

done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        ADUC_RootKeyPackageUtils_Destroy(tempPkg);
        free(tempPkg);
        tempPkg = NULL;
    }

    if (rootKeyPackageValue != NULL)
    {
        json_value_free(rootKeyPackageValue);
    }

    free(rootKeyPackageJsonString);

    *rootKeyPackage = tempPkg;

    return result;
}

/**
 * @brief Checks if the rootkey represented by @p keyId is in the disabledRootKeys of @p rootKeyPackage
 *
 * @param keyId the id of the key to check
 * @return true if the key is in the disabledKeys section, false if it isn't
 */
bool RootKeyUtility_RootKeyIsDisabled(const ADUC_RootKeyPackage* rootKeyPackage, const char* keyId)
{
    if (rootKeyPackage == NULL)
    {
        return true;
    }

    const size_t numDisabledKeys = VECTOR_size(rootKeyPackage->protectedProperties.disabledRootKeys);

    for (size_t i = 0; i < numDisabledKeys; ++i)
    {
        STRING_HANDLE* disabledKey = VECTOR_element(rootKeyPackage->protectedProperties.disabledRootKeys, i);

        if (strcmp(STRING_c_str(*disabledKey), keyId) == 0)
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Searches the local store for the key with the keyId @p keyId and returns it as a CryptoKeyHandle
 *
 * @param keyId the keyId for the root key to look for
 * @return the CryptoKeyHandle associated with the keyId on success; NULL on failure
 */
CryptoKeyHandle RootKeyUtility_SearchLocalStoreForKey(const char* keyId)
{
    CryptoKeyHandle key = NULL;

    if (localStore == NULL)
    {
        return key;
    }

    const size_t numStoreRootKeys = VECTOR_size(localStore->protectedProperties.rootKeys);

    for (size_t i = 0; i < numStoreRootKeys; ++i)
    {
        ADUC_RootKey* rootKey = VECTOR_element(localStore->protectedProperties.rootKeys, i);

        if (strcmp(STRING_c_str(rootKey->kid), keyId) == 0 && !RootKeyUtility_RootKeyIsDisabled(localStore, keyId))
        {
            key = MakeCryptoKeyHandleFromADUC_RootKey(rootKey);
        }
    }

    return key;
}

/**
 * @brief GEts the key for @p keyId from the local store and allocates @p key
 * @details the caller must free key using CryptoUtils_FreeCryptoKeyHandle() on key
 * @param key the handle to allocate the CryptoKeyHandle for @p kid
 * @param keyId
 * @return ADUC_Result
 */
ADUC_Result RootKeyUtility_GetKeyForKeyIdFromLocalStore(CryptoKeyHandle* key, const char* keyId)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    CryptoKeyHandle tempKey = NULL;

    tempKey = RootKeyUtility_SearchLocalStoreForKey(keyId);

    if (tempKey == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_NO_ROOTKEY_FOUND_FOR_KEYID;
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;

done:

    return result;
}

/**
 * @brief Helper function that returns a CryptoKeyHandle associated with the kid
 * @details The caller must free the returned Key with the CryptoUtils_FreeCryptoKeyHandle() function
 * @param key the handle to allocate the CryptoKeyHandle for @p kid
 * @param kid the key identifier associated with the key
 * @returns a value of ADUC_Result
 */
ADUC_Result RootKeyUtility_GetKeyForKid(CryptoKeyHandle* key, const char* kid)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    CryptoKeyHandle tempKey = NULL;
#ifdef USE_LOCAL_STORE
    if (localStore == NULL)
    {
        ADUC_Result loadResult = RootKeyUtility_LoadPackageFromDisk(localStore, ADUC_ROOTKEY_STORE_PACKAGE_PATH);

        if (IsAducResultCodeFailure(loadResult.ResultCode))
        {
            result = loadResult;
            goto done;
        }
    }

    if (RootKeyUtility_RootKeyIsDisabled(localStore, kid))
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNING_ROOTKEY_IS_DISABLED;
        goto done;
    }
#endif
    const RSARootKey* hardcodedRsaRootKeys = RootKeyList_GetHardcodedRsaRootKeys();

    const unsigned numberKeys = RootKeyList_numHardcodedKeys();
    for (unsigned i = 0; i < numberKeys; ++i)
    {
        if (strcmp(hardcodedRsaRootKeys[i].kid, kid) == 0)
        {
            tempKey = MakeCryptoKeyHandleFromRSARootkey(hardcodedRsaRootKeys[i]);
        }
    }

#ifdef USE_LOCAL_STORE
    if (tempKey == NULL)
    {
        ADUC_Result fetchResult = RootKeyUtility_GetKeyForKeyIdFromLocalStore(&tempKey, kid);

        if (IsAducResultCodeFailure(fetchResult.ResultCode))
        {
            result = fetchResult;
            goto done;
        }
    }

#else

    if (tempKey == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_NO_ROOTKEY_FOUND_FOR_KEYID;
        goto done;
    }

#endif

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    *key = tempKey;

    return result;
}
