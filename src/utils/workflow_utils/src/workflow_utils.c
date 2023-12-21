/**
 * @file workflow_utils.c
 * @brief Utility functions for workflow data.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/workflow_utils.h"
#include "aduc/adu_types.h"
#include "aduc/aduc_inode.h" // ADUC_INODE_SENTINEL_VALUE
#include "aduc/c_utils.h"
#include "aduc/config_utils.h"
#include "aduc/extension_manager.h"
#include "aduc/hash_utils.h"
#include "aduc/logging.h"
#include "aduc/parser_utils.h"
#include "aduc/path_utils.h"
#include "aduc/reporting_utils.h"
#include "aduc/result.h"
#include "aduc/string_c_utils.h"
#include "aduc/system_utils.h"
#include "aduc/types/update_content.h"
#include "aduc/types/workflow.h"
#include "aduc/workflow_internal.h"
#include "azure_c_shared_utility/crt_abstractions.h" // for mallocAndStrcpy_s
#include "azure_c_shared_utility/strings.h" // for STRING_*
#include "jws_utils.h"
#include "root_key_util.h"

#include <parson.h>
#include <stdarg.h> // for va_*
#include <stdlib.h> // for malloc, atoi
#include <string.h>

#include <aducpal/limits.h> // for PATH_MAX
#include <aducpal/strings.h> // strcasecmp

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
#define DEFAULT_STEP_TYPE "reference"
#define WORKFLOW_PROPERTY_FIELD_INSTRUCTIONS_DOT_STEPS "instructions.steps"
#define UPDATE_MANIFEST_PROPERTY_FIELD_DETACHED_MANIFEST_FILE_ID "detachedManifestFileId"
#define STEP_PROPERTY_FIELD_DETACHED_MANIFEST_FILE_ID UPDATE_MANIFEST_PROPERTY_FIELD_DETACHED_MANIFEST_FILE_ID
#define STEP_PROPERTY_FIELD_TYPE "type"
#define STEP_PROPERTY_FIELD_HANDLER "handler"
#define STEP_PROPERTY_FIELD_FILES "files"
#define STEP_PROPERTY_FIELD_HANDLER_PROPERTIES "handlerProperties"

#define WORKFLOW_CHILDREN_BLOCK_SIZE 10
#define WORKFLOW_MAX_SUCCESS_ERC 8

/**
 * @brief Maximum length for the 'resultDetails' string.
 */
#define WORKFLOW_RESULT_DETAILS_MAX_LENGTH 1024

/* external linkage */
extern ExtensionManager_Download_Options Default_ExtensionManager_Download_Options;

// forward decls
const JSON_Object* _workflow_get_fileurls_map(ADUC_WorkflowHandle handle);

//
// Private functions - this is an adapter for the underlying ADUC_Workflow object.
//

/**
 * @brief Frees an ADUC_Property object.
 * @param property The property to be freed.
 */
static void ADUC_Property_UnInit(ADUC_Property* property)
{
    if (property == NULL)
    {
        return;
    }

    free(property->Name);
    property->Name = NULL;

    free(property->Value);
    property->Value = NULL;
}

/**
 * @brief Allocates the memory for the ADUC_Property struct member values
 * @param outProperty A pointer to an ADUC_Property struct whose member values will be allocated
 * @param name The property name
 * @param value The property value
 * @returns True if successfully allocated, False if failure
 */
static bool ADUC_Property_Init(ADUC_Property* outProperty, const char* name, const char* value)
{
    bool success = false;

    if (outProperty == NULL)
    {
        return false;
    }

    if (name == NULL || value == NULL)
    {
        return false;
    }

    outProperty->Name = NULL;
    outProperty->Value = NULL;

    if (mallocAndStrcpy_s(&(outProperty->Name), name) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(outProperty->Value), value) != 0)
    {
        goto done;
    }

    success = true;

done:

    if (!success)
    {
        ADUC_Property_UnInit(outProperty);
    }

    return success;
}

/**
 * @brief Frees an array of ADUC_Property of size @p propertiesCount
 * @param propertiesCount the size of @p propertiesArray
 * @param propertiesArray a pointer to an array of ADUC_Property structs
 */
static void ADUC_Properties_FreeArray(size_t propertiesCount, ADUC_Property* propertiesArray)
{
    for (size_t i = 0; i < propertiesCount; ++i)
    {
        ADUC_Property_UnInit(&propertiesArray[i]);
    }

    free(propertiesArray);
}

/**
 * @brief Allocates memory and populate with ADUC_Property object from a Parson JSON_Object.
 *
 * Caller MUST assume that this method allocates the memory for the returned ADUC_Property pointer.
 *
 * @param propertiesObj JSON Object that contains the properties to be returned.
 * @param propertiesCount A size_t* where the count of output properties will be stored.
 * @returns If success, a pointer to an array of ADUC_Property object. Otherwise, returns NULL.
 *  Caller must call ADUC_FileEntityArray_Free() to free the array.
 */
ADUC_Property* ADUC_PropertiesArray_AllocAndInit(const JSON_Object* propertiesObj, size_t* propertiesCount)
{
    ADUC_Property* tempPropertyArray = NULL;

    if (propertiesCount == NULL || propertiesObj == NULL)
    {
        return NULL;
    }
    *propertiesCount = 0;

    size_t tempPropertyCount = json_object_get_count(propertiesObj);

    if (tempPropertyCount == 0)
    {
        Log_Error("No properties");
        goto done;
    }

    tempPropertyArray = calloc(tempPropertyCount, sizeof(*tempPropertyArray));

    if (tempPropertyArray == NULL)
    {
        goto done;
    }

    for (size_t properties_index = 0; properties_index < tempPropertyCount; ++properties_index)
    {
        ADUC_Property* currProperty = tempPropertyArray + properties_index;

        const char* propertiesName = json_object_get_name(propertiesObj, properties_index);
        const char* propertiesValue = json_value_get_string(json_object_get_value_at(propertiesObj, properties_index));
        if (!ADUC_Property_Init(currProperty, propertiesName, propertiesValue))
        {
            goto done;
        }
    }

    *propertiesCount = tempPropertyCount;

done:

    if (*propertiesCount == 0 && tempPropertyCount > 0)
    {
        ADUC_Properties_FreeArray(tempPropertyCount, tempPropertyArray);
        tempPropertyArray = NULL;
    }

    return tempPropertyArray;
}

/**
 * @brief Free the ADUC_RelatedFile struct members
 * @param hash a pointer to an ADUC_RelatedFile
 */
static void ADUC_RelatedFile_UnInit(ADUC_RelatedFile* relatedFile)
{
    free(relatedFile->FileId);
    relatedFile->FileId = NULL;

    free(relatedFile->DownloadUri);
    relatedFile->DownloadUri = NULL;

    free(relatedFile->FileName);
    relatedFile->FileName = NULL;

    ADUC_Hash_FreeArray(relatedFile->HashCount, relatedFile->Hash);
    relatedFile->HashCount = 0;
    relatedFile->Hash = NULL;

    ADUC_Properties_FreeArray(relatedFile->PropertiesCount, relatedFile->Properties);
    relatedFile->PropertiesCount = 0;
    relatedFile->Properties = NULL;
}

static bool ADUC_RelatedFile_Init(
    ADUC_RelatedFile* relatedFile,
    const char* fileId,
    const char* downloadUri,
    const char* fileName,
    size_t hashCount,
    ADUC_Hash* hashes,
    size_t propertiesCount,
    ADUC_Property* properties)
{
    bool success = false;
    ADUC_Property* tempPropertiesArray = NULL;

    if (relatedFile == NULL || fileId == NULL || downloadUri == NULL || fileName == NULL || hashes == NULL
        || properties == NULL)
    {
        return false;
    }

    relatedFile->HashCount = hashCount;
    ADUC_Hash* tempHashArray = calloc(hashCount, sizeof(*tempHashArray));
    if (tempHashArray == NULL)
    {
        goto done;
    }

    for (int i = 0; i < hashCount; ++i)
    {
        if (!ADUC_Hash_Init(&tempHashArray[i], hashes[i].value, hashes[i].type))
        {
            goto done;
        }
    }

    relatedFile->PropertiesCount = propertiesCount;
    tempPropertiesArray = (ADUC_Property*)malloc(propertiesCount * sizeof(ADUC_Property));
    if (tempPropertiesArray == NULL)
    {
        goto done;
    }

    for (int i = 0; i < propertiesCount; ++i)
    {
        if (!ADUC_Property_Init(&tempPropertiesArray[i], properties[i].Name, properties[i].Value))
        {
            goto done;
        }
    }

    if (mallocAndStrcpy_s(&(relatedFile->FileId), fileId) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(relatedFile->DownloadUri), downloadUri) != 0)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(&(relatedFile->FileName), fileName) != 0)
    {
        goto done;
    }

    // transfer ownership
    relatedFile->HashCount = hashCount;
    relatedFile->Hash = tempHashArray;
    tempHashArray = NULL;

    relatedFile->PropertiesCount = propertiesCount;
    relatedFile->Properties = tempPropertiesArray;
    tempPropertiesArray = NULL;

    success = true;

done:

    if (!success)
    {
        if (tempHashArray != NULL)
        {
            ADUC_Hash_FreeArray(hashCount, tempHashArray);
            tempHashArray = NULL;
        }

        if (tempPropertiesArray != NULL)
        {
            ADUC_Properties_FreeArray(propertiesCount, tempPropertiesArray);
            tempPropertiesArray = NULL;
        }

        // do not call ADUC_RelatedFile_UnInit(relatedFile) here as it will
        // get cleaned up by caller for the failure.
    }

    return success;
}

static void _workflow_free_update_file_inodes(ADUC_Workflow* wf)
{
    if (wf != NULL)
    {
        free(wf->UpdateFileInodes);
        wf->UpdateFileInodes = NULL;
    }
}

static bool workflow_init_update_file_inodes(ADUC_Workflow* wf)
{
    if (wf == NULL || wf->UpdateFileInodes != NULL)
    {
        return false;
    }

    size_t count = workflow_get_update_files_count((ADUC_WorkflowHandle)wf);
    if (count == 0)
    {
        return false;
    }

    if ((wf->UpdateFileInodes = (ino_t*)malloc(sizeof(ino_t) * count)) == NULL)
    {
        return false;
    }

    for (int i = 0; i < count; ++i)
    {
        wf->UpdateFileInodes[i] = ADUC_INODE_SENTINEL_VALUE;
    }

    return true;
}

/**
 * @brief Frees an array of ADUC_RelatedFile of size @p relatedFileCount
 * @param relatedFileCount the size of @p relatedFileArray
 * @param relatedFileArray a pointer to an array of ADUC_RelatedFile structs
 */
void ADUC_RelatedFile_FreeArray(size_t relatedFileCount, ADUC_RelatedFile* relatedFileArray)
{
    if (relatedFileArray == NULL || relatedFileCount == 0)
    {
        return;
    }

    for (size_t relatedFile_index = 0; relatedFile_index < relatedFileCount; ++relatedFile_index)
    {
        ADUC_RelatedFile* relatedFileEntity = relatedFileArray + relatedFile_index;
        ADUC_RelatedFile_UnInit(relatedFileEntity);
    }
    free(relatedFileArray);
}

/**
 * @brief Allocates memory and populate with ADUC_RelatedFile object from a Parson JSON_Object.
 *
 * Caller MUST assume that this method allocates the memory for the returned ADUC_RelatedFile pointer.
 *
 * @param handle The workflow handle.
 * @param relatedFileObj JSON Object that contains the relatedFiles to be returned.
 * @param relatedFileCount A size_t* where the count of output relatedFiles will be stored.
 * @returns If success, a pointer to an array of ADUC_RelatedFile object. Otherwise, returns NULL.
 * @details Caller must call ADUC_RelatedFileArray_Free() to free the array. On error, this function
 * will set extendedResultCode
 */
ADUC_RelatedFile* ADUC_RelatedFileArray_AllocAndInit(
    ADUC_WorkflowHandle handle, const JSON_Object* relatedFileObj, size_t* relatedFileCount)
{
    bool success = false;

    ADUC_RelatedFile* tempRelatedFileArray = NULL;
    const JSON_Object* fileUrls = NULL;

    if (relatedFileObj == NULL || relatedFileCount == NULL)
    {
        return false;
    }
    *relatedFileCount = 0;

    size_t tempRelatedFileCount = json_object_get_count(relatedFileObj);

    if (tempRelatedFileCount == 0)
    {
        Log_Error("No relatedFiles.");
        goto done;
    }

    tempRelatedFileArray = calloc(tempRelatedFileCount, sizeof(*tempRelatedFileArray));

    if (tempRelatedFileArray == NULL)
    {
        goto done;
    }

    for (size_t relatedFile_index = 0; relatedFile_index < tempRelatedFileCount; ++relatedFile_index)
    {
        const char* fileName = NULL;
        const char* uri = NULL;
        size_t hashCount = 0;
        ADUC_Hash* tempHashes = NULL;
        size_t propertiesCount = 0;
        ADUC_Property* tempProperties = NULL;

        ADUC_RelatedFile* currentRelatedFile = tempRelatedFileArray + relatedFile_index;

        JSON_Object* relatedFileValueObj =
            json_value_get_object(json_object_get_value_at(relatedFileObj, relatedFile_index));
        if (relatedFileValueObj == NULL)
        {
            Log_Error("no relatedFile");
            goto done;
        }

        // fileId
        const char* fileId = json_object_get_name(relatedFileObj, relatedFile_index);
        if (IsNullOrEmpty(fileId))
        {
            Log_Error("empty file id at %d", relatedFile_index);
            goto done;
        }

        // downloadUri
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
            goto done;
        }

        // fileName
        fileName = json_object_get_string(relatedFileValueObj, "fileName");

        // hashes
        {
            JSON_Object* hashesObj = json_object_get_object(relatedFileValueObj, "hashes");
            if (hashesObj == NULL)
            {
                Log_Error("'hashes' missing at %d", relatedFile_index);
                goto done;
            }

            tempHashes = ADUC_HashArray_AllocAndInit(hashesObj, &hashCount);
            if (tempHashes == NULL)
            {
                goto done;
            }
        }

        // properties
        {
            JSON_Object* propertiesObj = json_object_get_object(relatedFileValueObj, "properties");
            if (propertiesObj == NULL)
            {
                Log_Error("'properties' missing at %d", relatedFile_index);
                goto done;
            }

            tempProperties = ADUC_PropertiesArray_AllocAndInit(propertiesObj, &propertiesCount);
            if (tempProperties == NULL)
            {
                goto done;
            }
        }

        if (!ADUC_RelatedFile_Init(
                currentRelatedFile, fileId, uri, fileName, hashCount, tempHashes, propertiesCount, tempProperties))
        {
            goto done;
        }
    }

    *relatedFileCount = tempRelatedFileCount;

    success = true;

done:

    if (!success)
    {
        ADUC_RelatedFile_FreeArray(tempRelatedFileCount, tempRelatedFileArray);
        tempRelatedFileArray = NULL;
    }

    return tempRelatedFileArray;
}

/**
 * @brief Parses the related files and assigns the relevant fields on the given ADUC_FileEntity.
 *
 * @param handle The workflow handle.
 * @param file the json object parsed from a file entry in the update metadata.
 * @param entity the file entity.
 * @returns true for success.
 */
static bool ParseFileEntityRelatedFiles(ADUC_WorkflowHandle handle, const JSON_Object* file, ADUC_FileEntity* entity)
{
    bool success = false;

    const JSON_Object* relatedFilesObj = json_object_get_object(file, ADUCITF_FIELDNAME_RELATEDFILES);
    if (relatedFilesObj == NULL)
    {
        // it's not necessarily an error if there are no related files for the file entity as one can
        // have a download handler that does not use/process related files.
        return true;
    }

    size_t tempRelatedFilesCount = 0;
    ADUC_RelatedFile* tempRelatedFiles =
        ADUC_RelatedFileArray_AllocAndInit(handle, relatedFilesObj, &tempRelatedFilesCount);
    if (tempRelatedFiles == NULL)
    {
        goto done;
    }

    entity->RelatedFileCount = tempRelatedFilesCount;
    entity->RelatedFiles = tempRelatedFiles;
    tempRelatedFiles = NULL;
    tempRelatedFilesCount = 0;

    success = true;

done:

    if (tempRelatedFilesCount > 0 && tempRelatedFiles != NULL)
    {
        ADUC_RelatedFile_FreeArray(tempRelatedFilesCount, tempRelatedFiles);
    }

    return success;
}

/**
 * @brief Parses the downloadHandlerId and related files for a file entry in the update metadata json.
 *
 * @param handle The workflow handle.
 * @param file the json object parsed from a file entry in the update metadata.
 * @param entity the file entity.
 * @returns true for success.
 */
static bool
ParseFileEntityDownloadHandler(ADUC_WorkflowHandle handle, const JSON_Object* file, ADUC_FileEntity* entity)
{
    bool success = false;
    const char* downloadHandlerId = NULL;
    const JSON_Object* downloadHandlerObj = NULL;

    if (entity == NULL)
    {
        return false;
    }

    downloadHandlerObj = json_object_get_object(file, ADUCITF_FIELDNAME_DOWNLOADHANDLER);
    if (downloadHandlerObj == NULL)
    {
        // it's ok not to have a download handler json object if there's none associated with the related file.
        return true;
    }

    downloadHandlerId = json_object_get_string(downloadHandlerObj, ADUCITF_FIELDNAME_DOWNLOADHANDLER_ID);
    if (IsNullOrEmpty(downloadHandlerId))
    {
        Log_Error("missing '%s' under '%s'", ADUCITF_FIELDNAME_DOWNLOADHANDLER_ID, ADUCITF_FIELDNAME_DOWNLOADHANDLER);
        goto done;
    }

    if (mallocAndStrcpy_s(&(entity->DownloadHandlerId), downloadHandlerId) != 0)
    {
        goto done;
    }

    if (!ParseFileEntityRelatedFiles(handle, file, entity))
    {
        goto done;
    }

    success = true;
done:

    return success;
}

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
 * @brief peeks at the properties under the "workflow" unprotected property
 *
 * @param updateActionJsonObj The JSON object of the update action JSON.
 * @param outWorkflowUpdateAction The output parameter for receiving the value of "action" under "workflow". May be set to NULL if there was none in the JSON.
 * @param outRootKeyPkgUrl The output parameter for receiving the value of "rootkeyPkgUrl" unprotected property from the updateAction json. Will not be set when error result is returned.
 * @param outWorkflowId_optional The output parameter for receiving the value of "id" under "workflow". May be set to NULL if there was none in the JSON.
 *
 * @details Caller must never free workflowId.
 * @returns ADUC_Result The result.
 */
ADUC_Result workflow_parse_peek_unprotected_workflow_properties(
    JSON_Object* updateActionJsonObj,
    ADUCITF_UpdateAction* outWorkflowUpdateAction,
    char** outRootKeyPkgUrl_optional,
    char** outWorkflowId_optional)
{
    ADUC_Result result = {
        .ResultCode = ADUC_GeneralResult_Failure,
        .ExtendedResultCode = 0,
    };

    ADUCITF_UpdateAction updateAction = ADUCITF_UpdateAction_Undefined;
    const char* workflowId = NULL;
    const char* rootkeyPkgUrl = NULL;

    char* tmpWorkflowId = NULL;
    char* tmpRootKeyPkgUrl = NULL;

    if (json_object_dothas_value(updateActionJsonObj, ADUCITF_FIELDNAME_WORKFLOW_DOT_ACTION))
    {
        updateAction = json_object_dotget_number(updateActionJsonObj, ADUCITF_FIELDNAME_WORKFLOW_DOT_ACTION);
        if (updateAction == 0)
        {
            result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_BAD_WORKFLOW_ACTION;
            goto done;
        }
    }

    // workflowId can be NULL in some cases.

    if (outWorkflowId_optional != NULL)
    {
        workflowId = json_object_dotget_string(updateActionJsonObj, WORKFLOW_PROPERTY_FIELD_WORKFLOW_DOT_ID);
        if (IsNullOrEmpty(workflowId))
        {
            result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_BAD_UPDATE_MANIFEST;
            goto done;
        }

        tmpWorkflowId = workflow_copy_string(workflowId);
        if (tmpWorkflowId == NULL)
        {
            result.ExtendedResultCode = ADUC_ERC_NOMEM;
            goto done;
        }
    }

    if (outRootKeyPkgUrl_optional != NULL)
    {
        rootkeyPkgUrl = json_object_dotget_string(updateActionJsonObj, ADUCITF_FIELDNAME_ROOTKEY_PACKAGE_URL);
        if (IsNullOrEmpty(rootkeyPkgUrl))
        {
            result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_EMPTY_OR_MISSING_ROOTKEY_PKG_URL;
            goto done;
        }

        tmpRootKeyPkgUrl = workflow_copy_string(rootkeyPkgUrl);
        if (tmpRootKeyPkgUrl == NULL)
        {
            result.ExtendedResultCode = ADUC_ERC_NOMEM;
            goto done;
        }
    }

    // Commit the optional out parameters now that nothing can goto done.
    if (outWorkflowUpdateAction != NULL)
    {
        *outWorkflowUpdateAction = updateAction;
    }

    if (outWorkflowId_optional != NULL && workflowId != NULL)
    {
        *outWorkflowId_optional = tmpWorkflowId;
        tmpWorkflowId = NULL;
    }

    if (outRootKeyPkgUrl_optional != NULL)
    {
        *outRootKeyPkgUrl_optional = tmpRootKeyPkgUrl;
        tmpRootKeyPkgUrl = NULL;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;

done:
    workflow_free_string(tmpWorkflowId);
    workflow_free_string(tmpRootKeyPkgUrl);

    return result;
}

/**
 * @brief Helper function for checking the hash of the updatemanifest is equal to the
 * hash held within the signature
 * @param updateActionObject update action JSON object.
 * @returns true on success and false on failure
 */
static bool Json_ValidateManifestHash(const JSON_Object* updateActionObject)
{
    bool success = false;

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
static ADUC_Result workflow_validate_update_manifest_signature(JSON_Object* updateActionObject)
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

/**
 * @brief Updates the workflow object's UpdateManifest JSON object based on the workflow object's UpdateAction JSON object.
 *
 * @param wf The workflow object.
 *
 * @return ADUC_Result The result
 */
static ADUC_Result UpdateWorkflowUpdateManifestObjFromUpdateActionObj(ADUC_Workflow* wf)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

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
            char* serialized = json_serialize_to_string(v);
            wf->UpdateManifestObject = json_value_get_object(json_parse_string(serialized));
            json_free_serialized_string(serialized);
        }
    }

    if (wf->UpdateManifestObject == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_BAD_UPDATE_MANIFEST;
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    return result;
}

/**
 * @brief Get the Detached Manifest Json Obj from the downloaded detached manifest file in the sandbox work folder.
 *
 * @param detachedUpdateManifestFilePath The detached update manifest file path.
 * @param outDetachedManifestJsonObj The output parameter for detached manifest JSON object.
 * @return ADUC_Result The result.
 */
ADUC_Result GetDetachedManifestJsonObjFromSandbox(
    const char* detachedUpdateManifestFilePath, JSON_Object** outDetachedManifestJsonObj)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    JSON_Value* rootValue = NULL;
    const char* updateManifestString = NULL;
    JSON_Object* detachedManifestJsonObj = NULL;

    rootValue = json_parse_file(detachedUpdateManifestFilePath);
    if (rootValue == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_BAD_DETACHED_UPDATE_MANIFEST_JSON_FILE;
        goto done;
    }

    updateManifestString = json_object_get_string(json_value_get_object(rootValue), ADUCITF_FIELDNAME_UPDATEMANIFEST);
    if (updateManifestString == NULL)
    {
        result.ExtendedResultCode =
            ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_DETACHED_UPDATE_MANIFEST_MISSING_UPDATEMANIFEST_PROPERTY;
        goto done;
    }

    detachedManifestJsonObj = json_value_get_object(json_parse_string(updateManifestString));
    if (detachedManifestJsonObj == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_BAD_DETACHED_UPDATE_MANIFEST;
        goto done;
    }

    // commit on success
    *outDetachedManifestJsonObj = detachedManifestJsonObj;
    detachedManifestJsonObj = NULL;

    result.ResultCode = ADUC_GeneralResult_Success;

done:
    json_value_free(rootValue);

    if (detachedManifestJsonObj != NULL)
    {
        json_value_free(json_object_get_wrapping_value(detachedManifestJsonObj));
    }

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("Failed getting valid detached manifest from sandbox, ERC %d", result.ExtendedResultCode);
    }

    return result;
}

/**
 * @brief Replaces the existing update manifest JSON object in the workflow object with that of the detached manifest.
 *
 * @param workFolder The work folder download sandbox.
 * @param detachedManifestFilePath The file path of detached manifest file.
 * @param wf The aduc workflow in which the UpdateManifestObject will be replaced.
 *
 * @return ADUC_Result The result.
 */
ADUC_Result ReplaceExistingUpdateManifestWithDetachedManifest(
    const char* workFolder, const char* detachedManifestFilePath, ADUC_Workflow* wf)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    JSON_Object* detachedManifestJsonObj = NULL;

    STRING_HANDLE detachedUpdateManifestFilePath =
        STRING_construct_sprintf("%s/%s", workFolder, detachedManifestFilePath);
    if (detachedUpdateManifestFilePath == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    result =
        GetDetachedManifestJsonObjFromSandbox(STRING_c_str(detachedUpdateManifestFilePath), &detachedManifestJsonObj);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    // Replace old manifest object with detached one.
    json_value_free(json_object_get_wrapping_value(wf->UpdateManifestObject));
    wf->UpdateManifestObject = detachedManifestJsonObj;
    detachedManifestJsonObj = NULL;

done:
    STRING_delete(detachedUpdateManifestFilePath);

    if (detachedManifestJsonObj != NULL)
    {
        json_value_free(json_object_get_wrapping_value(detachedManifestJsonObj));
    }

    return result;
}

/**
 * @brief Downloads the detached v4+ update manifest and replaces existing one with it upon success.
 *
 * @param handle The workflow handle.
 * @return ADUC_Result The result.
 */

static ADUC_Result DownloadAndUseDetachedManifest(ADUC_Workflow* wf)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    ADUC_FileEntity* fileEntity = NULL;
    const char* workFolder = NULL;
    int sandboxCreateResult = -1;

    if (wf == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_ERROR_BAD_PARAM;
        return result;
    }

    // There's only 1 file entity when the primary update manifest is detached.
    if (!workflow_get_update_file(handle_from_workflow(wf), 0, fileEntity))
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_MISSING_DETACHED_UPDATE_MANIFEST_ENTITY;
        goto done;
    }

    workFolder = workflow_get_workfolder(handle_from_workflow(wf));

    sandboxCreateResult = ADUC_SystemUtils_MkSandboxDirRecursive(workFolder);
    if (sandboxCreateResult != 0)
    {
        Log_Error("Unable to create folder %s, error %d", workFolder, sandboxCreateResult);
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_DETACHED_UPDATE_MANIFEST_DOWNLOAD_FAILED;
        goto done;
    }

    // Download the detached update manifest file.
    result = ExtensionManager_Download(
        fileEntity,
        handle_from_workflow(wf),
        &Default_ExtensionManager_Download_Options,
        NULL /* downloadProgressCallback */);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        workflow_set_result_details(
            handle_from_workflow(wf), "Cannot download primary detached update manifest file.");
        goto done;
    }

    result = ReplaceExistingUpdateManifestWithDetachedManifest(workFolder, fileEntity->TargetFilename, wf);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

done:
    if (fileEntity != NULL)
    {
        ADUC_FileEntity_Uninit(fileEntity);
        free(fileEntity);
    }

    return result;
}

/**
 * @brief A helper function for getting the JSON_Value from file or string.
 *
 * @param isFile A boolean indicates whether @p source is an input file, or an input JSON string.
 * @param source An input file or JSON string.
 * @param outJsonValue The output parameter for json value.
 *
 * @returns ADUC_Result The result.
 */
ADUC_Result workflow_parse_json(bool isFile, const char* source, JSON_Value** outJsonValue)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    JSON_Value* updateActionJson = NULL;

    if (source == NULL || outJsonValue == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_ERROR_BAD_PARAM;
        return result;
    }

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

    *outJsonValue = updateActionJson;
    updateActionJson = NULL;
    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (updateActionJson != NULL)
    {
        json_value_free(updateActionJson);
    }

    return result;
}

/**
 * @brief A helper function for parsing workflow data from file, or from string.
 *
 * @param updateActionJson The update action JSON value.
 * @param validateManifest A boolean indicates whether to validate the manifest.
 * @param handle An output workflow object handle.
 * @return ADUC_Result The result.
 */
ADUC_Result _workflow_parse(const JSON_Value* updateActionJson, bool validateManifest, ADUC_WorkflowHandle* handle)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    ADUC_Workflow* wf = NULL;
    JSON_Value* updateActionJsonClone = NULL;
    ADUCITF_UpdateAction updateAction = ADUCITF_UpdateAction_Undefined;

    if (handle == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_ERROR_BAD_PARAM;
        return result;
    }

    *handle = NULL;

    wf = malloc(sizeof(*wf));
    if (wf == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    memset(wf, 0, sizeof(*wf));

    updateActionJsonClone = json_value_deep_copy(updateActionJson);
    updateActionJson = NULL;

    // commit ownership of cloned JSON_Value to the workflow's UpdateActionObject.
    wf->UpdateActionObject = json_value_get_object(updateActionJsonClone);
    updateActionJsonClone = NULL;

    // At this point, we have had a side-effect of committing to the
    // wf->UpdateActionObject.
    //
    // Now, if not a cancel update action, then update the
    // wf->UpdateManifestObj only after validating update manifest signature.
    //
    // After that, if a detached manifest exists in the manifest, then
    // overwrite wf->UpdateManifestObj after downloading and verifying the
    // detached manifest.

    // 'cancel' action doesn't contains UpdateManifest and UpdateSignature.
    // Skip this part.
    workflow_parse_peek_unprotected_workflow_properties(
        wf->UpdateActionObject,
        &updateAction,
        NULL /* outRootKeyPkgUrl_optional */,
        NULL /* outWorkflowId_optional */);
    if (updateAction != ADUCITF_UpdateAction_Cancel)
    {
        ADUC_Result tmpResult = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

        // Skip signature validation if specified.
        // Also, some (partial) action data may not contain an UpdateAction,
        // e.g., Components-Update manifest that delivered as part of Bundle Updates,
        // We will skip the validation for these cases.
        if (updateAction != ADUCITF_UpdateAction_Undefined && validateManifest)
        {
            tmpResult = workflow_validate_update_manifest_signature(wf->UpdateActionObject);
            if (IsAducResultCodeFailure(tmpResult.ResultCode))
            {
                result = tmpResult;
                goto done;
            }
        }

        tmpResult = UpdateWorkflowUpdateManifestObjFromUpdateActionObj(wf);
        if (IsAducResultCodeFailure(tmpResult.ResultCode))
        {
            result = tmpResult;
            goto done;
        }

        // Starting from version 4, the update manifest can contain both embedded manifest,
        // or a downloadable update manifest file (files["manifest"] contains the update manifest file info)
        if (!IsNullOrEmpty(json_object_get_string(
                wf->UpdateManifestObject, UPDATE_MANIFEST_PROPERTY_FIELD_DETACHED_MANIFEST_FILE_ID)))
        {
            tmpResult = DownloadAndUseDetachedManifest(wf);
            if (IsAducResultCodeFailure(tmpResult.ResultCode))
            {
                result = tmpResult;
                goto done;
            }
        }

        if (validateManifest)
        {
            int manifestVersion = workflow_get_update_manifest_version(handle_from_workflow(wf));
            if (manifestVersion < SUPPORTED_UPDATE_MANIFEST_VERSION_MIN
                || manifestVersion > SUPPORTED_UPDATE_MANIFEST_VERSION_MAX)
            {
                Log_Error(
                    "Bad update manifest version: %d. (min: %d, max: %d)",
                    manifestVersion,
                    SUPPORTED_UPDATE_MANIFEST_VERSION_MIN,
                    SUPPORTED_UPDATE_MANIFEST_VERSION_MAX);
                result.ExtendedResultCode = ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_UNSUPPORTED_UPDATE_MANIFEST_VERSION;
                goto done;
            }
        }
    }

    *handle = wf;
    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;

done:

    if (updateActionJsonClone != NULL)
    {
        json_value_free(updateActionJsonClone);
    }

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        free(wf);
        wf = NULL;
    }

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
void workflow_set_step_index(ADUC_WorkflowHandle handle, size_t stepIndex)
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
    return (int)wf->StepIndex;
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
        Log_Debug("set prop '%s' to '%s'", property, value);
        return JSONSuccess == json_object_set_string(wf->PropertiesObject, property, value);
    }

    Log_Debug("set prop '%s' to null", property);
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

char* _workflow_copy_config_downloads_folder(const size_t max_size)
{
    char* downloads_folder_path = NULL;

    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    if (config == NULL)
    {
        goto done; /* Config is unitialized or we could not get a reference */
    }

    size_t download_folder_len = ADUC_StrNLen(config->downloadsFolder, max_size);

    if (download_folder_len == 0 || download_folder_len == max_size)
    {
        Log_Error("Invalid base sandbox dir: '%s'", config->downloadsFolder);
        goto done;
    }

    if (!mallocAndStrcpy_s(&downloads_folder_path, config->downloadsFolder))
    {
        goto done;
    }

done:

    ADUC_ConfigInfo_ReleaseInstance(config);

    return downloads_folder_path;
}

// Returns the download base directory
char* workflow_get_root_sandbox_dir(const ADUC_WorkflowHandle handle)
{
    char* ret = NULL;
    char* tempRet = NULL;
    char* pwf = NULL;

    ADUC_WorkflowHandle p = workflow_get_parent(handle);
    if (p != NULL)
    {
        pwf = workflow_get_workfolder(p);

        if (pwf == NULL)
        {
            Log_Error("Failed to get parent workfolder");
            goto done;
        }

        size_t pwf_len = ADUC_StrNLen(pwf, PATH_MAX);

        if (pwf_len == 0 || pwf_len == PATH_MAX)
        {
            Log_Error("Invalid parent workfolder: '%s'", pwf);
            goto done;
        }

        tempRet = pwf;
        pwf = NULL;
        Log_Debug("Using parent workfolder: '%s'", tempRet);
    }
    else
    {
        tempRet = _workflow_copy_config_downloads_folder(PATH_MAX);
        if (tempRet == NULL)
        {
            Log_Error("Copying config download folder failed");
            goto done;
        }
    }

    if (mallocAndStrcpy_s(&ret, tempRet) != 0)
    {
        goto done;
    }

done:

    free(tempRet);
    free(pwf);

    return ret;
}

// Workfolder =  [root.sandboxfolder]  "/"  ( [parent.workfolder | parent.id]  "/" )+  [handle.workfolder | handle.id]
char* workflow_get_workfolder(const ADUC_WorkflowHandle handle)
{
    char* ret = NULL;
    char* base_sandbox_dir = NULL;
    char* id = NULL;

    // If workfolder explicitly specified, use it.
    char* wf = workflow_get_string_property(handle, WORKFLOW_PROPERTY_FIELD_WORKFOLDER);
    if (wf != NULL)
    {
        Log_Debug("Property '%s' not NULL - returning cached workfolder '%s'", WORKFLOW_PROPERTY_FIELD_WORKFOLDER, wf);
        ret = wf;
        goto done;
    }

    // Return ([parent's workfolder] or [default sandbox folder]) + "/" + [workflow id];
    base_sandbox_dir = workflow_get_root_sandbox_dir(handle);

    if (base_sandbox_dir == NULL)
    {
        goto done;
    }

    id = workflow_get_id(handle);
    size_t id_len = ADUC_StrNLen(id, PATH_MAX);

    if (id_len == 0 || id_len == PATH_MAX)
    {
        Log_Error("Workflow id is too long to be in a path: '%s'", id);
        goto done;
    }

    ret = PathUtils_ConcatenateDirAndFolderPaths(base_sandbox_dir, id);
    if (ret == NULL)
    {
        Log_Error("Failed to concatenate dir and folder paths");
        goto done;
    }

done:

    free(id);
    free(base_sandbox_dir);

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

bool workflow_get_update_file(ADUC_WorkflowHandle handle, size_t index, ADUC_FileEntity* entity)
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

    bool succeeded = false;
    const JSON_Object* files = NULL;
    const JSON_Object* file = NULL;
    const JSON_Object* fileUrls = NULL;
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
        goto done;
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
        sizeInBytes = (size_t)json_object_get_number(file, ADUCITF_FIELDNAME_SIZEINBYTES);
    }

    if (!ADUC_FileEntity_Init(entity, fileId, name, uri, arguments, tempHash, tempHashCount, sizeInBytes))
    {
        Log_Error("Invalid file entity arguments");
        goto done;
    }

    // ADUC_FileEntity_Init makes a deep copy of the hash so must free tempHash to avoid memory leak.
    ADUC_Hash_FreeArray(tempHashCount, tempHash);
    tempHash = NULL;

    if (!ParseFileEntityDownloadHandler(handle, file, entity))
    {
        goto done;
    }

    succeeded = true;

done:
    if (!succeeded)
    {
        entity->Hash = NULL; // Manually free hash array below that is pointed to by tempHash...
        ADUC_FileEntity_Uninit(entity);
    }

    if (tempHash != NULL)
    {
        ADUC_Hash_FreeArray(tempHashCount, tempHash);
    }

    return succeeded;
}

bool workflow_get_update_file_by_name(ADUC_WorkflowHandle handle, const char* fileName, ADUC_FileEntity* entity)
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

    bool succeeded = false;
    const JSON_Object* files = NULL;
    const JSON_Object* file = NULL;
    const JSON_Object* fileUrls = NULL;
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
    for (size_t i = 0; i < count; i++)
    {
        if ((file = json_value_get_object(json_object_get_value_at(files, i))) != NULL
            && ADUCPAL_strcasecmp(json_object_get_string(file, "fileName"), fileName) == 0)
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
        sizeInBytes = (size_t)json_object_get_number(file, ADUCITF_FIELDNAME_SIZEINBYTES);
    }

    if (!ADUC_FileEntity_Init(entity, fileId, name, uri, arguments, tempHash, tempHashCount, sizeInBytes))
    {
        Log_Error("Invalid file entity arguments");
        goto done;
    }

    if (!ParseFileEntityDownloadHandler(handle, file, entity))
    {
        goto done;
    }

    succeeded = true;

done:
    if (!succeeded)
    {
        entity->Hash = NULL; // will be freed with tempHash below
        ADUC_FileEntity_Uninit(entity);

        if (tempHash != NULL)
        {
            ADUC_Hash_FreeArray(tempHashCount, tempHash);
        }
    }

    return succeeded;
}

/**
 * @brief Gets the inode associated with the update file entity at the specified index.
 *
 * @param handle A workflow data object handle.
 * @param index An index of the file to get.
 * @return ino_t The inode, or ADUC_INODE_SENTINEL_VALUE if inode has not been set yet.
 */
ino_t workflow_get_update_file_inode(ADUC_WorkflowHandle handle, size_t index)
{
    ino_t ret = ADUC_INODE_SENTINEL_VALUE;

    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        Log_Warn("bad handle");
    }
    else if ((wf->UpdateFileInodes != NULL) && (index < workflow_get_update_files_count(handle)))
    {
        ret = (wf->UpdateFileInodes)[index];
    }

    return ret;
}

/**
 * @brief Sets the inode associated with the update file entity at the specified index.
 *
 * @param handle A workflow data object handle.
 * @param index An index of the file to get.
 * @param inode The inode.
 * @return bool true on success.
 */
bool workflow_set_update_file_inode(ADUC_WorkflowHandle handle, size_t index, ino_t inode)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL)
    {
        Log_Warn("bad handle");
        return false;
    }

    size_t count = workflow_get_update_files_count(handle);
    if (index >= count)
    {
        Log_Warn("index %d out of range %d", index, count);
        return false;
    }

    if (wf->UpdateFileInodes == NULL)
    {
        if (!workflow_init_update_file_inodes(wf))
        {
            Log_Warn("init inodes");
            return false;
        }
    }

    wf->UpdateFileInodes[index] = inode;
    return true;
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

ADUC_Result _workflow_init_helper(ADUC_WorkflowHandle handle)
{
    ADUC_Result result = { ADUC_GeneralResult_Failure };

    ADUC_Workflow* wf = workflow_from_handle(handle);
    wf->Parent = NULL;
    wf->ChildCount = 0;
    wf->ChildrenMax = 0;
    wf->Children = 0;
    wf->PropertiesObject = json_value_get_object(json_value_init_object());
    if (wf->PropertiesObject == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    wf->ResultsObject = json_value_get_object(json_value_init_object());
    if (wf->ResultsObject == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    wf->ResultDetails = STRING_new();
    wf->InstalledUpdateId = STRING_new();
    wf->Result.ResultCode = ADUC_Result_Failure;
    wf->Result.ExtendedResultCode = 0;

    if ((wf->ResultExtraExtendedResultCodes = VECTOR_create(sizeof(ADUC_Result_t))) == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    wf->UpdateFileInodes = NULL;

    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;

done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error(
            "Failed to init workflow handle. result:%d (erc:0x%X)", result.ResultCode, result.ExtendedResultCode);
        if (handle != NULL)
        {
            workflow_uninit(handle);
        }
    }

    return result;
}

ADUC_Result
workflow_init_from_file(const char* updateManifestFile, bool validateManifest, ADUC_WorkflowHandle* outWorkflowHandle)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    ADUC_WorkflowHandle workflowHandle = NULL;

    JSON_Value* rootJsonValue = NULL;

    if (updateManifestFile == NULL || outWorkflowHandle == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_ERROR_BAD_PARAM;
        goto done;
    }

    *outWorkflowHandle = NULL;

    result = workflow_parse_json(true /* isFile */, updateManifestFile, &rootJsonValue);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = _workflow_parse(rootJsonValue, validateManifest, &workflowHandle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = _workflow_init_helper(workflowHandle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    *outWorkflowHandle = workflowHandle;
    workflowHandle = NULL;

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    json_value_free(rootJsonValue);

    if (workflowHandle != NULL)
    {
        workflow_free(workflowHandle);
        workflowHandle = NULL;
    }
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error(
            "Failed to init workflow handle. result:%d (erc:0x%X)", result.ResultCode, result.ExtendedResultCode);
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
ADUC_Result workflow_create_from_inline_step(ADUC_WorkflowHandle base, size_t stepIndex, ADUC_WorkflowHandle* handle)
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

    size_t fileCount = json_object_get_count(baseFiles);
    while (fileCount > 0)
    {
        fileCount--;

        const char* baseFileId = json_object_get_name(baseFiles, fileCount);

        // If file is in the instruction, merge their properties.
        bool fileRequired = false;

        size_t stepFilesCount = json_array_get_count(stepFiles);
        while (stepFilesCount > 0)
        {
            stepFilesCount--;

            // Note: step's files is an array of file ids.
            const char* stepFileId = json_array_get_string(stepFiles, stepFilesCount);

            if (baseFileId != NULL && stepFileId != NULL && strcmp(baseFileId, stepFileId) == 0)
            {
                // Found.
                fileRequired = true;

                // Done with this file, remove it (and free).
                json_array_remove(stepFiles, stepFilesCount);
                break;
            }
        }

        // Remove file from base files list, if no longer needed.
        if (!fileRequired)
        {
            json_object_remove(baseFiles, json_object_get_name(baseFiles, fileCount));
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

    // Update the cached workfolder to use the source workflow id.
    // Needs to be done before transferring parsed JSON obj below.
    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    if (config != NULL)
    {
        workflow_set_workfolder(targetHandle, "%s/%s", config->downloadsFolder, workflow_peek_id(sourceHandle));
        ADUC_ConfigInfo_ReleaseInstance(config);
    }
    else
    {
        Log_Error("Failed to set workfolder for target workflow. ConfigInfo is NULL.");
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
 * @param updateManifestJsonStr A JSON string containing update manifest data.
 * @param validateManifest A boolean indicates whether to validate the update manifest.
 * @param handle An output workflow object handle.
 * @return ADUC_Result
 */
ADUC_Result workflow_init(const char* updateManifestJsonStr, bool validateManifest, ADUC_WorkflowHandle* handle)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    JSON_Value* rootJsonValue = NULL;

    if (updateManifestJsonStr == NULL || handle == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_WORKFLOW_UTIL_ERROR_BAD_PARAM;
        goto done;
    }

    memset(handle, 0, sizeof(*handle));

    result = workflow_parse_json(false /* isFile */, updateManifestJsonStr, &rootJsonValue);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = _workflow_parse(rootJsonValue, validateManifest, handle);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = _workflow_init_helper(*handle);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    json_value_free(rootJsonValue);

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

        if (wf->ResultExtraExtendedResultCodes != NULL)
        {
            VECTOR_destroy(wf->ResultExtraExtendedResultCodes);
            wf->ResultExtraExtendedResultCodes = NULL;
        }
    }

    _workflow_free_updateaction(handle);
    _workflow_free_updatemanifest(handle);
    _workflow_free_properties(handle);
    _workflow_free_results_object(handle);

    _workflow_free_update_file_inodes(wf);

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

    if (parent != NULL && workflow_is_cancel_requested(parent))
    {
        if (!workflow_request_cancel(handle))
        {
            Log_Warn("Workflow cancellation request failed. (workflow level %d)", wf->Level);
        }
    }
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
size_t workflow_get_children_count(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    return wf != NULL ? (size_t)wf->ChildCount : 0;
}

/**
 * @brief Get child workflow at specified @p index.
 *
 * @param handle A workflow object handle.
 * @param index Index of a child workflow to get. -1 indicates getting the handle at the end of the list
 * @return A child workflow object handle, or NULL if index out of range.
 */
ADUC_WorkflowHandle workflow_get_child(ADUC_WorkflowHandle handle, size_t index)
{
    if (handle == NULL)
    {
        return NULL;
    }

    ADUC_Workflow* wf = workflow_from_handle(handle);

    if (index >= wf->ChildCount)
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
        index = (int)wf->ChildCount;
    }
    else
    {
        // Make room for the new child.
        size_t bytes = (wf->ChildCount - (size_t)index) * sizeof(ADUC_Workflow*);
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
        index = (int)wf->ChildCount - 1;
    }

    if (index < 0 || index >= wf->ChildCount)
    {
        return NULL;
    }

    ADUC_WorkflowHandle child = wf->Children[index];

    if (index < wf->ChildCount - 1)
    {
        size_t bytes = sizeof(ADUC_Workflow*) * wf->ChildCount - (size_t)(index + 1);
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
        ADUC_Result result;
        memset(&result, 0, sizeof(result));
        return result;
    }

    return wf->Result;
}

void workflow_add_erc(ADUC_WorkflowHandle handle, ADUC_Result_t erc)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf != NULL && wf->ResultExtraExtendedResultCodes != NULL)
    {
        if (VECTOR_push_back(wf->ResultExtraExtendedResultCodes, &erc, 1) != 0)
        {
            Log_Warn("push ", wf->Level);
        }
    }
}

STRING_HANDLE workflow_get_extra_ercs(ADUC_WorkflowHandle handle)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf == NULL || wf->ResultExtraExtendedResultCodes == NULL)
    {
        return NULL;
    }

    return ADUC_ReportingUtils_StringHandleFromVectorInt32(
        wf->ResultExtraExtendedResultCodes, WORKFLOW_MAX_SUCCESS_ERC);
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

    ADUC_Workflow* deferredWorkflow = wf->DeferredReplacementWorkflow;
    wf->DeferredReplacementWorkflow = NULL;
    workflow_transfer_data(handle /* wfTarget */, deferredWorkflow /* wfSource */);

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

bool workflow_request_cancel(ADUC_WorkflowHandle handle)
{
    if (handle == NULL)
    {
        return false;
    }

    bool success = workflow_set_boolean_property(handle, WORKFLOW_PROPERTY_FIELD_CANCEL_REQUESTED, true);
    size_t childCount = workflow_get_children_count(handle);
    for (size_t i = 0; i < childCount; i++)
    {
        success = success && workflow_request_cancel(workflow_get_child(handle, i));
    }
    return success;
}

bool workflow_is_cancel_requested(ADUC_WorkflowHandle handle)
{
    return workflow_get_boolean_property(handle, WORKFLOW_PROPERTY_FIELD_CANCEL_REQUESTED);
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
    size_t fileCount = json_object_get_count(baseFiles);

    do
    {
        fileCount--;
        JSON_Object* baseFile = json_object(json_object_get_value_at(baseFiles, fileCount));

        // If file is in the instruction, merge their properties.
        bool fileRequired = false;
        size_t instFilesCount = json_array_get_count(instFiles);

        do
        {
            instFilesCount--;

            const char* baseFilename = json_object_get_string(baseFile, ADUCITF_FIELDNAME_FILENAME);

            JSON_Object* instFile = json_array_get_object(instFiles, instFilesCount);
            const char* instFilename = json_object_get_string(instFile, ADUCITF_FIELDNAME_FILENAME);

            if (baseFilename != NULL && instFilename != NULL && strcmp(baseFilename, instFilename) == 0)
            {
                // Found.
                fileRequired = true;

                size_t valuesCount = json_object_get_count(instFile);
                do
                {
                    valuesCount--;
                    const char* key = json_object_get_name(instFile, valuesCount);
                    JSON_Value* val = json_value_deep_copy(json_object_get_value_at(instFile, valuesCount));
                    json_object_set_value(baseFile, key, val);
                } while (valuesCount > 0);

                // Done with this file, remove it (and free).
                json_array_remove(instFiles, instFilesCount);
                break;
            }
        } while (instFilesCount > 0);

        // Remove file from base files list, if no longer needed.
        if (!fileRequired)
        {
            json_object_remove(baseFiles, json_object_get_name(baseFiles, fileCount));
        }

    } while (fileCount > 0);

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
        return DEFAULT_STEP_TYPE;
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
 * @param handle[in] A workflow data object handle.
 * @param stepIndex[in] A step index.
 * @param entity[out] An output reference step update manifest file entity object.
 * @return true on success.
 */
bool workflow_get_step_detached_manifest_file(ADUC_WorkflowHandle handle, size_t stepIndex, ADUC_FileEntity* entity)
{
    size_t count = workflow_get_instructions_steps_count(handle);
    if (stepIndex >= count)
    {
        return false;
    }

    bool succeeded = false;
    bool fileEntityInited = false;
    JSON_Object* step = json_array_get_object(workflow_get_instructions_steps_array(handle), stepIndex);
    const char* fileId = json_object_get_string(step, STEP_PROPERTY_FIELD_DETACHED_MANIFEST_FILE_ID);
    const JSON_Object* files = _workflow_get_update_manifest_files_map(handle);
    const JSON_Object* file = json_object_get_object(files, fileId);
    const JSON_Object* fileUrls = NULL;
    const char* uri = NULL;
    const char* name = NULL;
    size_t tempHashCount = 0;
    ADUC_Hash* tempHash = NULL;

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
        sizeInBytes = (size_t)json_object_get_number(file, ADUCITF_FIELDNAME_SIZEINBYTES);
    }

    if (!ADUC_FileEntity_Init(entity, fileId, name, uri, NULL /*arguments*/, tempHash, tempHashCount, sizeInBytes))
    {
        Log_Error("Invalid file entity arguments");
        goto done;
    }

    fileEntityInited = true;

    if (!ParseFileEntityDownloadHandler(handle, file, entity))
    {
        goto done;
    }

    succeeded = true;

done:

    if (!succeeded && fileEntityInited)
    {
        ADUC_FileEntity_Uninit(entity);
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

/**
 * @brief Gets the file path of the entity target update under the download work folder sandbox.
 *
 * @param workflowHandle The workflow handle.
 * @param entity The file entity.
 * @param outFilePath The resultant work folder file path to the file entity.
 * @return bool true if success
 * @remark Caller will own the STRING_HANDLE outFilePath and must call STRING_delete on it.
 */
bool workflow_get_entity_workfolder_filepath(
    ADUC_WorkflowHandle workflowHandle, const ADUC_FileEntity* entity, STRING_HANDLE* outFilePath)
{
    bool result = false;
    char dir[1024] = { 0 };
    char* workFolder = workflow_get_workfolder(workflowHandle);
    if (workFolder == NULL)
    {
        goto done;
    }

    sprintf(dir, "%s/%s", workFolder, entity->TargetFilename);

    if (dir[0] != 0)
    {
        STRING_HANDLE temp = STRING_construct(dir);

        if (temp == NULL)
        {
            goto done;
        }

        *outFilePath = temp;
    }

    result = true;

done:
    workflow_free_string(workFolder);

    return result;
}

bool workflow_get_force_update(ADUC_WorkflowHandle workflowHandle)
{
    ADUC_Workflow* wf = workflow_from_handle(workflowHandle);
    if (wf == NULL)
    {
        return false;
    }

    return wf->ForceUpdate;
}

void workflow_set_force_update(ADUC_WorkflowHandle handle, bool forceUpdate)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf != NULL)
    {
        wf->ForceUpdate = forceUpdate;
    }
}

bool workflow_init_workflow_handle(ADUC_WorkflowData* workflowData)
{
    ADUC_Workflow* wf = malloc(sizeof(*wf));
    if (wf == NULL)
    {
        return false;
    }

    memset(wf, 0, sizeof(*wf));

    workflowData->WorkflowHandle = wf;
    return true;
}

bool workflow_set_update_action_object(ADUC_WorkflowHandle handle, JSON_Object* jsonObj)
{
    ADUC_Workflow* wf = workflow_from_handle(handle);
    if (wf != NULL)
    {
        wf->UpdateActionObject = jsonObj;
        return true;
    }

    return false;
}

EXTERN_C_END
