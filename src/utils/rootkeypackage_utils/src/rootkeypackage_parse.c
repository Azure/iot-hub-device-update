
#include "aduc/rootkeypackage_parse.h"
#include <aduc/logging.h>
#include <aduc/string_c_utils.h> // for IsNullOrEmpty
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include <base64_utils.h> // for Base64URLDecode

EXTERN_C_BEGIN

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
    unsigned long version = 0;

    if (protectedPropertiesObj == NULL || outPackage == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG;
        goto done;
    }

    version = json_object_get_number(protectedPropertiesObj, "version");
    if (version == 0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY;
        goto done;
    }

    (outPackage->protectedProperties).version = version;
    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ERC %d parsing 'version' property.", result.ResultCode);
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

    published = json_object_get_number(protectedPropertiesObj, "published");
    if (published <= 0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY;
        goto done;
    }

    (outPackage->protectedProperties).publishedTime = published;
    result.ResultCode = ADUC_GeneralResult_Success;

done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ERC %d parsing 'published' property.", result.ResultCode);
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

    kidsArray = json_object_get_array(protectedPropertiesObj, "disabledRootKeys");
    if (kidsArray == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY;
        goto done;
    }

    count = json_array_get_count(kidsArray);
    if (count == 0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY;
        goto done;
    }

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
            STRING_HANDLE* h = (STRING_HANDLE*)VECTOR_element(kids, i);
            STRING_delete(*h);
        }
        VECTOR_destroy(kids);
    }

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ERC %d parsing 'disabledRootKeys' property.", result.ResultCode);
    }

    return result;
}

/**
 * @brief Parses "alg" SHA hash algorithm property.
 *
 * @param jsonObj The json object with the "alg" property.
 * @param outAlg The output parameter to hold the parsed algorithm upon success.
 * @return ADUC_Result The result.
 */
ADUC_Result RootKeyPackage_ParseShaHashAlg(JSON_Object* jsonObj, ADUC_RootKeyShaAlgorithm* outAlg)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    ADUC_RootKeyShaAlgorithm alg = ADUC_RootKeyShaAlgorithm_INVALID;
    JSON_Object* hashJsonArrayElementObj = NULL;
    const char* val = NULL;

    if (jsonObj == NULL || outAlg == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG;
        return result;
    }

    val = json_object_get_string(hashJsonArrayElementObj, "alg");
    if (val == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY;
        goto done;
    }

    if (strcmp(val, "SHA256") == 0)
    {
        alg = ADUC_RootKeyShaAlgorithm_SHA256;
    }
    else if (strcmp(val, "SHA384") == 0)
    {
        alg = ADUC_RootKeyShaAlgorithm_SHA384;
    }
    else if (strcmp(val, "SHA512") == 0)
    {
        alg = ADUC_RootKeyShaAlgorithm_SHA512;
    }
    else
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_UNSUPPORTED_HASH_ALGORITHM;
        goto done;
    }

    *outAlg = alg;
    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ERC %d parsing 'alg' property.", result.ResultCode);
    }

    return result;
}

/**
 * @brief Parses the SHA hash value from the JSON object.
 *
 * @param jsonObj The JSON object with the property that is the hash value.
 * @param propertyName The property name for the hash value, such as "hash", or "sig".
 * @param outHashBuffer The output parameter const buffer handle for holding the binary hash data.
 * @return ADUC_Result The result.
 */
ADUC_Result
RootKeyPackage_ParseShaHashValue(JSON_Object* jsonObj, const char* propertyName, CONSTBUFFER_HANDLE* outHashBuffer)
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
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY;
        goto done;
    }

    out_len = Base64URLDecode(val, &buf);
    if (out_len == 0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_INVALID_ENCODING;
        goto done;
    }

    buffer = CONSTBUFFER_CreateWithMoveMemory(buf, out_len);
    if (buffer == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }
    buf = NULL;

    *outHashBuffer = buffer;
    buffer = NULL;

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (buf != NULL)
    {
        free(buf);
    }

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

    hashesArray = json_object_get_array(protectedPropertiesObj, "disabledSigningKeys");
    if (hashesArray == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY;
        goto done;
    }

    count = json_array_get_count(hashesArray);
    if (count == 0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY;
        goto done;
    }

    hashes = VECTOR_create(sizeof(ADUC_RootKeyPackage_Hash));
    if (hashes == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    for (size_t i = 0; i < count; ++i)
    {
        ADUC_RootKeyPackage_Hash hashElement = { .alg = ADUC_RootKeyShaAlgorithm_INVALID, .hash = NULL };
        ADUC_RootKeyShaAlgorithm tmpAlg = ADUC_RootKeyShaAlgorithm_INVALID;
        CONSTBUFFER_HANDLE hashBuf = NULL;

        JSON_Object* hashJsonArrayElementObj = json_array_get_object(hashesArray, i);
        if (hashJsonArrayElementObj == NULL)
        {
            result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_PARSE_MISSING_REQUIRED_PROPERTY;
            goto done;
        }

        result = RootKeyPackage_ParseShaHashAlg(hashJsonArrayElementObj, &tmpAlg);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }

        result = RootKeyPackage_ParseShaHashValue(hashJsonArrayElementObj, "hash", &hashBuf);
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            goto done;
        }

        hashElement.alg = tmpAlg;
        hashElement.hash = hashBuf;

        if (VECTOR_push_back(hashes, &hashElement, 1) != 0)
        {
            // can't add to vector, so free it
            CONSTBUFFER_DecRef(hashElement.hash);
            hashElement.hash = NULL;

            result.ExtendedResultCode = ADUC_ERC_NOMEM;
            goto done;
        }
        memset(&hashElement, 0, sizeof(hashElement));
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
            if (node != NULL && node->hash != NULL)
            {
                CONSTBUFFER_DecRef(node->hash);
            }
        }

        VECTOR_destroy(hashes);
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

    protectedPropertiesObj = json_object_get_object(rootObj, "protected");
    if (protectedPropertiesObj == NULL)
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

    JSON_Array* signaturesArray = json_object_get_array(rootObj, "signatures");
    if (signaturesArray == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_UTIL_ERROR_BAD_ARG;
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    return result;
}

EXTERN_C_END
