/**
 * @file diagnostics_workflow.c
 * @brief Implementation for functions handling the Diagnostic Log Upload workflow
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "diagnostics_workflow.h"
#include "diagnostics_result.h"

#include <aduc/logging.h>
#include <aduc/string_c_utils.h>
#include <aduc/system_utils.h>
#include <azure_blob_storage_file_upload_utility.h>
#include <diagnostics_devicename.h>
#include <diagnostics_interface.h>
#include <file_info_utils.h>
#include <operation_id_utils.h>
#include <parson_json_utils.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Expected Diagnostics Config file format:
 * {
        "logComponents":[
            {
                "componentName":"DU",
                "logPath":"/var/logs/adu/"
            },
            {
                "componentName":"DO",
                "logPath":"/var/cache/do/"
            },
            ...
        ],
        "maxKilobytesToUploadPerLogPath":5
    }
 */

/**
 * @brief Fieldname for the array of log components in the Diagnostics JSON Config File
 */
#define DIAGNOSTICS_CONFIG_FILE_LOG_COMPONENTS_FIELDNAME "logComponents"

/**
 * @brief Fieldname for the name of the component having its logs collected in the Diagnostics JSON Config File
 */
#define DIAGNOSTICS_CONFIG_FILE_COMPONENT_FIELDNAME_COMPONENTNAME "componentName"

/**
 * @brief Fieldname for the location of logs on the device for a component in the Diagnostics JSON Config File
 */
#define DIAGNOSTICS_CONFIG_FILE_COMPONENT_FIELDNAME_LOGPATH "logPath"

/**
 * @brief Fieldname for the maximum number of bytes to upload per diagnostics workflow
 */
#define DIAGNOSTICS_CONFIG_FILE_FIELDNAME_MAXKILOBYTESTOUPLOADPERLOGPATH "maxKilobytesToUploadPerLogPath"

/**
 * @brief Maximum number of kilobytes allowed to be uploaded per log path
 */
#define DIAGNOSTICS_MAX_KILOBYTES_PER_LOG_PATH 100000 /* 1000 KB or 100 MB */

/**
 * @brief Returns the DiagnosticsLogComponent object @p index within @p workflowData
 * @param workflowData the DiagnosticsWorkflowData vector from which to get the DiagnosticsComponent
 * @param index the index from which to retrieve the value
 * @returns NULL if index is out of range; the DiagnosticsComponent otherwise
 */
const DiagnosticsLogComponent*
DiagnosticsWorkflow_GetLogComponentElem(const DiagnosticsWorkflowData* workflowData, unsigned int index)
{
    if (workflowData == NULL || index >= VECTOR_size(workflowData->components))
    {
        return NULL;
    }

    return (const DiagnosticsLogComponent*)VECTOR_element(workflowData->components, index);
}
/**
 * @brief Uninitializes a logComponent pointer
 * @param logComponent pointer to the logComponent whose members will be freed
 */
void DiagnosticsWorkflow_LogComponentUninit(DiagnosticsLogComponent* logComponent)
{
    if (logComponent == NULL)
    {
        return;
    }

    STRING_delete(logComponent->componentName);
    logComponent->componentName = NULL;

    STRING_delete(logComponent->logPath);
    logComponent->logPath = NULL;
}

/**
 * @brief Sets the memory in @p memory which points to the storage location in @p sasCredential to 0 before calling STRING_delete() on sasCredential
 * @param sasCredential credential to be deleted
 * @param memory a pointer to the memory used for @p sasCredential holding the sasCredential string value
 */
void DiagnosticsComponent_SecurelyFreeSasCredential(STRING_HANDLE* sasCredential, char** memory)
{
    if (memory == NULL || *memory == NULL)
    {
        return;
    }

    memset(*memory, 0, STRING_length(*sasCredential));

    STRING_delete(*sasCredential);

    *sasCredential = NULL;
    *memory = NULL;
}

/**
 * @brief Creates a STRING_HANDLE using the memory pointer in @p memory with the value @p sasCredential
 * @details Free using DiagnosticsComponent_SecurelyFreeSasCredential
 * @param sasCredential the sasCredential string to load into memory
 * @param memory pointer to be loaded with the address of memory
 * @returns a STRING_HANDLE on success; NULL on failure
 */
STRING_HANDLE DiagnosticsComponent_CreateSasCredential(const char* sasCredential, char** memory)
{
    _Bool succeeded = false;
    STRING_HANDLE handle = NULL;
    if (memory == NULL || sasCredential == NULL)
    {
        goto done;
    }

    if (mallocAndStrcpy_s(memory, sasCredential) != 0)
    {
        goto done;
    }

    handle = STRING_new_with_memory(*memory);

    if (handle == NULL)
    {
        goto done;
    }

    succeeded = true;
done:

    if (!succeeded)
    {
        DiagnosticsComponent_SecurelyFreeSasCredential(&handle, memory);
    }

    return handle;
}
/**
 * @brief initializes a log component using a JSON_Object representing the component in the config file
 * @param component the componentObj to intitialize
 * @param componentObj JSON Object with the associated components for the log component
 * @returns true on success; false on failure
 */
_Bool DiagnosticsComponent_LogComponentInitFromObj(DiagnosticsLogComponent* component, JSON_Object* componentObj)
{
    if (componentObj == NULL || component == NULL)
    {
        return false;
    }

    memset(component, 0, sizeof(*component));

    _Bool succeeded = false;
    char* componentName = NULL;
    char* logPath = NULL;

    if (!ADUC_JSON_GetStringFieldFromObj(
            componentObj, DIAGNOSTICS_CONFIG_FILE_COMPONENT_FIELDNAME_COMPONENTNAME, &componentName))
    {
        goto done;
    }

    if (!ADUC_JSON_GetStringFieldFromObj(componentObj, DIAGNOSTICS_CONFIG_FILE_COMPONENT_FIELDNAME_LOGPATH, &logPath))
    {
        goto done;
    }

    component->componentName = STRING_construct(componentName);

    if (component->componentName == NULL)
    {
        goto done;
    }

    component->logPath = STRING_construct(logPath);

    if (component->logPath == NULL)
    {
        goto done;
    }

    succeeded = true;

done:

    if (!succeeded)
    {
        DiagnosticsWorkflow_LogComponentUninit(component);
    }

    free(componentName);
    free(logPath);

    return succeeded;
}

/**
 * @brief Initializes @p workflowData with the contents of @p filePath
 * @param workflowData the workflowData structure to be intialized.
 * @param filePath path to the diagnostics configuration file
 * @returns true on successful configuration, false on failure
 */
_Bool DiagnosticsWorkflow_InitFromFile(DiagnosticsWorkflowData* workflowData, const char* filePath)
{
    JSON_Value* fileJsonValue = NULL;

    if (workflowData == NULL || filePath == NULL)
    {
        return false;
    }

    fileJsonValue = json_parse_file(filePath);

    if (fileJsonValue == NULL)
    {
        return false;
    }

    _Bool succeeded = DiagnosticsWorkflow_InitFromJSON(workflowData, fileJsonValue);

    json_value_free(fileJsonValue);

    return succeeded;
}

/**
 * @brief Initializes @p workflowData with the contents of @p fileJsonValue
 * @param workflowData the structure to be initialized
 * @param fileJsonValue a JSON_Value representation of a diagnostics-config.json file
 * @returns true on successful configuration; false on failures
 */
_Bool DiagnosticsWorkflow_InitFromJSON(DiagnosticsWorkflowData* workflowData, JSON_Value* fileJsonValue)
{
    if (workflowData == NULL || fileJsonValue == NULL)
    {
        return false;
    }

    _Bool succeeded = false;

    memset(workflowData, 0, sizeof(*workflowData));

    JSON_Object* fileJsonObj = json_value_get_object(fileJsonValue);

    if (fileJsonObj == NULL)
    {
        goto done;
    }

    unsigned int maxKilobytesToUploadPerLogPath = 0;

    if (!ADUC_JSON_GetUnsignedIntegerField(
            fileJsonValue,
            DIAGNOSTICS_CONFIG_FILE_FIELDNAME_MAXKILOBYTESTOUPLOADPERLOGPATH,
            &maxKilobytesToUploadPerLogPath))
    {
        Log_Warn("DiagnosticsWorkflow_Init failed to retrieve maxKilobytesPerLogPath from diagnostics config");
        goto done;
    }

    if (maxKilobytesToUploadPerLogPath < 1)
    {
        Log_Warn(
            "DiagnosticsWorkflow_Init maxKilobytesPerLogPath config set to invalid value: %i",
            maxKilobytesToUploadPerLogPath);
        goto done;
    }

    if (maxKilobytesToUploadPerLogPath > DIAGNOSTICS_MAX_KILOBYTES_PER_LOG_PATH)
    {
        maxKilobytesToUploadPerLogPath = DIAGNOSTICS_MAX_KILOBYTES_PER_LOG_PATH;
    }

    workflowData->maxBytesToUploadPerLogPath = maxKilobytesToUploadPerLogPath * 1024;

    JSON_Array* componentArray = json_object_get_array(fileJsonObj, DIAGNOSTICS_CONFIG_FILE_LOG_COMPONENTS_FIELDNAME);

    if (componentArray == NULL)
    {
        goto done;
    }

    const size_t numComponents = json_array_get_count(componentArray);

    if (numComponents == 0)
    {
        goto done;
    }

    workflowData->components = VECTOR_create(sizeof(DiagnosticsLogComponent));

    if (workflowData->components == NULL)
    {
        goto done;
    }

    for (size_t i = 0; i < numComponents; ++i)
    {
        JSON_Object* componentObj = json_array_get_object(componentArray, i);

        DiagnosticsLogComponent logComponent = {};

        if (!DiagnosticsComponent_LogComponentInitFromObj(&logComponent, componentObj))
        {
            goto done;
        }

        if (VECTOR_push_back(workflowData->components, &logComponent, 1) != 0)
        {
            goto done;
        }
    }

    succeeded = true;
done:

    if (!succeeded)
    {
        Log_Error("DiagnosticsWorkflow_Init failed");
        DiagnosticsWorkflow_UnInit(workflowData);
    }

    return succeeded;
}

/**
 * @brief UnInitializes @p workflowData's data members
 * @param workflowData the workflowData structure to be unintialized.
 */
void DiagnosticsWorkflow_UnInit(DiagnosticsWorkflowData* workflowData)
{
    if (workflowData == NULL)
    {
        return;
    }

    if (workflowData->components != NULL)
    {
        const size_t numComponents = VECTOR_size(workflowData->components);

        for (size_t i = 0; i < numComponents; i++)
        {
            DiagnosticsLogComponent* logComponent =
                (DiagnosticsLogComponent*)VECTOR_element(workflowData->components, i);

            DiagnosticsWorkflow_LogComponentUninit(logComponent);
        }

        VECTOR_destroy(workflowData->components);
    }

    memset(workflowData, 0, sizeof(*workflowData));
}

/**
 * @brief Discovers the logs described by @p logComponent and stores them in @p fileNames
 * @param fileNames vector to be filled with the files to be uploaded for @p logComponent
 * @param logComponent descriptor for the component for which we're going to discover logs
 * @param maxUploadSize maximum number of bytes of logs allowed to be discovered for upload
 * @returns a value of Diagnostics_Result indicating the status of this component's upload
 */
Diagnostics_Result DiagnosticsWorkflow_GetFilesForComponent(
    VECTOR_HANDLE* fileNames, const DiagnosticsLogComponent* logComponent, const unsigned int maxUploadSize)
{
    if (logComponent == NULL || maxUploadSize == 0 || fileNames == 0)
    {
        return Diagnostics_Result_Failure;
    }

    Diagnostics_Result result = Diagnostics_Result_Failure;

    if (!FileInfoUtils_GetNewestFilesInDirUnderSize(fileNames, STRING_c_str(logComponent->logPath), maxUploadSize))
    {
        result = Diagnostics_Result_NoLogsFound;
        Log_Debug(
            "DiagnosticsWorkflow_UploadComponent No files found for logComponent: %s",
            STRING_c_str(logComponent->componentName));
        goto done;
    }

    result = Diagnostics_Result_Success;

done:

    return result;
}

/**
 * @brief Uploads the logs held within @p fileNames described by @p logComponent
 * @param fileNames vector of files to be uploaded for @p logComponent
 * @param logComponent descriptor for the component for which we're going to upload logs
 * @param deviceName name of the device the DiagnosticsWorkflow is running on
 * @param operationId the id associated with this upload request sent down by Diagnostics Service
 * @param storageSasUrl credential to be used for the Azure Blob Storage upload
 * @returns a value of Diagnostics_Result indicating the status of this component's upload
 */
Diagnostics_Result DiagnosticsWorkflow_UploadFilesForComponent(
    VECTOR_HANDLE fileNames,
    const DiagnosticsLogComponent* logComponent,
    const char* deviceName,
    const char* operationId,
    const char* storageSasUrl)
{
    if (fileNames == NULL || logComponent == NULL || deviceName == NULL || operationId == NULL
        || storageSasUrl == NULL)
    {
        return Diagnostics_Result_Failure;
    }

    char* storageSasCredentialMemory = NULL;
    Diagnostics_Result result = Diagnostics_Result_Failure;

    BlobStorageInfo blobInfo = {};

    if (logComponent->componentName == NULL || logComponent->logPath == NULL)
    {
        Log_Error("DiagnosticsWorkflow_UploadFilesForComponent called with unitialized DiagnosticsComponent");
        goto done;
    }

    blobInfo.storageSasCredential =
        DiagnosticsComponent_CreateSasCredential(storageSasUrl, &storageSasCredentialMemory);

    if (blobInfo.storageSasCredential == NULL)
    {
        goto done;
    }

    blobInfo.virtualDirectoryPath = STRING_new();
    // Virtual Directory path: <device-name>/<operation-id>/<component-name>/<files>
    if (STRING_sprintf(
            blobInfo.virtualDirectoryPath,
            "%s/%s/%s/",
            deviceName,
            operationId,
            STRING_c_str(logComponent->componentName))
        != 0)
    {
        goto done;
    }

    if (!AzureBlobStorageFileUploadUtility_UploadFilesToContainer(
            &blobInfo, 1, fileNames, STRING_c_str(logComponent->logPath)))
    {
        result = Diagnostics_Result_UploadFailed;
        Log_Warn(
            "DiagnosticsWorkflow_UploadFilesForComponent File upload failed for logComponent: %s",
            STRING_c_str(logComponent->componentName));
        goto done;
    }

    result = Diagnostics_Result_Success;
done:

    STRING_delete(blobInfo.virtualDirectoryPath);
    blobInfo.virtualDirectoryPath = NULL;

    DiagnosticsComponent_SecurelyFreeSasCredential(&blobInfo.storageSasCredential, &storageSasCredentialMemory);

    return result;
}

/**
 * @brief Helper function for freeing the contents of a vector of vectors of STRING_HANDLE type
 * @details Does not free @p logComponentFileNames itself just its contents
 * @param logComponentFileNames a vector of vectors of STRING_HANDLE type
 */
void DiagnosticsWorkflow_UnInitLogComponentFileNames(VECTOR_HANDLE logComponentFileNames)
{
    if (logComponentFileNames == NULL)
    {
        return;
    }

    const size_t logComponentFileNamesSize = VECTOR_size(logComponentFileNames);

    for (int i = 0; i < logComponentFileNamesSize; ++i)
    {
        VECTOR_HANDLE* fileNames = VECTOR_element(logComponentFileNames, i);

        const size_t fileNamesSize = VECTOR_size(*fileNames);

        for (int j = 0; j < fileNamesSize; ++j)
        {
            STRING_HANDLE* fileName = VECTOR_element(*fileNames, j);

            STRING_delete(*fileName);
        }

        VECTOR_destroy(*fileNames);
    }
}

/**
 * @brief Uploads the diagnostic logs described by @p workflowData
 * @param workflowData the workflowData structure describing the log components
 * @param jsonString the string from the diagnostics_interface describing where to upload the logs
 */
void DiagnosticsWorkflow_DiscoverAndUploadLogs(const DiagnosticsWorkflowData* workflowData, const char* jsonString)
{
    Log_Info("Starting Diagnostics Log Upload");

    Diagnostics_Result result = Diagnostics_Result_Failure;

    JSON_Value* cloudMsgJson = NULL;
    char* deviceName = NULL;
    STRING_HANDLE operationId = NULL;

    STRING_HANDLE storageSasCredential = NULL;
    char* storageSasCredentialMemory = NULL;

    VECTOR_HANDLE logComponentFileNames = NULL;

    if (jsonString == NULL)
    {
        goto done;
    }

    if (workflowData == NULL)
    {
        result = Diagnostics_Result_NoDiagnosticsComponents;
        goto done;
    }

    const size_t numComponents = VECTOR_size(workflowData->components);

    if (numComponents == 0)
    {
        result = Diagnostics_Result_NoDiagnosticsComponents;
        goto done;
    }

    cloudMsgJson = json_parse_string(jsonString);

    if (cloudMsgJson == NULL)
    {
        goto done;
    }

    JSON_Object* cloudMsgObj = json_value_get_object(cloudMsgJson);

    if (cloudMsgObj == NULL)
    {
        goto done;
    }

    operationId = STRING_construct(json_object_get_string(cloudMsgObj, DIAGNOSTICSITF_FIELDNAME_OPERATIONID));

    if (operationId == NULL)
    {
        result = Diagnostics_Result_NoOperationId;
        goto done;
    }

    storageSasCredential = DiagnosticsComponent_CreateSasCredential(
        json_object_get_string(cloudMsgObj, DIAGNOSTICSITF_FIELDNAME_SASURL), &storageSasCredentialMemory);

    if (storageSasCredential == NULL)
    {
        result = Diagnostics_Result_NoSasCredential;
        goto done;
    }

    const unsigned int uploadSizePerComponent = workflowData->maxBytesToUploadPerLogPath;

    if (uploadSizePerComponent == 0)
    {
        goto done;
    }

    if (!DiagnosticsComponent_GetDeviceName(&deviceName))
    {
        goto done;
    }

    logComponentFileNames = VECTOR_create(sizeof(VECTOR_HANDLE));

    if (logComponentFileNames == NULL)
    {
        goto done;
    }

    //
    // Perform Discovery
    //
    for (size_t i = 0; i < numComponents; ++i)
    {
        const DiagnosticsLogComponent* logComponent = DiagnosticsWorkflow_GetLogComponentElem(workflowData, i);

        if (logComponent == NULL || logComponent->componentName == NULL || logComponent->logPath == NULL)
        {
            Log_Error("DiagnosticsWorkflow_UploadLogs WorkflowData has uninitialized components");
            goto done;
        }

        VECTOR_HANDLE discoveredFileNames = NULL;

        result = DiagnosticsWorkflow_GetFilesForComponent(&discoveredFileNames, logComponent, uploadSizePerComponent);

        if (result != Diagnostics_Result_Success || discoveredFileNames == NULL)
        {
            goto done;
        }

        if (VECTOR_push_back(logComponentFileNames, &discoveredFileNames, 1) != 0)
        {
            goto done;
        }
    }

    //
    // Perform Upload
    //
    for (size_t i = 0; i < numComponents; ++i)
    {
        const DiagnosticsLogComponent* logComponent = DiagnosticsWorkflow_GetLogComponentElem(workflowData, i);

        if (logComponent == NULL || logComponent->componentName == NULL || logComponent->logPath == NULL)
        {
            Log_Error("DiagnosticsWorkflow_UploadLogs WorkflowData has uninitialized components");
            goto done;
        }

        const VECTOR_HANDLE* discoveredLogFileNames = VECTOR_element(logComponentFileNames, i);

        if (discoveredLogFileNames == NULL)
        {
            goto done;
        }

        result = DiagnosticsWorkflow_UploadFilesForComponent(
            *discoveredLogFileNames,
            logComponent,
            deviceName,
            STRING_c_str(operationId),
            STRING_c_str(storageSasCredential));

        if (result != Diagnostics_Result_Success)
        {
            goto done;
        }
    }

    result = Diagnostics_Result_Success;

done:

    //
    // Report state back to the iothub
    //
    if (operationId == NULL)
    {
        DiagnosticsInterface_ReportStateAndResultAsync(result, "");
    }
    else
    {
        DiagnosticsInterface_ReportStateAndResultAsync(result, STRING_c_str(operationId));

        //
        // Required to prevent duplicate requests coming down from the service after
        // restart or a connection refresh
        //
        if (!OperationIdUtils_StoreCompletedOperationId(STRING_c_str(operationId)))
        {
            Log_Warn("Unable to record completed operation-id: %s", STRING_c_str(operationId));
        }
    }

    if (logComponentFileNames != NULL)
    {
        DiagnosticsWorkflow_UnInitLogComponentFileNames(logComponentFileNames);
        VECTOR_destroy(logComponentFileNames);
    }

    free(deviceName);

    STRING_delete(operationId);

    DiagnosticsComponent_SecurelyFreeSasCredential(&storageSasCredential, &storageSasCredentialMemory);

    json_value_free(cloudMsgJson);
}
