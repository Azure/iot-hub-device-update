/**
 * @file adu_core.h
 * @brief Describes types used in ADUC agent code.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_TYPES_ADU_CORE_H
#define ADUC_TYPES_ADU_CORE_H

#include "aduc/adu_types.h"
#include "aduc/result.h"
#include "aduc/types/download.h"
#include "aduc/types/update_content.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h> // uint64_t

EXTERN_C_BEGIN

/**
 * @brief Opaque token passed to callbacks.
 */
typedef void* ADUC_Token;
typedef void* ADUC_WorkflowDataToken;

/**
 * @brief Method called from upper-layer when async work is completed.
 *
 * @param workCompletionToken Token received at method call.
 * @param result Result of work.
 * @param isAsync true when worker thread, false when main thread.
 */
typedef void (*WorkCompletionCallbackFunc)(const void* workCompletionToken, ADUC_Result result, bool isAsync);

typedef struct tagADUC_WorkCompletionData
{
    WorkCompletionCallbackFunc WorkCompletionCallback;
    const void* WorkCompletionToken;
} ADUC_WorkCompletionData;

//
// Idle callback
//

/**
 * @brief Callback method to indicate (return to) Idle state.
 *
 * @param token Opaque token.
 * @param workflowId Current workflow identifier.
 */
typedef void (*IdleCallbackFunc)(ADUC_Token token, const char* workflowId);

//
// Download callback.
//

/**
 * @brief Callback method to do download.
 *
 * Must not block!
 *
 * @param token Opaque token.
 * @param workCompletionData Method and value to call when work completes if *Result_InProgress returned.
 * @param workflowData Data about what to download.
 */
typedef ADUC_Result (*DownloadCallbackFunc)(
    ADUC_Token token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData);

//
// Backup callback.
//

/**
 * @brief Callback method to do backup.
 *
 * Must not block!
 *
 * @param token Opaque token.
 * @param workCompletionData Method and value to call when work completes if *Result_InProgress returned.
 * @param workflowData Data about what to backup.
 */
typedef ADUC_Result (*BackupCallbackFunc)(
    ADUC_Token token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData);

//
// Install callback.
//

/**
 * @brief Signature for Install callback.
 *
 * Must not block!
 *
 * @param token Opaque token.
 * @param workCompletionData Method and value to call when work completes if *Result_InProgress returned.
 * @param workflowData Data about what to install.
 */
typedef ADUC_Result (*InstallCallbackFunc)(
    ADUC_Token token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData);

// Apply callback.

/**
 * @brief Signature for Apply callback.
 *
 * Must not block!
 *
 * @param token Opaque token.
 * @param workCompletionData Method and value to call when work completes if *Result_InProgress returned.
 * @param info Data about what to apply.
 */
typedef ADUC_Result (*ApplyCallbackFunc)(
    ADUC_Token Token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData);

//
// Restore callback.
//

/**
 * @brief Callback method to do restore.
 *
 * Must not block!
 *
 * @param token Opaque token.
 * @param workCompletionData Method and value to call when work completes if *Result_InProgress returned.
 * @param workflowData Data about what to restore.
 */
typedef ADUC_Result (*RestoreCallbackFunc)(
    ADUC_Token token, const ADUC_WorkCompletionData* workCompletionData, ADUC_WorkflowDataToken workflowData);

// Cancel callback.

/**
 * @brief Signature for Cancel callback.
 *
 * @param token Opaque token.
 * @param workflowData The workflow data object.
 */
typedef void (*CancelCallbackFunc)(ADUC_Token token, ADUC_WorkflowDataToken workflowData);

/**
 * @brief Checks if the given content is installed.
 *
 * @param token Opaque token.
 * @param workflowData The workflow data object.
 * @return ADUC_Result True if the content is installed.
 */
typedef ADUC_Result (*IsInstalledCallbackFunc)(ADUC_Token token, ADUC_WorkflowDataToken workflowData);

//
// Sandbox creation/destruction callback
//

/**
 * @brief Callback method to create a download/install/apply sandbox.
 *
 * @param workflowId Unique workflow identifier.
 * @param workFolder Location of sandbox, or NULL if no sandbox is required, e.g. fileless OS.
 * Must be allocated using malloc.
 */
typedef ADUC_Result (*SandboxCreateCallbackFunc)(ADUC_Token token, const char* workflowId, char* workFolder);

/**
 * @brief Callback method to destroy a download/install/apply sandbox.
 *
 * @param token Opaque token.
 * @param workflowId Unique workflow identifier.
 * @param workFolder Sandbox that was returned from SandboxCreate - can be NULL.
 */
typedef void (*SandboxDestroyCallbackFunc)(ADUC_Token token, const char* workflowId, const char* workFolder);

/**
 * @brief Called regularly to allow for cooperative multitasking while working.
 * @param token Opaque token.
 * @param workflowData The workflow data object.
 */
typedef void (*DoWorkCallbackFunc)(ADUC_Token Token, ADUC_WorkflowDataToken workflowData);

// ADUC_UpdateActionCallbacks structure, to be filled out by upper-layer.

/**
 * @brief Defines methods that respond to an UpdateAction.
 */
typedef struct tagADUC_UpdateActionCallbacks
{
    // Message handlers.

    IdleCallbackFunc IdleCallback; /**< Idle state handler. */
    DownloadCallbackFunc DownloadCallback; /**< Download message handler. */
    BackupCallbackFunc BackupCallback; /**< Backup message handler. */
    InstallCallbackFunc InstallCallback; /**< Install message handler. */
    ApplyCallbackFunc ApplyCallback; /**< Apply message handler. */
    RestoreCallbackFunc RestoreCallback; /**< Restore message handler. */
    CancelCallbackFunc CancelCallback; /**< Cancel message handler. */

    IsInstalledCallbackFunc IsInstalledCallback; /**< IsInstalled function pointer */

    SandboxCreateCallbackFunc SandboxCreateCallback; /**< Sandbox creation message handler. */
    SandboxDestroyCallbackFunc SandboxDestroyCallback; /**< Sandbox destruction message handler. */

    DoWorkCallbackFunc DoWorkCallback; /**< DoWork function. */

    ADUC_Token PlatformLayerHandle; /**< Opaque token, passed to callbacks. */
} ADUC_UpdateActionCallbacks;

// clang-format off

typedef enum tagADUC_ResultCode
{
    // Failure codes.
    ADUC_Result_Failure = 0,
    ADUC_Result_Failure_Cancelled = -1,     /**< Primary task failed due to 'Cancel' action. */

    // Success codes.
    ADUC_Result_Success = 1,                         /**< General success. */
    ADUC_Result_Success_Cache_Miss = 2,              /**< General success when cache miss. */

    ADUC_Result_Register_Success = 100,              /**< Succeeded. */

    ADUC_Result_Idle_Success=200,                    /**< Succeeded. */

    ADUC_Result_SandboxCreate_Success = 300,          /**< Succeeded. */

    ADUC_Result_DeploymentInProgress_Success = 400,   /**< Entering DeploymentInProgress state and sending ACK of ProcessDeployment update action Succeeded. */

    ADUC_Result_Download_Success = 500,                  /**< Succeeded. */
    ADUC_Result_Download_InProgress = 501,               /**< Async operation started. CompletionCallback will be called when complete. */
    ADUC_Result_Download_Skipped_FileExists = 502,       /**< Download skipped. File already exists and hash validation passed. */

    ADUC_Result_Download_Skipped_UpdateAlreadyInstalled = 503, /**< Download succeeded. Also indicates that the Installed Criteria is met. */
    ADUC_Result_Download_Skipped_NoMatchingComponents = 504, /**< Download succeeded. Also indicates that no matchings components for this update. */

    ADUC_Result_Download_Handler_SuccessSkipDownload = 520,  /**< Succeeded. DownloadHandler was able to produce the update. Agent must skip downloading. */
    ADUC_Result_Download_Handler_RequiredFullDownload = 521, /**< Not a failure. Agent fallback to downloading the update is required. */

    ADUC_Result_Install_Success = 600,                       /**< Succeeded. */
    ADUC_Result_Install_InProgress = 601,                    /**< Async operation started. CompletionCallback will be called when complete. */

    ADUC_Result_Install_Skipped_UpdateAlreadyInstalled = 603, /**< Install succeeded. Also indicates that the Installed Criteria is met. */
    ADUC_Result_Install_Skipped_NoMatchingComponents = 604,  /**< Install succeeded. Also indicates that no matchings components for this update. */

    ADUC_Result_Install_RequiredImmediateReboot = 605,       /**< Succeeded. An immediate device reboot is required, to complete the task. */
    ADUC_Result_Install_RequiredReboot = 606,                /**< Succeeded. A deferred device reboot is required, to complete the task. */
    ADUC_Result_Install_RequiredImmediateAgentRestart = 607, /**< Succeeded. An immediate agent restart is required, to complete the task. */
    ADUC_Result_Install_RequiredAgentRestart = 608,          /**< Succeeded. A deferred agent restart is required, to complete the task. */

    ADUC_Result_Apply_Success = 700,                         /**< Succeeded. */
    ADUC_Result_Apply_InProgress = 701,                      /**< Async operation started. CompletionCallback will be called when complete. */

    ADUC_Result_Apply_RequiredImmediateReboot = 705,         /**< Succeeded. An immediate device reboot is required, to complete the task. */
    ADUC_Result_Apply_RequiredReboot = 706,                  /**< Succeeded. A deferred device reboot is required, to complete the task. */
    ADUC_Result_Apply_RequiredImmediateAgentRestart = 707,   /**< Succeeded. An immediate agent restart is required, to complete the task. */
    ADUC_Result_Apply_RequiredAgentRestart = 708,            /**< Succeeded. A deferred agent restart is required, to complete the task. */

    ADUC_Result_Cancel_Success = 800,            /**< Succeeded. */
    ADUC_Result_Cancel_UnableToCancel = 801,     /**< Not a failure. Cancel is best effort. */

    ADUC_Result_IsInstalled_Installed = 900,     /**< Succeeded and content is installed. */
    ADUC_Result_IsInstalled_NotInstalled = 901,  /**< Succeeded and content is not installed */

    ADUC_Result_Backup_Success = 1000,                       /**< Succeeded. */
    ADUC_Result_Backup_Success_Unsupported = 1001,           /**< Succeeded to proceed with the workflow, but the action is not implemented/supported in the content handler. */
    ADUC_Result_Backup_InProgress = 1002,                    /**< Async operation started. CompletionCallback will be called when complete. */

    ADUC_Result_Restore_Success = 1100,                      /**< Succeeded. */
    ADUC_Result_Restore_Success_Unsupported = 1101,          /**< Succeeded to proceed with the workflow, but the action is not implemented/supported in the content handler. */
    ADUC_Result_Restore_InProgress = 1102,                   /**< Async operation started. CompletionCallback will be called when complete. */

    ADUC_Result_Restore_RequiredImmediateReboot = 1105,         /**< Succeeded. An immediate device reboot is required, to complete the task. */
    ADUC_Result_Restore_RequiredReboot = 1106,                  /**< Succeeded. A deferred device reboot is required, to complete the task. */
    ADUC_Result_Restore_RequiredImmediateAgentRestart = 1107,   /**< Succeeded. An immediate agent restart is required, to complete the task. */
    ADUC_Result_Restore_RequiredAgentRestart = 1108,            /**< Succeeded. A deferred agent restart is required, to complete the task. */
} ADUC_ResultCode;

#define AducResultCodeIndicatesInProgress(resultCode)                                                  \
    ((resultCode) == ADUC_Result_Download_InProgress || (resultCode) == ADUC_Result_Backup_InProgress || \
    (resultCode) == ADUC_Result_Install_InProgress || (resultCode) == ADUC_Result_Apply_InProgress || \
    (resultCode) == ADUC_Result_Restore_InProgress)

// clang-format on

EXTERN_C_END

#endif // ADUC_TYPES_ADU_CORE_H
