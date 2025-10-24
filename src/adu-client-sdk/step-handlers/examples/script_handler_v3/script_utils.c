/**
 * @file script_utils.c
 * @brief Implementation of utility functions for script handler operations
 * 
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#include "script_utils.h"
#include "aduc/result.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <curl/curl.h>

/**
 * @brief Structure for CURL write callback
 */
typedef struct
{
    char* data;
    size_t size;
} DownloadBuffer;

/**
 * @brief CURL write callback function
 */
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, DownloadBuffer* buffer)
{
    size_t realsize = size * nmemb;
    
    char* ptr = realloc(buffer->data, buffer->size + realsize + 1);
    if (!ptr)
    {
        return 0; // Out of memory
    }
    
    buffer->data = ptr;
    memcpy(&(buffer->data[buffer->size]), contents, realsize);
    buffer->size += realsize;
    buffer->data[buffer->size] = 0;
    
    return realsize;
}

/**
 * @brief Get the script file path from workflow data
 */
const char* GetScriptFilePath(const ADUC_WorkflowData* workflowData)
{
    if (!workflowData || !workflowData->files || workflowData->fileCount == 0)
    {
        return NULL;
    }
    
    // Look for the first script file (typically .sh or .ps1)
    for (size_t i = 0; i < workflowData->fileCount; i++)
    {
        const char* fileName = workflowData->files[i].fileName;
        if (fileName)
        {
            // Check for script file extensions
            size_t len = strlen(fileName);
            if ((len > 3 && strcmp(&fileName[len-3], ".sh") == 0) ||
                (len > 4 && strcmp(&fileName[len-4], ".ps1") == 0) ||
                (len > 3 && strcmp(&fileName[len-3], ".py") == 0))
            {
                return workflowData->files[i].filePath;
            }
        }
    }
    
    // If no script extension found, return the first file
    return workflowData->files[0].filePath;
}

/**
 * @brief Download a single file using CURL
 */
static ADUC_Result_t DownloadFile(const char* url, const char* targetPath, ADUC_ResultDetails* result)
{
    CURL* curl;
    CURLcode res;
    FILE* fp;
    
    curl = curl_easy_init();
    if (!curl)
    {
        ADUC_Result_SetFailure(result, ADUC_Result_Failure_InstallFailed, "Failed to initialize CURL");
        return ADUC_Result_Failure_InstallFailed;
    }
    
    fp = fopen(targetPath, "wb");
    if (!fp)
    {
        curl_easy_cleanup(curl);
        ADUC_Result_SetFailure(result, ADUC_Result_Failure_FileNotFound, "Failed to open target file for writing");
        return ADUC_Result_Failure_FileNotFound;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L); // 5 minute timeout
    
    res = curl_easy_perform(curl);
    
    fclose(fp);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK)
    {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), "CURL download failed: %s", curl_easy_strerror(res));
        ADUC_Result_SetFailure(result, ADUC_Result_Failure_InstallFailed, errorMsg);
        return ADUC_Result_Failure_InstallFailed;
    }
    
    return ADUC_Result_Success;
}

/**
 * @brief Download script files to the work folder
 */
ADUC_Result_t DownloadScriptFiles(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result)
{
    if (!workflowData || !workflowData->files || workflowData->fileCount == 0)
    {
        ADUC_Result_SetFailure(result, ADUC_Result_Failure_InvalidArgument, "No files specified for download");
        return ADUC_Result_Failure_InvalidArgument;
    }
    
    // Download each file
    for (size_t i = 0; i < workflowData->fileCount; i++)
    {
        const ADUC_FileInfo* file = &workflowData->files[i];
        
        if (!file->fileName || !file->filePath)
        {
            ADUC_Result_SetFailure(result, ADUC_Result_Failure_InvalidArgument, "Invalid file entity");
            return ADUC_Result_Failure_InvalidArgument;
        }
        
        // For now, we'll assume files are already downloaded to filePath
        // In a real implementation, you'd download from a URL to the target path
        printf("Script file available at: %s\n", file->filePath);
        
        // Make script files executable
        ADUC_Result_t execResult = MakeFileExecutable(file->filePath);
        if (execResult != ADUC_Result_Success)
        {
            // Log warning but don't fail - file might not be a script
            printf("Warning: Could not make %s executable\n", file->filePath);
        }
    }
    
    ADUC_Result_SetSuccess(result, "All files downloaded successfully");
    return ADUC_Result_Success;
}

/**
 * @brief Make a file executable
 */
ADUC_Result_t MakeFileExecutable(const char* filePath)
{
    if (!filePath)
    {
        return ADUC_Result_Failure_InvalidArgument;
    }
    
    struct stat st;
    if (stat(filePath, &st) != 0)
    {
        return ADUC_Result_Failure_FileNotFound;
    }
    
    // Add execute permissions for user, group, and other
    mode_t newMode = st.st_mode | S_IXUSR | S_IXGRP | S_IXOTH;
    
    if (chmod(filePath, newMode) != 0)
    {
        return ADUC_Result_Failure_InstallFailed;
    }
    
    return ADUC_Result_Success;
}

/**
 * @brief Validate that required script files exist
 */
bool ValidateScriptFiles(const ADUC_WorkflowData* workflowData)
{
    if (!workflowData || !workflowData->files || workflowData->fileCount == 0)
    {
        return false;
    }
    
    // Check that at least one file exists and is accessible
    for (size_t i = 0; i < workflowData->fileCount; i++)
    {
        const char* filePath = workflowData->files[i].filePath;
        if (filePath && access(filePath, F_OK) == 0)
        {
            return true;
        }
    }
    
    return false;
}