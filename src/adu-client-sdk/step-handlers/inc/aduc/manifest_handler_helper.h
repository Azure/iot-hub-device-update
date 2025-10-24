/**
 * @file manifest_handler_helper.h
 * @brief Helper functions for manifest handling operations
 *
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#ifndef ADUC_MANIFEST_HANDLER_HELPER_H
#define ADUC_MANIFEST_HANDLER_HELPER_H

#include "aduc/result.h"
#include "aduc/workflow_data.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Manifest data structure
 */
typedef struct
{
    char* rawJson;                             ///< Raw JSON manifest
    char* updateId;                           ///< Update ID
    char* provider;                           ///< Update provider
    char* name;                               ///< Update name
    char* version;                            ///< Update version
    ADUC_FileEntity* files;                   ///< Array of files
    size_t fileCount;                         ///< Number of files
} ADUC_ManifestData;

/**
 * @brief Parse a JSON manifest string
 * @param manifestJson The JSON manifest string
 * @param manifestData The manifest data structure to populate
 * @return ADUC_Result_t indicating success or failure
 */
ADUC_Result_t ADUC_ManifestHelper_ParseJson(const char* manifestJson, ADUC_ManifestData* manifestData);

/**
 * @brief Free manifest data
 * @param manifestData The manifest data to free
 */
void ADUC_ManifestHelper_Free(ADUC_ManifestData* manifestData);

#ifdef __cplusplus
}
#endif

#endif // ADUC_MANIFEST_HANDLER_HELPER_H
