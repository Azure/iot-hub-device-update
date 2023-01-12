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
#include <azure_blob_storage_file_upload_utility.h>
#include <diagnostics_config_utils.h>
#include <diagnostics_devicename.h>
#include <diagnostics_interface.h>
#include <file_info_utils.h>
#include <operation_id_utils.h>
#include <parson_json_utils.h>
#include <stdio.h>
#include <stdlib.h>

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
        const DiagnosticsLogComponent* logComponent = DiagnosticsConfigUtils_GetLogComponentElem(workflowData, i);

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
        const DiagnosticsLogComponent* logComponent = DiagnosticsConfigUtils_GetLogComponentElem(workflowData, i);

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
