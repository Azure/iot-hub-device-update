/**
 * @file workflow_data.h
 * @brief Step Handler SDK Workflow Data Interface
 *
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#ifndef ADUC_WORKFLOW_DATA_H
#define ADUC_WORKFLOW_DATA_H

#include "aduc/exports.h"
#include "aduc/types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Workflow data structure
 *
 * Contains all the information needed by content handlers to process updates.
 */
typedef struct tagADUC_WorkflowData
{
    /**
     * @brief Update identifier
     */
    ADUC_UpdateId updateId;

    /**
     * @brief Workflow identifier
     */
    const char* workflowId;

    /**
     * @brief Work folder path for this update
     */
    const char* workFolder;

    /**
     * @brief Update manifest as JSON string
     */
    const char* updateManifest;

    /**
     * @brief Handler properties as JSON string
     */
    const char* handlerProperties;

    /**
     * @brief Array of files included in this update
     */
    ADUC_FileInfo* files;

    /**
     * @brief Number of files in the files array
     */
    size_t fileCount;

    /**
     * @brief Progress callback function
     */
    ADUC_ProgressCallback progressCallback;

    /**
     * @brief Progress callback context
     */
    void* progressContext;

    /**
     * @brief Status callback function
     */
    ADUC_StatusCallback statusCallback;

    /**
     * @brief Status callback context
     */
    void* statusContext;

    /**
     * @brief Log callback function
     */
    ADUC_LogCallback logCallback;

    /**
     * @brief Log callback context
     */
    void* logContext;

} ADUC_WorkflowData;

/**
 * @brief Get the work folder path from workflow data
 *
 * @param workflowData The workflow data
 * @return const char* The work folder path, or NULL if not available
 */
ADUC_SDK_EXPORT const char* ADUC_WorkflowData_GetWorkFolder(const ADUC_WorkflowData* workflowData);

/**
 * @brief Get the update manifest from workflow data
 *
 * @param workflowData The workflow data
 * @return const char* The update manifest as JSON, or NULL if not available
 */
ADUC_SDK_EXPORT const char* ADUC_WorkflowData_GetUpdateManifest(const ADUC_WorkflowData* workflowData);

/**
 * @brief Get handler properties from workflow data
 *
 * @param workflowData The workflow data
 * @return const char* The handler properties as JSON, or NULL if not available
 */
ADUC_SDK_EXPORT const char* ADUC_WorkflowData_GetHandlerProperties(const ADUC_WorkflowData* workflowData);

/**
 * @brief Get file count from workflow data
 *
 * @param workflowData The workflow data
 * @return size_t Number of files in the update
 */
ADUC_SDK_EXPORT size_t ADUC_WorkflowData_GetFileCount(const ADUC_WorkflowData* workflowData);

/**
 * @brief Get file info by index from workflow data
 *
 * @param workflowData The workflow data
 * @param index File index (0-based)
 * @return const ADUC_FileInfo* File information, or NULL if index is invalid
 */
ADUC_SDK_EXPORT const ADUC_FileInfo* ADUC_WorkflowData_GetFile(const ADUC_WorkflowData* workflowData, size_t index);

/**
 * @brief Report progress to the update agent
 *
 * @param workflowData The workflow data
 * @param bytesTransferred Number of bytes transferred
 * @param totalBytes Total number of bytes to transfer
 * @param percentage Completion percentage (0-100)
 */
ADUC_SDK_EXPORT void ADUC_WorkflowData_ReportProgress(
    const ADUC_WorkflowData* workflowData,
    uint64_t bytesTransferred,
    uint64_t totalBytes,
    int32_t percentage);

/**
 * @brief Report status to the update agent
 *
 * @param workflowData The workflow data
 * @param state Current update state
 * @param message Status message (optional)
 */
ADUC_SDK_EXPORT void ADUC_WorkflowData_ReportStatus(
    const ADUC_WorkflowData* workflowData,
    ADUC_UpdateState state,
    const char* message);

/**
 * @brief Log a message through the update agent
 *
 * @param workflowData The workflow data
 * @param level Log level
 * @param message Log message
 */
ADUC_SDK_EXPORT void ADUC_WorkflowData_Log(
    const ADUC_WorkflowData* workflowData,
    ADUC_LogLevel level,
    const char* message);

#ifdef __cplusplus
}
#endif

#endif // ADUC_WORKFLOW_DATA_H
