/**
 * @file trust_utils.cpp
 * @brief Unit Tests for trust utils.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "trust_utils.h"
#include <aduc/hash_utils.h>
#include <aduc/logging.h>
#include <aduc/types/update_content.h>
#include <jws_utils.h>
#include <stdlib.h>

/**
 * @brief Helper function for checking the hash of the updatemanifest is equal to the
 * hash held within the signature
 * @param updateActionObject update action JSON object.
 * @returns true on success and false on failure
 */
static bool Json_ValidateManifestHash(const JSON_Object* updateActionObject)
{
    _Bool success = false;

    JSON_Value* signatureValue = NULL;
    char* jwtPayload = NULL;

    if (updateActionObject == NULL)
    {
        Log_Error("NULL updateActionObject");
        goto done;
    }

    const char* updateManifestStr = json_object_get_string(updateActionObject, ADUCITF_FIELDNAME_UPDATEMANIFEST);

    if (updateManifestStr == NULL)
    {
        Log_Error("No updateManifest field in updateActionJson ");
        goto done;
    }

    const char* updateManifestb64Signature =
        json_object_get_string(updateActionObject, ADUCITF_FIELDNAME_UPDATEMANIFESTSIGNATURE);
    if (updateManifestb64Signature == NULL)
    {
        Log_Error("No updateManifestSignature within the updateActionJson");
        goto done;
    }

    if (!GetPayloadFromJWT(updateManifestb64Signature, &jwtPayload))
    {
        Log_Error("Retrieving the payload from the manifest failed.");
        goto done;
    }

    signatureValue = json_parse_string(jwtPayload);
    if (signatureValue == NULL)
    {
        Log_Error("updateManifestSignature contains an invalid body");
        goto done;
    }

    const char* b64SignatureManifestHash =
        json_object_get_string(json_object(signatureValue), ADUCITF_JWT_FIELDNAME_HASH);
    if (b64SignatureManifestHash == NULL)
    {
        Log_Error("updateManifestSignature does not contain a hash value. Cannot validate the manifest!");
        goto done;
    }

    success = ADUC_HashUtils_IsValidBufferHash(
        (const uint8_t*)updateManifestStr, strlen(updateManifestStr), b64SignatureManifestHash, SHA256);

done:

    json_value_free(signatureValue);

    free(jwtPayload);
    return success;
}

/**
 * @brief Validates the update manifest signature.
 * @param updateActionObject The update action JSON object.
 *
 * @return ADUC_Result The result.
 */
ADUC_Result validate_update_manifest_signature(JSON_Object* updateActionObject)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    const char* manifestSignature = NULL;
    JWSResult jwsResult = JWSResult_Failed;

    if (updateActionObject == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_ERROR_BAD_PARAM;
        return result;
    }

    manifestSignature = json_object_get_string(updateActionObject, ADUCITF_FIELDNAME_UPDATEMANIFESTSIGNATURE);
    if (manifestSignature == NULL)
    {
        Log_Error("Invalid manifest. Does not contain a signature");
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_MANIFEST_VALIDATION_FAILED;
        goto done;
    }

    jwsResult = VerifyJWSWithSJWK(manifestSignature);
    if (jwsResult != JWSResult_Success)
    {
        if (jwsResult == JWSResult_DisallowedSigningKey)
        {
            Log_Error("Signing Key for the update metadata was on the disallowed signing key list");
            result.ExtendedResultCode = ADUC_ERC_ROOTKEY_SIGNING_KEY_IS_DISABLED;
        }
        else
        {
            result.ExtendedResultCode = MAKE_ADUC_EXTENDEDRESULTCODE_FOR_FACILITY_ADUC_FACILITY_INFRA_MGMT(
                ADUC_COMPONENT_JWS_UPDATE_MANIFEST_VALIDATION, jwsResult);
        }

        goto done;
    }

    if (!Json_ValidateManifestHash(updateActionObject))
    {
        // Handle failed hash case
        Log_Error("Json_ValidateManifestHash failed");
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_MANIFEST_VALIDATION_FAILED;
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
done:
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error(
            "Manifest signature validation failed with result: '%s' (%u). ERC: ADUC_COMPONENT_JWS_UPDATE_MANIFEST_VALIDATION",
            jws_result_to_str(jwsResult),
            jwsResult);
    }

    return result;
}
