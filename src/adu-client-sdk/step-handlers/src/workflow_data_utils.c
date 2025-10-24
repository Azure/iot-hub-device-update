/**
 * @file workflow_data_utils.c
 * @brief Utility functions for working with ADUC_WorkflowData
 *
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#include "aduc/workflow_data.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a new workflow data instance
 */
ADUC_WorkflowData* ADUC_WorkflowData_Create(void)
{
    ADUC_WorkflowData* workflowData = (ADUC_WorkflowData*)calloc(1, sizeof(ADUC_WorkflowData));
    if (!workflowData)
    {
        return NULL;
    }

    // Initialize all fields to default values
    workflowData->updateId = NULL;
    workflowData->workFolder = NULL;
    workflowData->updateManifest = NULL;
    workflowData->handlerProperties = NULL;
    workflowData->files = NULL;
    workflowData->fileCount = 0;
    workflowData->logCallback = NULL;
    workflowData->logContext = NULL;
    workflowData->statusCallback = NULL;
    workflowData->statusContext = NULL;

    return workflowData;
}

/**
 * @brief Free a workflow data instance
 */
void ADUC_WorkflowData_Free(ADUC_WorkflowData* workflowData)
{
    if (workflowData)
    {
        free(workflowData->updateId);
        free(workflowData->workFolder);
        free(workflowData->updateManifest);
        free(workflowData->handlerProperties);

        // Free file entities
        if (workflowData->files)
        {
            for (size_t i = 0; i < workflowData->fileCount; i++)
            {
                free(workflowData->files[i].fileName);
                free(workflowData->files[i].downloadUri);
                free(workflowData->files[i].targetFilePath);
                free(workflowData->files[i].fileHash);
            }
            free(workflowData->files);
        }

        free(workflowData);
    }
}

/**
 * @brief Set the update ID for workflow data
 */
ADUC_Result_t ADUC_WorkflowData_SetUpdateId(ADUC_WorkflowData* workflowData, const char* updateId)
{
    if (!workflowData || !updateId)
    {
        return ADUC_Result_Failure_InvalidArgument;
    }

    free(workflowData->updateId);
    workflowData->updateId = strdup(updateId);

    return workflowData->updateId ? ADUC_Result_Success : ADUC_Result_Failure_OutOfMemory;
}

/**
 * @brief Set the work folder for workflow data
 */
ADUC_Result_t ADUC_WorkflowData_SetWorkFolder(ADUC_WorkflowData* workflowData, const char* workFolder)
{
    if (!workflowData || !workFolder)
    {
        return ADUC_Result_Failure_InvalidArgument;
    }

    free(workflowData->workFolder);
    workflowData->workFolder = strdup(workFolder);

    return workflowData->workFolder ? ADUC_Result_Success : ADUC_Result_Failure_OutOfMemory;
}

/**
 * @brief Add a file entity to workflow data
 */
ADUC_Result_t ADUC_WorkflowData_AddFile(ADUC_WorkflowData* workflowData, const ADUC_FileEntity* file)
{
    if (!workflowData || !file)
    {
        return ADUC_Result_Failure_InvalidArgument;
    }

    // Reallocate files array
    ADUC_FileEntity* newFiles = (ADUC_FileEntity*)realloc(
        workflowData->files,
        (workflowData->fileCount + 1) * sizeof(ADUC_FileEntity)
    );

    if (!newFiles)
    {
        return ADUC_Result_Failure_OutOfMemory;
    }

    workflowData->files = newFiles;

    // Copy the file entity
    ADUC_FileEntity* targetFile = &workflowData->files[workflowData->fileCount];
    memset(targetFile, 0, sizeof(ADUC_FileEntity));

    if (file->fileName)
    {
        targetFile->fileName = strdup(file->fileName);
        if (!targetFile->fileName)
        {
            return ADUC_Result_Failure_OutOfMemory;
        }
    }

    if (file->downloadUri)
    {
        targetFile->downloadUri = strdup(file->downloadUri);
        if (!targetFile->downloadUri)
        {
            free(targetFile->fileName);
            return ADUC_Result_Failure_OutOfMemory;
        }
    }

    if (file->targetFilePath)
    {
        targetFile->targetFilePath = strdup(file->targetFilePath);
        if (!targetFile->targetFilePath)
        {
            free(targetFile->fileName);
            free(targetFile->downloadUri);
            return ADUC_Result_Failure_OutOfMemory;
        }
    }

    if (file->fileHash)
    {
        targetFile->fileHash = strdup(file->fileHash);
        if (!targetFile->fileHash)
        {
            free(targetFile->fileName);
            free(targetFile->downloadUri);
            free(targetFile->targetFilePath);
            return ADUC_Result_Failure_OutOfMemory;
        }
    }

    targetFile->sizeInBytes = file->sizeInBytes;

    workflowData->fileCount++;
    return ADUC_Result_Success;
}

/**
 * @brief Get the number of files in workflow data
 */
size_t ADUC_WorkflowData_GetFileCount(const ADUC_WorkflowData* workflowData)
{
    return workflowData ? workflowData->fileCount : 0;
}

/**
 * @brief Get a file entity by index
 */
const ADUC_FileEntity* ADUC_WorkflowData_GetFile(const ADUC_WorkflowData* workflowData, size_t index)
{
    if (!workflowData || index >= workflowData->fileCount)
    {
        return NULL;
    }

    return &workflowData->files[index];
}
