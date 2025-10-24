/**
 * @file workflow_data_utils.h
 * @brief Utility functions for working with ADUC_WorkflowData
 * 
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#ifndef ADUC_WORKFLOW_DATA_UTILS_H
#define ADUC_WORKFLOW_DATA_UTILS_H

#include "aduc/workflow_data.h"
#include "aduc/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new workflow data instance
 * @return Pointer to new workflow data, or NULL on failure
 */
ADUC_WorkflowData* ADUC_WorkflowData_Create(void);

/**
 * @brief Free a workflow data instance
 * @param workflowData The workflow data to free
 */
void ADUC_WorkflowData_Free(ADUC_WorkflowData* workflowData);

/**
 * @brief Set the update ID for workflow data
 * @param workflowData The workflow data to modify
 * @param updateId The update ID to set
 * @return ADUC_Result_t indicating success or failure
 */
ADUC_Result_t ADUC_WorkflowData_SetUpdateId(ADUC_WorkflowData* workflowData, const char* updateId);

/**
 * @brief Set the work folder for workflow data
 * @param workflowData The workflow data to modify
 * @param workFolder The work folder path to set
 * @return ADUC_Result_t indicating success or failure
 */
ADUC_Result_t ADUC_WorkflowData_SetWorkFolder(ADUC_WorkflowData* workflowData, const char* workFolder);

/**
 * @brief Add a file entity to workflow data
 * @param workflowData The workflow data to modify
 * @param file The file entity to add
 * @return ADUC_Result_t indicating success or failure
 */
ADUC_Result_t ADUC_WorkflowData_AddFile(ADUC_WorkflowData* workflowData, const ADUC_FileEntity* file);

/**
 * @brief Get the number of files in workflow data
 * @param workflowData The workflow data to query
 * @return Number of files
 */
size_t ADUC_WorkflowData_GetFileCount(const ADUC_WorkflowData* workflowData);

/**
 * @brief Get a file entity by index
 * @param workflowData The workflow data to query
 * @param index The file index
 * @return Pointer to file entity, or NULL if index is invalid
 */
const ADUC_FileEntity* ADUC_WorkflowData_GetFile(const ADUC_WorkflowData* workflowData, size_t index);

#ifdef __cplusplus
}
#endif

#endif // ADUC_WORKFLOW_DATA_UTILS_H