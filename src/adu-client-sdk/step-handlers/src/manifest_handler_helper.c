/**
 * @file manifest_handler_helper.c
 * @brief Helper functions for manifest handling operations
 * 
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#include "aduc/manifest_handler_helper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Parse a JSON manifest string
 */
ADUC_Result_t ADUC_ManifestHelper_ParseJson(const char* manifestJson, ADUC_ManifestData* manifestData)
{
    if (!manifestJson || !manifestData)
    {
        return ADUC_Result_Failure_InvalidArgument;
    }
    
    // This is a simplified implementation
    // In a real implementation, you would use a JSON parser like cJSON
    memset(manifestData, 0, sizeof(ADUC_ManifestData));
    
    // For now, just copy the raw JSON
    manifestData->rawJson = strdup(manifestJson);
    if (!manifestData->rawJson)
    {
        return ADUC_Result_Failure_OutOfMemory;
    }
    
    return ADUC_Result_Success;
}

/**
 * @brief Free manifest data
 */
void ADUC_ManifestHelper_Free(ADUC_ManifestData* manifestData)
{
    if (manifestData)
    {
        free(manifestData->rawJson);
        free(manifestData->updateId);
        free(manifestData->provider);
        free(manifestData->name);
        free(manifestData->version);
        
        if (manifestData->files)
        {
            for (size_t i = 0; i < manifestData->fileCount; i++)
            {
                free(manifestData->files[i].fileName);
                free(manifestData->files[i].downloadUri);
                free(manifestData->files[i].fileHash);
            }
            free(manifestData->files);
        }
        
        memset(manifestData, 0, sizeof(ADUC_ManifestData));
    }
}