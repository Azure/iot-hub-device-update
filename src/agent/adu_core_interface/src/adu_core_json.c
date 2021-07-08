/**
 * @file adu_core_json.c
 * @brief Defines types and methods required to parse JSON from the urn:azureiot:AzureDeviceUpdateCore:1 interface.
 *
 * Note that the types in this file must be kept in sync with the AzureDeviceUpdateCore interface.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#include <stdio.h>
#include <stdlib.h>

#include "aduc/adu_core_export_helpers.h"
#include "aduc/adu_core_interface.h"
#include "aduc/hash_utils.h"
#include <aduc/c_utils.h>
#include <aduc/logging.h>
#include <jws_utils.h>

_Bool ADUC_JSON_GetStringField(const JSON_Value* updateActionJson, const char* jsonFieldName, char** value);

const char* ADUC_JSON_GetStringFieldPtr(const JSON_Value* updateActionJson, const char* jsonFieldName);

_Bool ADUC_JSON_GetUpdateManifestStringField(
    const JSON_Value* updateActionJson, const char* jsonFieldName, char** value);

JSON_Value* ADUC_JSON_GetUpdateManifestRoot(const JSON_Value* updateActionJson);

/**
 * @brief Convert UpdateState to string representation.
 *
 * @param updateState State to convert.
 * @return const char* String representation.
 */
const char* ADUCITF_StateToString(ADUCITF_State updateState)
{
    switch (updateState)
    {
    case ADUCITF_State_Idle:
        return "Idle";
    case ADUCITF_State_DownloadStarted:
        return "DownloadStarted";
    case ADUCITF_State_DownloadSucceeded:
        return "DownloadSucceeded";
    case ADUCITF_State_InstallStarted:
        return "InstallStarted";
    case ADUCITF_State_InstallSucceeded:
        return "InstallSucceeded";
    case ADUCITF_State_ApplyStarted:
        return "ApplyStarted";
    case ADUCITF_State_Failed:
        return "Failed";
    }

    return "<Unknown>";
}

/**
 * @brief Convert UpdateAction to string representation.
 *
 * @param updateAction Action to convert.
 * @return const char* String representation.
 */
const char* ADUCITF_UpdateActionToString(ADUCITF_UpdateAction updateAction)
{
    switch (updateAction)
    {
    case ADUCITF_UpdateAction_Download:
        return "Download";
    case ADUCITF_UpdateAction_Install:
        return "Install";
    case ADUCITF_UpdateAction_Apply:
        return "Apply";
    case ADUCITF_UpdateAction_Cancel:
        return "Cancel";
    }

    return "<Unknown>";
}

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
        if (updateActionJson != NULL)
        {
            json_value_free(updateActionJson);
            updateActionJson = NULL;
        }
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
    if (!json_object_has_value_of_type(root_object, ADUCITF_FIELDNAME_ACTION, JSONNumber))
    {
        Log_Error("Invalid json - '%s' missing or incorrect", ADUCITF_FIELDNAME_ACTION);
        goto done;
    }

    *updateAction = (unsigned)json_object_get_number(root_object, ADUCITF_FIELDNAME_ACTION);

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
 * @brief Parse the update action JSON for the UpdateId value.
 *
 * Sample JSON:
 * {
 *      "updateManifest":"{
 *      ...
 *      \"updateId\": {
 *          \"provider\": \"Azure\",
 *          \"name\":\"IOT-Firmware\",
 *          \"version\":\"1.2.0.0\"
 *      },
 *      ...
 *     }",
 *
 * }
 *
 * @param updateActionJson UpdateAction JSON to parse.
 * @param updateId The returned installed content ID string. Caller must call free().
 * @return _Bool True if call was successful.
 */
_Bool ADUC_Json_GetUpdateId(const JSON_Value* updateActionJson, ADUC_UpdateId** updateId)
{
    _Bool success = false;
    ADUC_UpdateId* tempUpdateID = NULL;

    *updateId = NULL;

    JSON_Value* updateManifestValue = ADUC_JSON_GetUpdateManifestRoot(updateActionJson);

    if (updateManifestValue == NULL)
    {
        Log_Error("updateManifest JSON is invalid");
        goto done;
    }

    JSON_Object* updateManifestObj = json_value_get_object(updateManifestValue);

    if (updateManifestObj == NULL)
    {
        Log_Error("updateManifestValue is not a JSON Object");
        goto done;
    }

    JSON_Value* updateIdValue = json_object_get_value(updateManifestObj, ADUCITF_FIELDNAME_UPDATEID);

    if (updateIdValue == NULL)
    {
        Log_Error("updateActionJson's updateManifest does not include an updateid field");
        goto done;
    }

    const char* provider = ADUC_JSON_GetStringFieldPtr(updateIdValue, ADUCITF_FIELDNAME_PROVIDER);
    const char* name = ADUC_JSON_GetStringFieldPtr(updateIdValue, ADUCITF_FIELDNAME_NAME);
    const char* version = ADUC_JSON_GetStringFieldPtr(updateIdValue, ADUCITF_FIELDNAME_VERSION);

    if (provider == NULL || name == NULL || version == NULL)
    {
        Log_Error("Invalid json. Missing required UpdateID fields");
        goto done;
    }

    tempUpdateID = ADUC_UpdateId_AllocAndInit(provider, name, version);
    if (tempUpdateID == NULL)
    {
        goto done;
    }

    success = true;

done:
    if (!success)
    {
        ADUC_UpdateId_Free(tempUpdateID);
        tempUpdateID = NULL;
    }

    json_value_free(updateManifestValue);

    *updateId = tempUpdateID;
    return success;
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
 * @brief Gets a string field from the update action JSON.
 *
 * @param updateActionJson The update action JSON.
 * @param jsonFieldName The name of the JSON field to get.
 * @param value The buffer to fill with the value from the JSON field. Caller must call free().
 * @return _Bool true if call succeeded. false otherwise.
 */
_Bool ADUC_JSON_GetStringField(const JSON_Value* updateActionJson, const char* jsonFieldName, char** value)
{
    _Bool succeeded = false;

    *value = NULL;

    JSON_Object* root_object = json_value_get_object(updateActionJson);
    if (root_object == NULL)
    {
        goto done;
    }

    const char* value_val = json_object_get_string(root_object, jsonFieldName);
    if (value_val == NULL)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(value, value_val) != 0)
    {
        goto done;
    }

    succeeded = true;

done:
    if (!succeeded)
    {
        if (*value != NULL)
        {
            free(*value);
            *value = NULL;
        }
    }

    return succeeded;
}

/**
 * @brief Returns the pointer to the @p jsonFieldName from the JSON_Value
 * @details if @p updateActionJson is free this value will become invalid
 * @param updateActionJson The update action JSON
 * @param jsonFieldName The name of the JSON field to get
 * @return on success a pointer to the string value of @p jsonFieldName, on failure NULL
 *
 */
const char* ADUC_JSON_GetStringFieldPtr(const JSON_Value* updateActionJson, const char* jsonFieldName)
{
    JSON_Object* object = json_value_get_object(updateActionJson);

    if (object == NULL)
    {
        return NULL;
    }

    return json_object_get_string(object, jsonFieldName);
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

/**
 * @brief Retrieves the updateManifest from the updateActionJson
 * @details Caller must free the returned JSON_Value with Parson's json_value_free
 * @param updateActionJson UpdateAction JSON to parse.
 * @return NULL in case of failure, the updateManifest JSON Value on success
 */
JSON_Value* ADUC_JSON_GetUpdateManifestRoot(const JSON_Value* updateActionJson)
{
    char* manifestString = NULL;
    if (!ADUC_JSON_GetStringField(updateActionJson, ADUCITF_FIELDNAME_UPDATEMANIFEST, &manifestString))
    {
        Log_Error("updateActionJson does not include an updateManifest field");
        return false;
    }

    JSON_Value* updateManifestRoot = json_parse_string(manifestString);
    free(manifestString);
    return updateManifestRoot;
}

/**
 * @brief Initializes the hashes contained within @p hashObj.
 *
 * Caller MUST assume that this method allocates the memory for the hashHandle
 *
 * @param hashObj JSON Object that contains the hashes to be loaded into @p hashArray
 * @param hashArray handle to be initialized with the hashes within the @p hashObj
 * @param hashCount value where the count of hashes within @p hashArray will be stored.
 * @returns True on success, false on failure
 */
ADUC_Hash* ADUC_HashArray_AllocAndInit(const JSON_Object* hashObj, size_t* hashCount)
{
    _Bool success = false;

    ADUC_Hash* tempHashArray = NULL;

    if (hashCount == NULL)
    {
        return false;
    }
    *hashCount = 0;

    size_t tempHashCount = json_object_get_count(hashObj);

    if (tempHashCount == 0)
    {
        Log_Error("No hashes present, not a valid file");
        goto done;
    }

    tempHashArray = (ADUC_Hash*)calloc(tempHashCount, sizeof(ADUC_Hash));

    if (tempHashArray == NULL)
    {
        goto done;
    }

    for (size_t hash_index = 0; hash_index < tempHashCount; ++hash_index)
    {
        ADUC_Hash* currHash = tempHashArray + hash_index;

        const char* hashType = json_object_get_name(hashObj, hash_index);
        const char* hashValue = json_value_get_string(json_object_get_value_at(hashObj, hash_index));
        if (!ADUC_Hash_Init(currHash, hashValue, hashType))
        {
            goto done;
        }
    }

    *hashCount = tempHashCount;

    success = true;

done:

    if (!success)
    {
        ADUC_Hash_FreeArray(tempHashCount, tempHashArray);
        tempHashArray = NULL;
        tempHashCount = 0;
    }

    *hashCount = tempHashCount;

    return tempHashArray;
}

/**
 * @brief Initializes the file entity
 * @param file the file entity to be initialized
 * @param fileId fileId for @p file
 * @param targetFileName fileName for @p file
 * @param downloadUri downlaodUri for @p file
 * @returns True on success and false on failure
 */
_Bool ADUC_FileEntity_Init(
    ADUC_FileEntity* file,
    const char* fileId,
    const char* targetFileName,
    const char* downloadUri,
    ADUC_Hash* hashArray,
    size_t hashCount)
{
    _Bool success = false;

    if (file == NULL)
    {
        return false;
    }

    if (fileId == NULL || targetFileName == NULL || downloadUri == NULL || hashArray == NULL)
    {
        return false;
    }

    memset(file, 0, sizeof(*file));

    if (mallocAndStrcpy_s(&(file->FileId), fileId) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(file->TargetFilename), targetFileName) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(file->DownloadUri), downloadUri) != 0)
    {
        goto done;
    }

    file->Hash = hashArray;

    file->HashCount = hashCount;

    success = true;
done:

    if (!success)
    {
        free(file->FileId);
        free(file->TargetFilename);
        free(file->DownloadUri);
    }
    return success;
}

/**
 * @brief Parse the update action JSON into a ADUC_FileEntity structure.
 *
 * Sample JSON:
 *
 * {
 *     ...,
 *     "files": {
 *         "0001": {
 *              "fileName": "filename",
 *              "sizeInBytes": "1024",
 *              "hashes":{
 *                  "sha256": "base64_encoded_hash_value"
 *               }
 *          },
 *         ...
 *      },
 *      ...
 *      "fileUrls": {
 *          "0001": "uri1"
 *       },
 *       ...
 * }
 *
 * @param updateActionJson UpdateAction Json to parse
 * @param fileCount Returned number of files.
 * @param files ADUC_FileEntity (size fileCount). Array to be freed using free(), objects must also be freed.
 * @return _Bool Success state.
 */
_Bool ADUC_Json_GetFiles(const JSON_Value* updateActionJson, unsigned int* fileCount, ADUC_FileEntity** files)
{
    _Bool succeeded = false;

    // Verify arguments
    if ((fileCount == NULL) || (files == NULL))
    {
        return false;
    }

    *fileCount = 0;
    *files = NULL;

    JSON_Value* updateManifestValue = ADUC_JSON_GetUpdateManifestRoot(updateActionJson);

    if (updateManifestValue == NULL)
    {
        goto done;
    }

    const JSON_Object* updateManifest = json_value_get_object(updateManifestValue);
    const JSON_Object* files_object = json_object_get_object(updateManifest, ADUCITF_FIELDNAME_FILES);

    if (files_object == NULL)
    {
        Log_Error("Invalid json - '%s' missing or incorrect", ADUCITF_FIELDNAME_FILES);
        goto done;
    }

    const size_t files_count = json_object_get_count(files_object);
    if (files_count == 0)
    {
        Log_Error("Invalid json Files count");
        goto done;
    }

    const JSON_Object* updateActionJsonObject = json_value_get_object(updateActionJson);
    const JSON_Object* file_url_object = json_object_get_object(updateActionJsonObject, ADUCITF_FIELDNAME_FILE_URLS);
    const size_t file_url_count = json_object_get_count(file_url_object);

    if (file_url_count == 0)
    {
        Log_Error("Invalid Files URL count");
        goto done;
    }

    if (file_url_count != files_count)
    {
        Log_Error("Json Files count does not match Files URL count");
        goto done;
    }

    *files = calloc(files_count, sizeof(ADUC_FileEntity));
    if (*files == NULL)
    {
        goto done;
    }

    *fileCount = files_count;

    for (size_t index = 0; index < files_count; ++index)
    {
        ADUC_FileEntity* cur_file = *files + index;

        const JSON_Object* file_obj = json_value_get_object(json_object_get_value_at(files_object, index));
        const JSON_Object* hash_obj = json_object_get_object(file_obj, ADUCITF_FIELDNAME_HASHES);

        if (hash_obj == NULL)
        {
            Log_Error("No hash for file @ %zu", index);
            goto done;
        }
        size_t tempHashCount = 0;
        ADUC_Hash* tempHash = ADUC_HashArray_AllocAndInit(hash_obj, &tempHashCount);
        if (tempHash == NULL)
        {
            Log_Error("Unable to parse hashes for file @ %zu", index);
            goto done;
        }

        const char* uri = json_value_get_string(json_object_get_value_at(file_url_object, index));
        const char* fileId = json_object_get_name(files_object, index);
        const char* name = json_object_get_string(file_obj, ADUCITF_FIELDNAME_FILENAME);

        if (!ADUC_FileEntity_Init(cur_file, fileId, name, uri, tempHash, tempHashCount))
        {
            ADUC_Hash_FreeArray(tempHashCount, tempHash);
            Log_Error("Invalid file arguments");
            goto done;
        }
    }

    succeeded = true;

done:
    if (!succeeded)
    {
        ADUC_FileEntityArray_Free(*fileCount, *files);
        *files = NULL;
        *fileCount = 0;
    }

    json_value_free(updateManifestValue);

    return succeeded;
}