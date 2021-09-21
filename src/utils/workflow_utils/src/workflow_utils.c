/**
 * @file workflow_utils.c
 * @brief Utility fuctions for workflow data.
 *
 * @copyright Copyright (c) Microsoft Corp.
 */
#include "aduc/workflow_utils.h"
#include "aduc/adu_types.h"
#include "aduc/c_utils.h"
#include "aduc/hash_utils.h"
#include "aduc/logging.h"
#include "aduc/parser_utils.h"
#include "aduc/result.h"
#include "aduc/string_c_utils.h"
#include "aduc/system_utils.h"
#include "aduc/types/update_content.h"
#include "jws_utils.h"

#include <azure_c_shared_utility/crt_abstractions.h> // for mallocAndStrcpy_s
#include <azure_c_shared_utility/strings.h> // for STRING_*
#include <parson.h>
#include <stdarg.h> // for va_*
#include <stdlib.h> // for calloc, atoi
#include <string.h>

#define DEFAULT_SANDBOX_ROOT_PATH ADUC_DOWNLOADS_FOLDER

#define WORKFLOW_PROPERTY_FIELD_ID "_id"
#define WORKFLOW_PROPERTY_FIELD_WORKFLOW_DOT_ID "workflow.id"
#define WORKFLOW_PROPERTY_FIELD_WORKFLOW_DOT_ACTION "workflow.action"
#define WORKFLOW_PROPERTY_FIELD_SANDBOX_ROOTPATH "_sandboxRootPath"
#define WORKFLOW_PROPERTY_FIELD_WORKFOLDER "_workFolder"
#define WORKFLOW_PROPERTY_FIELD_CANCEL_REQUESTED "_cancelRequested"
#define WORKFLOW_PROPERTY_FIELD_REBOOT_REQUESTED "_rebootRequested"
#define WORKFLOW_PROPERTY_FIELD_IMMEDIATE_REBOOT_REQUESTED "_immediateRebootRequested"
#define WORKFLOW_PROPERTY_FIELD_AGENT_RESTART_REQUESTED "_agentRestartRequested"
#define WORKFLOW_PROPERTY_FIELD_IMMEDIATE_AGENT_RESTART_REQUESTED "_immediateAgentRestartRequested"
#define WORKFLOW_PROPERTY_FIELD_SELECTED_COMPONENTS "_selectedComponents"

#define WORKFLOW_CHILDREN_BLOCK_SIZE 10

/**
 * @brief Maximum length for the 'resultDetails' string.
 */
#define WORKFLOW_RESULT_DETAILS_MAX_LENGTH 1024

/**
 * @brief Minimum supported Update Manifest version
 */
#define MINIMUM_SUPPORTED_UPDATE_MANIFEST_VERSION 2

EXTERN_C_BEGIN

/**
 * @brief A struct contianing data needed for an update workflow.
 * 
 */
typedef struct tagADUC_Workflow
{
    JSON_Object* UpdateActionObject;
    JSON_Object* UpdateManifestObject;
    JSON_Object* PropertiesObject;
    JSON_Object* ResultsObject;

    ADUCITF_State State;
    ADUC_Result Result;
    STRING_HANDLE ResultDetails;
    STRING_HANDLE InstalledUpdateId;

    struct tagADUC_Workflow* Parent;
    struct tagADUC_Workflow** Children;
    size_t ChildrenMax;
    size_t ChildCount;

    bool OperationInProgress; /**< Is an upper-level method currently in progress? */
    bool OperationCancelled; /**< Was the operation in progress requested to cancel? */

} ADUC_Workflow;

static ADUCITF_State g_LastReportedState; /**< Previously reported state (for state validation). */

//
// Private functions - this is an adapter for the underlying ADUC_Workflow object.
//

/**
 * @brief Deep copy string. Caller must call workflow_free_string() when done.
 * 
 * @param s Input string.
 * @return A copy of input string if succeeded. Otherwise, return NULL.
 */
char* _workflow_copy_string(const char* s)
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
                Log_Error("No upadteManifest\n%s", s);
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

        if (validateManifest)
        {
            int manifestVersion = workflow_get_update_manifest_version(handle_from_workflow(wf));
            if (manifestVersion < MINIMUM_SUPPORTED_UPDATE_MANIFEST_VERSION)
            {
                result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_UNSUPPORTED_UPDATE_MANIFEST_VERSION;
                goto done;
            }
        }
    }

    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;

done:

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
 * @brief Get update manifest vesion.
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

    return JSONSuccess == json_object_set_string(wf->PropertiesObject, property, value);
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

bool workflow_set_workfolder(ADUC_WorkflowHandle handle, const char* workfolder)
{
    return workflow_set_string_property(handle, WORKFLOW_PROPERTY_FIELD_WORKFOLDER, workfolder);
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
 * @brief Get 'updateManifest.bundledUpdates' array.
 * 
 * @param handle A workflow object handle.
 * @return const JSON_Object* A map containing bundle update manifest files information.
 */
JSON_Array* _workflow_get_update_manifest_bundle_updates_map(ADUC_WorkflowHandle handle)
{
    const JSON_Object* o = _workflow_get_update_manifest(handle);
    return json_object_get_array(o, "bundledUpdates");
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
 *         Caller must call 'workflow_free_string' function to free the memery when done.
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
 *         Caller must call 'workflow_free_string' function to free the memery when done.
 */
char* workflow_get_installed_criteria(ADUC_WorkflowHandle handle)
{
    return workflow_get_update_manifest_string_property(handle, ADUCITF_FIELDNAME_INSTALLEDCRITERIA);
}

/**
 * @brief Get the Update Manifest 'compatibility' array, in serialized json string format. 
 * 
 * @param handle A workflow handle.
 * @return char* If success, returns a serialized json string. Otherwise, returns NULL.
 *         Caller must call 'workflow_free_string' function to free the memery when done.
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

/**
 * @brief Get the last reported state.
 * 
 * @return ADUCITF_State Return the last reported agent state.
 */
ADUCITF_State workflow_get_last_reported_state()
{
    return g_LastReportedState;
}

/**
 * @brief Set the last reported agent state.
 * 
 * @param lastReportedState The agent state reported to the IoT Hub.
 */
void workflow_set_last_reported_state(ADUCITF_State lastReportedState)
{
    g_LastReportedState = lastReportedState;
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
    ADUC_Workflow* wf = workflow_from_handle(handle);
    return (wf != NULL) && wf->OperationInProgress;
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
    ADUC_Workflow* wf = workflow_from_handle(handle);
    return wf != NULL && wf->OperationCancelled;
}

/**
 * @brief Get an Update Action code.
 * 
 * @param handle A workflow object handle.
 * @return ADUCITF_UpdateAction Returns ADUCITF_UpdateAction_Undefined if no specified.
 * Otherwise, returns ADUCITF_UpdateAction_Download, ADUCITF_UpdateAction_Install, ADUCITF_UpdateAction_Apply
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
    *entity = NULL;
    const char* uri = NULL;
    const char* fileId = NULL;
    const char* name = NULL;
    const char* arguments = NULL;

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

    name = json_object_get_string(file, ADUCITF_FIELDNAME_FILENAME);
    arguments = json_object_get_string(file, ADUCITF_FIELDNAME_ARGUMENTS);

    const JSON_Object* hashObj = json_object_get_object(file, ADUCITF_FIELDNAME_HASHES);

    size_t tempHashCount = 0;
    ADUC_Hash* tempHash = ADUC_HashArray_AllocAndInit(hashObj, &tempHashCount);
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

    *entity = malloc(sizeof(**entity));
    if (*entity == NULL)
    {
        goto done;
    }

    if (!ADUC_FileEntity_Init(*entity, fileId, name, uri, arguments, tempHash, tempHashCount, sizeInBytes))
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

bool workflow_get_first_update_file_of_type(ADUC_WorkflowHandle handle, const char* fileType, ADUC_FileEntity** entity)
{
    if (entity == NULL)
    {
        return false;
    }

    _Bool succeeded = false;
    const JSON_Object* files = NULL;
    const JSON_Object* file = NULL;
    const JSON_Object* fileUrls = NULL;
    *entity = NULL;
    const char* uri = NULL;
    const char* fileId = NULL;
    const char* name = NULL;
    const char* arguments = NULL;

    if ((files = _workflow_get_update_manifest_files_map(handle)) == NULL)
    {
        goto done;
    }

    size_t count = json_object_get_count(files);
    size_t index;
    for (index = 0; index < count; index++)
    {
        file = json_object(json_object_get_value_at(files, index));
        const char* t = json_object_get_string(file, "fileType");
        if (t != NULL && strcmp(t, fileType) == 0)
        {
            fileId = json_object_get_name(files, index);
            break;
        }
        file = NULL;
    }

    // No file with matching 'fileType'?
    if (file == NULL)
    {
        goto done;
    }

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

    name = json_object_get_string(file, ADUCITF_FIELDNAME_FILENAME);
    arguments = json_object_get_string(file, ADUCITF_FIELDNAME_ARGUMENTS);

    const JSON_Object* hashObj = json_object_get_object(file, ADUCITF_FIELDNAME_HASHES);

    size_t tempHashCount = 0;
    ADUC_Hash* tempHash = ADUC_HashArray_AllocAndInit(hashObj, &tempHashCount);
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

    *entity = malloc(sizeof(**entity));
    if (*entity == NULL)
    {
        goto done;
    }

    if (!ADUC_FileEntity_Init(*entity, fileId, name, uri, arguments, tempHash, tempHashCount, sizeInBytes))
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
 * @brief Get total count of Components Update files in the Bundle Update.
 * 
 * @param handle A workflow object handle.
 * @return Total count of components update files.
 */
size_t workflow_get_bundle_updates_count(ADUC_WorkflowHandle handle)
{
    JSON_Array* files = _workflow_get_update_manifest_bundle_updates_map(handle);
    return json_array_get_count(files);
}

/**
 * @brief Get a 'bundleFiles' entity at specified @p index.
 * 
 * @param handle A workflow object handle.
 * @param index  File entity index.
 * @param entity Output file entity object.
 * @return True if success.
 */
bool workflow_get_bundle_updates_file(ADUC_WorkflowHandle handle, size_t index, ADUC_FileEntity** entity)
{
    if (entity == NULL)
    {
        return false;
    }

    size_t count = workflow_get_bundle_updates_count(handle);
    if (index >= count)
    {
        return false;
    }

    _Bool succeeded = false;
    const JSON_Array* files = NULL;
    const JSON_Object* file = NULL;
    const JSON_Object* fileUrls = NULL;
    *entity = NULL;
    const char* uri = NULL;
    const char* fileId = NULL;
    const char* name = NULL;
    const char* arguments = NULL;

    if ((files = _workflow_get_update_manifest_bundle_updates_map(handle)) == NULL)
    {
        goto done;
    }

    file = json_array_get_object(files, index);
    fileId = json_object_get_string(file, "fileId");

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

    name = json_object_get_string(file, ADUCITF_FIELDNAME_FILENAME);
    arguments = json_object_get_string(file, ADUCITF_FIELDNAME_ARGUMENTS);

    const JSON_Object* hashObj = json_object_get_object(file, ADUCITF_FIELDNAME_HASHES);

    size_t tempHashCount = 0;
    ADUC_Hash* tempHash = ADUC_HashArray_AllocAndInit(hashObj, &tempHashCount);
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

    *entity = malloc(sizeof(**entity));
    if (*entity == NULL)
    {
        goto done;
    }

    if (!ADUC_FileEntity_Init(*entity, fileId, name, uri, arguments, tempHash, tempHashCount, sizeInBytes))
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
 * @brief Get a property of type 'string' in the workflow update manfiest.
 * 
 * @param handle A workflow object handle.
 * @param propertyName The name of a property to get.
 * @return A copy of specified property. Caller must call workflow_free_string when done with the value.
 */
char* workflow_get_update_manifest_string_property(ADUC_WorkflowHandle handle, const char* propertyName)
{
    return _workflow_copy_string(workflow_peek_update_manifest_string(handle, propertyName));
}

/**
 * @brief Get a 'Compatibility' entry of the workflow at a specified @p index.
 * 
 * @param handle A workflow object handle.
 * @param index Index of the compatibility set to.
 * @return A copy of compatibility entry. Calle must call workflow_free_string when done with the value.
 */
char* workflow_get_update_manifest_compatibility(ADUC_WorkflowHandle handle, size_t index)
{
    JSON_Array* array = _workflow_peek_update_manifest_array(handle, "compatibility");
    JSON_Object* object = json_array_get_object(array, index);
    char* output = NULL;
    if (object != NULL)
    {
        char* s = json_serialize_to_string(json_object_get_wrapping_value(object));
        output = _workflow_copy_string(s);
        json_free_serialized_string(s);
    }

    return output;
}

/**
 * @brief Get update type of the specified workflow.
 * 
 * @param handle A workflow object handle.
 * @return An UpdateType string.
 */
char* workflow_get_update_type(ADUC_WorkflowHandle handle)
{
    return workflow_get_update_manifest_string_property(handle, ADUCITF_FIELDNAME_UPDATETYPE);
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
 * @brief Transfer action data from @p sourceHandle to @p targetHandle.
 * The sourceHandle will no longer contains transfered action data.
 * Caller should not use sourceHandle for other workflow related purposes.
 * 
 * @param targetHandle 
 * @param sourceHandle 
 * @return true 
 * @return false 
 */
bool workflow_update_action_data(ADUC_WorkflowHandle targetHandle, ADUC_WorkflowHandle sourceHandle)
{
    ADUC_Workflow* wfTarget = workflow_from_handle(targetHandle);
    ADUC_Workflow* wfSource = workflow_from_handle(sourceHandle);

    if (wfSource == NULL || wfSource->UpdateActionObject == NULL)
    {
        return false;
    }

    if (wfTarget->UpdateActionObject != NULL)
    {
        json_value_free(json_object_get_wrapping_value(wfTarget->UpdateActionObject));
    }
    wfTarget->UpdateActionObject = wfSource->UpdateActionObject;
    wfSource->UpdateActionObject = NULL;

    if (wfTarget->UpdateManifestObject != NULL)
    {
        json_value_free(json_object_get_wrapping_value(wfTarget->UpdateManifestObject));
    }
    wfTarget->UpdateManifestObject = wfSource->UpdateManifestObject;
    wfSource->UpdateManifestObject = NULL;

    return true;
}

/**
 * @brief Instantiate and initialize workflow object with info from the given jsonData.
 * 
 * @param updateManifestJson A JSON string containing udpate manifest data.
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
        // Reutrn 'workflow.id' from Action json data.
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
    return _workflow_copy_string(workflow_peek_id(handle));
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
bool workflow_set_root_state(ADUC_WorkflowHandle handle, ADUCITF_State state)
{
    return workflow_set_state(workflow_get_root(handle), state);
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
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_COPY_UPDATE_ACITON_FROM_BASE_FAILURE;
        goto done;
    }

    JSON_Object* updateActionObject = json_object(updateActionValue);

    updateManifestValue = json_value_deep_copy(json_object_get_wrapping_value(wfBase->UpdateManifestObject));
    if (updateManifestValue == NULL)
    {
        Log_Error("Cannot copy Update Manifest json from base");
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_COPY_UPDATE_ACITON_FROM_BASE_FAILURE;
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

EXTERN_C_END