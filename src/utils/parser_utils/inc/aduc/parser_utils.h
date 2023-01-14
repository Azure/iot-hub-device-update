/**
 * @file parser_utils.h
 * @brief Utilities for the Device Update Agent extensibility.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef PARSER_UTILS_H
#define PARSER_UTILS_H

#include "aduc/types/hash.h"
#include "aduc/types/update_content.h"
#include "parson.h"

EXTERN_C_BEGIN

// /**
//  * @brief Gets a string field from the update action JSON.
//  *
//  * @param updateActionJson The update action JSON.
//  * @param jsonFieldName The name of the JSON field to get.
//  * @param value The buffer to fill with the value from the JSON field. Caller must call free().
//  * @return bool true if call succeeded. false otherwise.
//  */
// bool ADUC_JSON_GetStringField(const JSON_Value* updateActionJson, const char* jsonFieldName, char** value);

// /**
//  * @brief Returns the pointer to the @p jsonFieldName from the JSON_Value
//  * @details if @p updateActionJson is free this value will become invalid
//  * @param updateActionJson The update action JSON
//  * @param jsonFieldName The name of the JSON field to get
//  * @return on success a pointer to the string value of @p jsonFieldName, on failure NULL
//  *
//  */
// const char* ADUC_JSON_GetStringFieldPtr(const JSON_Value* updateActionJson, const char* jsonFieldName);

/**
 * @brief Retrieves the updateManifest from the updateActionJson
 * @details Caller must free the returned JSON_Value with Parson's json_value_free
 * @param updateActionJson UpdateAction JSON to parse.
 * @return NULL in case of failure, the updateManifest JSON Value on success
 */
JSON_Value* ADUC_JSON_GetUpdateManifestRoot(const JSON_Value* updateActionJson);

/**
 * @brief Allocates memory and populate with ADUC_Hash object from a Parson JSON_Object..
 *
 * Caller MUST assume that this method allocates the memory for the hashHandle
 *
 * @param hashObj JSON Object that contains the hashes to be returned.
 * @param hashCount value where the count of output hashes will be stored.
 * @returns If success, a pointer to an array of ADUC_Hash object. Otherwise, returns NULL.
 */
ADUC_Hash* ADUC_HashArray_AllocAndInit(const JSON_Object* hashObj, size_t* hashCount);

/**
 * @brief Initializes the file entity
 * @param fileEntity the file entity to be initialized. Caller MUST zero-out before calling.
 * @param fileId fileId for @p fileEntity
 * @param targetFileName fileName for @p fileEntity
 * @param downloadUri downloadUri for @p fileEntity
 * @param arguments arguments for @p fileEntity (payload for down-level update handler)
 * @param hashArray a hash array for @p fileEntity
 * @param hashCount a hash count of @p hashArray
 * @param sizeInBytes file size (in bytes)
 * @returns True on success and false on failure
 */
bool ADUC_FileEntity_Init(
    ADUC_FileEntity* fileEntity,
    const char* fileId,
    const char* targetFileName,
    const char* downloadUri,
    const char* arguments,
    ADUC_Hash* hashArray,
    size_t hashCount,
    size_t sizeInBytes);

/**
 * @brief Free memory allocated for the specified ADUC_FileEntity object's member.
 *
 * @param entity File entity to be freed.
 */
void ADUC_FileEntity_Uninit(ADUC_FileEntity* entity);

/**
 * @brief Frees each FileEntity's members and the ADUC_FileEntity array
 * @param filecount the total number of files held within files
 * @param files a pointer to an array of ADUC_FileEntity objects
 */
void ADUC_FileEntityArray_Free(unsigned int fileCount, ADUC_FileEntity* files);

/**
 * @brief Free memory allocated for the specified ADUC_FileEntity object's member.
 *
 * @param entity File url to be freed.
 */
void ADUC_FileUrl_Uninit(ADUC_FileEntity* entity);

/**
 * @brief Frees each FileUrl's members and the ADUC_FileUrl array
 * @param filecount the total number of files held within files
 * @param files a pointer to an array of ADUC_FileUrl objects
 */
void ADUC_FileUrlArray_Free(unsigned int fileCount, ADUC_FileUrl* fileUrls);

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
bool ADUC_Json_GetUpdateId(const JSON_Value* updateActionJson, struct tagADUC_UpdateId** updateId);

/**
 * @brief Allocates and sets the UpdateId fields
 * @param provider the provider for the UpdateId
 * @param name the name for the UpdateId
 * @param version the version for the UpdateId
 *
 * @returns An UpdateId on success, NULL on failure
 */
ADUC_UpdateId* ADUC_UpdateId_AllocAndInit(const char* provider, const char* name, const char* version);

EXTERN_C_END

#endif // PARSER_UTILS_H
