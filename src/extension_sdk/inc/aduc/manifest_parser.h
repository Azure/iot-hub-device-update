/**
 * @file manifest_parser.h
 * @brief Deployment manifest parser for ADU Gen2 agent.
 *
 * Parses the Gen1-compatible JSON deployment manifest into C structures
 * used by the workflow engine. This header is the source of truth for the
 * deployment data model (ADUC_DeploymentStep, ADUC_StepFile, etc.).
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_MANIFEST_PARSER_H
#define ADUC_MANIFEST_PARSER_H

#include "aduc/extension_types.h"
#include "aduc/step_handler_vtable.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Represents a single deployment step (inline or reference).
 */
typedef struct ADUC_DeploymentStep
{
    const char* stepId;                         /**< Step identifier (e.g. "step_0") */
    const char* handlerType;                    /**< Handler type (e.g. "microsoft/swupdate:2") */
    const char* handlerConfigJson;              /**< Handler properties serialized as JSON string */
    const char* installedCriteria;              /**< Criteria to check if already installed */
    const char* componentId;                    /**< Target component ID (NULL if device-level) */
    const char* componentGroup;                 /**< Target component group (NULL if device-level) */
    const ADUC_PropertyMap* componentProperties;/**< Additional component properties (NULL if none) */
    const ADUC_StepFile* files;                 /**< Pointer into manifest's file array for this step */
    size_t fileCount;                           /**< Number of files referenced by this step */
    const char** requires;                      /**< Step IDs this step depends on (from handlerProperties) */
    size_t requiresCount;
    const char** skipOnFailed;                  /**< Skip if any of these failed (from handlerProperties) */
    size_t skipOnFailedCount;
    const char** runOnFailed;                   /**< Only run if any of these failed (from handlerProperties) */
    size_t runOnFailedCount;
} ADUC_DeploymentStep;

/**
 * @brief Update identity (provider/name/version triple).
 */
typedef struct ADUC_UpdateId
{
    char provider[64];
    char name[128];
    char version[32];
} ADUC_UpdateId;

/**
 * @brief Top-level parsed manifest structure.
 */
typedef struct ADUC_ParsedManifest
{
    char workflowId[64];            /**< Workflow identifier */
    ADUC_UpdateId updateId;         /**< Update identity */
    ADUC_DeploymentStep* steps;     /**< Array of deployment steps */
    size_t stepCount;               /**< Number of steps */
    ADUC_StepFile* files;           /**< Global file table */
    size_t fileCount;               /**< Number of files in the global table */
} ADUC_ParsedManifest;

/**
 * @brief Parse a deployment manifest JSON string into structured data.
 *
 * @param json      Pointer to the JSON string.
 * @param jsonLen   Length of the JSON string (excluding null terminator).
 * @param outManifest  On success, receives a pointer to the parsed manifest.
 *                     Caller must free with ADUC_Manifest_Free().
 * @return ADUC_Result2 with code 0 on success.
 */
ADUC_Result2 ADUC_Manifest_Parse(const char* json, size_t jsonLen, ADUC_ParsedManifest** outManifest);

/**
 * @brief Free a parsed manifest and all associated memory.
 *
 * @param manifest  Pointer returned by ADUC_Manifest_Parse(). Safe to pass NULL.
 */
void ADUC_Manifest_Free(ADUC_ParsedManifest* manifest);

/**
 * @brief Look up a file in the manifest's global file table by file ID.
 *
 * @param manifest  The parsed manifest.
 * @param fileId    File identifier string (e.g. "f1").
 * @return Pointer to the file entry, or NULL if not found.
 */
const ADUC_StepFile* ADUC_Manifest_GetFile(const ADUC_ParsedManifest* manifest, const char* fileId);

#ifdef __cplusplus
}
#endif

#endif // ADUC_MANIFEST_PARSER_H
