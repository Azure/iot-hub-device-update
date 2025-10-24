/**
 * @file script_utils.h
 * @brief Utility functions for script handler operations
 * 
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#ifndef SCRIPT_UTILS_H
#define SCRIPT_UTILS_H

#include "aduc/result.h"
#include "aduc/workflow_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the script file path from workflow data
 * @param workflowData The workflow data containing file information
 * @return The path to the script file, or NULL if not found
 */
const char* GetScriptFilePath(const ADUC_WorkflowData* workflowData);

/**
 * @brief Download script files to the work folder
 * @param workflowData The workflow data containing download information
 * @param result Result details structure to populate
 * @return ADUC_Result_t indicating success or failure
 */
ADUC_Result_t DownloadScriptFiles(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result);

/**
 * @brief Make a file executable
 * @param filePath Path to the file to make executable
 * @return ADUC_Result_t indicating success or failure
 */
ADUC_Result_t MakeFileExecutable(const char* filePath);

/**
 * @brief Validate that required script files exist
 * @param workflowData The workflow data to validate
 * @return True if all required files exist, false otherwise
 */
bool ValidateScriptFiles(const ADUC_WorkflowData* workflowData);

#ifdef __cplusplus
}
#endif

#endif // SCRIPT_UTILS_H