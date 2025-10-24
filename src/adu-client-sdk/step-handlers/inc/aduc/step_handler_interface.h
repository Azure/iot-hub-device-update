/**
 * @file step_handler_interface.h
 * @brief Step Handler Interface API for Azure Device Update
 * 
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#ifndef ADUC_STEP_HANDLER_INTERFACE_H
#define ADUC_STEP_HANDLER_INTERFACE_H

#include "aduc/result.h"
#include "aduc/workflow_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Forward declaration of step handler
 */
typedef struct ADUC_StepHandler ADUC_StepHandler;

/**
 * @brief Step handler initialization function
 */
typedef ADUC_Result_t (*ADUC_StepHandler_InitializeFunc)(ADUC_StepHandler* handler, const char* config);

/**
 * @brief Step handler download function
 */
typedef ADUC_Result_t (*ADUC_StepHandler_DownloadFunc)(ADUC_StepHandler* handler, const ADUC_WorkflowData* workflowData);

/**
 * @brief Step handler backup function
 */
typedef ADUC_Result_t (*ADUC_StepHandler_BackupFunc)(ADUC_StepHandler* handler, const ADUC_WorkflowData* workflowData);

/**
 * @brief Step handler install function
 */
typedef ADUC_Result_t (*ADUC_StepHandler_InstallFunc)(ADUC_StepHandler* handler, const ADUC_WorkflowData* workflowData);

/**
 * @brief Step handler apply function
 */
typedef ADUC_Result_t (*ADUC_StepHandler_ApplyFunc)(ADUC_StepHandler* handler, const ADUC_WorkflowData* workflowData);

/**
 * @brief Step handler restore function
 */
typedef ADUC_Result_t (*ADUC_StepHandler_RestoreFunc)(ADUC_StepHandler* handler, const ADUC_WorkflowData* workflowData);

/**
 * @brief Step handler cancel function
 */
typedef ADUC_Result_t (*ADUC_StepHandler_CancelFunc)(ADUC_StepHandler* handler, const ADUC_WorkflowData* workflowData);

/**
 * @brief Step handler is installed check function
 */
typedef ADUC_Result_t (*ADUC_StepHandler_IsInstalledFunc)(ADUC_StepHandler* handler, const ADUC_WorkflowData* workflowData);

/**
 * @brief Step handler cleanup function
 */
typedef void (*ADUC_StepHandler_CleanupFunc)(ADUC_StepHandler* handler);

/**
 * @brief Step Handler structure
 */
struct ADUC_StepHandler
{
    ADUC_StepHandler_InitializeFunc Initialize;  ///< Initialize the handler
    ADUC_StepHandler_DownloadFunc Download;      ///< Download phase handler
    ADUC_StepHandler_BackupFunc Backup;          ///< Backup phase handler
    ADUC_StepHandler_InstallFunc Install;        ///< Install phase handler
    ADUC_StepHandler_ApplyFunc Apply;            ///< Apply phase handler
    ADUC_StepHandler_RestoreFunc Restore;        ///< Restore phase handler
    ADUC_StepHandler_CancelFunc Cancel;          ///< Cancel operation handler
    ADUC_StepHandler_IsInstalledFunc IsInstalled; ///< Is installed check handler
    ADUC_StepHandler_CleanupFunc Cleanup;        ///< Cleanup handler
    void* context;                               ///< Handler-specific context
};

/**
 * @brief Create a new step handler interface
 * @return Pointer to new step handler, or NULL on failure
 */
ADUC_StepHandler* ADUC_StepHandler_Create(void);

/**
 * @brief Free a step handler interface
 * @param handler The step handler to free
 */
void ADUC_StepHandler_Free(ADUC_StepHandler* handler);

#ifdef __cplusplus
}
#endif

#endif // ADUC_STEP_HANDLER_INTERFACE_H