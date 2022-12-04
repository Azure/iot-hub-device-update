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
#include <azure_c_shared_utility/constbuffer.h>
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include "crypto_lib.h"
#include "base64_utils.h"
#include "root_key_list.h"
#include "root_key_result.h"
#include "aduc/rootkeypackage_parse.h"
#include "aduc/rootkeypackage_types.h"
#include "aduc/rootkeypackage_utils.h"

//
// Root Key Validation Helper Functions
//


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

RootKeyUtility_ValidationResult RootKeyUtility_ValidatePackageWithKey(const ADUC_RootKeyPackage* rootKeyPackage, const RSARootKey rootKey)
{
    RootKeyUtility_ValidationResult result = RootKeyUtility_ValidationResult_Failure;
    CryptoKeyHandle rootKeyCryptoKey = NULL;

    if (rootKeyPackage == NULL )
    {
        goto done;
    }

    size_t signatureIndex = 0;
    if (! RootKeyUtility_GetSignatureForKey(&signatureIndex, rootKeyPackage,rootKey.kid))
    {
        result = RootKeyUtility_ValidationResult_SignatureForKeyNotFound;
        goto done;
    }

    // Get the signature
    ADUC_RootKeyPackage_Hash* signature = (ADUC_RootKeyPackage_Hash*)VECTOR_element(rootKeyPackage->signatures,signatureIndex);

    if (signature == NULL)
    {
        result = RootKeyUtility_ValidationResult_SignatureForKeyNotFound;
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
        result = RootKeyUtility_ValidationResult_SignatureValidationFailed;
        goto done;
    }

    result = RootKeyUtility_ValidationResult_Success;

done:

    if (rootKeyCryptoKey != NULL)
    {
        CryptoUtils_FreeCryptoKeyHandle(rootKeyCryptoKey);
    }

    return result;
}

RootKeyUtility_ValidationResult RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(const ADUC_RootKeyPackage* rootKeyPackage )
{
    RootKeyUtility_ValidationResult result = RootKeyUtility_ValidationResult_Failure;

    VECTOR_HANDLE hardcodedRootKeys = NULL;

    const RSARootKey* hardcodedRsaKeys = RootKeyList_GetHardcodedRsaRootKeys();
    const size_t numHardcodedKeys = RootKeyList_numHardcodedKeys();

    for (size_t i = 0; i < numHardcodedKeys; ++i)
    {
        const RSARootKey rootKey = hardcodedRsaKeys[i];

        result = RootKeyUtility_ValidatePackageWithKey(rootKeyPackage,rootKey);

        if (result != RootKeyUtility_ValidationResult_Success)
        {
            // TODO: Logging info for what key failed
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

RootKeyUtility_InstallationResult RootKeyUtil_WriteRootKeyPackageToFileAtomically(const ADUC_RootKeyPackage* rootKeyPackage, const STRING_HANDLE fileDest)
{
    RootKeyUtility_InstallationResult result = RootKeyUtility_InstallationResult_Failure;
    JSON_Value* rootKeyPackageValue = NULL;

    char* rootKeyPackageSerializedString = NULL;
    STRING_HANDLE tempFileName = NULL;

    if (rootKeyPackage == NULL || fileDest == NULL || STRING_length(fileDest) == 0)
    {
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
        goto done;
    }

    // Switch the names
    if (rename(STRING_c_str(tempFileName),STRING_c_str(fileDest)) != 0)
    {
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
        FILE* f = fopen(STRING_c_str(tempFileName),'w');
        if (f != NULL)
        {
            fclose(f);
            remove(STRING_c_str(tempFileName));
        }

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

