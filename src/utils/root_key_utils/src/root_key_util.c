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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <aduc/result.h>
#include "aduc/rootkeypackage_parse.h"
#include "aduc/rootkeypackage_types.h"
#include "aduc/rootkeypackage_utils.h"
#include <azure_c_shared_utility/constbuffer.h>
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include "crypto_lib.h"
#include "base64_utils.h"
#include "root_key_list.h"

//
// Root Key Validation Helper Functions
//

/**
 * @brief Helper function for mkaing a CryptoKeyHandle from an ADUC_RootKety
 *
 * @param rootKey ADUC_RootKey to make into a crypto key handle
 * @return NULL on failure, a CryptoKeyHandle on sucess
 */
CryptoKeyHandle MakeCryptoKeyHandleFromADUC_RootKey(const ADUC_RootKey* rootKey)
{
    if (rootKey == NULL)
    {
        return NULL;
    }

    CryptoKeyHandle key = NULL;

    const CONSTBUFFER* modulus;
    switch(rootKey->keyType){
        case ADUC_RootKey_KeyType_RSA:
        modulus = CONSTBUFFER_GetContent(rootKey->rsaParameters.n);

        key = RSAKey_ObjFromModulusBytesExponentInt(modulus->buffer,modulus->size,rootKey->rsaParameters.e);
        break;

        case ADUC_RootKey_KeyType_INVALID:
        // TODO: Some logging info
        break;

        default:
        break;
    }

    return key;
}

/**
 * @brief Initializes an aDUC_RootKey with the parameters in an RSARootKey
 * @details Caller must take care to de-init the rootKey using ADUC_RootKey_DeInit()
 * @param rootKey rootKey pointer to initialize with the data
 * @param rsaKey rsaKey to use for intiialization
 * @return True on success; false on failure
 */
_Bool InitializeADUC_RootKey_From_RSARootKey(ADUC_RootKey* rootKey, const RSARootKey rsaKey )
{
    _Bool success = false;
    STRING_HANDLE kid = NULL;
    uint8_t* modulus = 0;

    if (rootKey == NULL )
    {
        goto done;
    }


    rootKey->keyType = ADUC_RootKey_KeyType_RSA;

    kid = STRING_construct(rsaKey.kid);

    if (kid == NULL)
    {
        goto done;
    }

    rootKey->kid = kid;

    const size_t modulusSize = Base64URLDecode(rsaKey.N,&modulus);

    if (modulusSize == 0)
    {
        goto done;
    }

    rootKey->rsaParameters.n = CONSTBUFFER_CreateWithMoveMemory(modulus,modulusSize);

    if (rootKey->rsaParameters.n == NULL)
    {
        goto done;
    }

    rootKey->rsaParameters.e = rsaKey.e;

    success = true;
done:

    if (! success)
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
_Bool RootKeyUtility_GetSignatureForKey(size_t* foundIndex, const ADUC_RootKeyPackage* rootKeyPackage, const char* seekKid)
{
    if (rootKeyPackage == NULL || seekKid == NULL)
    {
        return false;
    }

    size_t numKeys = VECTOR_size(rootKeyPackage->protectedProperties.rootKeys);

    for (int i=0 ; i < numKeys; ++i)
    {
        ADUC_RootKey* root_key = (ADUC_RootKey*)VECTOR_element(rootKeyPackage->protectedProperties.rootKeys,i);

        if (root_key == NULL)
        {
            return false;
        }

        if (strcmp(STRING_c_str(root_key->kid),seekKid) == 0)
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
_Bool RootKeyUtility_GetHardcodedKeysAsAducRootKeys(VECTOR_HANDLE* aducRootKeyVector)
{
    _Bool success = false;

    VECTOR_HANDLE tempHandle = VECTOR_create(sizeof(ADUC_RootKey));

    if (tempHandle == NULL)
    {
        goto done;
    }

    const RSARootKey* rsaKeyList = RootKeyList_GetHardcodedRsaRootKeys();

    const size_t rsaKeyListSize = RootKeyList_numHardcodedKeys();

    for (int i=0 ; i < rsaKeyListSize; ++i)
    {
        RSARootKey key = rsaKeyList[i];
        ADUC_RootKey rootKey = {};
        if (! InitializeADUC_RootKey_From_RSARootKey(&rootKey,key))
        {
            goto done;
        }
        VECTOR_push_back(tempHandle,&rootKey,1);
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

    if (! success)
    {
        if (tempHandle != NULL)
        {
            const size_t tempHandleSize = VECTOR_size(tempHandle);
            for (size_t i=0 ; i < tempHandleSize ; ++i)
            {
                ADUC_RootKey* rootKey = VECTOR_element(tempHandle,i);

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

    if (rootKeyPackage == NULL )
    {
        goto done;
    }

    size_t signatureIndex = 0;
    if (! RootKeyUtility_GetSignatureForKey(&signatureIndex, rootKeyPackage,rootKey.kid))
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_SIGNATURE_FOR_KEY_NOT_FOUND;
        goto done;
    }

    // Get the signature
    ADUC_RootKeyPackage_Hash* signature = (ADUC_RootKeyPackage_Hash*)VECTOR_element(rootKeyPackage->signatures,signatureIndex);

    if (signature == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_UNEXPECTED;
        goto done;
    }

    rootKeyCryptoKey = RootKeyUtility_GetKeyForKid(rootKey.kid);

    if (rootKeyCryptoKey == NULL)
    {
        goto done;
    }

    // Now we have the key and the signature.
    const char* protectedProperties = STRING_c_str(rootKeyPackage->protectedPropertiesJsonString);
    const size_t protectedPropertiesLength = STRING_length(rootKeyPackage->protectedPropertiesJsonString);

    // key, signature, and hash plus
    const CONSTBUFFER* signatureHash = CONSTBUFFER_GetContent(signature->hash);

    if (! CryptoUtils_IsValidSignature("rs256",signatureHash->buffer,signatureHash->size,(const uint8_t*)protectedProperties,protectedPropertiesLength,rootKeyCryptoKey) ){
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
ADUC_Result RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(const ADUC_RootKeyPackage* rootKeyPackage )
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    VECTOR_HANDLE hardcodedRootKeys = NULL;

    const RSARootKey* hardcodedRsaKeys = RootKeyList_GetHardcodedRsaRootKeys();
    const size_t numHardcodedKeys = RootKeyList_numHardcodedKeys();

    for (size_t i = 0; i < numHardcodedKeys; ++i)
    {
        const RSARootKey rootKey = hardcodedRsaKeys[i];

        result = RootKeyUtility_ValidatePackageWithKey(rootKeyPackage,rootKey);

        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }
    }

done:

    if (hardcodedRootKeys != NULL )
    {
        const size_t hardcodedRootKeysLength = VECTOR_size(hardcodedRootKeys);
        for(size_t i =0 ; i < hardcodedRootKeysLength; ++i )
        {
            ADUC_RootKey* rootKey = (ADUC_RootKey*)VECTOR_element(hardcodedRootKeys,i);

            ADUC_RootKey_DeInit(rootKey);
        }
        VECTOR_destroy(hardcodedRootKeys);
    }

    return result;
}

ADUC_Result RootKeyUtil_WriteRootKeyPackageToFileAtomically(const ADUC_RootKeyPackage* rootKeyPackage, const STRING_HANDLE fileDest)
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

    // Write this to the temp file
    rootKeyPackageValue = json_parse_string(rootKeyPackageSerializedString);

    if (rootKeyPackageValue == NULL)
    {
        goto done;
    }

    tempFileName = STRING_construct_sprintf("%s-temp",STRING_c_str(fileDest));

    if (tempFileName == NULL)
    {
        goto done;
    }

    if (json_serialize_to_file(rootKeyPackageValue,STRING_c_str(tempFileName)) != JSONSuccess)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_CANNOT_WRITE_PACKAGE_TO_STORE;
        goto done;
    }

    // Switch the names
    if (rename(STRING_c_str(tempFileName),STRING_c_str(fileDest)) != 0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYUTIL_ROOTKEYPACKAGE_CANT_RENAME_TO_STORE;
        goto done;
    }

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
        STRING_delete(tempFileName);
    }

    return result;
}
/**
 * @brief Helper function that returns a CryptoKeyHandle associated with the kid
 * @details The caller must free the returned Key with the CryptoUtils_FreeCryptoKeyHandle() function
 * @param kid the key identifier associated with the key
 * @returns the CryptoKeyHandle on success, null on failure
 */
CryptoKeyHandle RootKeyUtility_GetKeyForKid(const char* kid)
{
    CryptoKeyHandle key = NULL;
    uint8_t* modulus = NULL;

    const RSARootKey* hardcodedRsaRootKeys = RootKeyList_GetHardcodedRsaRootKeys();

    const unsigned numberKeys = RootKeyList_numHardcodedKeys();
    for (unsigned i = 0; i < numberKeys; ++i)
    {
        if (strcmp(hardcodedRsaRootKeys[i].kid, kid) == 0)
        {

            const size_t modulusSize = Base64URLDecode(hardcodedRsaRootKeys[i].N,&modulus);

            if (modulusSize == 0)
            {
                goto done;
            }

            key = RSAKey_ObjFromModulusBytesExponentInt(modulus,modulusSize,hardcodedRsaRootKeys[i].e);
        }
    }

done:

    if (modulus != NULL)
    {
        free(modulus);
    }

    return key;
}

