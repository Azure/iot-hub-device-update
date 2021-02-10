/**
 * @file adu_core_exports.h
 * @brief Describes methods to be exported from platform-specific ADUC agent code.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#ifndef ADUC_ADU_CORE_EXPORTS_H
#define ADUC_ADU_CORE_EXPORTS_H

#include "aduc/result.h"
#include <aduc/c_utils.h>
#include <stddef.h>
#include <stdint.h> // uint64_t

EXTERN_C_BEGIN

/**
 * @brief Opaque token passed to callbacks.
 */
typedef void* ADUC_Token;

/**
 * @brief Method called from upper-layer when async work is completed.
 *
 * @param workCompletionToken Token received at method call.
 * @param result Result of work.
 */
typedef void (*WorkCompletionCallbackFunc)(const void* workCompletionToken, ADUC_Result result);

typedef struct tagADUC_WorkCompletionData
{
    WorkCompletionCallbackFunc WorkCompletionCallback;
    const void* WorkCompletionToken;
} ADUC_WorkCompletionData;

//
// Lower Layer Result
//

/**
 * @brief ResultCode value that is used when there is a failure within the lower layer
 */
typedef enum tagADUC_LowerLayerResult
{
    ADUC_LowerLayerResult_Failure = 0, /**< Failed. */
} ADUC_LowerLayerResult;

//
// Idle callback
//

/**
 * @brief Return value for Idle calls.
 */
typedef enum tagADUC_IdleResult
{
    ADUC_IdleResult_Failure = 0, /**< Failed. */

    ADUC_IdleResult_Success = 1, /**< Succeeded. */
} ADUC_IdleResult;

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
 * @brief Encapsulates the hash and the hash type
 */
typedef struct tagADUC_Hash
{
    char* value; /** The value of the actual hash */
    char* type; /** The type of hash held in the entry*/
} ADUC_Hash;

/**
 * @brief Describes a specific file to download.
 */
typedef struct tagADUC_FileEntity
{
    char* TargetFilename; /**< File name to store content in DownloadUri to. */
    char* DownloadUri; /**< The URI of the file to download. */
    ADUC_Hash* Hash; /**< Array of ADUC_Hashes containing the hash options for the file*/
    size_t HashCount; /**< Total number of hashes in the array of hashes */
    char* FileId; /**< Id for the file */
} ADUC_FileEntity;

/**
 * @brief Describes Prepare info
 */
typedef struct tagADUC_PrepareInfo
{
    char* updateType; /**< Handler UpdateType. */
    char* updateTypeName; /**< Provider/Name in the UpdateType string. */
    unsigned int updateTypeVersion; /**< Version number in the UpdateType string. */
    unsigned int fileCount; /**< Number of files in #Files list. */
    ADUC_FileEntity* files; /**< Array of #ADUC_FileEntity objects describing what to download. */
} ADUC_PrepareInfo;

typedef struct tagADUC_UpdateId
{
    char* Provider; /**< Provider for the update */
    char* Name; /**< Name for the update */
    char* Version; /**< Version for the update*/
} ADUC_UpdateId;

/**
 * @brief Defines download progress state for download progress callback.
 */
typedef enum tagADUC_DownloadProgressState
{
    ADUC_DownloadProgressState_NotStarted = 0,
    ADUC_DownloadProgressState_InProgress,
    ADUC_DownloadProgressState_Completed,
    ADUC_DownloadProgressState_Cancelled,
    ADUC_DownloadProgressState_Error
} ADUC_DownloadProgressState;

/**
 * @brief Function signature for callback to send download progress to.
 */
typedef void (*ADUC_DownloadProgressCallback)(
    const char* workflowId,
    const char* fileId,
    ADUC_DownloadProgressState state,
    uint64_t bytesTransferred,
    uint64_t bytesTotal);

/**
 * @brief Describes what to Download. Sent to Download call.
 */
typedef struct tagADUC_DownloadInfo
{
    char* WorkFolder; /**< Folder to download content to. */

    unsigned int FileCount; /**< Number of files in #Files list. */
    ADUC_FileEntity* Files; /**< Array of #ADUC_FileEntity objects describing what to download. */

    ADUC_DownloadProgressCallback NotifyDownloadProgress; /**< Callback to provide download progress. */
} ADUC_DownloadInfo;

/**
 * @brief Valid values for ADUC_Result.ResultCode for Download().
 *
 * Implementation specific error should be set in ADUC_Result ExtendedResultCode.
 */
typedef enum tagADUC_DownloadResult
{
    ADUC_DownloadResult_Failure = 0, /**< Failed. */
    ADUC_DownloadResult_Cancelled = -1, /**< Cancelled. */

    ADUC_DownloadResult_Success = 1, /**< Succeeded. */
    ADUC_DownloadResult_InProgress =
        2, /**< Async operation started. CompletionCallback will be called when complete. */
} ADUC_DownloadResult;

/**
 * @brief Callback method to do download.
 *
 * Must not block!
 *
 * @param token Opaque token.
 * @param workflowId Current workflow identifier.
 * @param workCompletionData Method and value to call when work completes if *Result_InProgress returned.
 * @param info Data about what to download.
 */
typedef ADUC_Result (*DownloadCallbackFunc)(
    ADUC_Token token,
    const char* workflowId,
    const char* updateType,
    const ADUC_WorkCompletionData* workCompletionData,
    const ADUC_DownloadInfo* info);

//
// Install callback.
//

/**
 * @brief Data required by Install method.
 */
typedef struct tagADUC_InstallInfo
{
    char* WorkFolder; /**< Folder containing content to be installed. */
} ADUC_InstallInfo;

/**
 * @brief Valid values for ADUC_Result ResultCode for Install.
 *
 * Implementation specific error should be set in ADUC_Result ExtendedResultCode.
 */
typedef enum tagADUC_InstallResult
{
    ADUC_InstallResult_Failure = 0, /**< Failed. */
    ADUC_InstallResult_Cancelled = -1, /**< Cancelled. */

    ADUC_InstallResult_Success = 1, /**< Succeeded. */
    ADUC_InstallResult_InProgress =
        2, /**< Async operation started. CompletionCallback will be called when complete. */
} ADUC_InstallResult;

/**
 * @brief Signature for Install callback.
 *
 * Must not block!
 *
 * @param token Opaque token.
 * @param workflowId Current workflow identifier.
 * @param workCompletionData Method and value to call when work completes if *Result_InProgress returned.
 * @param info Data about what to install.
 */
typedef ADUC_Result (*InstallCallbackFunc)(
    ADUC_Token token,
    const char* workflowId,
    const ADUC_WorkCompletionData* workCompletionData,
    const ADUC_InstallInfo* info);

// Apply callback.

/**
 * @brief Data required by Apply method.
 */
typedef struct tagADUC_ApplyInfo
{
    char* WorkFolder; /**< Folder containing content to be applied. */
} ADUC_ApplyInfo;

/**
 * @brief Valid values for ADUC_Result ResultCode for Apply.
 *
 * Implementation specific error should be set in ADUC_Result ExtendedResultCode.
 */
typedef enum tagADUC_ApplyResult
{
    ADUC_ApplyResult_Failure = 0, /**< Failed. */
    ADUC_ApplyResult_Cancelled = -1, /**< Cancelled. */

    ADUC_ApplyResult_Success = 1, /**< Succeeded. */
    ADUC_ApplyResult_InProgress = 2, /**< Async operation started. CompletionCallback will be called when complete. */
    ADUC_ApplyResult_SuccessRebootRequired = 3, /**< Succeeded. Reboot required to apply. */
    ADUC_ApplyResult_SuccessAgentRestartRequired = 4, /**< Succeeded. Agent restart required to apply. */
} ADUC_ApplyResult;

/**
 * @brief Signature for Apply callback.
 *
 * Must not block!
 *
 * @param token Opaque token.
 * @param workflowId Current workflow identifier.
 * @param workCompletionData Method and value to call when work completes if *Result_InProgress returned.
 * @param info Data about what to apply.
 */
typedef ADUC_Result (*ApplyCallbackFunc)(
    ADUC_Token Token,
    const char* workflowId,
    const ADUC_WorkCompletionData* workCompletionData,
    const ADUC_ApplyInfo* info);

// Cancel callback.

/**
 * @brief Return value for Cancel calls.
 */
typedef enum tagADUC_CancelResult
{
    ADUC_CancelResult_Failure = 0, /**< General failure. */

    ADUC_CancelResult_Success = 1, /**< Succeeded (cancelled). */
    ADUC_CancelResult_UnableToCancel = 2, /**< Not a failure. Cancel is best effort. */
} ADUC_CancelResult;

/**
 * @brief Signature for Cancel callback.
 *
 * @param token Opaque token.
 * @param workflowId Current workflow identifier.
 */
typedef void (*CancelCallbackFunc)(ADUC_Token token, const char* workflowId);

/**
 * @brief Return value for IsInstalled calls.
 */
typedef enum tagADUC_IsInstalledResult
{
    ADUC_IsInstalledResult_Failure = 0, /**< General failure. */

    ADUC_IsInstalledResult_Installed = 1, /**< Succeeded and content is installed. */
    ADUC_IsInstalledResult_NotInstalled = 2, /**< Succeeded and content is not installed */
} ADUC_IsInstalledResult;

/**
 * @brief Checks if the given content is installed.
 *
 * @param token Opaque token.
 * @param workflowId The unique ID for the workflow.
 * @param installedCriteria The installed criteria string.
 * @return ADUC_Result True if the content is installed.
 */
typedef ADUC_Result (*IsInstalledCallbackFunc)(
    ADUC_Token token, const char* workflowId, const char* updateType, const char* installedCriteria);

//
// Sandbox creation/destruction callback
//

/**
 * @brief Return value for SandboxCreate calls.
 */
typedef enum tagADUC_SandboxCreateResult
{
    ADUC_SandboxCreateResult_Failure = 0, /**< Failed. */

    ADUC_SandboxCreateResult_Success = 1, /**< Succeeded. */
} ADUC_SandboxCreateResult;

/**
 * @brief Callback method to create a download/install/apply sandbox.
 *
 * @param workflowId Unique workflow identifier.
 * @param workFolder Location of sandbox, or NULL if no sandbox is required, e.g. fileless OS.
 * Must be allocated using malloc.
 */
typedef ADUC_Result (*SandboxCreateCallbackFunc)(ADUC_Token token, const char* workflowId, char** workFolder);

/**
 * @brief Callback method to destroy a download/install/apply sandbox.
 *
 * @param token Opaque token.
 * @param workflowId Unique workflow identifier.
 * @param workFolder Sandbox that was returned from SandboxCreate - can be NULL.
 */
typedef void (*SandboxDestroyCallbackFunc)(ADUC_Token token, const char* workflowId, const char* workFolder);

/**
 * @brief Return value for Prepare calls.
 */
typedef enum tagADUC_PrepareResult
{
    ADUC_PrepareResult_Failure = 0, /**< Failed. */
    ADUC_PrepareResult_Success = 1, /**< Succeeded. */

} ADUC_PrepareResult;

/**
 * @brief Callback method to do parepare.
 *
 * Must not block!
 *
 * @param token Opaque token.
 * @param workflowId Current workflow identifier.
 * @param metadata
 */
typedef ADUC_Result (*PrepareCallbackFunc)(
    ADUC_Token token, const char* workflowId, const ADUC_PrepareInfo* prepareInfo);

/**
 * @brief Called regularly to allow for cooperative multitasking while working.
 * @param token Opaque token.
 * @param workflowId Unique workflow identifier.
 */
typedef void (*DoWorkCallbackFunc)(ADUC_Token Token, const char* workflowId);

// RegisterData structure, to be filled out by upper-layer.

/**
 * @brief Defines methods that respond to an UpdateAction.
 */
typedef struct tagADUC_RegisterData
{
    // Message handlers.

    IdleCallbackFunc IdleCallback; /**< Idle state handler. */
    DownloadCallbackFunc DownloadCallback; /**< Download message handler. */
    InstallCallbackFunc InstallCallback; /**< Install message handler. */
    ApplyCallbackFunc ApplyCallback; /**< Apply message handler. */
    CancelCallbackFunc CancelCallback; /**< Cancel message handler. */

    IsInstalledCallbackFunc IsInstalledCallback; /**< IsInstalled function pointer */

    SandboxCreateCallbackFunc SandboxCreateCallback; /**< Sandbox creation message handler. */
    SandboxDestroyCallbackFunc SandboxDestroyCallback; /**< Sandbox destruction message handler. */

    PrepareCallbackFunc PrepareCallback; /**< Prepare message handler. */

    DoWorkCallbackFunc DoWorkCallback; /**< DoWork function. */

    ADUC_Token Token; /**< Opaque token, passed to callbacks. */
} ADUC_RegisterData;

//
// Exported methods.
//

/**
 * @brief Valid values for ADUC_Result ResultCode for Register.
 *
 * Implementation specific error should be set in ADUC_Result ExtendedResultCode.
 */
typedef enum tagADUC_RegisterResult
{
    ADUC_RegisterResult_Failure = 0, /**< Failed. */

    ADUC_RegisterResult_Success = 1, /**< Succeeded. */
} ADUC_RegisterResult;

/**
 * @brief Register this module for callbacks.
 *
 * @param data Information about this module (e.g. callback methods)
 * @param argc Count of optional arguments.
 * @param argv Initialization arguments, size is argc.
 * @return ADUC_Result Result code.
 */
ADUC_Result ADUC_Register(ADUC_RegisterData* data, unsigned int argc, const char** argv);

/**
 * @brief Unregister the callback module.
 *
 * @param token Opaque token that was set in #ADUC_RegisterData.
 */
void ADUC_Unregister(ADUC_Token token);

/**
 * @brief Reboot the system.
 *
 * @returns int errno, 0 if success.
 */
int ADUC_RebootSystem();

/**
 * @brief Restart the ADU Agent.
 *
 * @returns int errno, 0 if success.
 */
int ADUC_RestartAgent();

#define AducResultCodeIndicatesInProgress(resultCode)                                                \
    ((resultCode) == ADUC_DownloadResult_InProgress || (resultCode) == ADUC_InstallResult_InProgress \
     || (resultCode) == ADUC_ApplyResult_InProgress)

EXTERN_C_END

#endif // ADUC_ADU_CORE_EXPORTS_H
