/**
 * @file content_downloader_helper.c
 * @brief Helper functions for content download operations
 * 
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#include "aduc/content_downloader_helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Create a download context
 */
ADUC_DownloadContext* ADUC_DownloadContext_Create(void)
{
    ADUC_DownloadContext* context = (ADUC_DownloadContext*)calloc(1, sizeof(ADUC_DownloadContext));
    if (!context)
    {
        return NULL;
    }
    
    context->downloadUri = NULL;
    context->targetPath = NULL;
    context->progressCallback = NULL;
    context->progressContext = NULL;
    context->totalBytes = 0;
    context->downloadedBytes = 0;
    
    return context;
}

/**
 * @brief Free a download context
 */
void ADUC_DownloadContext_Free(ADUC_DownloadContext* context)
{
    if (context)
    {
        free(context->downloadUri);
        free(context->targetPath);
        free(context);
    }
}

/**
 * @brief Set download URI
 */
ADUC_Result_t ADUC_DownloadContext_SetUri(ADUC_DownloadContext* context, const char* uri)
{
    if (!context || !uri)
    {
        return ADUC_Result_Failure_InvalidArgument;
    }
    
    free(context->downloadUri);
    context->downloadUri = strdup(uri);
    
    return context->downloadUri ? ADUC_Result_Success : ADUC_Result_Failure_OutOfMemory;
}

/**
 * @brief Set target path
 */
ADUC_Result_t ADUC_DownloadContext_SetTargetPath(ADUC_DownloadContext* context, const char* path)
{
    if (!context || !path)
    {
        return ADUC_Result_Failure_InvalidArgument;
    }
    
    free(context->targetPath);
    context->targetPath = strdup(path);
    
    return context->targetPath ? ADUC_Result_Success : ADUC_Result_Failure_OutOfMemory;
}