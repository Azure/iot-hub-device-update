/**
 * @file workflow_utils.c
 * @brief Utility functions for workflow data.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/workflow_utils.h"
#include "aduc/adu_types.h"
#include "aduc/c_utils.h"
#include "aduc/extension_manager.h"
#include "aduc/hash_utils.h"
#include "aduc/logging.h"
#include "aduc/parser_utils.h"
#include "aduc/result.h"
#include "aduc/string_c_utils.h"
#include "aduc/system_utils.h"
#include "aduc/types/update_content.h"
#include "aduc/types/workflow.h"
#include "aduc/workflow_internal.h"
#include "jws_utils.h"

#include <azure_c_shared_utility/crt_abstractions.h> // for mallocAndStrcpy_s
#include <azure_c_shared_utility/strings.h> // for STRING_*
#include <parson.h>
#include <stdarg.h> // for va_*
#include <stdlib.h> // for calloc, atoi
#include <string.h>
#include <strings.h> // for strcasecmp

#define DEFAULT_SANDBOX_ROOT_PATH ADUC_DOWNLOADS_FOLDER

#define WORKFLOW_PROPERTY_FIELD_ID "_id"
#define WORKFLOW_PROPERTY_FIELD_RETRYTIMESTAMP "_retryTimestamp"
#define WORKFLOW_PROPERTY_FIELD_WORKFLOW_DOT_ID "workflow.id"
#define WORKFLOW_PROPERTY_FIELD_WORKFLOW_DOT_RETRYTIMESTAMP "workflow.retryTimestamp"
#define WORKFLOW_PROPERTY_FIELD_WORKFLOW_DOT_ACTION "workflow.action"
#define WORKFLOW_PROPERTY_FIELD_SANDBOX_ROOTPATH "_sandboxRootPath"
#define WORKFLOW_PROPERTY_FIELD_WORKFOLDER "_workFolder"
#define WORKFLOW_PROPERTY_FIELD_CANCEL_REQUESTED "_cancelRequested"
#define WORKFLOW_PROPERTY_FIELD_REBOOT_REQUESTED "_rebootRequested"
#define WORKFLOW_PROPERTY_FIELD_IMMEDIATE_REBOOT_REQUESTED "_immediateRebootRequested"
#define WORKFLOW_PROPERTY_FIELD_AGENT_RESTART_REQUESTED "_agentRestartRequested"
#define WORKFLOW_PROPERTY_FIELD_IMMEDIATE_AGENT_RESTART_REQUESTED "_immediateAgentRestartRequested"
#define WORKFLOW_PROPERTY_FIELD_SELECTED_COMPONENTS "_selectedComponents"

// V4 and later.
#define DEAULT_STEP_TYPE "reference"
#define WORKFLOW_PROPERTY_FIELD_INSTRUCTIONS_DOT_STEPS "instructions.steps"
#define UPDATE_MANIFEST_PROPERTY_FIELD_DETACHED_MANIFEST_FILE_ID "detachedManifestFileId"
#define STEP_PROPERTY_FIELD_DETACHED_MANIFEST_FILE_ID UPDATE_MANIFEST_PROPERTY_FIELD_DETACHED_MANIFEST_FILE_ID
#define STEP_PROPERTY_FIELD_TYPE "type"
#define STEP_PROPERTY_FIELD_HANDLER "handler"
#define STEP_PROPERTY_FIELD_FILES "files"
#define STEP_PROPERTY_FIELD_HANDLER_PROPERTIES "handlerProperties"

#define WORKFLOW_CHILDREN_BLOCK_SIZE 10

/**
 * @brief Maximum length for the 'resultDetails' string.
 */
#define WORKFLOW_RESULT_DETAILS_MAX_LENGTH 1024

#define SUPPORTED_UPDATE_MANIFEST_VERSION 4

EXTERN_C_BEGIN

//
// Private functions - this is an adapter for the underlying ADUC_Workflow object.
//

/**
 * @brief Deep copy string. Caller must call workflow_free_string() when done.
 *
 * @param s Input string.
 * @return A copy of input string if succeeded. Otherwise, return NULL.
 */
char* workflow_copy_string(const char* s)
{
    char* ret = NULL;
    if (mallocAndStrcpy_s(&ret, s) == 0)
    {
        return ret;
    }
    return NULL;
}

/**
 * @brief Convert ADUC_Workflow* to ADUC_WorkflowHandle.
 */
ADUC_WorkflowHandle handle_from_workflow(ADUC_Workflow* workflow)
{
    return (ADUC_WorkflowHandle)(workflow);
}

/**
 * @brief Convert ADUC_WorkflowHandle to ADUC_Workflow*.
 */
ADUC_Workflow* workflow_from_handle(ADUC_WorkflowHandle handle)
{
    return (ADUC_Workflow*)(handle);
}

/**
 * @brief Gets workflow id (properties["_id"]).
 *
 * @param handle A workflow handle object.
 * @return A read-only '_id' string.
 */
const char* _workflow_get_properties_id(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        return NULL;
    }

    if (wf->PropertiesObject != NULL && json_object_has_value(wf->PropertiesObject, WORKFLOW_PROPERTY_FIELD_ID))
    {
        return json_object_get_string(wf->PropertiesObject, WORKFLOW_PROPERTY_FIELD_ID);
    }

    return NULL;
}

/**
 * @brief Gets workflow retryTimestamp (properties["_retryTimestamp"]).
 *
 * @param handle A workflow handle object.
 * @return A read-only '_retryTimestamp' string.
 */
const char* _workflow_get_properties_retryTimestamp(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    const char* result = NULL;

    if (wf != NULL && wf->PropertiesObject != NULL
        && json_object_has_value(wf->PropertiesObject, WORKFLOW_PROPERTY_FIELD_RETRYTIMESTAMP))
    {
        result = json_object_get_string(wf->PropertiesObject, WORKFLOW_PROPERTY_FIELD_RETRYTIMESTAMP);
    }

    return result;
}

/**
 * @brief Helper function for checking the hash of the updatemanifest is equal to the
 * hash held within the signature
 * @param updateActionJson JSON value created form an updateActionJsonString and contains
 * both the updateManifest and the updateManifestSignature
 * @returns true on success and false on failure
 */
_Bool _Json_ValidateManifestHash(const JSON_Value* updateActionJson)
{
    _Bool success = false;

    JSON_Value* signatureValue = NULL;
    char* jwtPayload = NULL;

    if (updateActionJson == NULL)
    {
        Log_Error("updateActionJson passed to _Json_ValidateManifestHash is NULL");
        goto done;
    }

    JSON_Object* updateActionObject = json_value_get_object(updateActionJson);

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
 * @brief A helper function for parsing workflow data from file, or from string.
 *
 * @param isFile A boolean indicates whether @p source is an input file, or an input JSON string.
 * @param source An input file or JSON string.
 * @param validateManifest A boolean indicates whether to validate the manifest.
 * @param handle An output workflow object handle.
 * @return ADUC_Result
 */
ADUC_Result _workflow_parse(bool isFile, const char* source, bool validateManifest, ADUC_WorkflowHandle* handle)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure };
    JSON_Value* updateActionJson = NULL;
    char* workFolder = NULL;
    STRING_HANDLE detachedUpdateManifestFilePath = NULL;
    ADUC_FileEntity* fileEntity = NULL;

    if (handle == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_ERROR_BAD_PARAM;
        return result;
    }

    *handle = NULL;

    ADUC_Workflow* wf = malloc(sizeof(*wf));
    if (wf == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    memset(wf, 0, sizeof(*wf));

    if (isFile)
    {
        updateActionJson = json_parse_file(source);
        if (updateActionJson == NULL)
        {
            Log_Error("Parse json file failed. '%s'", source);
            result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_INVALID_ACTION_JSON_FILE;
            goto done;
        }
    }
    else
    {
        updateActionJson = json_parse_string(source);
        if (updateActionJson == NULL)
        {
            Log_Error("Invalid json root.");
            result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_INVALID_ACTION_JSON_STRING;
            goto done;
        }
    }

    if (json_value_get_type(updateActionJson) != JSONObject)
    {
        Log_Error("Invalid json root type.");
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_INVALID_ACTION_JSON;
        goto done;
    }

    JSON_Object* updateActionObject = json_value_get_object(updateActionJson);
    ADUCITF_UpdateAction updateAction = ADUCITF_UpdateAction_Undefined;
    if (json_object_dothas_value(updateActionObject, ADUCITF_FIELDNAME_WORKFLOW_DOT_ACTION))
    {
        updateAction = json_object_dotget_number(updateActionObject, ADUCITF_FIELDNAME_WORKFLOW_DOT_ACTION);
    }

    wf->UpdateActionObject = updateActionObject;

    // 'cancel' action doesn't contains UpdateManifest and UpdateSignature.
    // Skip this part.
    if (updateAction != ADUCITF_UpdateAction_Cancel)
    {
        // Skip signature validation if specified.
        // Also, some (partial) action data may not contain an UpdateAction,
        // e.g., Components-Update manifest that delivered as part of Bundle Updates,
        // We will skip the validation for these cases.
        if (validateManifest && (updateAction != ADUCITF_UpdateAction_Undefined))
        {
            const char* manifestSignature =
                json_object_get_string(updateActionObject, ADUCITF_FIELDNAME_UPDATEMANIFESTSIGNATURE);
            if (manifestSignature == NULL)
            {
                Log_Error("Invalid manifest. Does not contain a signature");
                result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_MANIFEST_VALIDATION_FAILED;
                goto done;
            }

            JWSResult jwsResult = VerifyJWSWithSJWK(manifestSignature);
            if (jwsResult != JWSResult_Success)
            {
                Log_Error("Manifest signature validation failed with result: %u", jwsResult);
                result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_MANIFEST_VALIDATION_FAILED;
                goto done;
            }

            if (!_Json_ValidateManifestHash(updateActionJson))
            {
                // Handle failed hash case
                Log_Error("_Json_ValidateManifestHash failed");
                result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_MANIFEST_VALIDATION_FAILED;
                goto done;
            }
        }

        // Parsing Update Manifest in a form of JSON string.
        if (json_object_has_value_of_type(wf->UpdateActionObject, ADUCITF_FIELDNAME_UPDATEMANIFEST, JSONString) == 1)
        {
            // Deserialize update manifest.
            const char* updateManifestString =
                json_object_get_string(wf->UpdateActionObject, ADUCITF_FIELDNAME_UPDATEMANIFEST);
            if (updateManifestString == NULL)
            {
                char* s = json_serialize_to_string(json_object_get_wrapping_value(wf->UpdateActionObject));
                Log_Error("No Update Manifest\n%s", s);
                json_free_serialized_string(s);
                result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_NO_UPDATE_MANIFEST;
                goto done;
            }

            wf->UpdateManifestObject = json_value_get_object(json_parse_string(updateManifestString));
        }
        // In case the Update Manifest is in a from of JSON object.
        else if (json_object_has_value_of_type(wf->UpdateActionObject, ADUCITF_FIELDNAME_UPDATEMANIFEST, JSONObject))
        {
            JSON_Value* v = json_object_get_value(wf->UpdateActionObject, ADUCITF_FIELDNAME_UPDATEMANIFEST);
            if (v != NULL)
            {
                char* s = json_serialize_to_string(v);
                wf->UpdateManifestObject = json_value_get_object(json_parse_string(s));
            }
        }

        if (wf->UpdateManifestObject == NULL)
        {
            result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_BAD_UPDATE_MANIFEST;
            goto done;
        }

        int manifestVersion = workflow_get_update_manifest_version(handle_from_workflow(wf));

        // Starting from version 4, the update manifest can contain both embedded manifest,
        // or a downloadable update manifest file (files["manifest"] contains the update manifest file info)
        int sandboxCreateResult = -1;
        const char* detachedManifestFileId =
            json_object_get_string(wf->UpdateManifestObject, UPDATE_MANIFEST_PROPERTY_FIELD_DETACHED_MANIFEST_FILE_ID);
        if (!IsNullOrEmpty(detachedManifestFileId))
        {
            // There's only 1 file entity when the primary update manifest is detached.
            if (!workflow_get_update_file(handle_from_workflow(wf), 0, &fileEntity))
            {
                result.ExtendedResultCode =
                    ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_MISSING_DETACHED_UPDATE_MANIFEST_ENTITY;
                goto done;
            }

            workFolder = workflow_get_workfolder(handle_from_workflow(wf));

            sandboxCreateResult = ADUC_SystemUtils_MkSandboxDirRecursive(workFolder);
            if (sandboxCreateResult != 0)
            {
                Log_Error("Unable to create folder %s, error %d", workFolder, sandboxCreateResult);
                result.ExtendedResultCode =
                    ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_DETACHED_UPDATE_MANIFEST_DOWNLOAD_FAILED;
                goto done;
            }

            // Download the detached update manifest file.
            result = ExtensionManager_Download(
                fileEntity,
                workflow_peek_id(handle_from_workflow(wf)),
                workFolder,
                (60 * 60 * 24) /* default : 24 hour */,
                NULL /* downloadProgressCallback */);
            if (IsAducResultCodeFailure(result.ResultCode))
            {
                workflow_set_result_details(
                    handle_from_workflow(wf), "Cannot download primary detached update manifest file.");
                goto done;
            }
            else
            {
                // Replace existing updateManifest with the one from detached update manifest file.
                detachedUpdateManifestFilePath =
                    STRING_construct_sprintf("%s/%s", workFolder, fileEntity->TargetFilename);
                JSON_Object* rootObj =
                    json_value_get_object(json_parse_file(STRING_c_str(detachedUpdateManifestFilePath)));
                const char* updateManifestString = json_object_get_string(rootObj, ADUCITF_FIELDNAME_UPDATEMANIFEST);
                JSON_Object* detachedManifest = json_value_get_object(json_parse_string(updateManifestString));
                json_value_free(json_object_get_wrapping_value(rootObj));

                if (detachedManifest == NULL)
                {
                    result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_BAD_DETACHED_UPDATE_MANIFEST;
                    goto done;
                }

                // Free old manifest value.
                json_value_free(json_object_get_wrapping_value(wf->UpdateManifestObject));
                wf->UpdateManifestObject = detachedManifest;
            }
        }

        if (validateManifest)
        {
            if (manifestVersion != SUPPORTED_UPDATE_MANIFEST_VERSION)
            {
                Log_Error(
                    "Bad update manifest version: %d. Expected: %d",
                    manifestVersion,
                    SUPPORTED_UPDATE_MANIFEST_VERSION);
                result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_UNSUPPORTED_UPDATE_MANIFEST_VERSION;
                goto done;
            }
        }
    }

    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;

done:

    workflow_free_file_entity(fileEntity);
    fileEntity = NULL;

    STRING_delete(detachedUpdateManifestFilePath);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        if (updateActionJson != NULL)
        {
            json_value_free(updateActionJson);
        }

        free(wf);
        wf = NULL;
    }

    *handle = wf;
    return result;
}

/**
 * @brief Free an UpdateActionObject.
 *
 * @param handle A workflow object handle.
 */
void _workflow_free_updateaction(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf != NULL && wf->UpdateActionObject != NULL)
    {
        json_value_free(json_object_get_wrapping_value(wf->UpdateActionObject));
        wf->UpdateActionObject = NULL;
    }
}

/**
 * @brief Free an UpdateManifestObject.
 *
 * @param handle A workflow object handle.
 */
void _workflow_free_updatemanifest(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf != NULL && wf->UpdateManifestObject != NULL)
    {
        json_value_free(json_object_get_wrapping_value(wf->UpdateManifestObject));
        wf->UpdateManifestObject = NULL;
    }
}

/**
 * @brief Free PropertyObject.
 *
 * @param handle A workflow object handle.
 */
void _workflow_free_properties(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf != NULL && wf->PropertiesObject != NULL)
    {
        json_value_free(json_object_get_wrapping_value(wf->PropertiesObject));
        wf->PropertiesObject = NULL;
    }
}

/**
 * @brief Free an ResultsObject.
 *
 * @param handle A workflow object handle.
 */
void _workflow_free_results_object(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf != NULL && wf->ResultsObject != NULL)
    {
        json_value_free(json_object_get_wrapping_value(wf->ResultsObject));
        wf->ResultsObject = NULL;
    }
}

/**
 * @brief Get the UpdateManifest property of type Array.
 *
 * @param handle A workflow object handle.
 * @param propertyName A name of the property to get.
 * @return JSON_Array object, if exist.
 */
JSON_Array* _workflow_peek_update_manifest_array(ADUC_WorkflowHandle handle, const char* propertyName)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL || handle == NULL || propertyName == NULL || *propertyName == 0)
    {
        return NULL;
    }
    return json_object_get_array(wf->UpdateManifestObject, propertyName);
}

//
// Setters and getters.
//

/**
 * @brief Get deserialized 'action' payload JSON_Object.
 * Note: This usually a json document received from the IoT Hub Device/Module Twin.
 */
const JSON_Object* _workflow_get_updateaction(ADUC_WorkflowHandle handle)
{
    if (handle == NULL)
    {
        return NULL;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);
    return wf->UpdateActionObject;
}

/**
 * @brief Get deserialized 'updateManifest' JSON_Object.
 */
const JSON_Object* _workflow_get_update_manifest(ADUC_WorkflowHandle handle)
{
    if (handle == NULL)
    {
        return NULL;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);
    return wf->UpdateManifestObject;
}

/**
 * @brief Get update manifest version.
 *
 * @param handle A workflow object handle.
 *
 * @return int The manifest version number. Return -1, if failed.
 */
int workflow_get_update_manifest_version(ADUC_WorkflowHandle handle)
{
    int versionNumber = -1;

    char* version = workflow_get_update_manifest_string_property(handle, "manifestVersion");
    if (IsNullOrEmpty(version))
    {
        goto done;
    }

    // Note: Version number older than 3 are decimal.
    versionNumber = atoi(version);

done:
    workflow_free_string(version);
    return versionNumber;
}

/**
 * @brief Set workflow id property. (PropertiesObject["_id"])
 *
 * @param handle A workflow object handle.
 * @param id A workflow id string.
 * @return True if success.
 */
bool _workflow_set_id(ADUC_WorkflowHandle handle, const char* id)
{
    if (handle == NULL)
    {
        return false;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);
    JSON_Status status = json_object_set_string(wf->PropertiesObject, WORKFLOW_PROPERTY_FIELD_ID, id);
    return status == JSONSuccess;
}

/**
 * @brief Set workflow level.
 *
 * @param handle A workflow object handle.
 * @param level A workflow level
 */
void workflow_set_level(ADUC_WorkflowHandle handle, int level)
{
    if (handle == NULL)
    {
        return;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);
    wf->Level = level;
}

/**
 * @brief Set workflow step index.
 *
 * @param handle A workflow object handle.
 * @param step A workflow step index.
 */
void workflow_set_step_index(ADUC_WorkflowHandle handle, int stepIndex)
{
    if (handle == NULL)
    {
        return;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);
    wf->StepIndex = stepIndex;
}

/**
 * @brief Get workflow level.
 *
 * @param handle A workflow object handle.
 * @return Returns -1 if the specified handle is invalid. Otherwise, return the workflow level.
 */
int workflow_get_level(ADUC_WorkflowHandle handle)
{
    if (handle == NULL)
    {
        return -1;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);
    return wf->Level;
}

/**
 * @brief Get workflow step index.
 *
 * @param handle A workflow object handle.
 * @return Returns -1 if the specified handle is invalid. Otherwise, return the workflow step index.
 */
int workflow_get_step_index(ADUC_WorkflowHandle handle)
{
    if (handle == NULL)
    {
        return -1;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);
    return wf->StepIndex;
}

/**
 * @brief Get a read-only string containing 'workflow.id' property
 *
 * @param handle A workflow object handle.
 * @return Workflow id string.
 */
const char* _workflow_peek_workflow_dot_id(ADUC_WorkflowHandle handle)
{
    if (handle == NULL)
    {
        return NULL;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf->UpdateActionObject != NULL
        && json_object_dothas_value(wf->UpdateActionObject, WORKFLOW_PROPERTY_FIELD_WORKFLOW_DOT_ID))
    {
        return json_object_dotget_string(wf->UpdateActionObject, WORKFLOW_PROPERTY_FIELD_WORKFLOW_DOT_ID);
    }

    return NULL;
}

/**
 * @brief Set workflow retryTimestamp property. (PropertiesObject["_retryTimestamp"])
 *
 * @param handle A workflow object handle.
 * @param id A workflow retryTimestamp string.
 * @return True if success.
 */
bool _workflow_set_retryTimestamp(ADUC_WorkflowHandle handle, const char* retryTimestamp)
{
    if (handle == NULL)
    {
        return false;
    }

    JSON_Status status;

    ADUC_Workflow* wf = workflow_from_handle(handle);

    status = json_object_set_string(wf->PropertiesObject, WORKFLOW_PROPERTY_FIELD_RETRYTIMESTAMP, retryTimestamp);

    return status == JSONSuccess;
}

/**
 * @brief Get a read-only string containing 'workflow.retryTimestamp' property
 *
 * @param handle A workflow object handle.
 * @return Workflow retryTimestamp string, or NULL if it was not specified by the service.
 */
const char* _workflow_peek_workflow_dot_retryTimestamp(ADUC_WorkflowHandle handle)
{
    if (handle == NULL)
    {
        return NULL;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);

    const char* result = NULL;

    if (wf->UpdateActionObject != NULL
        && json_object_dothas_value(wf->UpdateActionObject, WORKFLOW_PROPERTY_FIELD_WORKFLOW_DOT_RETRYTIMESTAMP))
    {
        result =
            json_object_dotget_string(wf->UpdateActionObject, WORKFLOW_PROPERTY_FIELD_WORKFLOW_DOT_RETRYTIMESTAMP);
    }

    return result;
}

/**
 * @brief Set or add string property to the workflow object.
 *
 * @param handle A workflow object handle.
 * @param property Name of the property.
 * @param value A string value to set.
 *              If @value is NULL, the specified property will be removed from the object.
 * @return bool Returns true of succeeded. Otherwise, return false.
 */
bool workflow_set_string_property(ADUC_WorkflowHandle handle, const char* property, const char* value)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        return false;
    }

    if (wf->PropertiesObject == NULL)
    {
        wf->PropertiesObject = json_object(json_value_init_object());
    }

    if (wf->PropertiesObject == NULL)
    {
        return false;
    }

    if (value != NULL)
    {
        return JSONSuccess == json_object_set_string(wf->PropertiesObject, property, value);
    }

    return JSONSuccess == json_object_set_null(wf->PropertiesObject, property);
}

char* workflow_get_string_property(ADUC_WorkflowHandle handle, const char* property)
{
    if (handle == NULL)
    {
        return NULL;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);

    if (wf->PropertiesObject == NULL || !json_object_has_value(wf->PropertiesObject, property))
    {
        return NULL;
    }

    const char* value = json_object_get_string(wf->PropertiesObject, property);

    if (value != NULL)
    {
        char* ret;
        if (mallocAndStrcpy_s(&ret, (char*)value) == 0)
        {
            return ret;
        }
    }

    return NULL;
}

bool workflow_set_boolean_property(ADUC_WorkflowHandle handle, const char* property, bool value)
{
    if (handle == NULL)
    {
        return false;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);

    if (wf->PropertiesObject == NULL)
    {
        return false;
    }

    return JSONSuccess == json_object_set_boolean(wf->PropertiesObject, property, value);
}

bool workflow_get_boolean_property(ADUC_WorkflowHandle handle, const char* property)
{
    if (handle == NULL)
    {
        return false;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);

    if (wf->PropertiesObject == NULL || !json_object_has_value(wf->PropertiesObject, property))
    {
        return false;
    }

    return json_object_get_boolean(wf->PropertiesObject, property);
}

bool workflow_set_workfolder(ADUC_WorkflowHandle handle, const char* format, ...)
{
    bool success = false;
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        return false;
    }

    if (format == NULL)
    {
        success = workflow_set_string_property(handle, WORKFLOW_PROPERTY_FIELD_WORKFOLDER, "");
    }
    else
    {
        char buffer[WORKFLOW_RESULT_DETAILS_MAX_LENGTH];
        va_list arg_list;
        va_start(arg_list, format);
        if (vsnprintf(buffer, WORKFLOW_RESULT_DETAILS_MAX_LENGTH, format, arg_list) >= 0)
        {
            success = workflow_set_string_property(handle, WORKFLOW_PROPERTY_FIELD_WORKFOLDER, buffer);
        }
        else
        {
            Log_Error("Cannot set workflow's workfolder.");
            success = false;
        }
        va_end(arg_list);
    }

    return success;
}

bool workflow_set_selected_components(ADUC_WorkflowHandle handle, const char* selectedComponents)
{
    return workflow_set_string_property(handle, WORKFLOW_PROPERTY_FIELD_SELECTED_COMPONENTS, selectedComponents);
}

const char* workflow_peek_selected_components(ADUC_WorkflowHandle handle)
{
    return workflow_get_string_property(handle, WORKFLOW_PROPERTY_FIELD_SELECTED_COMPONENTS);
}

bool workflow_set_sandbox(ADUC_WorkflowHandle handle, const char* sandbox)
{
    if (handle == NULL)
    {
        return false;
    }

    ADUC_WorkflowHandle root = workflow_get_root(handle);

    if (!workflow_set_string_property(root, WORKFLOW_PROPERTY_FIELD_SANDBOX_ROOTPATH, sandbox))
    {
        Log_Error("Cannot set sandbox root path.");
        return false;
    }

    return true;
}

// Workfolder =  [root.sandboxfolder]  "/"  ( [parent.workfolder | parent.id]  "/" )+  [handle.workfolder | handle.id]
char* workflow_get_workfolder(ADUC_WorkflowHandle handle)
{
    char dir[1024] = { 0 };

    char* pwf = NULL;
    char* ret = NULL;
    char* id = workflow_get_id(handle);

    // If workfolder explicitly specified, use it.
    char* wf = workflow_get_string_property(handle, WORKFLOW_PROPERTY_FIELD_WORKFOLDER);
    if (wf != NULL)
    {
        return wf;
    }

    // Return ([parent's workfolder] or [default sandbox folder]) + "/" + [workflow id];
    ADUC_WorkflowHandle p = workflow_get_parent(handle);
    if (p != NULL)
    {
        pwf = workflow_get_workfolder(p);
        sprintf(dir, "%s/%s", pwf, id);
    }
    else
    {
        Log_Info("Sandbox root path not set. Use default: '%s'", DEFAULT_SANDBOX_ROOT_PATH);
        sprintf(dir, "%s/%s", DEFAULT_SANDBOX_ROOT_PATH, id);
    }

    free(pwf);
    free(id);

    if (dir[0] != 0)
    {
        mallocAndStrcpy_s(&ret, dir);
    }

    return ret;
}

/**
 * @brief Get 'updateManifest.files' map.
 *
 * @param handle A workflow object handle.
 * @return const JSON_Object* A map containing update files information.
 */
const JSON_Object* _workflow_get_update_manifest_files_map(ADUC_WorkflowHandle handle)
{
    const JSON_Object* o = _workflow_get_update_manifest(handle);
    return json_object_dotget_object(o, "files");
}

/**
 * @brief Get 'fileUrls' map.
 *
 * @param handle A workflow object handle.
 * @return const JSON_Object* A json object (map) containing update files information.
 */
const JSON_Object* _workflow_get_fileurls_map(ADUC_WorkflowHandle handle)
{
    const JSON_Object* o = _workflow_get_updateaction(handle);
    return o == NULL ? NULL : json_object_dotget_object(o, "fileUrls");
}

/**
 * @brief Return an update id of this workflow.
 * This id should be reported to the cloud once the update installed successfully.
 *
 * @param handle A workflow object handle.
 * @param[out] updateId A pointer to the output ADUC_UpdateId struct.
 *                      Must call 'workflow_free_update_id' function to free the memory when done.
 *
 * @return ADUC_Result Return ADUC_GeneralResult_Success if success. Otherwise, return ADUC_GeneralResult_Failure with extendedResultCode.
 */
ADUC_Result workflow_get_expected_update_id(ADUC_WorkflowHandle handle, ADUC_UpdateId** updateId)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure };
    if (!ADUC_Json_GetUpdateId(json_object_get_wrapping_value(_workflow_get_updateaction(handle)), updateId))
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_INVALID_UPDATE_ID;
    }
    else
    {
        result.ResultCode = ADUC_GeneralResult_Success;
    }
    return result;
}

/**
 * @brief Return an update id of this workflow.
 * This id should be reported to the cloud once the update installed successfully.
 *
 * @param handle A workflow object handle.
 *
 * @return char* Expected update id string.
 *         Caller must call 'workflow_free_string' function to free the memory when done.
 */
char* workflow_get_expected_update_id_string(ADUC_WorkflowHandle handle)
{
    const JSON_Object* manifest = _workflow_get_update_manifest(handle);
    const char* provider = json_object_dotget_string(manifest, "updateId.provider");
    const char* name = json_object_dotget_string(manifest, "updateId.name");
    const char* version = json_object_dotget_string(manifest, "updateId.version");
    char* id = NULL;

    if (provider == NULL || name == NULL || version == NULL)
    {
        goto done;
    }

    id = ADUC_StringFormat("{\"provider\":\"%s\",\"name\":\"%s\",\"version\":\"%s\"}", provider, name, version);

done:
    return id;
}

void workflow_free_update_id(ADUC_UpdateId* updateId)
{
    ADUC_UpdateId_UninitAndFree(updateId);
}

/**
 * @brief Get installed-criteria string from this workflow.
 * @param handle A workflow object handle.
 * @return Returns installed-criteria string.
 *         Caller must call 'workflow_free_string' function to free the memory when done.
 */
char* workflow_get_installed_criteria(ADUC_WorkflowHandle handle)
{
    // For Update Manifest v4, customer can specify installedCriteria in 'handlerProperties' map.
    char* installedCriteria = workflow_copy_string(
        workflow_peek_update_manifest_handler_properties_string(handle, ADUCITF_FIELDNAME_INSTALLEDCRITERIA));

    return installedCriteria;
}

/**
 * @brief Get the Update Manifest 'compatibility' array, in serialized json string format.
 *
 * @param handle A workflow handle.
 * @return char* If success, returns a serialized json string. Otherwise, returns NULL.
 *         Caller must call 'workflow_free_string' function to free the memory when done.
 */
char* workflow_get_compatibility(ADUC_WorkflowHandle handle)
{
    const JSON_Object* manifestObject = _workflow_get_update_manifest(handle);
    if (manifestObject == NULL)
    {
        return NULL;
    }

    JSON_Value* compats = json_object_get_value(manifestObject, "compatibility");
    if (compats != NULL)
    {
        return json_serialize_to_string(compats);
    }

    return NULL;
}

void workflow_set_operation_in_progress(ADUC_WorkflowHandle handle, bool inProgress)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        Log_Warn("Setting 'OperationInProgress' when there's no active workflow. (value:%d)", inProgress);
        return;
    }

    wf->OperationInProgress = inProgress;
}

bool workflow_get_operation_in_progress(ADUC_WorkflowHandle handle)
{
    bool result = false;

    ADUC_Workflow* wf = workflow_from_handle(handle);

    if (wf != NULL)
    {
        result = wf->OperationInProgress;
    }

    return result;
}

void workflow_set_operation_cancel_requested(ADUC_WorkflowHandle handle, bool cancel)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        Log_Warn("Setting 'OperationCancelled' when there's no active workflow. (value:%d)", cancel);
        return;
    }

    wf->OperationCancelled = cancel;
}

bool workflow_get_operation_cancel_requested(ADUC_WorkflowHandle handle)
{
    bool result = false;

    ADUC_Workflow* wf = workflow_from_handle(handle);

    if (wf != NULL)
    {
        result = wf->OperationCancelled;
    }

    return result;
}

/**
 * @brief sets both operation in progress and cancel requested to false
 * @param handle The workflow handle
 */
void workflow_clear_inprogress_and_cancelrequested(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        Log_Warn("clearing when no active workflow.");
        return;
    }

    wf->OperationInProgress = false;
    wf->OperationCancelled = false;
}

/**
 * @brief Get an Update Action code.
 *
 * @param handle A workflow object handle.
 * @return ADUCITF_UpdateAction Returns ADUCITF_UpdateAction_Undefined if not specified.
 * Otherwise, returns ADUCITF_UpdateAction_*
 * or ADUCITF_UpdateAction_Cancel.
 */
ADUCITF_UpdateAction workflow_get_action(ADUC_WorkflowHandle handle)
{
    const JSON_Object* o = _workflow_get_updateaction(handle);
    if (o == NULL)
    {
        return ADUCITF_UpdateAction_Undefined;
    }

    ADUCITF_UpdateAction action = ADUCITF_UpdateAction_Undefined;
    if (json_object_dothas_value(o, WORKFLOW_PROPERTY_FIELD_WORKFLOW_DOT_ACTION))
    {
        action = (ADUCITF_UpdateAction)json_object_dotget_number(o, WORKFLOW_PROPERTY_FIELD_WORKFLOW_DOT_ACTION);
    }

    return action;
}

// Public functions - always return a copy of value.
size_t workflow_get_update_files_count(ADUC_WorkflowHandle handle)
{
    const JSON_Object* files = _workflow_get_update_manifest_files_map(handle);
    return files == NULL ? 0 : json_object_get_count(files);
}

bool workflow_get_update_file(ADUC_WorkflowHandle handle, size_t index, ADUC_FileEntity** entity)
{
    if (entity == NULL)
    {
        return false;
    }

    size_t count = workflow_get_update_files_count(handle);
    if (index >= count)
    {
        return false;
    }

    _Bool succeeded = false;
    const JSON_Object* files = NULL;
    const JSON_Object* file = NULL;
    const JSON_Object* fileUrls = NULL;
    ADUC_FileEntity* newEntity = NULL;
    *entity = NULL;
    const char* uri = NULL;
    const char* fileId = NULL;
    const char* name = NULL;
    const char* arguments = NULL;
    size_t tempHashCount = 0;
    ADUC_Hash* tempHash = NULL;

    if ((files = _workflow_get_update_manifest_files_map(handle)) == NULL)
    {
        goto done;
    }

    fileId = json_object_get_name(files, index);

    if ((file = json_value_get_object(json_object_get_value_at(files, index))) == NULL)
    {
        goto done;
    }

    // Find fileurls map in this workflow, and its enclosing workflow(s).
    ADUC_WorkflowHandle h = handle;

    do
    {
        if ((fileUrls = _workflow_get_fileurls_map(h)) != NULL)
        {
            uri = json_object_get_string(fileUrls, fileId);
        }
        h = workflow_get_parent(h);
    } while (uri == NULL && h != NULL);

    if (uri == NULL)
    {
        Log_Error("Cannot find URL for fileId '%s'", fileId);
    }

    name = json_object_get_string(file, ADUCITF_FIELDNAME_FILENAME);
    arguments = json_object_get_string(file, ADUCITF_FIELDNAME_ARGUMENTS);

    const JSON_Object* hashObj = json_object_get_object(file, ADUCITF_FIELDNAME_HASHES);

    tempHash = ADUC_HashArray_AllocAndInit(hashObj, &tempHashCount);
    if (tempHash == NULL)
    {
        Log_Error("Unable to parse hashes for file @ %zu", index);
        goto done;
    }

    size_t sizeInBytes = 0;
    if (json_object_has_value(file, ADUCITF_FIELDNAME_SIZEINBYTES))
    {
        sizeInBytes = json_object_get_number(file, ADUCITF_FIELDNAME_SIZEINBYTES);
    }

    newEntity = malloc(sizeof(*newEntity));
    if (newEntity == NULL)
    {
        goto done;
    }

    if (!ADUC_FileEntity_Init(newEntity, fileId, name, uri, arguments, tempHash, tempHashCount, sizeInBytes))
    {
        Log_Error("Invalid file entity arguments");
        goto done;
    }

    *entity = newEntity;
    succeeded = true;

done:
    if (!succeeded)
    {
        if (newEntity != NULL)
        {
            newEntity->Hash = NULL; // Manually free hash array below...
            ADUC_FileEntity_Uninit(newEntity);
            free(newEntity);
        }

        if (tempHash != NULL)
        {
            ADUC_Hash_FreeArray(tempHashCount, tempHash);
        }
    }

    return succeeded;
}

bool workflow_get_update_file_by_name(ADUC_WorkflowHandle handle, const char* fileName, ADUC_FileEntity** entity)
{
    if (entity == NULL)
    {
        return false;
    }

    size_t count = workflow_get_update_files_count(handle);
    if (count == 0)
    {
        return false;
    }

    _Bool succeeded = false;
    const JSON_Object* files = NULL;
    const JSON_Object* file = NULL;
    const JSON_Object* fileUrls = NULL;
    ADUC_FileEntity* newEntity = NULL;
    *entity = NULL;
    const char* uri = NULL;
    const char* fileId = NULL;
    const char* name = NULL;
    const char* arguments = NULL;
    size_t tempHashCount = 0;
    ADUC_Hash* tempHash = NULL;

    if ((files = _workflow_get_update_manifest_files_map(handle)) == NULL)
    {
        goto done;
    }

    // Find file by name.
    for (int i = 0; i < count; i++)
    {
        if ((file = json_value_get_object(json_object_get_value_at(files, i))) != NULL
            && strcasecmp(json_object_get_string(file, "fileName"), fileName) == 0)
        {
            fileId = json_object_get_name(files, i);
            break;
        }
    }

    if (fileId == NULL)
    {
        goto done;
    }

    // Find fileurls map in this workflow, and its enclosing workflow(s).
    ADUC_WorkflowHandle h = handle;

    do
    {
        if ((fileUrls = _workflow_get_fileurls_map(h)) != NULL)
        {
            uri = json_object_get_string(fileUrls, fileId);
        }
        h = workflow_get_parent(h);
    } while (uri == NULL && h != NULL);

    if (uri == NULL)
    {
        Log_Error("Cannot find URL for fileId '%s'", fileId);
    }

    name = json_object_get_string(file, ADUCITF_FIELDNAME_FILENAME);
    arguments = json_object_get_string(file, ADUCITF_FIELDNAME_ARGUMENTS);

    const JSON_Object* hashObj = json_object_get_object(file, ADUCITF_FIELDNAME_HASHES);

    tempHash = ADUC_HashArray_AllocAndInit(hashObj, &tempHashCount);
    if (tempHash == NULL)
    {
        Log_Error("Unable to parse hashes for fileId", fileId);
        goto done;
    }

    size_t sizeInBytes = 0;
    if (json_object_has_value(file, ADUCITF_FIELDNAME_SIZEINBYTES))
    {
        sizeInBytes = json_object_get_number(file, ADUCITF_FIELDNAME_SIZEINBYTES);
    }

    newEntity = malloc(sizeof(*newEntity));
    if (newEntity == NULL)
    {
        goto done;
    }

    if (!ADUC_FileEntity_Init(newEntity, fileId, name, uri, arguments, tempHash, tempHashCount, sizeInBytes))
    {
        Log_Error("Invalid file entity arguments");
        goto done;
    }

    *entity = newEntity;
    succeeded = true;

done:
    if (!succeeded)
    {
        if (newEntity != NULL)
        {
            newEntity->Hash = NULL; // Manually free hash array below...
            ADUC_FileEntity_Uninit(newEntity);
            free(newEntity);
        }

        if (tempHash != NULL)
        {
            ADUC_Hash_FreeArray(tempHashCount, tempHash);
        }
    }

    return succeeded;
}

/**
 * @brief Uninitialize and free specified file entity object.
 *
 * @param entity A file entity object.
 */
void workflow_free_file_entity(ADUC_FileEntity* entity)
{
    ADUC_FileEntity_Uninit(entity);
    free(entity);
}

/**
 * @brief Get an Update Manifest property (string) without copying the value.
 * Caller must not free the pointer.
 *
 * @param handle A workflow object handle.
 * @param[in] propertyName Name of the property to get.
 * @return const char* A reference to property value. Caller must not free this pointer.
 */
const char* workflow_peek_update_manifest_string(ADUC_WorkflowHandle handle, const char* propertyName)
{
    const JSON_Object* manifest = _workflow_get_update_manifest(handle);
    const char* value = json_object_get_string(manifest, propertyName);
    return value;
}

/**
 * @brief Get a property of type 'string' in the workflow update manifest.
 *
 * @param handle A workflow object handle.
 * @param propertyName The name of a property to get.
 * @return A copy of specified property. Caller must call workflow_free_string when done with the value.
 */
char* workflow_get_update_manifest_string_property(ADUC_WorkflowHandle handle, const char* propertyName)
{
    return workflow_copy_string(workflow_peek_update_manifest_string(handle, propertyName));
}

/**
 * @brief Get a 'Compatibility' entry of the workflow at a specified @p index.
 *
 * @param handle A workflow object handle.
 * @param index Index of the compatibility set to.
 * @return A copy of compatibility entry. Caller must call workflow_free_string when done with the value.
 */
char* workflow_get_update_manifest_compatibility(ADUC_WorkflowHandle handle, size_t index)
{
    JSON_Array* array = _workflow_peek_update_manifest_array(handle, "compatibility");
    JSON_Object* object = json_array_get_object(array, index);
    char* output = NULL;
    if (object != NULL)
    {
        char* s = json_serialize_to_string(json_object_get_wrapping_value(object));
        output = workflow_copy_string(s);
        json_free_serialized_string(s);
    }

    return output;
}

/**
 * @brief Get a string copy of the update type for the specified workflow.
 *
 * @param handle A workflow object handle.
 * @return An UpdateType string. Caller owns it and must free with workflow_free_string.
 */
char* workflow_get_update_type(ADUC_WorkflowHandle handle)
{
    return workflow_get_update_manifest_string_property(handle, ADUCITF_FIELDNAME_UPDATETYPE);
}

/**
 * @brief Gets the update type of the specified workflow.
 *
 * @param handle A workflow object handle.
 * @return An UpdateType string. Caller does not own the string so must not free it.
 */
const char* workflow_peek_update_type(ADUC_WorkflowHandle handle)
{
    return workflow_peek_update_manifest_string(handle, ADUCITF_FIELDNAME_UPDATETYPE);
}

ADUC_Result _workflow_init_helper(ADUC_WorkflowHandle* handle)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure };

    ADUC_Workflow* wf = workflow_from_handle(*handle);
    wf->Parent = NULL;
    wf->ChildCount = 0;
    wf->ChildrenMax = 0;
    wf->Children = 0;
    wf->PropertiesObject = json_value_get_object(json_value_init_object());
    if (wf->PropertiesObject == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_ERROR_NO_MEM;
        goto done;
    }

    wf->ResultsObject = json_value_get_object(json_value_init_object());
    if (wf->ResultsObject == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_ERROR_NO_MEM;
        goto done;
    }

    wf->ResultDetails = STRING_new();
    wf->InstalledUpdateId = STRING_new();

    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;

done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error(
            "Failed to init workflow handle. result:%d (erc:0x%X)", result.ResultCode, result.ExtendedResultCode);
        if (handle != NULL)
        {
            workflow_free(*handle);
            *handle = NULL;
        }
    }

    return result;
}

ADUC_Result workflow_init_from_file(const char* updateManifestFile, bool validateManifest, ADUC_WorkflowHandle* handle)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure };
    if (updateManifestFile == NULL || handle == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_ERROR_BAD_PARAM;
        goto done;
    }

    result = _workflow_parse(true, updateManifestFile, validateManifest, handle);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = _workflow_init_helper(handle);

done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error(
            "Failed to init workflow handle. result:%d (erc:0x%X)", result.ResultCode, result.ExtendedResultCode);
        if (handle != NULL)
        {
            workflow_free(*handle);
            *handle = NULL;
        }
    }

    return result;
}

/**
 * @brief Get 'updateManifest.instructions.steps' array.
 *
 * @param handle A workflow object handle.
 * @return const JSON_Object* Array containing update instructions steps.
 */
static JSON_Array* workflow_get_instructions_steps_array(ADUC_WorkflowHandle handle)
{
    const JSON_Object* o = _workflow_get_update_manifest(handle);
    return json_object_dotget_array(o, WORKFLOW_PROPERTY_FIELD_INSTRUCTIONS_DOT_STEPS);
}

/**
 * @brief Create a new workflow data handler using specified step data from base workflow.
 * Note: The 'workfolder' of the returned workflow data object will be the same as the base's.
 *
 * Example step data:
 *
 * {
 *   "handler": "microsoft/script:1",
 *   "files": [
 *     "f81347aeb04c14ebf"
 *   ],
 *   "handlerProperties": {
 *     "arguments": "--pre"
 *   }
 * }
 *
 * @param base The base workflow containing valid Update Action and Manifest.
 * @param stepIndex A step index.
 * @param handle An output workflow handle.
 * @return ADUC_Result
 */
ADUC_Result workflow_create_from_inline_step(ADUC_WorkflowHandle base, int stepIndex, ADUC_WorkflowHandle* handle)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure };
    JSON_Status jsonStatus = JSONFailure;
    JSON_Value* updateActionValue = NULL;
    JSON_Value* updateManifestValue = NULL;
    ADUC_Workflow* wf = NULL;
    JSON_Array* steps = workflow_get_instructions_steps_array(base);
    JSON_Value* stepValue = json_array_get_value(steps, stepIndex);

    if (stepValue == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_INVALID_STEP_INDEX;
        goto done;
    }

    *handle = NULL;

    ADUC_Workflow* wfBase = workflow_from_handle(base);

    wf = malloc(sizeof(*wf));
    if (wf == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    memset(wf, 0, sizeof(*wf));

    updateActionValue = json_value_deep_copy(json_object_get_wrapping_value(wfBase->UpdateActionObject));
    if (updateActionValue == NULL)
    {
        Log_Error("Cannot copy Update Action json from base");
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_COPY_UPDATE_ACTION_FROM_BASE_FAILURE;
        goto done;
    }

    JSON_Object* updateActionObject = json_object(updateActionValue);

    updateManifestValue = json_value_deep_copy(json_object_get_wrapping_value(wfBase->UpdateManifestObject));
    if (updateManifestValue == NULL)
    {
        Log_Error("Cannot copy Update Manifest json from base");
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_COPY_UPDATE_ACTION_FROM_BASE_FAILURE;
        goto done;
    }

    JSON_Object* updateManifestObject = json_object(updateManifestValue);
    JSON_Object* stepObject = json_object(stepValue);

    char* currentStepData = json_serialize_to_string_pretty(stepValue);
    Log_Debug("Processing current step:\n%s", currentStepData);
    json_free_serialized_string(currentStepData);

    // Replace 'updateType' with step's handler type.
    const char* updateType = json_object_get_string(stepObject, STEP_PROPERTY_FIELD_HANDLER);
    if (updateType == NULL || *updateType == 0)
    {
        Log_Error("Invalid step entry.");
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_PARSE_STEP_ENTRY_NO_HANDLER_TYPE;
        goto done;
    }
    jsonStatus = json_object_set_string(updateManifestObject, ADUCITF_FIELDNAME_UPDATETYPE, updateType);
    if (jsonStatus == JSONFailure)
    {
        Log_Error("Cannot update step entry updateType.");
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_COPY_UPDATE_ACTION_SET_UPDATE_TYPE_FAILURE;
        goto done;
    }

    // Copy 'handlerProperties'.
    JSON_Value* handlerProperties =
        json_value_deep_copy(json_object_get_value(stepObject, STEP_PROPERTY_FIELD_HANDLER_PROPERTIES));
    jsonStatus =
        json_object_set_value(updateManifestObject, STEP_PROPERTY_FIELD_HANDLER_PROPERTIES, handlerProperties);
    if (jsonStatus == JSONFailure)
    {
        json_value_free(handlerProperties);
        handlerProperties = NULL;
        Log_Error("Cannot copy 'handlerProperties'.");
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_COPY_UPDATE_ACTION_COPY_HANDLER_PROPERTIES_FAILED;
        goto done;
    }

    // Keep only file needed by this step entry. Remove the rest.
    JSON_Array* stepFiles = json_object_get_array(stepObject, ADUCITF_FIELDNAME_FILES);
    JSON_Object* baseFiles = json_object_get_object(updateManifestObject, ADUCITF_FIELDNAME_FILES);
    int fileCount = json_object_get_count(baseFiles);
    for (int b = fileCount - 1; b >= 0; b--)
    {
        const char* baseFileId = json_object_get_name(baseFiles, b);

        // If file is in the instruction, merge their properties.
        bool fileRequired = false;
        int stepFilesCount = json_array_get_count(stepFiles);
        for (int i = stepFilesCount - 1; i >= 0; i--)
        {
            // Note: step's files is an array of file ids.
            const char* stepFileId = json_array_get_string(stepFiles, i);

            if (baseFileId != NULL && stepFileId != NULL && strcmp(baseFileId, stepFileId) == 0)
            {
                // Found.
                fileRequired = true;

                // Done with this file, remove it (and free).
                json_array_remove(stepFiles, i);
                break;
            }
        }

        // Remove file from base files list, if no longer needed.
        if (!fileRequired)
        {
            json_object_remove(baseFiles, json_object_get_name(baseFiles, b));
        }
    }

    // Remove 'instructions' list...
    json_object_set_null(updateManifestObject, "instructions");

    wf->UpdateActionObject = updateActionObject;
    wf->UpdateManifestObject = updateManifestObject;

    {
        char* baseWorkfolder = workflow_get_workfolder(base);
        workflow_set_workfolder(wf, baseWorkfolder);
        workflow_free_string(baseWorkfolder);
    }

    *handle = wf;
    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;

done:
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        json_value_free(updateActionValue);
        json_value_free(updateManifestValue);
        workflow_free(wf);
    }

    return result;
}

/**
 * @brief Transfer data from @p sourceHandle to @p targetHandle.
 * The sourceHandle will no longer contains transferred action data.
 * Caller should not use sourceHandle for other workflow related purposes.
 *
 * @param targetHandle The target workflow handle
 * @param sourceHandle The source workflow handle
 * @return true if transfer succeeded.
 */
bool workflow_transfer_data(ADUC_WorkflowHandle targetHandle, ADUC_WorkflowHandle sourceHandle)
{
    ADUC_Workflow* wfTarget = workflow_from_handle(targetHandle);
    ADUC_Workflow* wfSource = workflow_from_handle(sourceHandle);

    if (wfSource == NULL || wfSource->UpdateActionObject == NULL)
    {
        return false;
    }

    // Transfer over the parsed JSON objects
    wfTarget->UpdateActionObject = wfSource->UpdateActionObject;
    wfSource->UpdateActionObject = NULL;

    wfTarget->UpdateManifestObject = wfSource->UpdateManifestObject;
    wfSource->UpdateManifestObject = NULL;

    wfTarget->PropertiesObject = wfSource->PropertiesObject;
    wfSource->PropertiesObject = NULL;

    return true;
}

/**
 * @brief Instantiate and initialize workflow object with info from the given jsonData.
 *
 * @param updateManifestJson A JSON string containing update manifest data.
 * @param validateManifest A boolean indicates whether to validate the update manifest.
 * @param handle An output workflow object handle.
 * @return ADUC_Result
 */
ADUC_Result workflow_init(const char* updateManifestJson, bool validateManifest, ADUC_WorkflowHandle* handle)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure };
    if (updateManifestJson == NULL || handle == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_ERROR_BAD_PARAM;
        goto done;
    }

    memset(handle, 0, sizeof(*handle));

    result = _workflow_parse(false, updateManifestJson, validateManifest, handle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = _workflow_init_helper(handle);

done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error(
            "Failed to init workflow handle. result:%d (erc:0x%X)", result.ResultCode, result.ExtendedResultCode);
        if (handle != NULL)
        {
            workflow_free(*handle);
            *handle = NULL;
        }
    }

    return result;
}

/**
 * @brief gets the current workflow step.
 *
 * @param handle A workflow data object handle.
 * @return the workflow step.
 */
ADUCITF_WorkflowStep workflow_get_current_workflowstep(ADUC_WorkflowHandle handle)
{
    ADUCITF_WorkflowStep step = ADUCITF_WorkflowStep_Undefined;

    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf != NULL)
    {
        step = wf->CurrentWorkflowStep;
    }

    return step;
}

/**
 * @brief sets the current workflow step.
 *
 * @param handle A workflow data object handle.
 * @param workflowStep The workflow step.
 */
void workflow_set_current_workflowstep(ADUC_WorkflowHandle handle, ADUCITF_WorkflowStep workflowStep)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf != NULL)
    {
        wf->CurrentWorkflowStep = workflowStep;
    }
}

/**
 * @brief Set workflow 'property._id'. This function creates a copy of the input id.
 *
 * @param handle A workflow object handle.
 * @param id A new id of the workflow.
 * @return Return true If succeeded.
 */
bool workflow_set_id(ADUC_WorkflowHandle handle, const char* id)
{
    return _workflow_set_id(handle, id);
}

/**
 * @brief Get a read-only workflow id.
 *
 * @param handle A workflow object handle.
 * @return Return the workflow id.
 */
const char* workflow_peek_id(ADUC_WorkflowHandle handle)
{
    if (handle == NULL)
    {
        return NULL;
    }

    // Return 'properties._id', if set.
    const char* id = _workflow_get_properties_id(handle);
    if (id == NULL)
    {
        // Return 'workflow.id' from Action json data.
        id = _workflow_peek_workflow_dot_id(handle);
    }

    return id;
}

/**
 * @brief Get a workflow id.
 *
 * @param handle A workflow object handle.
 * @return const char* If success, returns pointer to workflow id string. Otherwise, returns NULL.
 * Caller must free the string by calling workflow_free_string() once done with the string.
 */
char* workflow_get_id(ADUC_WorkflowHandle handle)
{
    return workflow_copy_string(workflow_peek_id(handle));
}

/**
 * @brief Explicit set workflow retryTimestamp for this workflow.
 *
 * @param handle A workflow data object handle.
 * @param retryTimestamp The retryTimestamp
 * @return Return true if succeeded.
 */
bool workflow_set_retryTimestamp(ADUC_WorkflowHandle handle, const char* retryTimestamp)
{
    return _workflow_set_retryTimestamp(handle, retryTimestamp);
}

/**
 * @brief Get a read-only workflow retryTimestamp.
 *
 * @param handle A workflow object handle.
 * @return Return the workflow retryTimestamp, if it was in json message.
 */
const char* workflow_peek_retryTimestamp(ADUC_WorkflowHandle handle)
{
    if (handle == NULL)
    {
        return NULL;
    }

    // Return 'properties._retryTimestamp', if set.
    const char* retryTimestamp = _workflow_get_properties_retryTimestamp(handle);
    if (retryTimestamp == NULL)
    {
        // Return 'workflow.retryTimestamp' from Action json data,
        // which could also be null or empty, as it's optional.
        retryTimestamp = _workflow_peek_workflow_dot_retryTimestamp(handle);
    }

    return retryTimestamp;
}

/**
 * @brief Free string buffer returned by workflow_get_* APIs.
 * @param string A string to free.
 */
void workflow_free_string(char* string)
{
    free(string);
}

/**
 * @brief Free workflow content.
 *
 * @param handle A workflow object handle.
 */
void workflow_uninit(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf != NULL)
    {
        STRING_delete(wf->ResultDetails);
        wf->ResultDetails = NULL;
        STRING_delete(wf->InstalledUpdateId);
        wf->InstalledUpdateId = NULL;
    }

    _workflow_free_updateaction(handle);
    _workflow_free_updatemanifest(handle);
    _workflow_free_properties(handle);
    _workflow_free_results_object(handle);

    // This should have been transferred, but free it if it's still around.
    if (wf != NULL && wf->DeferredReplacementWorkflow != NULL)
    {
        workflow_free(wf->DeferredReplacementWorkflow);
        wf->DeferredReplacementWorkflow = NULL;
    }
}

/**
 * @brief Free workflow content and free the workflow object.
 *
 * @param handle A workflow object handle.
 */
void workflow_free(ADUC_WorkflowHandle handle)
{
    if (handle == NULL)
    {
        return;
    }

    // Remove existing child workflow handle(s)
    while (workflow_get_children_count(handle) > 0)
    {
        ADUC_WorkflowHandle child = workflow_remove_child(handle, 0);
        workflow_free(child);
    }

    workflow_uninit(handle);
    free(handle);
}

/**
 * @brief Set workflow parent.
 *
 * @param handle A child workflow object handle.
 * @param parent A parent workflow object handle.
 */
void workflow_set_parent(ADUC_WorkflowHandle handle, ADUC_WorkflowHandle parent)
{
    if (handle == NULL)
    {
        return;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);
    wf->Parent = workflow_from_handle(parent);
    wf->Level = workflow_get_level(parent) + 1;
}

/**
 * @brief Get the root workflow object linked by the @p handle.
 *
 * @param handle A workflow object handle.
 * @return ADUC_WorkflowHandle
 */
ADUC_WorkflowHandle workflow_get_root(ADUC_WorkflowHandle handle)
{
    if (handle == NULL)
    {
        return NULL;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);
    while (wf->Parent != NULL)
    {
        wf = wf->Parent;
    }
    return (ADUC_WorkflowHandle)wf;
}

/**
 * @brief Get the parent workflow object linked by the @p handle.
 *
 * @param handle A workflow object handle.
 * @return ADUC_WorkflowHandle
 */
ADUC_WorkflowHandle workflow_get_parent(ADUC_WorkflowHandle handle)
{
    if (handle == NULL)
    {
        return NULL;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);
    return (ADUC_WorkflowHandle)wf->Parent;
}

/**
 * @brief Get workflow's children count.
 *
 * @param handle A workflow object handle.
 * @return Total child workflow count.
 */
int workflow_get_children_count(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    return wf != NULL ? wf->ChildCount : 0;
}

/**
 * @brief Get child workflow at specified @p index.
 *
 * @param handle A workflow object handle.
 * @param index Index of a child workflow to get.
 * @return A child workflow object handle, or NULL if index out of range.
 */
ADUC_WorkflowHandle workflow_get_child(ADUC_WorkflowHandle handle, int index)
{
    if (handle == NULL)
    {
        return NULL;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (index == -1)
    {
        index = wf->ChildCount - 1;
    }

    if (index < 0 || index >= wf->ChildCount)
    {
        return NULL;
    }

    return handle_from_workflow(wf->Children[index]);
}

// To append, pass index (-1).
bool workflow_insert_child(ADUC_WorkflowHandle handle, int index, ADUC_WorkflowHandle childHandle)
{
    bool succeeded = false;
    ADUC_Workflow* wf = workflow_from_handle(handle);

    // Expand array if needed.
    if (wf->ChildCount == wf->ChildrenMax)
    {
        size_t newSize = wf->ChildrenMax + WORKFLOW_CHILDREN_BLOCK_SIZE;
        ADUC_Workflow** newArray = malloc(newSize * sizeof(ADUC_Workflow*));
        if (wf->Children != NULL)
        {
            memcpy(newArray, wf->Children, wf->ChildrenMax * sizeof(ADUC_Workflow*));
            free(wf->Children);
        }
        wf->Children = newArray;
        wf->ChildrenMax = newSize;
    }

    if (index < 0 || index >= wf->ChildCount)
    {
        index = wf->ChildCount;
    }
    else
    {
        // Make room for the new child.
        size_t bytes = (wf->ChildCount - index) * sizeof(ADUC_Workflow*);
        memmove(wf->Children + (index + 1), wf->Children + index, bytes);
    }

    wf->Children[index] = workflow_from_handle(childHandle);
    wf->ChildCount++;
    workflow_set_parent(childHandle, handle);

    succeeded = true;
    return succeeded;
}

/**
 * @brief Remove child workflow at specified @p index
 *
 * @param handle
 * @param index An index of the child workflow to be remove.
 * @return ADUC_WorkflowHandle Returns removed child, if succeeded.
 */
ADUC_WorkflowHandle workflow_remove_child(ADUC_WorkflowHandle handle, int index)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);

    // Remove last?
    if (index == -1)
    {
        index = wf->ChildCount - 1;
    }

    if (index < 0 || index >= wf->ChildCount)
    {
        return NULL;
    }

    ADUC_WorkflowHandle child = wf->Children[index];

    if (index < wf->ChildCount - 1)
    {
        int bytes = sizeof(ADUC_Workflow*) * wf->ChildCount - (index + 1);
        memmove(wf->Children + index, wf->Children + (index + 1), bytes);
    }

    wf->ChildCount--;

    workflow_set_parent(child, NULL);
    return child;
}

//
// Workflow state.
//
ADUCITF_State workflow_get_root_state(ADUC_WorkflowHandle handle)
{
    return workflow_get_state(workflow_get_root(handle));
}

ADUCITF_State workflow_get_state(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        return ADUCITF_State_None;
    }

    return wf->State;
}

// Save this workflow's state at the root.
bool workflow_set_state(ADUC_WorkflowHandle handle, ADUCITF_State state)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        return false;
    }

    wf->State = state;

    return true;
}

void workflow_set_result_details(ADUC_WorkflowHandle handle, const char* format, ...)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        return;
    }

    if (format == NULL)
    {
        STRING_empty(wf->ResultDetails);
    }
    else
    {
        char buffer[WORKFLOW_RESULT_DETAILS_MAX_LENGTH];
        va_list arg_list;
        va_start(arg_list, format);
        vsnprintf(buffer, WORKFLOW_RESULT_DETAILS_MAX_LENGTH, format, arg_list);
        va_end(arg_list);

        STRING_delete(wf->ResultDetails);
        wf->ResultDetails = STRING_construct(buffer);
    }
}

void workflow_set_installed_update_id(ADUC_WorkflowHandle handle, const char* installedUpdateId)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        return;
    }

    if (installedUpdateId == NULL)
    {
        STRING_empty(wf->InstalledUpdateId);
    }
    else
    {
        wf->InstalledUpdateId = STRING_construct(installedUpdateId);
    }
}

// The status either stored in the workflow object itself, or in root object.
JSON_Object* _workflow_find_state(ADUC_WorkflowHandle handle, const char* workflowId)
{
    if (handle == NULL || workflowId == NULL || *workflowId == 0)
    {
        return NULL;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf->ResultsObject != NULL && json_object_has_value(wf->ResultsObject, workflowId))
    {
        return json_object_get_object(wf->ResultsObject, workflowId);
    }

    ADUC_Workflow* wfRoot = workflow_from_handle(workflow_get_root(handle));
    if (wfRoot->ResultsObject != NULL && json_object_has_value(wfRoot->ResultsObject, workflowId))
    {
        return json_object_get_object(wfRoot->ResultsObject, workflowId);
    }

    return NULL;
}

void workflow_set_result(ADUC_WorkflowHandle handle, ADUC_Result result)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf != NULL)
    {
        wf->Result = result;
    }
}

ADUC_Result workflow_get_result(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        ADUC_Result result = {};
        return result;
    }

    return wf->Result;
}

const char* workflow_peek_result_details(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        return NULL;
    }

    return STRING_c_str(wf->ResultDetails);
}

const char* workflow_peek_installed_update_id(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        return NULL;
    }

    return STRING_c_str(wf->InstalledUpdateId);
}

void workflow_set_cancellation_type(ADUC_WorkflowHandle handle, ADUC_WorkflowCancellationType cancellationType)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf != NULL)
    {
        wf->CancellationType = cancellationType;
    }
}

ADUC_WorkflowCancellationType workflow_get_cancellation_type(ADUC_WorkflowHandle handle)
{
    ADUC_WorkflowCancellationType result;

    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        result = ADUC_WorkflowCancellationType_Normal;
    }
    else
    {
        result = wf->CancellationType;
    }

    return result;
}

/**
 * @brief sets both cancellation type to retry and retry timestamp.
 * @param handle the workflow handle
 * @param retryToken the retry token with which to update
 * @return true if succeeded.
 */
bool workflow_update_retry_deployment(ADUC_WorkflowHandle handle, const char* retryToken)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    bool result = false;

    if (wf != NULL)
    {
        wf->CancellationType = ADUC_WorkflowCancellationType_Retry;
        result = _workflow_set_retryTimestamp(handle, retryToken);
    }

    return result;
}

/**
 * @brief if operation is in progress on the current workflow, it will set the next workflow as a deferred workflow on the current workflow and set cancellation type to Replacement.
 * @remark The caller must not free the next workflow handle when true is returned as it is owned by the current workflow as a deferred field. Conversely, if false is returned, then
 *             the caller must free the current workflow handle and set the next workflow handle as the current in the workflow data.
 * @param currentWorkflowHandle The workflow handle on the current workflow Data.
 * @param nextWorkflowHandle The workflow handle for the next workflow update deployment.
 * @return true if there was an operation in progress and the next workflow handle got deferred until workCompletionCallback processing.
 */
bool workflow_update_replacement_deployment(
    ADUC_WorkflowHandle currentWorkflowHandle, ADUC_WorkflowHandle nextWorkflowHandle)
{
    bool wasDeferred = false;

    ADUC_Workflow* currentWorkflow = workflow_from_handle(currentWorkflowHandle);

    if (currentWorkflow->OperationInProgress)
    {
        currentWorkflow->CancellationType = ADUC_WorkflowCancellationType_Replacement;
        currentWorkflow->OperationCancelled = true;
        currentWorkflow->DeferredReplacementWorkflow =
            nextWorkflowHandle; // upon return, caller must release ownership as it's owned by current workflow now
        wasDeferred = true;
    }

    return wasDeferred;
}

/**
 * @brief Resets state for retry and replacement deployment processing.
 * @param wf The ADUC workflow state internal representation of a handle.
 */
static void reset_state_for_processing_deployment(ADUC_Workflow* wf)
{
    wf->CurrentWorkflowStep = ADUCITF_WorkflowStep_ProcessDeployment;
    wf->OperationInProgress = false;
    wf->OperationCancelled = false;
    wf->CancellationType = ADUC_WorkflowCancellationType_None;
}

/**
 * @brief Resets state to process the deferred workflow deployment, which is also transferred to the current.
 * @param handle The workflow handle on the current workflow Data.
 */
void workflow_update_for_replacement(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);

    ADUC_Workflow* deferred = wf->DeferredReplacementWorkflow;
    wf->DeferredReplacementWorkflow = NULL;
    workflow_transfer_data(handle /* wfTarget */, deferred /* wfSource */);

    reset_state_for_processing_deployment(wf);
}

/**
 * @brief Resets state to reprocess the current workflow deployment.
 * @param handle The workflow handle on the current workflow Data.
 */
void workflow_update_for_retry(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);

    reset_state_for_processing_deployment(wf);
}

// If succeeded, free existing install state data and replace with a new one.
// If failed, no changes to the handle.
bool workflow_read_state_from_file(ADUC_WorkflowHandle handle, const char* stateFilename)
{
    if (handle == NULL || stateFilename == NULL || *stateFilename == 0)
    {
        return false;
    }

    JSON_Value* rootValue = json_parse_file(stateFilename);
    if (rootValue == NULL)
    {
        return 0;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf->ResultsObject != NULL)
    {
        json_value_free(json_object_get_wrapping_value(wf->ResultsObject));
    }

    wf->ResultsObject = json_value_get_object(rootValue);
    return true;
}

bool workflow_is_cancel_requested(ADUC_WorkflowHandle handle)
{
    return workflow_get_boolean_property(workflow_get_root(handle), WORKFLOW_PROPERTY_FIELD_CANCEL_REQUESTED);
}

bool workflow_is_agent_restart_requested(ADUC_WorkflowHandle handle)
{
    return workflow_get_boolean_property(workflow_get_root(handle), WORKFLOW_PROPERTY_FIELD_AGENT_RESTART_REQUESTED);
}

bool workflow_is_immediate_agent_restart_requested(ADUC_WorkflowHandle handle)
{
    return workflow_get_boolean_property(
        workflow_get_root(handle), WORKFLOW_PROPERTY_FIELD_IMMEDIATE_AGENT_RESTART_REQUESTED);
}

bool workflow_is_reboot_requested(ADUC_WorkflowHandle handle)
{
    return workflow_get_boolean_property(workflow_get_root(handle), WORKFLOW_PROPERTY_FIELD_REBOOT_REQUESTED);
}

bool workflow_is_immediate_reboot_requested(ADUC_WorkflowHandle handle)
{
    return workflow_get_boolean_property(
        workflow_get_root(handle), WORKFLOW_PROPERTY_FIELD_IMMEDIATE_REBOOT_REQUESTED);
}

bool workflow_request_reboot(ADUC_WorkflowHandle handle)
{
    return workflow_set_boolean_property(workflow_get_root(handle), WORKFLOW_PROPERTY_FIELD_REBOOT_REQUESTED, true);
}

bool workflow_request_immediate_reboot(ADUC_WorkflowHandle handle)
{
    return workflow_set_boolean_property(
        workflow_get_root(handle), WORKFLOW_PROPERTY_FIELD_IMMEDIATE_REBOOT_REQUESTED, true);
}

bool workflow_request_agent_restart(ADUC_WorkflowHandle handle)
{
    return workflow_set_boolean_property(
        workflow_get_root(handle), WORKFLOW_PROPERTY_FIELD_AGENT_RESTART_REQUESTED, true);
}

bool workflow_request_immediate_agent_restart(ADUC_WorkflowHandle handle)
{
    return workflow_set_boolean_property(
        workflow_get_root(handle), WORKFLOW_PROPERTY_FIELD_IMMEDIATE_AGENT_RESTART_REQUESTED, true);
}

/**
 * @brief Compare id of @p handle0 and @p handle1
 *
 * @param handle0
 * @param handle1
 * @return 0 if ids are equal.
 */
int workflow_id_compare(ADUC_WorkflowHandle handle0, ADUC_WorkflowHandle handle1)
{
    char* id0 = workflow_get_id(handle0);
    char* id1 = workflow_get_id(handle1);
    int result = -1;

    if (id0 == NULL || id1 == NULL)
    {
        Log_Error("Missing workflow id (id0:%s, id1:%s)", id0, id1);
        goto done;
    }

    result = strcmp(id0, id1);

done:
    workflow_free_string(id0);
    workflow_free_string(id1);
    return result;
}

/**
 * @brief Compare id of @p handle and @p workflowId. No memory is allocated or freed by this function.
 *
 * @param handle The handle for first workflow id.
 * @param workflowId The c-string for the second workflow id.
 * @return int Returns 0 if ids are equal.
 */
bool workflow_isequal_id(ADUC_WorkflowHandle handle, const char* workflowId)
{
    const char* id = workflow_peek_id(handle);
    if (id == NULL)
    {
        Log_Error("invalid handle: null id");
        return false;
    }

    return workflowId != NULL && strcmp(id, workflowId) == 0;
}

/**
 * @brief Create a new workflow data handler using base workflow and serialized 'instruction' json string.
 * Note: The 'workfolder' of the returned workflow data object will be the same as the base's.
 *
 * @param base The base workflow containing valid Update Action and Manifest.
 * @param instruction A serialized json string containing single 'installItem' from an instruction file.
 * @param handle An output workflow handle.
 * @return ADUC_Result
 */
ADUC_Result
workflow_create_from_instruction(ADUC_WorkflowHandle base, const char* instruction, ADUC_WorkflowHandle* handle)
{
    ADUC_Result result;

    // Parse instruction json.
    JSON_Value* instructionValue = json_parse_string(instruction);
    if (instructionValue == NULL)
    {
        Log_Error("Invalid intruction entry.");
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_PARSE_INSTRUCTION_ENTRY_FAILURE;
        goto done;
    }

    result = workflow_create_from_instruction_value(base, instructionValue, handle);

done:
    json_value_free(instructionValue);
    return result;
}

/**
 * @brief Create a new workflow data handler using base workflow and serialized 'instruction' json string.
 * Note: The 'workfolder' of the returned workflow data object will be the same as the base's.
 *
 * @param base The base workflow containing valid Update Action and Manifest.
 * @param instruction A JSON_Value object contain install-item data.
 * @param handle An output workflow handle.
 * @return ADUC_Result
 */
ADUC_Result
workflow_create_from_instruction_value(ADUC_WorkflowHandle base, JSON_Value* instruction, ADUC_WorkflowHandle* handle)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure };
    JSON_Status jsonStatus = JSONFailure;
    JSON_Value* updateActionValue = NULL;
    JSON_Value* updateManifestValue = NULL;
    ADUC_Workflow* wf = NULL;

    if (base == NULL || instruction == NULL || handle == NULL)
    {
        goto done;
    }

    *handle = NULL;

    ADUC_Workflow* wfBase = workflow_from_handle(base);

    wf = malloc(sizeof(*wf));
    if (wf == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    memset(wf, 0, sizeof(*wf));

    updateActionValue = json_value_deep_copy(json_object_get_wrapping_value(wfBase->UpdateActionObject));
    if (updateActionValue == NULL)
    {
        Log_Error("Cannot copy Update Action json from base");
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_COPY_UPDATE_ACTION_FROM_BASE_FAILURE;
        goto done;
    }

    JSON_Object* updateActionObject = json_object(updateActionValue);

    updateManifestValue = json_value_deep_copy(json_object_get_wrapping_value(wfBase->UpdateManifestObject));
    if (updateManifestValue == NULL)
    {
        Log_Error("Cannot copy Update Manifest json from base");
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_COPY_UPDATE_ACTION_FROM_BASE_FAILURE;
        goto done;
    }

    JSON_Object* updateManifestObject = json_object(updateManifestValue);
    JSON_Object* instructionObject = json_object(instruction);

    char* currentInstructionData = json_serialize_to_string_pretty(instruction);
    Log_Debug("Processing current instruction:\n%s", currentInstructionData);
    json_free_serialized_string(currentInstructionData);

    // Replace 'updateType'.
    const char* updateType = json_object_get_string(instructionObject, ADUCITF_FIELDNAME_UPDATETYPE);
    if (updateType == NULL || *updateType == 0)
    {
        Log_Error("Invalid instruction entry.");
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_PARSE_INSTRUCTION_ENTRY_NO_UPDATE_TYPE;
        goto done;
    }
    jsonStatus = json_object_set_string(updateManifestObject, ADUCITF_FIELDNAME_UPDATETYPE, updateType);
    if (jsonStatus == JSONFailure)
    {
        Log_Error("Cannot update instruction entry updateType.");
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_COPY_UPDATE_ACTION_SET_UPDATE_TYPE_FAILURE;
        goto done;
    }

    // Keep only file needed by this entry. Remove the rest.
    JSON_Array* instFiles = json_object_get_array(instructionObject, ADUCITF_FIELDNAME_FILES);
    JSON_Object* baseFiles = json_object_get_object(updateManifestObject, ADUCITF_FIELDNAME_FILES);
    int fileCount = json_object_get_count(baseFiles);
    for (int b = fileCount - 1; b >= 0; b--)
    {
        JSON_Object* baseFile = json_object(json_object_get_value_at(baseFiles, b));

        // If file is in the instruction, merge their properties.
        bool fileRequired = false;
        int instFilesCount = json_array_get_count(instFiles);
        for (int i = instFilesCount - 1; i >= 0; i--)
        {
            const char* baseFilename = json_object_get_string(baseFile, ADUCITF_FIELDNAME_FILENAME);

            JSON_Object* instFile = json_array_get_object(instFiles, i);
            const char* instFilename = json_object_get_string(instFile, ADUCITF_FIELDNAME_FILENAME);

            if (baseFilename != NULL && instFilename != NULL && strcmp(baseFilename, instFilename) == 0)
            {
                // Found.
                fileRequired = true;
                int valuesCount = json_object_get_count(instFile);
                for (int v = valuesCount - 1; v >= 0; v--)
                {
                    const char* key = json_object_get_name(instFile, v);
                    JSON_Value* val = json_value_deep_copy(json_object_get_value_at(instFile, v));
                    json_object_set_value(baseFile, key, val);
                }
                // Done with this file, remove it (and free).
                json_array_remove(instFiles, i);
                break;
            }
        }

        // Remove file from base files list, if no longer needed.
        if (!fileRequired)
        {
            json_object_remove(baseFiles, json_object_get_name(baseFiles, b));
        }
    }

    wf->UpdateActionObject = updateActionObject;
    wf->UpdateManifestObject = updateManifestObject;

    {
        char* baseWorkfolder = workflow_get_workfolder(base);
        workflow_set_workfolder(wf, baseWorkfolder);
        workflow_free_string(baseWorkfolder);
    }

    *handle = wf;
    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;

done:
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        json_value_free(updateActionValue);
        json_value_free(updateManifestValue);
        workflow_free(wf);
    }

    return result;
}

/**
 * @brief Get update manifest instructions steps count.
 *
 * @param handle A workflow object handle.
 *
 * @return Total count of the update instruction steps.
 */
size_t workflow_get_instructions_steps_count(ADUC_WorkflowHandle handle)
{
    return json_array_get_count(workflow_get_instructions_steps_array(handle));
}

/**
 * @brief Get a read-only update manifest step type
 *
 * @param handle A workflow object handle.
 * @param stepIndex A step index.
 *
 * @return const char* Read-only step type string, or NULL if specified step or step 'type' property doesn't exist.
 */
const char* workflow_peek_step_type(ADUC_WorkflowHandle handle, size_t stepIndex)
{
    JSON_Object* step = json_array_get_object(workflow_get_instructions_steps_array(handle), stepIndex);
    if (step == NULL)
    {
        return NULL;
    }

    const char* stepType = json_object_get_string(step, STEP_PROPERTY_FIELD_TYPE);
    if (stepType == NULL)
    {
        return DEAULT_STEP_TYPE;
    }

    return stepType;
}

/**
 * @brief Get a read-only handlerProperties string value.
 *
 * @param handle A workflow object handle.
 * @param propertyName
 *
 * @return A read-only string value of specified property in handlerProperties map.
 */
const char*
workflow_peek_update_manifest_handler_properties_string(ADUC_WorkflowHandle handle, const char* propertyName)
{
    const JSON_Object* manifest = _workflow_get_update_manifest(handle);
    const JSON_Object* properties = json_object_get_object(manifest, STEP_PROPERTY_FIELD_HANDLER_PROPERTIES);
    return json_object_get_string(properties, propertyName);
}

/**
 * @brief Returns whether the specified step is an 'inline' step.
 *
 * @param handle A workflow object handle.
 * @param stepIndex A step index.
 *
 * @return bool Return true if the specified step exists, and is inline step.
 * Otherwise, return false.
 *
 */
bool workflow_is_inline_step(ADUC_WorkflowHandle handle, size_t stepIndex)
{
    JSON_Object* step = json_array_get_object(workflow_get_instructions_steps_array(handle), stepIndex);
    if (step == NULL)
    {
        return false;
    }

    const char* stepType = json_object_get_string(step, STEP_PROPERTY_FIELD_TYPE);
    if (stepType != NULL && strcmp(stepType, "reference") == 0)
    {
        return false;
    }

    return true;
}

/**
 * @brief
 *
 * @param handle A workflow object handle.
 * @param stepIndex A step index.
 * @return const char* Read-only step handler name, or NULL for a 'reference' step.
 */
const char* workflow_peek_update_manifest_step_handler(ADUC_WorkflowHandle handle, size_t stepIndex)
{
    JSON_Object* step = json_array_get_object(workflow_get_instructions_steps_array(handle), stepIndex);
    if (step == NULL)
    {
        return false;
    }

    const char* stepHandler = json_object_get_string(step, STEP_PROPERTY_FIELD_HANDLER);
    return stepHandler;
}

/**
 * @brief Gets a reference step update manifest file at specified index.
 *
 * @param handle A workflow data object handle.
 * @param stepIndex A step index.
 * @param entity An output reference step update manifest file entity object.
 *               Caller must free the object with workflow_free_file_entity().
 * @return true If succeeded.
 */
bool workflow_get_step_detached_manifest_file(ADUC_WorkflowHandle handle, size_t stepIndex, ADUC_FileEntity** entity)
{
    if (entity == NULL)
    {
        return false;
    }

    size_t count = workflow_get_instructions_steps_count(handle);
    if (stepIndex >= count)
    {
        return false;
    }

    _Bool succeeded = false;
    JSON_Object* step = json_array_get_object(workflow_get_instructions_steps_array(handle), stepIndex);
    const char* fileId = json_object_get_string(step, STEP_PROPERTY_FIELD_DETACHED_MANIFEST_FILE_ID);
    const JSON_Object* files = _workflow_get_update_manifest_files_map(handle);
    const JSON_Object* file = json_object_get_object(files, fileId);
    const JSON_Object* fileUrls = NULL;
    const char* uri = NULL;
    const char* name = NULL;
    size_t tempHashCount = 0;
    ADUC_Hash* tempHash = NULL;

    *entity = NULL;

    // Find fileurls map in this workflow, and its enclosing workflow(s).
    ADUC_WorkflowHandle h = handle;

    do
    {
        if ((fileUrls = _workflow_get_fileurls_map(h)) == NULL)
        {
            Log_Warn("'fileUrls' property not found.");
        }
        else
        {
            uri = json_object_get_string(fileUrls, fileId);
        }
        h = workflow_get_parent(h);
    } while (uri == NULL && h != NULL);

    if (uri == NULL)
    {
        goto done;
    }

    name = json_object_get_string(file, ADUCITF_FIELDNAME_FILENAME);
    const JSON_Object* hashObj = json_object_get_object(file, ADUCITF_FIELDNAME_HASHES);

    tempHash = ADUC_HashArray_AllocAndInit(hashObj, &tempHashCount);
    if (tempHash == NULL)
    {
        Log_Error("Unable to parse hashes for file @ %zu", stepIndex);
        goto done;
    }

    size_t sizeInBytes = 0;
    if (json_object_has_value(file, ADUCITF_FIELDNAME_SIZEINBYTES))
    {
        sizeInBytes = json_object_get_number(file, ADUCITF_FIELDNAME_SIZEINBYTES);
    }

    *entity = malloc(sizeof(**entity));
    if (*entity == NULL)
    {
        goto done;
    }

    if (!ADUC_FileEntity_Init(*entity, fileId, name, uri, NULL /*arguments*/, tempHash, tempHashCount, sizeInBytes))
    {
        Log_Error("Invalid file entity arguments");
        goto done;
    }

    succeeded = true;

done:

    if (!succeeded)
    {
        if (*entity != NULL)
        {
            ADUC_FileEntity_Uninit(*entity);
            free(*entity);
            *entity = NULL;
        }
    }

    return succeeded;
}

/**
 * @brief Gets a serialized json string of the specified workflow's Update Manifest.
 *
 * @param handle A workflow data object handle.
 * @param pretty Whether to format the output string.
 * @return char* An output json string. Caller must free the string with workflow_free_string().
 */
char* workflow_get_serialized_update_manifest(ADUC_WorkflowHandle handle, bool pretty)
{
    const JSON_Object* o = _workflow_get_update_manifest(handle);
    if (pretty)
    {
        return json_serialize_to_string_pretty(json_object_get_wrapping_value(o));
    }

    return json_serialize_to_string(json_object_get_wrapping_value(o));
}

EXTERN_C_END
