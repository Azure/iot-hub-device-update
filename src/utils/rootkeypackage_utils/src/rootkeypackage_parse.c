/**
 * @file rootkeypackage_parse.c
 * @brief rootkeypackage_parse implementation.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/rootkeypackage_parse.h"
#include "aduc/rootkeypackage_json_properties.h"
#include <aduc/logging.h>
#include <aduc/string_c_utils.h> // for IsNullOrEmpty
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include <base64_utils.h> // for Base64URLDecode
#include <math.h> // for isnan, isinf

#if defined(isnan) && defined(isinf)
#    define ADUC_IS_NUMBER_INVALID(x) (isnan((x)) || isinf((x)))
#else
#    define ADUC_IS_NUMBER_INVALID(x) (((x) * 0.0) != 0.0)
#endif

/*
    Using 0 to indicate invalid RSA exponent since json_object_get_number
    returns 0 when the json property value is not json number type and because 0 is
    not one of the 5 known Fermat primes of 3, 5, 17, 257 and 65537 for base b = 2
    n generalized Fermat primes. The current root keys use the industry standard of
    65537 (0x010001), or "QAQD" as base64 url encoded string.
 */
#define INVALID_EXPONENT 0

EXTERN_C_BEGIN

/**
 * @brief Frees resources for an ADUC_RootKey object.
 *
 * @param node The pointer to the root key object.
 */
void ADUC_RootKey_DeInit(ADUC_RootKey* node)
{
    if (node == NULL)
    {
        return;
    }

    if (node->kid != NULL)
    {
        STRING_delete(node->kid);
        node->kid = NULL;
    }

    node->keyType = ADUC_RootKey_KeyType_INVALID;

    if (node->rsaParameters.n != NULL)
    {
        CONSTBUFFER_DecRef(node->rsaParameters.n);
        node->rsaParameters.n = NULL;
    }

    node->rsaParameters.e = 0;
}

/**
 * @brief Deinitializes members of ADUC_RootKeyPackage_Hash node
 *
 * @param node The node.
 */
void ADUC_RootKeyPackage_Hash_DeInit(ADUC_RootKeyPackage_Hash* node)
{
    if (node == NULL)
    {
        return;
    }

    if (node->hash != NULL)
    {
        CONSTBUFFER_DecRef(node->hash);
        node->hash = NULL;
    }
}

/**
 * @brief Deinitializes members of ADUC_RootKeyPackage_Signature node
 *
 * @param node The node.
 */
void ADUC_RootKeyPackage_Signature_DeInit(ADUC_RootKeyPackage_Signature* node)
{
    if (node == NULL)
    {
        return;
    }

    if (node->signature != NULL)
    {
        CONSTBUFFER_DecRef(node->signature);
        node->signature = NULL;
    }
}

/**
 * @brief Parses the isTest protected property in accordance with rootkeypackage.schema.json
 *
 * @param protectedPropertiesObj The root JSON object.
 * @param[out] outPackage The root key package object to write parsed version data.
 *
 * @return The result.
 */
ADUC_Result RootKeyPackage_ParseIsTest(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    int isTestValue = 0;

    if (protectedPropertiesObj == NULL || outPackage == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG;
        goto done;
    }

    isTestValue = json_object_get_boolean(protectedPropertiesObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_ISTEST);
    if (isTestValue == -1)
    {
        isTestValue = 0; // assume non-test if missing
    }

    (outPackage->protectedProperties).isTest = (bool)isTestValue;
    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ERC %d parsing '" ADUC_ROOTKEY_PACKAGE_PROPERTY_ISTEST "' property.", result.ResultCode);
    }

    return result;
}

/**
 * @brief Parses the version protected property in accordance with rootkeypackage.schema.json
 *
 * @param protectedPropertiesObj The root JSON object.
 * @param[out] outPackage The root key package object to write parsed version data.
 *
 * @return The result.
 */
ADUC_Result RootKeyPackage_ParseVersion(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    double versionNum = 0.0;

    if (protectedPropertiesObj == NULL || outPackage == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG;
        goto done;
    }

    versionNum = json_object_get_number(protectedPropertiesObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_VERSION);
    if (versionNum == 0.0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY_VERSION;
        goto done;
    }

    (outPackage->protectedProperties).version = (unsigned long)versionNum;
    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ERC %d parsing '" ADUC_ROOTKEY_PACKAGE_PROPERTY_VERSION "' property.", result.ResultCode);
    }

    return result;
}

/**
 * @brief Parses the published protected property in accordance with rootkeypackage.schema.json
 *
 * @param protectedPropertiesObj The protected properties JSON object.
 * @param[out] outPackage The root key package object to write parsed published data.
 *
 * @return The result.
 */
ADUC_Result RootKeyPackage_ParsePublished(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    time_t published = 0;

    if (protectedPropertiesObj == NULL || outPackage == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG;
        return result;
    }

    published = (time_t)json_object_get_number(protectedPropertiesObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_PUBLISHED);
    if (published <= 0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY_PUBLISHED;
        goto done;
    }

    (outPackage->protectedProperties).publishedTime = published;
    result.ResultCode = ADUC_GeneralResult_Success;

done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ERC %d parsing '" ADUC_ROOTKEY_PACKAGE_PROPERTY_PUBLISHED "' property.", result.ResultCode);
    }

    return result;
}

/**
 * @brief Parses the disabledRootKeys protected property in accordance with rootkeypackage.schema.json
 *
 * @param protectedPropertiesObj The protected properties JSON object.
 * @param[out] outPackage The root key package object to write parsed disabledRootKeys data.
 *
 * @return The result.
 */
ADUC_Result RootKeyPackage_ParseDisabledRootKeys(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    JSON_Array* kidsArray = NULL;
    VECTOR_HANDLE kids = NULL;
    size_t count = 0;

    if (protectedPropertiesObj == NULL || outPackage == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG;
        return result;
    }

    kidsArray = json_object_get_array(protectedPropertiesObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_DISABLED_ROOT_KEYS);
    if (kidsArray == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY_DISABLEDROOTKEYS;
        goto done;
    }

    count = json_array_get_count(kidsArray);

    kids = VECTOR_create(sizeof(STRING_HANDLE));
    if (kids == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    for (size_t i = 0; i < count; ++i)
    {
        const char* kid = NULL;
        STRING_HANDLE kidHandle = NULL;
        if ((kid = json_array_get_string(kidsArray, i)) == NULL || (kidHandle = STRING_construct(kid)) == NULL)
        {
            result.ExtendedResultCode = ADUC_ERC_NOMEM;
            goto done;
        }

        if (VECTOR_push_back(kids, &kidHandle, 1) != 0)
        {
            STRING_delete(kidHandle);
            result.ExtendedResultCode = ADUC_ERC_NOMEM;
            goto done;
        }
        kidHandle = NULL;
    }

    (outPackage->protectedProperties).disabledRootKeys = kids;
    kids = NULL;

    result.ResultCode = ADUC_GeneralResult_Success;

done:
    if (kids != NULL)
    {
        size_t cnt = VECTOR_size(kids);
        for (size_t i = 0; i < cnt; ++i)
        {
            STRING_HANDLE* h = VECTOR_element(kids, i);
            STRING_delete(*h);
        }
        VECTOR_destroy(kids);
    }

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error(
            "ERC %d parsing '" ADUC_ROOTKEY_PACKAGE_PROPERTY_DISABLED_ROOT_KEYS "' property.", result.ResultCode);
    }

    return result;
}

/**
 * @brief Parses "alg" hash algorithm property.
 *
 * @param jsonObj The json object with the "alg" property.
 * @param outAlg The output parameter to hold the parsed hash algorithm upon success.
 * @return ADUC_Result The result.
 */
ADUC_Result RootKeyPackage_ParseHashAlg(JSON_Object* jsonObj, SHAversion* outAlg)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    SHAversion alg = SHA256;
    const char* val = NULL;

    if (jsonObj == NULL || outAlg == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG;
        return result;
    }

    val = json_object_get_string(jsonObj, "alg");
    if (val == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_HASHING_PROPERTY_ALG;
        goto done;
    }

    if (!ADUC_HashUtils_GetShaVersionForTypeString(val, &alg) || !ADUC_HashUtils_IsValidHashAlgorithm(alg))
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_INVALID_HASH_ALGORITHM;
        goto done;
    }

    *outAlg = alg;
    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ERC %d parsing hash '" ADUC_ROOTKEY_PACKAGE_PROPERTY_ALG "' property.", result.ResultCode);
    }

    return result;
}

/**
 * @brief Parses "alg" signing algorithm property.
 *
 * @param jsonObj The json object with the "alg" property.
 * @param outAlg The output parameter to hold the parsed signing algorithm upon success.
 * @return ADUC_Result The result.
 */
ADUC_Result RootKeyPackage_ParseSigningAlg(JSON_Object* jsonObj, ADUC_RootKeySigningAlgorithm* outAlg)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    ADUC_RootKeySigningAlgorithm alg = ADUC_RootKeySigningAlgorithm_INVALID;
    const char* val = NULL;

    if (jsonObj == NULL || outAlg == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG;
        return result;
    }

    val = json_object_get_string(jsonObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_ALG);
    if (val == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_SIGNATURE_PROPERTY_ALG;
        goto done;
    }

    if (strcmp(val, ADUC_ROOTKEY_PACKAGE_PROPERTY_SIGNATURE_ALG_RS256) == 0)
    {
        alg = ADUC_RootKeySigningAlgorithm_RS256;
    }
    else if (strcmp(val, ADUC_ROOTKEY_PACKAGE_PROPERTY_SIGNATURE_ALG_RS384) == 0)
    {
        alg = ADUC_RootKeySigningAlgorithm_RS384;
    }
    else if (strcmp(val, ADUC_ROOTKEY_PACKAGE_PROPERTY_SIGNATURE_ALG_RS512) == 0)
    {
        alg = ADUC_RootKeySigningAlgorithm_RS512;
    }
    else
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_INVALID_SIGNING_ALGORITHM;
        goto done;
    }

    *outAlg = alg;
    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ERC %d parsing signing '" ADUC_ROOTKEY_PACKAGE_PROPERTY_ALG "' property.", result.ResultCode);
    }

    return result;
}

/**
 * @brief Parses a base64 URLUInt value from the JSON object.
 *
 * @param jsonObj The JSON object with the property that is the hash value.
 * @param propertyName The property name for the value containing the Base64 URLUInt encoded data.
 * @param outHashBuffer The output parameter const buffer handle for holding the binary data.
 * @return ADUC_Result The result.
 */
ADUC_Result RootKeyPackage_ParseBase64URLUIntJsonString(
    JSON_Object* jsonObj, const char* propertyName, CONSTBUFFER_HANDLE* outHashBuffer)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    uint8_t* buf = NULL;
    size_t out_len = 0;
    CONSTBUFFER_HANDLE buffer = NULL;
    const char* val = NULL;

    if (jsonObj == NULL || IsNullOrEmpty(propertyName) || outHashBuffer == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG;
        return result;
    }

    val = json_object_get_string(jsonObj, propertyName);
    if (val == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY_HASH_OR_SIG;
        goto done;
    }

    out_len = Base64URLDecode(val, &buf);
    if (out_len == 0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_INVALID_ENCODING;
        goto done;
    }

    buffer = CONSTBUFFER_Create(buf, out_len);
    if (buffer == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    *outHashBuffer = buffer;
    buffer = NULL;

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    free(buf);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ERC %d parsing '%s' property.", result.ResultCode, propertyName);
    }

    return result;
}

/**
 * @brief Parses the disabledSigningKeys protected property in accordance with rootkeypackage.schema.json
 *
 * @param protectedPropertiesObj The protected properties JSON object.
 * @param[out] outPackage The root key package object to write parsed disabledSigningKeys data.
 *
 * @return The result.
 */
ADUC_Result
RootKeyPackage_ParseDisabledSigningKeys(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    JSON_Array* hashesArray = NULL;
    VECTOR_HANDLE hashes = NULL;
    size_t count = 0;

    if (protectedPropertiesObj == NULL || outPackage == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG;
        return result;
    }

    hashesArray = json_object_get_array(protectedPropertiesObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_DISABLED_SIGNING_KEYS);
    if (hashesArray == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY_DISABLEDSIGNINGKEYS;
        goto done;
    }

    count = json_array_get_count(hashesArray);

    hashes = VECTOR_create(sizeof(ADUC_RootKeyPackage_Hash));
    if (hashes == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    for (size_t i = 0; i < count; ++i)
    {
        // These are SHA256 (or stronger) Hash of the public key of signing key.
        ADUC_RootKeyPackage_Hash hashElement = { .alg = SHA256, .hash = NULL };
        SHAversion tmpAlg = SHA256;
        CONSTBUFFER_HANDLE hashBuf = NULL;

        JSON_Object* hashJsonArrayElementObj = json_array_get_object(hashesArray, i);
        if (hashJsonArrayElementObj == NULL)
        {
            result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_GETOBJ_DISABLEDSIGNINGKEYS_ELEMENT;
            goto done;
        }

        result = RootKeyPackage_ParseHashAlg(hashJsonArrayElementObj, &tmpAlg);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }

        result = RootKeyPackage_ParseBase64URLUIntJsonString(
            hashJsonArrayElementObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_HASH, &hashBuf);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }
        hashElement.alg = tmpAlg;
        hashElement.hash = hashBuf;

        if (VECTOR_push_back(hashes, &hashElement, 1) != 0)
        {
            ADUC_RootKeyPackage_Hash_DeInit(&hashElement);

            result.ExtendedResultCode = ADUC_ERC_NOMEM;
            goto done;
        }
    }

    (outPackage->protectedProperties).disabledSigningKeys = hashes;
    hashes = NULL;

    result.ResultCode = ADUC_GeneralResult_Success;

done:
    if (hashes != NULL)
    {
        size_t cnt = VECTOR_size(hashes);
        for (size_t i = 0; i < cnt; ++i)
        {
            ADUC_RootKeyPackage_Hash* node = (ADUC_RootKeyPackage_Hash*)VECTOR_element(hashes, i);
            ADUC_RootKeyPackage_Hash_DeInit(node);
        }

        VECTOR_destroy(hashes);
    }

    return result;
}

/**
 * @brief Parses the kid-to-rootKeyDefinition mappings in the rootKeys JSON object.
 *
 * @param rootKeysObj The JSON object with the KIDs that map to root key definition.
 * @param index The index into the JSON array.
 * @param outRootKeys The vector to add ADUC_RootKey elements.
 * @return ADUC_Result The result.
 */
static ADUC_Result ParseRootKey(JSON_Object* rootKeysObj, size_t index, VECTOR_HANDLE* outRootKeys)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    JSON_Object* rootKeyDefinition = NULL;
    STRING_HANDLE kidStrHandle = NULL;
    ADUC_RootKey_KeyType keyType = ADUC_RootKey_KeyType_INVALID;
    CONSTBUFFER_HANDLE rsa_modulus = NULL;
    unsigned int rsa_exponent = 0;

    ADUC_RootKey rootKey;
    memset(&rootKey, 0, sizeof(rootKey));

    size_t modulus_len = 0;
    uint8_t* modulus_buf = NULL;

    const char* keytypeStr = NULL;
    const char* n_modulusStr = NULL;
    double e_exponent = INVALID_EXPONENT;

    const char* kid = json_object_get_name(rootKeysObj, index);
    if (IsNullOrEmpty(kid))
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_INVALID_KEY_ID;
        goto done;
    }

    kidStrHandle = STRING_construct(kid);
    if (kidStrHandle == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    rootKeyDefinition = json_object_get_object(rootKeysObj, kid);
    if (rootKeyDefinition == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UNEXPECTED;
        goto done;
    }

    keytypeStr = json_object_get_string(rootKeyDefinition, ADUC_ROOTKEY_PACKAGE_PROPERTY_KEY_TYPE);
    if (IsNullOrEmpty(keytypeStr))
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY_KEYTYPE;
        goto done;
    }

    if (strcmp(keytypeStr, ADUC_ROOTKEY_PACKAGE_PROPERTY_KEY_TYPE_RSA) == 0)
    {
        keyType = ADUC_RootKey_KeyType_RSA;
        n_modulusStr = json_object_get_string(rootKeyDefinition, ADUC_ROOTKEY_PACKAGE_PROPERTY_RSA_MODULUS);

        if (IsNullOrEmpty(n_modulusStr))
        {
            result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_INVALID_MODULUS;
            goto done;
        }

        e_exponent = json_object_get_number(rootKeyDefinition, ADUC_ROOTKEY_PACKAGE_PROPERTY_RSA_EXPONENT);
        if (e_exponent == INVALID_EXPONENT || ADUC_IS_NUMBER_INVALID(e_exponent))
        {
            result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_INVALID_EXPONENT;
            goto done;
        }

        rsa_exponent = (unsigned int)e_exponent;

        modulus_len = Base64URLDecode(n_modulusStr, &modulus_buf);
        if (modulus_len == 0)
        {
            result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_INVALID_RSA_PARAMETERS;
            goto done;
        }

        rsa_modulus = CONSTBUFFER_Create(modulus_buf, modulus_len);
        if (rsa_modulus == NULL)
        {
            result.ExtendedResultCode = ADUC_ERC_NOMEM;
            goto done;
        }
    }
    else
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_UNSUPPORTED_KEYTYPE;
        goto done;
    }

    rootKey.kid = kidStrHandle;
    rootKey.keyType = keyType;
    rootKey.rsaParameters.n = rsa_modulus;
    rootKey.rsaParameters.e = rsa_exponent;

    if (VECTOR_push_back(*outRootKeys, &rootKey, 1) != 0)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    // commit transfer
    kidStrHandle = NULL;
    rsa_modulus = NULL;
    memset(&rootKey, 0, sizeof(rootKey));
    result.ResultCode = ADUC_GeneralResult_Success;

done:
    if (kidStrHandle != NULL)
    {
        STRING_delete(kidStrHandle);
    }

    free(modulus_buf);

    if (rsa_modulus != NULL)
    {
        CONSTBUFFER_DecRef(rsa_modulus);
    }

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("Failed parse of rootkey, ERC %d", result.ResultCode);
    }

    return result;
}

/**
 * @brief Parses the rootKeys protected property in accordance with rootkeypackage.schema.json
 *
 * @param protectedPropertiesObj The protected properties JSON object.
 * @param[out] outPackage The root key package object to write parsed rootKeys data.
 *
 * @return The result.
 */
ADUC_Result RootKeyPackage_ParseRootKeys(JSON_Object* protectedPropertiesObj, ADUC_RootKeyPackage* outPackage)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    size_t cnt = 0;

    JSON_Object* rootKeysObj = NULL;
    VECTOR_HANDLE rootKeys = NULL;

    if (protectedPropertiesObj == NULL || outPackage == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG;
        return result;
    }

    rootKeysObj = json_object_get_object(protectedPropertiesObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_ROOTKEYS);
    if (protectedPropertiesObj == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY_ROOTKEYS;
        goto done;
    }

    cnt = json_object_get_count(rootKeysObj);
    if (cnt == 0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_ROOTKEYS_EMPTY;
        goto done;
    }

    rootKeys = VECTOR_create(sizeof(ADUC_RootKey));
    if (rootKeys == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    for (size_t i = 0; i < cnt; ++i)
    {
        result = ParseRootKey(rootKeysObj, i, &rootKeys);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }
    }

    outPackage->protectedProperties.rootKeys = rootKeys;
    rootKeys = NULL;
    result.ResultCode = ADUC_GeneralResult_Success;

done:
    if (rootKeys != NULL)
    {
        size_t root_key_vec_size = VECTOR_size(rootKeys);
        for (size_t i = 0; i < root_key_vec_size; ++i)
        {
            ADUC_RootKey* node = (ADUC_RootKey*)VECTOR_element(rootKeys, i);
            ADUC_RootKey_DeInit(node);
        }

        VECTOR_destroy(rootKeys);
    }

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ERC %d parsing 'protected' property.", result.ResultCode);
    }

    return result;
}

/**
 * @brief Parses the protected properties in accordance with rootkeypackage.schema.json
 *
 * @param protectedPropertiesObj The protected properties JSON object.
 * @param[out] outPackage The root key package object to write parsed protected properties data.
 *
 * @return The result.
 */
ADUC_Result RootKeyPackage_ParseProtectedProperties(JSON_Object* rootObj, ADUC_RootKeyPackage* outPackage)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    JSON_Object* protectedPropertiesObj = NULL;

    if (rootObj == NULL || outPackage == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG;
        return result;
    }

    protectedPropertiesObj = json_object_get_object(rootObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_PROTECTED);
    if (protectedPropertiesObj == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY_PROTECTED;
        goto done;
    }

    result = RootKeyPackage_ParseIsTest(protectedPropertiesObj, outPackage);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = RootKeyPackage_ParseVersion(protectedPropertiesObj, outPackage);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = RootKeyPackage_ParsePublished(protectedPropertiesObj, outPackage);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = RootKeyPackage_ParseDisabledRootKeys(protectedPropertiesObj, outPackage);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = RootKeyPackage_ParseDisabledSigningKeys(protectedPropertiesObj, outPackage);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = RootKeyPackage_ParseRootKeys(protectedPropertiesObj, outPackage);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ERC %d parsing 'protected' property.", result.ResultCode);
    }

    return result;
}

ADUC_Result RootKeyPackage_ParseProtectedPropertiesString(JSON_Object* rootObj, ADUC_RootKeyPackage* outPackage)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    STRING_HANDLE protectedPropertiesStringHandle = NULL;
    JSON_Value* protectedPropertiesValue = NULL;
    char* protectedPropertiesString = NULL;

    if (rootObj == NULL || outPackage == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG;
        goto done;
    }

    protectedPropertiesValue = json_object_get_value(rootObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_PROTECTED);

    if (protectedPropertiesValue == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY_PROTECTED;
        goto done;
    }

    protectedPropertiesString = json_serialize_to_string(protectedPropertiesValue);

    if (protectedPropertiesString == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    protectedPropertiesStringHandle = STRING_construct(protectedPropertiesString);

    if (protectedPropertiesStringHandle == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
    outPackage->protectedPropertiesJsonString = protectedPropertiesStringHandle;
    protectedPropertiesStringHandle = NULL;

done:
    free(protectedPropertiesString);
    STRING_delete(protectedPropertiesStringHandle);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ERC %d parsing 'protected' property to string", result.ResultCode);
    }

    return result;
}
/**
 * @brief Parses the signatures properties in accordance with rootkeypackage.schema.json
 *
 * @param rootObj The root JSON object.
 * @param[out] outPackage The root key package object to write parsed signatures data.
 *
 * @return The result.
 */
ADUC_Result RootKeyPackage_ParseSignatures(JSON_Object* rootObj, ADUC_RootKeyPackage* outPackage)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    VECTOR_HANDLE signatures = NULL;

    JSON_Array* signaturesArray = json_object_get_array(rootObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_SIGNATURES);
    if (signaturesArray == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY_SIGNATURES;
        goto done;
    }

    size_t cnt = json_array_get_count(signaturesArray);
    if (cnt == 0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_SIGNATURES_EMPTY;
        goto done;
    }

    signatures = VECTOR_create(sizeof(ADUC_RootKeyPackage_Signature));
    if (signatures == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    for (size_t i = 0; i < cnt; ++i)
    {
        ADUC_RootKeyPackage_Signature signatureElement = { .alg = ADUC_RootKeySigningAlgorithm_INVALID,
                                                           .signature = NULL };
        ADUC_RootKeySigningAlgorithm tmpAlg = ADUC_RootKeySigningAlgorithm_INVALID;
        CONSTBUFFER_HANDLE signatureBuf = NULL;

        JSON_Object* hashJsonArrayElementObj = json_array_get_object(signaturesArray, i);
        if (hashJsonArrayElementObj == NULL)
        {
            result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_GETOBJ_SIGNATURES_ELEMENT;
            goto done;
        }

        result = RootKeyPackage_ParseSigningAlg(hashJsonArrayElementObj, &tmpAlg);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }

        result = RootKeyPackage_ParseBase64URLUIntJsonString(
            hashJsonArrayElementObj, ADUC_ROOTKEY_PACKAGE_PROPERTY_SIG, &signatureBuf);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }

        signatureElement.alg = tmpAlg;
        signatureElement.signature = signatureBuf;

        if (VECTOR_push_back(signatures, &signatureElement, 1) != 0)
        {
            // can't add to vector, so free it
            CONSTBUFFER_DecRef(signatureElement.signature);
            signatureElement.signature = NULL;

            result.ExtendedResultCode = ADUC_ERC_NOMEM;
            goto done;
        }
        memset(&signatureElement, 0, sizeof(signatureElement));
    }

    outPackage->signatures = signatures;
    signatures = NULL;

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (signatures != NULL)
    {
        size_t sig_cnt = VECTOR_size(signatures);
        for (size_t i = 0; i < sig_cnt; ++i)
        {
            ADUC_RootKeyPackage_Signature* node = (ADUC_RootKeyPackage_Signature*)VECTOR_element(signatures, i);
            ADUC_RootKeyPackage_Signature_DeInit(node);
        }

        VECTOR_destroy(signatures);
    }

    return result;
}

EXTERN_C_END
