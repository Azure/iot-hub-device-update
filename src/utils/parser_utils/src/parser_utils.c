/**
 * @file parser_utils.c
 * @brief Implements utilities for parsing the common data types.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/parser_utils.h"
#include "aduc/hash_utils.h"
#include "aduc/logging.h"
#include "parson_json_utils.h"

#include <azure_c_shared_utility/crt_abstractions.h> // for mallocAndStrcpy_s
#include <stdlib.h> // for calloc

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
        return NULL;
    }

    JSON_Value* updateManifestRoot = json_parse_string(manifestString);
    free(manifestString);
    return updateManifestRoot;
}

/**
 * @brief Allocates memory and populate with ADUC_Hash object from a Parson JSON_Object.
 *
 * Caller MUST assume that this method allocates the memory for the returned ADUC_Hash pointer.
 *
 * @param hashObj JSON Object that contains the hashes to be returned.
 * @param hashCount A size_t* where the count of output hashes will be stored.
 * @returns If success, a pointer to an array of ADUC_Hash object. Otherwise, returns NULL.
 *  Caller must call ADUC_FileEntityArray_Free() to free the array.
 */
ADUC_Hash* ADUC_HashArray_AllocAndInit(const JSON_Object* hashObj, size_t* hashCount)
{
    bool success = false;

    ADUC_Hash* tempHashArray = NULL;

    if (hashCount == NULL)
    {
        return false;
    }
    *hashCount = 0;

    size_t tempHashCount = json_object_get_count(hashObj);

    if (tempHashCount == 0)
    {
        Log_Error("No hashes.");
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
 * @brief Free memory allocated for the specified ADUC_FileEntity object's member.
 *
 * @param entity File entity to be freed.
 */
void ADUC_FileEntity_Uninit(ADUC_FileEntity* entity)
{
    if (entity == NULL)
    {
        return;
    }
    free(entity->DownloadUri);
    free(entity->TargetFilename);
    free(entity->FileId);
    free(entity->Arguments);
    ADUC_Hash_FreeArray(entity->HashCount, entity->Hash);
    memset(entity, 0, sizeof(*entity));
}

/**
 * @brief Free ADUC_FileEntity object array.
 *
 * Caller should assume files object is invalid after this method returns.
 *
 * @param fileCount Count of objects in files.
 * @param files Array of ADUC_FileEntity objects to free.
 */
void ADUC_FileEntityArray_Free(unsigned int fileCount, ADUC_FileEntity* files)
{
    for (unsigned int index = 0; index < fileCount; ++index)
    {
        ADUC_FileEntity_Uninit(files + index);
    }

    free(files);
}

/**
 * @brief Initializes the file entity
 * @param file the file entity to be initialized
 * @param fileId fileId for @p fileEntity
 * @param targetFileName fileName for @p fileEntity
 * @param downloadUri downloadUri for @p fileEntity
 * @param arguments arguments for @p fileEntity (payload for down-level update handler)
 * @param hashArray a hash array for @p fileEntity
 * @param hashCount a hash count of @p hashArray
 * @param sizeInBytes file size (in bytes)
 * @returns True on success and false on failure
 * @details All strings and hashArray are deep-copied. hashArray ownership is not transferred.
 */
bool ADUC_FileEntity_Init(
    ADUC_FileEntity* fileEntity,
    const char* fileId,
    const char* targetFileName,
    const char* downloadUri,
    const char* arguments,
    ADUC_Hash* hashArray,
    size_t hashCount,
    size_t sizeInBytes)
{
    bool success = false;
    ADUC_Hash* tempHashArray = NULL;

    if (fileEntity == NULL)
    {
        return false;
    }

    // Note: downloadUri could be empty when the agent resuming 'install' or 'apply' action.
    if (fileId == NULL || targetFileName == NULL || hashArray == NULL)
    {
        return false;
    }

    memset(fileEntity, 0, sizeof(*fileEntity));

    if (mallocAndStrcpy_s(&(fileEntity->FileId), fileId) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(fileEntity->TargetFilename), targetFileName) != 0)
    {
        goto done;
    }

    if (downloadUri == NULL)
    {
        fileEntity->DownloadUri = NULL;
    }
    else if (mallocAndStrcpy_s(&(fileEntity->DownloadUri), downloadUri) != 0)
    {
        goto done;
    }

    if (arguments != NULL && mallocAndStrcpy_s(&(fileEntity->Arguments), arguments) != 0)
    {
        goto done;
    }

    // Make a deep copy of hashArray
    tempHashArray = (ADUC_Hash*)calloc(hashCount, sizeof(*hashArray));
    if (tempHashArray == NULL)
    {
        goto done;
    }

    for (size_t i = 0; i < hashCount; ++i)
    {
        if (!ADUC_Hash_Init(&tempHashArray[i], hashArray[i].value, hashArray[i].type))
        {
            goto done;
        }
    }

    fileEntity->Hash = tempHashArray;
    tempHashArray = NULL;

    fileEntity->HashCount = hashCount;

    fileEntity->SizeInBytes = sizeInBytes;

    success = true;

done:
    ADUC_Hash_FreeArray(hashCount, tempHashArray);

    if (!success)
    {
        ADUC_FileEntity_Uninit(fileEntity);
    }

    return success;
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
 * @return bool True if call was successful.
 */
bool ADUC_Json_GetUpdateId(const JSON_Value* updateActionJson, ADUC_UpdateId** updateId)
{
    bool success = false;
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
        ADUC_UpdateId_UninitAndFree(tempUpdateID);
        tempUpdateID = NULL;
    }

    json_value_free(updateManifestValue);

    *updateId = tempUpdateID;
    return success;
}

/**
 * @brief Allocates and sets the UpdateId fields
 * @details Caller should free the allocated ADUC_UpdateId* using ADUC_UpdateId_UninitAndFree()
 * @param provider the provider for the UpdateId
 * @param name the name for the UpdateId
 * @param version the version for the UpdateId
 *
 * @returns An UpdateId on success, NULL on failure
 */
ADUC_UpdateId* ADUC_UpdateId_AllocAndInit(const char* provider, const char* name, const char* version)
{
    bool success = false;
    ADUC_UpdateId* updateId = NULL;

    if (provider == NULL || name == NULL || version == NULL)
    {
        Log_Error("Invalid call");
        return NULL;
    }

    updateId = (ADUC_UpdateId*)calloc(1, sizeof(ADUC_UpdateId));
    if (updateId == NULL)
    {
        Log_Error("ADUC_UpdateId_AllocAndInit called with a NULL updateId handle");
        goto done;
    }

    if (mallocAndStrcpy_s(&(updateId->Provider), provider) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(updateId->Name), name) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(updateId->Version), version) != 0)
    {
        goto done;
    }

    success = true;

done:

    if (!success)
    {
        ADUC_UpdateId_UninitAndFree(updateId);
        updateId = NULL;
    }

    return updateId;
}
