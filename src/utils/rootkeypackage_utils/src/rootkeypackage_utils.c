/**
 * @file rootkeypackage_utils.c
 * @brief rootkeypackage_utils implementation.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/rootkeypackage_utils.h"
#include "aduc/rootkeypackage_json_properties.h"
#include "aduc/rootkeypackage_parse.h"
#include <aduc/c_utils.h> // for EXTERN_C_BEGIN, EXTERN_C_END, HTTP_URL_HANDLE
#include <aduc/string_c_utils.h> // for IsNullOrEmpty
#include <azure_c_shared_utility/string_token.h>
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include <base64_utils.h>
#include <ctype.h> // tolower
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

    result = RootKeyPackage_ParseProtectedPropertiesString(rootObj, &pkg);
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

/**
 * @brief Helper function for comparing two ADUC_RootKeys
 *
 * @param lKey left root key to compare
 * @param rKey right root key to compare
 * @return True if equal; false if not
 */
_Bool ADUC_RootKeyPackageUtils_RootKeysAreEqual(const ADUC_RootKey* lKey, const ADUC_RootKey* rKey)
{
    if (lKey == rKey)
    {
        return true;
    }
    else if (lKey == NULL || rKey == NULL)
    {
        return false;
    }

    if (STRING_compare(lKey->kid, rKey->kid) != 0)
    {
        return false;
    }

    if (lKey->keyType != rKey->keyType)
    {
        return false;
    }

    if (lKey->rsaParameters.e != rKey->rsaParameters.e)
    {
        return false;
    }

    return CONSTBUFFER_HANDLE_contain_same(lKey->rsaParameters.n, rKey->rsaParameters.n);
}

/**
 * @brief Helper function for comparing two ADUC_RootKeyPackage_Hashes
 *
 * @param lKey left hash to compare
 * @param rKey right hash to compare
 * @return True if equal; false if not
 */
_Bool ADUC_RootKeyPackageUtils_RootKeyPackage_Hash_AreEqual(
    const ADUC_RootKeyPackage_Hash* lHash, const ADUC_RootKeyPackage_Hash* rHash)
{
    if (lHash == rHash)
    {
        return true;
    }
    else if (lHash == NULL || rHash == NULL)
    {
        return false;
    }

    if (lHash->alg != rHash->alg)
    {
        return false;
    }

    return CONSTBUFFER_HANDLE_contain_same(lHash->hash, rHash->hash);
}

/**
 * @brief Helper function for comparing two ADUC_RootKeyPackage_ProtectedProperties
 *
 * @param lKey left ADUC_RootKeyPackage_ProtectedProperties to compare
 * @param rKey right ADUC_RootKeyPackage_ProtectedProperties to compare
 * @return True if equal; false if not
 */
_Bool ADUC_RootKeyPackageUtils_ProtectedProperties_AreEqual(
    const ADUC_RootKeyPackage_ProtectedProperties* lProp, const ADUC_RootKeyPackage_ProtectedProperties* rProp)
{
    if (lProp == rProp)
    {
        return true;
    }
    else if (lProp == NULL || rProp == NULL)
    {
        return false;
    }

    if (lProp->version != rProp->version)
    {
        return false;
    }

    if (lProp->publishedTime != rProp->publishedTime)
    {
        return false;
    }

    const size_t lPackNumDisabledRootKeys = VECTOR_size(lProp->disabledRootKeys);
    const size_t rPackNumDisabledRootKeys = VECTOR_size(rProp->disabledRootKeys);

    if (lPackNumDisabledRootKeys != rPackNumDisabledRootKeys)
    {
        return false;
    }

    for (size_t i = 0; i < lPackNumDisabledRootKeys; ++i)
    {
        STRING_HANDLE* lKeyId = VECTOR_element(lProp->disabledRootKeys, i);
        STRING_HANDLE* rKeyId = VECTOR_element(rProp->disabledRootKeys, i);

        if (STRING_compare(*lKeyId, *rKeyId) != 0)
        {
            return false;
        }
    }

    const size_t lPackNumDisabledSigningKeys = VECTOR_size(lProp->disabledSigningKeys);
    const size_t rPackNumDisabledSigningKeys = VECTOR_size(rProp->disabledSigningKeys);

    if (lPackNumDisabledSigningKeys != rPackNumDisabledSigningKeys)
    {
        return false;
    }

    for (size_t i = 0; i < lPackNumDisabledSigningKeys; ++i)
    {
        ADUC_RootKeyPackage_Hash* lPackHash = VECTOR_element(lProp->disabledSigningKeys, i);
        ADUC_RootKeyPackage_Hash* rPackHash = VECTOR_element(rProp->disabledSigningKeys, i);

        if (!ADUC_RootKeyPackageUtils_RootKeyPackage_Hash_AreEqual(lPackHash, rPackHash))
        {
            return false;
        }
    }

    const size_t lPackNumRootKeys = VECTOR_size(lProp->rootKeys);
    const size_t rPackNumRootKeys = VECTOR_size(rProp->rootKeys);

    if (lPackNumRootKeys != rPackNumRootKeys)
    {
        return false;
    }

    for (size_t i = 0; i < lPackNumRootKeys; ++i)
    {
        ADUC_RootKey* lPackRootKey = VECTOR_element(lProp->rootKeys, i);
        ADUC_RootKey* rPackRootKey = VECTOR_element(rProp->rootKeys, i);

        if (!ADUC_RootKeyPackageUtils_RootKeysAreEqual(lPackRootKey, rPackRootKey))
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief Helper function for comparing two ADUC_RootKeyPackage_Signature
 *
 * @param lKey left ADUC_RootKeyPackage_Signature to compare
 * @param rKey right ADUC_RootKeyPackage_Signature to compare
 * @return True if equal; false if not
 */
_Bool ADUC_RootKeyPackageUtils_RootKeyPackage_Signatures_AreEqual(
    const ADUC_RootKeyPackage_Signature* lSigs, const ADUC_RootKeyPackage_Signature* rSigs)
{
    if (lSigs == rSigs)
    {
        return true;
    }
    else if (lSigs == NULL || rSigs == NULL)
    {
        return false;
    }

    if (lSigs->alg != rSigs->alg)
    {
        return false;
    }
    return CONSTBUFFER_HANDLE_contain_same(lSigs->signature, rSigs->signature);
}

/**
 * @brief Checks to see if the two packages are correct
 *
 * @param lPack a package to compare
 * @param rPack the other package to compare
 * @return true if equal, false otherwise
 */
_Bool ADUC_RootKeyPackageUtils_AreEqual(const ADUC_RootKeyPackage* lPack, const ADUC_RootKeyPackage* rPack)
{
    if (STRING_compare(lPack->protectedPropertiesJsonString, rPack->protectedPropertiesJsonString) != 0)
    {
        return false;
    }

    if (!ADUC_RootKeyPackageUtils_ProtectedProperties_AreEqual(
            &lPack->protectedProperties, &rPack->protectedProperties))
    {
        return false;
    }

    const size_t lPackNumSignatures = VECTOR_size(lPack->signatures);
    const size_t rPackNumSignatures = VECTOR_size(rPack->signatures);

    if (lPackNumSignatures != rPackNumSignatures)
    {
        return false;
    }

    for (size_t i = 0; i < lPackNumSignatures; ++i)
    {
        ADUC_RootKeyPackage_Signature* lSig = VECTOR_element(lPack->signatures, i);
        ADUC_RootKeyPackage_Signature* rSig = VECTOR_element(rPack->signatures, i);

        if (!ADUC_RootKeyPackageUtils_RootKeyPackage_Signatures_AreEqual(lSig, rSig))
        {
            return false;
        }
    }

    return true;
}

STRING_HANDLE RootKeyPackage_SigningAlgToString(const ADUC_RootKeySigningAlgorithm alg)
{
    STRING_HANDLE algStr = NULL;

    switch (alg)
    {
    case ADUC_RootKeySigningAlgorithm_RS256:
        algStr = STRING_construct("RS256");
        break;
    case ADUC_RootKeySigningAlgorithm_RS384:
        algStr = STRING_construct("RS384");
        break;
    case ADUC_RootKeySigningAlgorithm_RS512:
        algStr = STRING_construct("RS512");
        break;
    case ADUC_RootKeySigningAlgorithm_INVALID:
    default:
        break;
    }

    return algStr;
}

JSON_Value* ADUC_RootKeyPackageUtils_SignatureToJsonValue(const ADUC_RootKeyPackage_Signature* signature)
{
    _Bool success = true;
    JSON_Value* sigJsonValue = NULL;
    char* encodedSignature = NULL;
    STRING_HANDLE algString = NULL;

    sigJsonValue = json_value_init_object();

    if (sigJsonValue == NULL)
    {
        goto done;
    }

    JSON_Object* sigJsonObj = json_value_get_object(sigJsonValue);

    const CONSTBUFFER* decodedSig = CONSTBUFFER_GetContent(signature->signature);

    if (decodedSig == NULL || decodedSig->size == 0)
    {
        goto done;
    }

    encodedSignature = Base64URLEncode(decodedSig->buffer, decodedSig->size);

    if (encodedSignature == NULL)
    {
        goto done;
    }

    JSON_Status jsonStatus = json_object_set_string(sigJsonObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_SIG, encodedSignature);

    if (jsonStatus != JSONSuccess)
    {
        goto done;
    }

    algString = RootKeyPackage_SigningAlgToString(signature->alg);

    if (algString == NULL)
    {
        goto done;
    }

    jsonStatus = json_object_set_string(sigJsonObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_ALG, STRING_c_str(algString));

done:

    if (!success)
    {
        if (sigJsonValue != NULL)
        {
            json_value_free(sigJsonValue);
            sigJsonValue = NULL;
        }
    }

    if (algString != NULL)
    {
        STRING_delete(algString);
    }

    free(encodedSignature);

    return sigJsonValue;
}
/**
 * @brief Serializes the ADUC_RootKeyPackage's contents to JSON in string form
 * @details it is the duty of the caller to free the returned string
 * @param rootKeyPackage root key package to serialize
 * @return the serialized version of the root key package in string form
 */
char* ADUC_RootKeyPackageUtils_SerializePackageToJsonString(const ADUC_RootKeyPackage* rootKeyPackage)
{
    JSON_Value* rootKeyPackageJsonValue = NULL;

    JSON_Value* protectedPropertiesJsonValue = NULL;

    JSON_Value* rootKeySignatureArrayValue = NULL;

    VECTOR_HANDLE sigJsonValueVector = NULL;

    char* retString = NULL;

    if (rootKeyPackage == NULL)
    {
        goto done;
    }

    rootKeyPackageJsonValue = json_value_init_object();

    if (rootKeyPackageJsonValue == NULL)
    {
        goto done;
    }

    JSON_Object* rootKeyPackageJsonObj = json_value_get_object(rootKeyPackageJsonValue);

    if (rootKeyPackage->protectedPropertiesJsonString == NULL
        || STRING_length(rootKeyPackage->protectedPropertiesJsonString) == 0)
    {
        goto done;
    }

    protectedPropertiesJsonValue = json_parse_string(STRING_c_str(rootKeyPackage->protectedPropertiesJsonString));

    if (protectedPropertiesJsonValue == NULL)
    {
        goto done;
    }

    if (json_object_set_value(
            rootKeyPackageJsonObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_PROTECTED, protectedPropertiesJsonValue)
        != JSONSuccess)
    {
        goto done;
    }

    //
    // commit to giving up ownership
    //
    protectedPropertiesJsonValue = NULL;

    rootKeySignatureArrayValue = json_value_init_array();
    if (rootKeySignatureArrayValue == NULL)
    {
        goto done;
    }

    JSON_Array* signatureArray = json_value_get_array(rootKeySignatureArrayValue);

    if (signatureArray == NULL)
    {
        goto done;
    }

    const size_t numSignatures = VECTOR_size(rootKeyPackage->signatures);

    for (size_t i = 0; i < numSignatures; ++i)
    {
        ADUC_RootKeyPackage_Signature* signature =
            (ADUC_RootKeyPackage_Signature*)VECTOR_element(rootKeyPackage->signatures, i);

        if (signature == NULL)
        {
            goto done;
        }

        JSON_Value* sigJsonValue = ADUC_RootKeyPackageUtils_SignatureToJsonValue(signature);

        if (json_array_append_value(signatureArray, sigJsonValue) != JSONSuccess)
        {
            goto done;
        }
    }

    if (json_object_set_value(
            rootKeyPackageJsonObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_SIGNATURES, rootKeySignatureArrayValue)
        != JSONSuccess)
    {
        goto done;
    }

    //
    // Commit to giving up ownership
    //
    rootKeySignatureArrayValue = NULL;

    retString = json_serialize_to_string(rootKeyPackageJsonValue);

    if (retString == NULL)
    {
        goto done;
    }

done:

    if (protectedPropertiesJsonValue != NULL)
    {
        json_value_free(protectedPropertiesJsonValue);
    }

    if (rootKeySignatureArrayValue != NULL)
    {
        json_value_free(rootKeySignatureArrayValue);
    }

    if (rootKeyPackageJsonValue != NULL)
    {
        json_value_free(rootKeyPackageJsonValue);
    }

    if (sigJsonValueVector != NULL)
    {
        const size_t sigJsonValueVectorSz = VECTOR_size(sigJsonValueVector);

        for (size_t i = 0; i < sigJsonValueVectorSz; ++i)
        {
            JSON_Value** sigValuePtr = (JSON_Value**)VECTOR_element(sigJsonValueVector, i);

            json_value_free(*sigValuePtr);
        }
        VECTOR_destroy(sigJsonValueVector);
    }

    return retString;
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

    if (rootKeyPackage->protectedPropertiesJsonString != NULL)
    {
        STRING_delete(rootKeyPackage->protectedPropertiesJsonString);
        rootKeyPackage->protectedPropertiesJsonString = NULL;
    }

    memset(rootKeyPackage, 0, sizeof(*rootKeyPackage));
}

EXTERN_C_END
