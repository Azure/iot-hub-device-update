/**
 * @file adu_core_json.c
 * @brief Defines types and methods required to parse JSON from the urn:azureiot:AzureDeviceUpdateCore:1 interface.
 *
 * Note that the types in this file must be kept in sync with the AzureDeviceUpdateCore interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <stdio.h>
#include <stdlib.h>

#include "aduc/adu_core_export_helpers.h"
#include "aduc/adu_core_interface.h"
#include "aduc/c_utils.h"
#include "aduc/hash_utils.h"
#include "aduc/logging.h"
#include "aduc/parser_utils.h"
#include "aduc/types/workflow.h"
#include "aduc/agent_workflow.h"
#include <jws_utils.h>
#include <parson_json_utils.h>

_Bool ADUC_JSON_GetUpdateManifestStringField(
    const JSON_Value* updateActionJson, const char* jsonFieldName, char** value);

/**
 * @brief Do (some) UpdateAction JSON validation and return root object.
 * @param updateActionJsonString  JSON string containing information about the UpdateAction.
 * @return JSON_Value* NULL on error.
 */
JSON_Value* ADUC_Json_GetRoot(const char* updateActionJsonString)
{
    _Bool succeeded = false;

    JSON_Value* updateActionJson = json_parse_string(updateActionJsonString);
    if (updateActionJson == NULL)
    {
        Log_Error("Invalid json root. JSON: %s", updateActionJsonString);
        goto done;
    }

    if (json_value_get_type(updateActionJson) != JSONObject)
    {
        Log_Error("Invalid json root type. JSON: %s", updateActionJsonString);
        goto done;
    }

    succeeded = true;

done:
    if (!succeeded)
    {
        json_value_free(updateActionJson);
        updateActionJson = NULL;
    }

    return updateActionJson;
}

/**
 * @brief Helper function that extracts the signature, validates it, and then stores the hash from the body of the signature in the hashValue
 * @param updateActionJson JSON Value created from an updateActionJsonString
 * @returns false on failure to validate and true when valid
 */
_Bool ADUC_Json_ValidateManifestSignature(const JSON_Value* updateActionJson)
{
    _Bool success = false;

    if (updateActionJson == NULL)
    {
        Log_Error("updateActionJson passed to ADUC_Json_ValidateManifestSignature is NULL");
        goto done;
    }

    const char* manifestSignature =
        ADUC_JSON_GetStringFieldPtr(updateActionJson, ADUCITF_FIELDNAME_UPDATEMANIFESTSIGNATURE);
    if (manifestSignature == NULL)
    {
        Log_Error("Invalid manifest. Does not contain a signature");
        goto done;
    }

    JWSResult result = VerifyJWSWithSJWK(manifestSignature);
    if (result != JWSResult_Success)
    {
        Log_Error("Manifest signature validation failed with result: %u", result);
        goto done;
    }

    success = true;

done:
    return success;
}

/**
 * @brief Helper function for checking the hash of the updatemanifest is equal to the
 * hash held within the signature
 * @param updateActionJson JSON value created form an updateActionJsonString and contains
 * both the updateManifest and the updateManifestSignature
 * @returns true on success and false on failure
 */
_Bool ADUC_Json_ValidateManifestHash(const JSON_Value* updateActionJson)
{
    _Bool success = false;

    JSON_Value* signatureValue = NULL;
    char* jwtPayload = NULL;

    if (updateActionJson == NULL)
    {
        Log_Error("updateActionJson passed to ADUC_Json_ValidateManifestHash is NULL");
        goto done;
    }

    const char* updateManifestStr = ADUC_JSON_GetStringFieldPtr(updateActionJson, ADUCITF_FIELDNAME_UPDATEMANIFEST);

    if (updateManifestStr == NULL)
    {
        Log_Error("No updateManifest field in updateActionJson ");
        goto done;
    }

    const char* updateManifestb64Signature =
        ADUC_JSON_GetStringFieldPtr(updateActionJson, ADUCITF_FIELDNAME_UPDATEMANIFESTSIGNATURE);
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

    signatureValue = ADUC_Json_GetRoot(jwtPayload);
    if (signatureValue == NULL)
    {
        Log_Error("updateManifestSignature contains an invalid body");
        goto done;
    }

    const char* b64SignatureManifestHash = ADUC_JSON_GetStringFieldPtr(signatureValue, ADUCITF_JWT_FIELDNAME_HASH);
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
 * @brief Do validation of the manifest within the UpdateActionJson .
 * @details Validates the base64 encoded updateManifestSignature and compares the hash of the updateManifest and the expected hash contained within the updateManifestSignature
 * @param updateActionJsonString JSON Value that should contain the updateManifest and updateManifestSignature
 * @return True if a valid manifest and false if not
 */
_Bool ADUC_Json_ValidateManifest(const JSON_Value* updateActionJson)
{
    _Bool succeeded = false;

    if (!ADUC_Json_ValidateManifestSignature(updateActionJson))
    {
        // Handle failed signature case
        Log_Error("ADUC_Json_ValidateManifestSignature failed");
        goto done;
    }

    if (!ADUC_Json_ValidateManifestHash(updateActionJson))
    {
        // Handle failed hash case
        Log_Error("ADUC_Json_ValidateManifestHash failed");
        goto done;
    }

    succeeded = true;
done:
    return succeeded;
}

/**
 * @brief Parse the update action JSON for the UpdateAction value.
 *
 * Sample JSON:
 *
 * {
 *     "action": 0,
 *     ...
 * }
 *
 *
 * @param updateActionJson UpdateAction Json to parse
 * @param updateAction Found value on success.
 * @return Bool Success state.
 */
_Bool ADUC_Json_GetUpdateAction(const JSON_Value* updateActionJson, unsigned* updateAction)
{
    _Bool succeeded = false;

    *updateAction = 0;

    JSON_Object* root_object = json_value_get_object(updateActionJson);
    if (root_object == NULL)
    {
        goto done;
    }

    // parson library will return 0 if get_number fails, so first check that the type is correct.
    if (!json_object_dothas_value_of_type(root_object, ADUCITF_FIELDNAME_WORKFLOW_DOT_ACTION, JSONNumber))
    {
        Log_Error("Invalid json - '%s' missing or incorrect", ADUCITF_FIELDNAME_WORKFLOW_DOT_ACTION);
        goto done;
    }

    *updateAction = (unsigned)json_object_dotget_number(root_object, ADUCITF_FIELDNAME_WORKFLOW_DOT_ACTION);

    succeeded = true;

done:
    return succeeded;
}

/**
 * @brief Parse the update action JSON for installedCriteria value.
 *
 * Sample JSON:
 * {
 *      "installedCriteria": "1.0.0.0"
 * }
 *
 * @param updateActionJson UpdateAction JSON to parse.
 * @param installedCriteria The returned installed criteria string. Caller must call free().
 * @return _Bool True if call was successful.
 */
_Bool ADUC_Json_GetInstalledCriteria(const JSON_Value* updateActionJson, char** installedCriteria)
{
    return ADUC_JSON_GetUpdateManifestStringField(
        updateActionJson, ADUCITF_FIELDNAME_INSTALLEDCRITERIA, installedCriteria);
}

/**
 * @brief Retrieves the updateType from @p updateActionJson's updateManifest which is stored in updateTypeStr
 * @param updateActionJson UpdateAction JSON to parse
 * @param updateTypeStr string to store the updateType in
 * @returns True on success, False on failure
 */
_Bool ADUC_Json_GetUpdateType(const JSON_Value* updateActionJson, char** updateTypeStr)
{
    return ADUC_JSON_GetUpdateManifestStringField(updateActionJson, ADUCITF_FIELDNAME_UPDATETYPE, updateTypeStr);
}

/**
 * @brief Gets a string field from the updateManifest JSON within the update action JSON
 *
 * @param updateActionJson The update Action JSON
 * @param jsonFieldName The name of the updateManifest JSON field to get
 * @param value The buffer to fill with the value from the JSON field. Caller must call free()
 *
 * @return _Bool true if call succeeded. False otherwise
 */
_Bool ADUC_JSON_GetUpdateManifestStringField(
    const JSON_Value* updateActionJson, const char* jsonFieldName, char** value)
{
    _Bool success = false;

    *value = NULL;

    JSON_Value* updateManifestValue = ADUC_JSON_GetUpdateManifestRoot(updateActionJson);

    if (updateManifestValue == NULL)
    {
        goto done;
    }

    if (!ADUC_JSON_GetStringField(updateManifestValue, jsonFieldName, value))
    {
        goto done;
    }

    success = true;

done:

    if (!success)
    {
        free(*value);
        *value = NULL;
    }

    json_value_free(updateManifestValue);

    return success;
}
