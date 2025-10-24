/**
 * @file content_handler.h
 * @brief Step Handler SDK Content Handler Interface
 * 
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#ifndef ADUC_CONTENT_HANDLER_H
#define ADUC_CONTENT_HANDLER_H

#include "aduc/exports.h"
#include "aduc/result.h"
#include "aduc/types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Forward declaration of workflow data structure
 */
typedef struct tagADUC_WorkflowData ADUC_WorkflowData;

/**
 * @brief Content Handler interface structure
 * 
 * This structure defines the interface that all content handlers must implement.
 * Each function pointer corresponds to a specific phase of the update process.
 */
typedef struct tagADUC_ContentHandler
{
    /**
     * @brief Download phase handler
     * @param workflowData The workflow data containing update information
     * @param result Output result details
     * @return ADUC_Result_t Result code
     */
    ADUC_Result_t (*Download)(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result);
    
    /**
     * @brief Backup phase handler
     * @param workflowData The workflow data containing update information
     * @param result Output result details
     * @return ADUC_Result_t Result code
     */
    ADUC_Result_t (*Backup)(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result);
    
    /**
     * @brief Install phase handler
     * @param workflowData The workflow data containing update information
     * @param result Output result details
     * @return ADUC_Result_t Result code
     */
    ADUC_Result_t (*Install)(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result);
    
    /**
     * @brief Apply phase handler
     * @param workflowData The workflow data containing update information
     * @param result Output result details
     * @return ADUC_Result_t Result code
     */
    ADUC_Result_t (*Apply)(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result);
    
    /**
     * @brief Restore phase handler
     * @param workflowData The workflow data containing update information
     * @param result Output result details
     * @return ADUC_Result_t Result code
     */
    ADUC_Result_t (*Restore)(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result);
    
    /**
     * @brief Cancel phase handler
     * @param workflowData The workflow data containing update information
     * @param result Output result details
     * @return ADUC_Result_t Result code
     */
    ADUC_Result_t (*Cancel)(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result);
    
    /**
     * @brief IsInstalled check handler
     * @param workflowData The workflow data containing update information
     * @param result Output result details
     * @return ADUC_Result_t Result code
     */
    ADUC_Result_t (*IsInstalled)(const ADUC_WorkflowData* workflowData, ADUC_ResultDetails* result);
    
    /**
     * @brief Cleanup function to free handler resources
     * @param handler The handler to clean up
     */
    void (*Free)(struct tagADUC_ContentHandler* handler);
    
    /**
     * @brief Handler-specific context data
     */
    void* context;
    
} ADUC_ContentHandler;

/**
 * @brief Standard content handler factory function signature
 * 
 * Each content handler extension must export a function with this signature
 * as their entry point.
 * 
 * @param logLevel The log level to use for this handler
 * @return ADUC_ContentHandler* Pointer to the created handler, or NULL on failure
 */
typedef ADUC_ContentHandler* (*ADUC_CreateContentHandler_Fn)(ADUC_LogLevel logLevel);

/**
 * @brief Standard content handler entry point function name
 * 
 * Extensions should export a function with this exact name.
 */
#define ADUC_CREATE_CONTENT_HANDLER_FUNCTION_NAME "CreateUpdateContentHandlerExtension"

#ifdef __cplusplus
}
#endif

#endif // ADUC_CONTENT_HANDLER_H