/**
 * @file agent_workflow.c
 * @brief Handles workflow requests coming in from the hub.
 *
 * The cloud-based orchestrator (CBO) holds the state machine, so the best we can do in this agent
 * is to react to the CBO update actions, and see if we think we're in the correct state.
 * If we are, we'll call an upper-level method to do the work.
 * If not, we'll fail the request.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/agent_workflow.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h> // PRIu64
#include <stdlib.h>

#include <time.h>

#include "aduc/adu_core_export_helpers.h" // ADUC_MethodCall_RestartAgent
#include "aduc/agent_orchestration.h"
#include "aduc/download_handler_factory.h" // ADUC_DownloadHandlerFactory_LoadDownloadHandler
#include "aduc/download_handler_plugin.h" // ADUC_DownloadHandlerPlugin_OnUpdateWorkflowCompleted
#include "aduc/logging.h"
#include "aduc/parser_utils.h" // ADUC_FileEntity_Uninit
#include "aduc/result.h"
#include "aduc/string_c_utils.h"
#include "aduc/system_utils.h"
#include "aduc/types/workflow.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_utils.h"

#include <pthread.h>

// fwd decl
void ADUC_Workflow_WorkCompletionCallback(const void* workCompletionToken, ADUC_Result result, bool isAsync);

// This lock is used for critical sections where main and worker thread could read/write to ADUC_workflowData
// It is used only at the top-level coarse granularity operations:
//     * (main thread) ADUC_Workflow_HandlePropertyUpdate
//     * (main thread and worker thread) ADUC_Workflow_WorkCompletionCallback
//         - when asynchronously called (worker thread) it takes the lock
static pthread_mutex_t s_workflow_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void s_workflow_lock(void)
{
    pthread_mutex_lock(&s_workflow_mutex);
}

static inline void s_workflow_unlock(void)
{
    pthread_mutex_unlock(&s_workflow_mutex);
}

static const char* ADUC_Workflow_CancellationTypeToString(ADUC_WorkflowCancellationType cancellationType)
{
    switch (cancellationType)
    {
    case ADUC_WorkflowCancellationType_None:
        return "None";
    case ADUC_WorkflowCancellationType_Normal:
        return "Normal";
    case ADUC_WorkflowCancellationType_Replacement:
        return "Replacement";
    case ADUC_WorkflowCancellationType_Retry:
        return "Retry";
    case ADUC_WorkflowCancellationType_ComponentChanged:
        return "ComponentChanged";
    }

    return "<Unknown>";
}

/**
 * @brief Convert WorkflowStep to string representation.
 *
 * @param workflowStep WorkflowStep to convert.
 * @return const char* String representation.
 */
static const char* ADUCITF_WorkflowStepToString(ADUCITF_WorkflowStep workflowStep)
{
    switch (workflowStep)
    {
    case ADUCITF_WorkflowStep_ProcessDeployment:
        return "ProcessDeployment";
    case ADUCITF_WorkflowStep_Download:
        return "Download";
    case ADUCITF_WorkflowStep_Backup:
        return "Backup";
    case ADUCITF_WorkflowStep_Install:
        return "Install";
    case ADUCITF_WorkflowStep_Apply:
        return "Apply";
    case ADUCITF_WorkflowStep_Restore:
        return "Restore";
    case ADUCITF_WorkflowStep_Undefined:
        return "Undefined";
    }

    return "<Unknown>";
}

/**
 * @brief Cleans up previously created sandboxes, excluding the current workflowId.
 *
 * @param context The context that is workflow data.
 * @param baseDir The base of the workflowId work folders, e.g. /var/lib/adu/downloads
 * @param workflowId The workflow id.
 */
static void CleanupSandbox(void* context, const char* baseDir, const char* workflowId)
{
    Log_Debug("begin cleanup for wf %s under %s", workflowId, baseDir);

    ADUC_WorkflowData* workflowData = (ADUC_WorkflowData*)context;
    if (!IsNullOrEmpty(baseDir) && !IsNullOrEmpty(workflowId) && workflowData != NULL)
    {
        char* workDirPath = ADUC_StringFormat("%s/%s", baseDir, workflowId);
        if (workDirPath == NULL)
        {
            Log_Error("workDirPath failed.");
        }
        else
        {
            const ADUC_UpdateActionCallbacks* updateActionCallbacks = &(workflowData->UpdateActionCallbacks);

            updateActionCallbacks->SandboxDestroyCallback(
                updateActionCallbacks->PlatformLayerHandle, workflowId, workDirPath);

            free(workDirPath);
        }
    }
    Log_Debug("end cleanup for wf %s under %s", workflowId, baseDir);
}

/**
 * @brief Cleans up previously created sandboxes, excluding the current workflowId.
 *
 * @param workflowData The workflow data.
 */
static void Cleanup_Previous_Sandboxes(ADUC_WorkflowData* workflowData)
{
    const char* current_workflowId = workflow_peek_id(workflowData->WorkflowHandle);
    char* workFolder = workflow_get_workfolder(workflowData->WorkflowHandle);
    int err = 0;

    ADUC_SystemUtils_ForEachDirFunctor functor = { .context = workflowData, .callbackFn = CleanupSandbox };

    Log_Debug("begin clean previous sandboxes");

    if (IsNullOrEmpty(workFolder))
    {
        Log_Error("Failed getting workFolder.");
        goto done;
    }

    // remove the "/<workflowId>" suffix because we want to remove other workflowId dirs
    char* lastSlash = strrchr(workFolder, '/');
    if (lastSlash == NULL)
    {
        err = -1;
        goto done;
    }

    *lastSlash = '\0';

    if (!SystemUtils_IsDir(workFolder, &err) || err != 0)
    {
        Log_Error("%s is not a dir", workFolder);
        goto done;
    }

    Log_Debug("Cleaning dirs under %s except %s", workFolder, current_workflowId);
    err = SystemUtils_ForEachDir(
        workFolder /* baseDir */, current_workflowId /* excludedDir */, &functor /* perDirActionFunctor */);
    if (err != 0)
    {
        Log_Error("foreach CleanupSandbox failed with: %d", err);
        goto done;
    }

done:
    workflow_free_string(workFolder);

    Log_Debug("end clean previous sandboxes");
}

/**
 * @brief Signature of method to perform an update action.
 */
typedef ADUC_Result (*ADUC_Workflow_OperationFunc)(ADUC_MethodCall_Data* methodCallData);

/**
 * @brief Signature of method called when OperationFunc completes synch, or after it calls completion callback.
 */
typedef void (*ADUC_Workflow_OperationCompleteFunc)(ADUC_MethodCall_Data* methodCallData, ADUC_Result result);

/**
 * @brief Map from a workflow step to a method that performs that step of the workflow, and the UpdateState to transition to
 * if that method is successful.
 */
// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
typedef struct tagADUC_WorkflowHandlerMapEntry
{
    const ADUCITF_WorkflowStep WorkflowStep; /**< Requested workflow step. */

    const ADUC_Workflow_OperationFunc OperationFunc; /**< Calls upper-level operation */

    const ADUC_Workflow_OperationCompleteFunc OperationCompleteFunc;

    const ADUCITF_State NextStateOnSuccess; /**< State to transition to on successful operation */

    /**< The next workflow step input to transition workflow after transitioning to above NextStateOnSuccess when current
     *     workflow step is above WorkflowStep. Using ADUCITF_WorkflowStep_Undefined means it ends the workflow.
     */
    const ADUCITF_WorkflowStep AutoTransitionWorkflowStepOnSuccess;

    const ADUCITF_State NextStateOnFailure; /**< State to transition to on failed operation */

    /**< The next workflow step input to transition workflow after transitioning to above NextStateOnFailure when current
     *     workflow step is above WorkflowStep. Using ADUCITF_WorkflowStep_Undefined means it ends the workflow.
     */
    const ADUCITF_WorkflowStep AutoTransitionWorkflowStepOnFailure;
} ADUC_WorkflowHandlerMapEntry;

// clang-format off
/**
 * @brief Workflow action table.
 *
 * Algorithm:
 *
 * - Find the Action (e.g. download) in the map.
 * - Call the OperationFunc, passing ADUC_WorkflowData and ADUC_MethodCall_Data objects.
 * - If OperationFunc is complete, i.e. result code NOT AducResultCodeIndicatesInProgress(result.ResultCode)
 *     OR IsAducResultCodeFailure(result.ResultCode), then call OperationCompleteFunc.
 * - Otherwise, assume an async operation is in progress. Set OperationInProgress to true.
 *     OperationFunc will call back asynchronously via WorkCompletionCallback when work is complete.
 * - OperationFunc and WorkCompletionCallback will both move to the NextState on success.
 * - After transition to the next state, then it will auto-transition to the next step
 *     of the workflow specified by NextWorkflowStepAfterNextState, but only if
 *     AutoTransitionApplicableUpdateAction is equal to the current update action of the workflow data.
 */
const ADUC_WorkflowHandlerMapEntry workflowHandlerMap[] = {
    { ADUCITF_WorkflowStep_ProcessDeployment,
        /* calls operation */                               ADUC_Workflow_MethodCall_ProcessDeployment,
        /* and on completion calls */                       ADUC_Workflow_MethodCall_ProcessDeployment_Complete,
        /* on success, transitions to state */              ADUCITF_State_DeploymentInProgress,
        /* on success auto-transitions to workflow step */  ADUCITF_WorkflowStep_Download,
        /* on failure, transitions to state */              ADUCITF_State_Failed,
        /* on failure auto-transitions to workflow step */  ADUCITF_WorkflowStep_Undefined,
    },

    { ADUCITF_WorkflowStep_Download,
        /* calls operation */                               ADUC_Workflow_MethodCall_Download,
        /* and on completion calls */                       ADUC_Workflow_MethodCall_Download_Complete,
        /* on success, transitions to state */              ADUCITF_State_DownloadSucceeded,
        /* on success auto-transitions to workflow step */  ADUCITF_WorkflowStep_Backup,
        /* on failure, transitions to state */              ADUCITF_State_Failed,
        /* on failure auto-transitions to workflow step */  ADUCITF_WorkflowStep_Undefined,
    },

    { ADUCITF_WorkflowStep_Backup,
        /* calls operation */                               ADUC_Workflow_MethodCall_Backup,
        /* and on completion calls */                       ADUC_Workflow_MethodCall_Backup_Complete,
        /* on success, transitions to state */              ADUCITF_State_BackupSucceeded,
        /* on success auto-transitions to workflow step */  ADUCITF_WorkflowStep_Install,
        /* Note: The default behavior of backup is that if Backup fails,
        the workflow will end and report failure immediately.
        To opt out of this design, in the content handler, the owner of the content handler
        will need to persist the result of ADUC_Workflow_MethodCall_Backup and return
        ADUC_Result_Backup_Success to let the workflow continue. */
        /* on failure, transitions to state */              ADUCITF_State_Failed,
        /* on failure auto-transitions to workflow step */  ADUCITF_WorkflowStep_Undefined,
    },

    { ADUCITF_WorkflowStep_Install,
        /* calls operation */                               ADUC_Workflow_MethodCall_Install,
        /* and on completion calls */                       ADUC_Workflow_MethodCall_Install_Complete,
        /* on success, transitions to state */              ADUCITF_State_InstallSucceeded,
        /* on success auto-transitions to workflow step */  ADUCITF_WorkflowStep_Apply,
        /* on failure, transitions to state */              ADUCITF_State_Failed,
        /* on failure auto-transitions to workflow step */  ADUCITF_WorkflowStep_Restore,
    },

    // Note: There's no "ApplySucceeded" state.  On success, we should return to Idle state.
    { ADUCITF_WorkflowStep_Apply,
        /* calls operation */                               ADUC_Workflow_MethodCall_Apply,
        /* and on completion calls */                       ADUC_Workflow_MethodCall_Apply_Complete,
        /* on success, transition to state */               ADUCITF_State_Idle,
        /* on success auto-transitions to workflow step */  ADUCITF_WorkflowStep_Undefined, // Undefined means end of workflow
        /* on failure, transitions to state */              ADUCITF_State_Failed,
        /* on failure auto-transitions to workflow step */  ADUCITF_WorkflowStep_Restore,
    },

    { ADUCITF_WorkflowStep_Restore,
        /* calls operation */                               ADUC_Workflow_MethodCall_Restore,
        /* and on completion calls */                       ADUC_Workflow_MethodCall_Restore_Complete,
        /* on success, transition to state */               ADUCITF_State_Idle,
        /* on success auto-transitions to workflow step */  ADUCITF_WorkflowStep_Undefined, // Undefined means end of workflow
        /* on failure, transitions to state */              ADUCITF_State_Failed,
        /* on failure auto-transitions to workflow step */  ADUCITF_WorkflowStep_Undefined,
    },
};

// clang-format on

/**
 * @brief Get the Workflow Handler Map Entry for a workflow step
 *
 * @param workflowStep The workflow step to find.
 * @return ADUC_WorkflowHandlerMapEntry* NULL if @p workflowStep not found.
 */
const ADUC_WorkflowHandlerMapEntry* GetWorkflowHandlerMapEntryForAction(ADUCITF_WorkflowStep workflowStep)
{
    const ADUC_WorkflowHandlerMapEntry* entry;
    const unsigned int map_count = ARRAY_SIZE(workflowHandlerMap);
    unsigned index;
    for (index = 0; index < map_count; ++index)
    {
        entry = workflowHandlerMap + index;
        if (entry->WorkflowStep == workflowStep)
        {
            break;
        }
    }
    if (index == map_count)
    {
        entry = NULL;
    }

    return entry;
}

/**
 * @brief Called regularly to allow for cooperative multitasking during work.
 *
 * @param workflowData Workflow metadata.
 */
void ADUC_Workflow_DoWork(ADUC_WorkflowData* workflowData)
{
    // As this method will be called many times, rather than call into adu_core_export_helpers to call into upper-layer,
    // just call directly into upper-layer here.
    const ADUC_UpdateActionCallbacks* updateActionCallbacks = &(workflowData->UpdateActionCallbacks);

    updateActionCallbacks->DoWorkCallback(updateActionCallbacks->PlatformLayerHandle, workflowData);
}

void ADUC_Workflow_HandleStartupWorkflowData(ADUC_WorkflowData* currentWorkflowData)
{
    if (currentWorkflowData == NULL)
    {
        Log_Info("No update content. Ignoring.");
        return;
    }

    if (currentWorkflowData->StartupIdleCallSent)
    {
        Log_Debug("StartupIdleCallSent true. Skipping.");
        return;
    }

    Log_Info("Perform startup tasks.");

    // NOTE: WorkflowHandle can be NULL when device first connected to the hub (no desired property).
    if (currentWorkflowData->WorkflowHandle == NULL)
    {
        Log_Info("There's no update actions in current workflow (first time connected to IoT Hub).");
    }
    else
    {
        // The default result for Idle state.
        // This will reset twin status code to 200 to indicate that we're successful (so far).
        const ADUC_Result result = { .ResultCode = ADUC_Result_Idle_Success };

        int desiredAction = workflow_get_action(currentWorkflowData->WorkflowHandle);
        if (desiredAction == ADUCITF_UpdateAction_Undefined)
        {
            goto done;
        }

        if (desiredAction == ADUCITF_UpdateAction_Cancel)
        {
            Log_Info("Received 'cancel' action on startup, reporting Idle state.");

            ADUC_WorkflowData_SetCurrentAction(desiredAction, currentWorkflowData);

            ADUC_Workflow_SetUpdateStateWithResult(currentWorkflowData, ADUCITF_State_Idle, result);

            goto done;
        }
        else if (desiredAction == ADUCITF_UpdateAction_ProcessDeployment)
        {
            ADUC_Result isInstalledResult = ADUC_Workflow_MethodCall_IsInstalled(currentWorkflowData);
            if (isInstalledResult.ResultCode == ADUC_Result_IsInstalled_Installed)
            {
                char* updateId = workflow_get_expected_update_id_string(currentWorkflowData->WorkflowHandle);
                ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle(currentWorkflowData, updateId);
                free(updateId);
                goto done;
            }
        }

        Log_Info("There's a pending '%s' action", ADUCITF_UpdateActionToString(desiredAction));
    }

    // There's a pending ProcessDeployment action in the twin.
    // We need to make sure we don't report an 'idle' state, if we can resume or retry the action.
    // In this case, we will set last reportedState to 'idle', so that we can continue.
    ADUC_WorkflowData_SetLastReportedState(ADUCITF_State_Idle, currentWorkflowData);

    ADUC_Workflow_HandleUpdateAction(currentWorkflowData);

done:

    // Once we set Idle state to the orchestrator we can start receiving update actions.
    currentWorkflowData->StartupIdleCallSent = true;
}

/**
 * @brief Handles updates to a 1 or more PnP Properties in the ADU Core interface.
 *
 * @param[in,out] currentWorkflowData The current ADUC_WorkflowData object.
 * @param[in] propertyUpdateValue The updated property value.
 * @param[in] forceUpdate Ensures that specifed @p propertyUpdateValue will be processed by force deferral if there is ongoing workflow processing.
 */
void ADUC_Workflow_HandlePropertyUpdate(
    ADUC_WorkflowData* currentWorkflowData, const unsigned char* propertyUpdateValue, bool forceUpdate)
{
    ADUC_WorkflowHandle nextWorkflow;

    ADUC_Result result = workflow_init((const char*)propertyUpdateValue, true /* shouldValidate */, &nextWorkflow);

    workflow_set_force_update(nextWorkflow, forceUpdate);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("Invalid desired update action data. Update data: (%s)", propertyUpdateValue);

        ADUC_Workflow_SetUpdateStateWithResult(currentWorkflowData, ADUCITF_State_Failed, result);
        return;
    }

    ADUCITF_UpdateAction nextUpdateAction = workflow_get_action(nextWorkflow);

    //
    // Take lock until goto done.
    //
    // N.B.
    // Lock must *NOT* be taken in HandleStartupWorkflowData and HandleUpdateAction or any functions they call
    //
    s_workflow_lock();

    if (currentWorkflowData->WorkflowHandle != NULL)
    {
        if (nextUpdateAction == ADUCITF_UpdateAction_Cancel)
        {
            ADUC_WorkflowCancellationType currentCancellationType =
                workflow_get_cancellation_type(currentWorkflowData->WorkflowHandle);
            if (currentCancellationType == ADUC_WorkflowCancellationType_None)
            {
                workflow_set_cancellation_type(
                    currentWorkflowData->WorkflowHandle, ADUC_WorkflowCancellationType_Normal);

                // call into handle update action for cancellation logic to invoke ADUC_Workflow_MethodCall_Cancel
                ADUC_Workflow_HandleUpdateAction(currentWorkflowData);

                goto done;
            }
            else
            {
                Log_Info(
                    "Ignoring duplicate '%s' action. Current cancellation type is already '%s'.",
                    ADUCITF_UpdateActionToString(nextUpdateAction),
                    ADUC_Workflow_CancellationTypeToString(currentCancellationType));
                goto done;
            }
        }
        else if (nextUpdateAction == ADUCITF_UpdateAction_ProcessDeployment)
        {
            if (!forceUpdate && workflow_id_compare(currentWorkflowData->WorkflowHandle, nextWorkflow) == 0)
            {
                // Possible retry of the current workflow.
                const char* currentRetryToken = workflow_peek_retryTimestamp(currentWorkflowData->WorkflowHandle);
                const char* newRetryToken = workflow_peek_retryTimestamp(nextWorkflow);

                if (!AgentOrchestration_IsRetryApplicable(currentRetryToken, newRetryToken))
                {
                    Log_Warn(
                        "Ignoring Retry. currentRetryToken '%s', nextRetryToken '%s'.",
                        newRetryToken ? newRetryToken : "(NULL)",
                        currentRetryToken ? currentRetryToken : "(NULL)");
                    goto done;
                }

                Log_Debug("Retry %s is applicable", newRetryToken);

                // Sets both cancellation type to Retry and updates the current retry token
                workflow_update_retry_deployment(currentWorkflowData->WorkflowHandle, newRetryToken);

                // call into handle update action for cancellation logic to invoke ADUC_Workflow_MethodCall_Cancel
                ADUC_Workflow_HandleUpdateAction(currentWorkflowData);
                goto done;
            }
            else
            {
                // Possible replacement with a new workflow.
                ADUCITF_State currentState = ADUC_WorkflowData_GetLastReportedState(currentWorkflowData);
                ADUCITF_WorkflowStep currentWorkflowStep =
                    workflow_get_current_workflowstep(currentWorkflowData->WorkflowHandle);

                if (currentState != ADUCITF_State_Idle && currentState != ADUCITF_State_Failed
                    && currentWorkflowStep != ADUCITF_WorkflowStep_Undefined)
                {
                    Log_Info(
                        "Replacement. workflow '%s' is being replaced with workflow '%s'.",
                        workflow_peek_id(currentWorkflowData->WorkflowHandle),
                        workflow_peek_id(nextWorkflow));

                    // If operation is in progress, then in the same critical section we set cancellation type to replacement
                    // and set the pending workflow on the handle for use by WorkCompletionCallback to continue on with the
                    // replacement deployment instead of going to idle and reporting the results as a cancel failure.
                    // Otherwise, if the operation is not in progress, in the same critical section it transfers the
                    // workflow handle of the new deployment into the current workflow data, so that we can handle the update action.
                    bool deferredReplacement =
                        workflow_update_replacement_deployment(currentWorkflowData->WorkflowHandle, nextWorkflow);

                    if (deferredReplacement)
                    {
                        Log_Info(
                            "Deferred Replacement workflow id [%s] since current workflow id [%s] was still in progress.",
                            workflow_peek_id(nextWorkflow),
                            workflow_peek_id(currentWorkflowData->WorkflowHandle));

                        // Ownership was transferred to current workflow so ensure it doesn't get freed.
                        nextWorkflow = NULL;

                        // call into handle update action for cancellation logic to invoke ADUC_Workflow_MethodCall_Cancel
                        ADUC_Workflow_HandleUpdateAction(currentWorkflowData);
                        goto done;
                    }

                    Log_Debug("deferral not needed. Processing '%s' now", workflow_peek_id(nextWorkflow));

                    workflow_transfer_data(
                        currentWorkflowData->WorkflowHandle /* wfTarget */, nextWorkflow /* wfSource */);

                    ADUC_Workflow_HandleUpdateAction(currentWorkflowData);
                    goto done;
                }

                // Fall through to handle new workflow
            }
        }
    }
    else
    {
        // This is a top level workflow, make sure that we set the working folder correctly.
        workflow_set_workfolder(nextWorkflow, "%s/%s", ADUC_DOWNLOADS_FOLDER, workflow_peek_id(nextWorkflow));
    }

    // Continue with the new workflow.
    workflow_free(currentWorkflowData->WorkflowHandle);
    currentWorkflowData->WorkflowHandle = nextWorkflow;

    nextWorkflow = NULL;

    workflow_set_cancellation_type(
        currentWorkflowData->WorkflowHandle,
        nextUpdateAction == ADUCITF_UpdateAction_Cancel ? ADUC_WorkflowCancellationType_Normal
                                                        : ADUC_WorkflowCancellationType_None);

    // If the agent has just started up but we have yet to report the installedUpdateId along with a state of 'Idle'
    // we want to ignore any further action received so we don't confuse the workflow which would interpret a state of 'Idle'
    // not accompanied with an installedUpdateId as a failed end state in some cases.
    // In this case we will go through our startup logic which would report the installedUpdateId with a state of 'Idle',
    // if we can determine that the update has been installed successfully (by calling IsInstalled()).
    // Otherwise we will honor and process the action requested.
    if (!currentWorkflowData->StartupIdleCallSent)
    {
        ADUC_Workflow_HandleStartupWorkflowData(currentWorkflowData);
    }
    else
    {
        ADUC_Workflow_HandleUpdateAction(currentWorkflowData);
    }

done:

    s_workflow_unlock();

    workflow_free(nextWorkflow);
    Log_Debug("PropertyUpdated event handler completed.");
}

/**
 * @brief Handle an incoming update action.
 * @remark Caller *must* be in a lock before calling
 *
 * @param workflowData Workflow metadata.
 */
void ADUC_Workflow_HandleUpdateAction(ADUC_WorkflowData* workflowData)
{
    unsigned int desiredAction = workflow_get_action(workflowData->WorkflowHandle);

    // Special case: Cancel is handled here.
    //
    // If Cancel action is received while another ProcessDeployment update action is in progress then the agent
    // should cancel the in progress action and the agent should set Idle state.
    //
    // If an operation completes with a failed state, the error should be reported to the service, and the agent
    // should set Failed state.  The CBO once it receives the Failed state will NOT send the agent a Cancel action.
    // Agent orchestration will auto-transition to Idle from Failed state.
    //
    //  Cancel should only be sent from the CBO when:
    // * An operation is in progress, to cancel the operation.
    // * After an operation fails to return the agent back to Idle state.
    // * A rollout end time has passed & the device has been offline and did not receive the previous command.
    //

    ADUC_WorkflowCancellationType cancellationType = workflow_get_cancellation_type(workflowData->WorkflowHandle);
    Log_Debug(
        "cancellationType(%d) => %s", cancellationType, ADUC_Workflow_CancellationTypeToString(cancellationType));

    bool isReplaceOrRetry = (cancellationType == ADUC_WorkflowCancellationType_Replacement)
        || (cancellationType == ADUC_WorkflowCancellationType_Retry);

    if (desiredAction == ADUCITF_UpdateAction_Cancel || cancellationType == ADUC_WorkflowCancellationType_Normal
        || ((desiredAction == ADUCITF_UpdateAction_ProcessDeployment) && isReplaceOrRetry))
    {
        if (workflow_get_operation_in_progress(workflowData->WorkflowHandle))
        {
            Log_Info(
                "Canceling request for in-progress operation. desiredAction: %s, cancellationType: %s",
                ADUCITF_UpdateActionToString(desiredAction),
                ADUC_Workflow_CancellationTypeToString(cancellationType));

            // This sets a marker that cancellation has been requested.
            workflow_set_operation_cancel_requested(workflowData->WorkflowHandle, true);

            // Call upper-layer to notify of cancel
            ADUC_Workflow_MethodCall_Cancel(workflowData);
            goto done;
        }
        else if (
            desiredAction == ADUCITF_UpdateAction_Cancel || cancellationType == ADUC_WorkflowCancellationType_Normal)
        {
            // Cancel without an operation in progress means return to Idle state.
            workflow_set_operation_cancel_requested(workflowData->WorkflowHandle, false);
            workflow_set_cancellation_type(workflowData->WorkflowHandle, ADUC_WorkflowCancellationType_None);

            Log_Info("Cancel received with no operation in progress - returning to Idle state");
            goto done;
        }
        else
        {
            workflow_set_operation_cancel_requested(workflowData->WorkflowHandle, false);
            workflow_set_cancellation_type(workflowData->WorkflowHandle, ADUC_WorkflowCancellationType_None);

            Log_Info("Replace/Retry when operation not in progress. Try to process workflow...");
            // Continue processing workflow below.
        }
    }

    // Ignore duplicate deployment that can be caused by token expiry connection refresh after about 40 minutes.
    if (workflow_isequal_id(workflowData->WorkflowHandle, workflowData->LastCompletedWorkflowId)
        && !workflow_get_force_update(workflowData->WorkflowHandle))
    {
        Log_Debug("Ignoring duplicate deployment %s, action %d", workflowData->LastCompletedWorkflowId, desiredAction);
        goto done;
    }

    //
    // Save the original action to the workflow data
    //
    ADUC_WorkflowData_SetCurrentAction(desiredAction, workflowData);

    //
    // Check if installed already
    // Note, must be done after setting current action for proper reporting.
    //
    ADUC_Result isInstalledResult = ADUC_Workflow_MethodCall_IsInstalled(workflowData);
    if (isInstalledResult.ResultCode == ADUC_Result_IsInstalled_Installed)
    {
        char* updateId = workflow_get_expected_update_id_string(workflowData->WorkflowHandle);
        ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle(workflowData, updateId);
        free(updateId);
        goto done;
    }

    //
    // Determine the current workflow step
    //
    ADUCITF_WorkflowStep nextStep = AgentOrchestration_GetWorkflowStep(desiredAction);
    workflow_set_current_workflowstep(workflowData->WorkflowHandle, nextStep);

    //
    // Cleanup any sandboxes other than the current workflowId.
    // Previous failed install/apply do not cleanup the sandbox to avoid
    // redownload of payloads when "retry failed" is issued by service for the
    // same workflowId.
    //
    // Do not cleanup the current workflowId sandbox because it might need a
    // payload to be able to evaluate IsInstalled and it may be there
    // already due to a reboot/restart after Apply or if something else caused
    // agent to restart.
    //
    if (nextStep == ADUCITF_WorkflowStep_ProcessDeployment)
    {
        Cleanup_Previous_Sandboxes(workflowData);
    }

    //
    // Transition to the next phase for this workflow
    //
    ADUC_Workflow_TransitionWorkflow(workflowData);

done:

    return;
}

/**
 * @brief Looks up the current workflow step in the state transition table and invokes a step transition if the workflow is not complete.
 * @remark This is called by worker thread at the end of work completion processing.
 *         It must be in a lock before calling this.
 *
 * @param workflowData The global context workflow data structure.
 * @param onSuccess Indicate whether it is a transition on success or on failure.
 */
void ADUC_Workflow_AutoTransitionWorkflow(ADUC_WorkflowData* workflowData, bool onSuccess)
{
    //
    // If the workflow's not complete, then auto-transition to the next step/phase of the workflow.
    // For example, Download just completed, so it should auto-transition with workflow step input of WorkflowStep_Install,
    // which will kick off the install operation.
    // Once that's kicked off, this thread will exit if the operation is async.
    //
    ADUCITF_WorkflowStep currentWorkflowStep = workflow_get_current_workflowstep(workflowData->WorkflowHandle);

    const ADUC_WorkflowHandlerMapEntry* postCompleteEntry = GetWorkflowHandlerMapEntryForAction(currentWorkflowStep);
    if (postCompleteEntry == NULL)
    {
        Log_Error("Invalid workflow step %u", currentWorkflowStep);
        return;
    }

    if (!onSuccess)
    {
        if (AgentOrchestration_IsWorkflowComplete(postCompleteEntry->AutoTransitionWorkflowStepOnFailure))
        {
            Log_Info("Workflow is Complete.");
        }
        else
        {
            workflow_set_current_workflowstep(
                workflowData->WorkflowHandle, postCompleteEntry->AutoTransitionWorkflowStepOnFailure);

            Log_Info(
                "workflow is not completed. AutoTransition to step: %s",
                ADUCITF_WorkflowStepToString(postCompleteEntry->AutoTransitionWorkflowStepOnFailure));

            ADUC_Workflow_TransitionWorkflow(workflowData);
        }
    }

    else
    {
        if (AgentOrchestration_IsWorkflowComplete(postCompleteEntry->AutoTransitionWorkflowStepOnSuccess))
        {
            Log_Info("Workflow is Complete.");
        }
        else
        {
            workflow_set_current_workflowstep(
                workflowData->WorkflowHandle, postCompleteEntry->AutoTransitionWorkflowStepOnSuccess);

            Log_Info(
                "workflow is not completed. AutoTransition to step: %s",
                ADUCITF_WorkflowStepToString(postCompleteEntry->AutoTransitionWorkflowStepOnSuccess));

            ADUC_Workflow_TransitionWorkflow(workflowData);
        }
    }
}

/**
 * @brief Transitions the workflow to the next workflow step, e.g. Download to Install, Install to Apply, etc.
 * @remark Must be in a lock
 *
 * @param workflowData The global context workflow data structure.
 */
void ADUC_Workflow_TransitionWorkflow(ADUC_WorkflowData* workflowData)
{
    ADUC_Result result;
    ADUCITF_WorkflowStep currentWorkflowStep = workflow_get_current_workflowstep(workflowData->WorkflowHandle);

    const ADUC_WorkflowHandlerMapEntry* entry = GetWorkflowHandlerMapEntryForAction(currentWorkflowStep);
    if (entry == NULL)
    {
        Log_Error("Invalid workflow step %u -- ignoring", currentWorkflowStep);
        goto done;
    }

    Log_Debug("Processing '%s' step", ADUCITF_WorkflowStepToString(entry->WorkflowStep));

    // Alloc this object on heap so that it will be valid for the entire (possibly async) operation func.
    ADUC_MethodCall_Data* methodCallData = calloc(1, sizeof(ADUC_MethodCall_Data));
    if (methodCallData == NULL)
    {
        goto done;
    }

    methodCallData->WorkflowData = workflowData;

    // workCompletionData is sent to the upper-layer which will pass the WorkCompletionToken back
    // when it makes the async work complete call.
    methodCallData->WorkCompletionData.WorkCompletionCallback = ADUC_Workflow_WorkCompletionCallback;
    methodCallData->WorkCompletionData.WorkCompletionToken = methodCallData;

    // Call into the upper-layer method to perform operation.
    Log_Debug("Setting operation_in_progress => true");
    workflow_set_operation_in_progress(workflowData->WorkflowHandle, true);

    // Perform an update operation.
    result = entry->OperationFunc(methodCallData);

    // Action is complete (i.e. we wont get a WorkCompletionCallback call from upper-layer) if:
    // * Upper-level did the work in a blocking manner.
    // * Method returned failure.
    // NOLINTNEXTLINE(misc-redundant-expression)
    if (!AducResultCodeIndicatesInProgress(result.ResultCode) || IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Debug("The synchronous operation is complete.");
        ADUC_Workflow_WorkCompletionCallback(methodCallData, result, false /* isAsync */);
    }

done:
    return;
}

/**
 * @brief Called when work is complete.
 *
 * @param workCompletionToken ADUC_MethodCall_Data pointer.
 * @param result Result of work.
 * @param result isAsync true if caller is on worker thread, false if from main thread.
 */
void ADUC_Workflow_WorkCompletionCallback(const void* workCompletionToken, ADUC_Result result, bool isAsync)
{
    // We own these objects, so no issue making them non-const.
    ADUC_MethodCall_Data* methodCallData = (ADUC_MethodCall_Data*)workCompletionToken;
    ADUC_WorkflowData* workflowData = methodCallData->WorkflowData;

    // NOLINTNEXTLINE(misc-redundant-expression)
    if (AducResultCodeIndicatesInProgress(result.ResultCode))
    {
        Log_Error("WorkComplete received InProgress result code - should not happen!");
        goto done;
    }

    // Need to avoid deadlock b/c main thread typically takes lock higher in the callstack above
    // TransistionWorkflow and processing DeploymentInProgress state is synchronous
    if (isAsync)
    {
        s_workflow_lock();
    }

    ADUCITF_WorkflowStep currentWorkflowStep = workflow_get_current_workflowstep(workflowData->WorkflowHandle);

    const ADUC_WorkflowHandlerMapEntry* entry = GetWorkflowHandlerMapEntryForAction(currentWorkflowStep);
    if (entry == NULL)
    {
        Log_Error("Invalid UpdateAction %u -- ignoring", currentWorkflowStep);
        goto done;
    }

    if (ADUC_WorkflowData_GetCurrentAction(workflowData) == ADUCITF_UpdateAction_Cancel)
    {
        Log_Error("workflow data current action should not be Cancel.");
        goto done;
    }

    Log_Info(
        "Action '%s' complete. Result: %d (%s), %d (0x%x)",
        ADUCITF_WorkflowStepToString(entry->WorkflowStep),
        result.ResultCode,
        result.ResultCode == 0 ? "failed" : "succeeded",
        result.ExtendedResultCode,
        result.ExtendedResultCode);

    entry->OperationCompleteFunc(methodCallData, result);

    if (IsAducResultCodeSuccess(result.ResultCode))
    {
        // Operation succeeded -- go to next state.

        const ADUCITF_State nextUpdateStateOnSuccess = entry->NextStateOnSuccess;

        Log_Info(
            "WorkCompletionCallback: %s succeeded. Going to state %s",
            ADUCITF_WorkflowStepToString(entry->WorkflowStep),
            ADUCITF_StateToString(nextUpdateStateOnSuccess));

        ADUC_Workflow_SetUpdateState(workflowData, nextUpdateStateOnSuccess);

        // Transitioning to idle (or failed) state frees and nulls-out the WorkflowHandle as a side-effect of
        // setting the update state.
        if (ADUC_WorkflowData_GetLastReportedState(workflowData) != ADUCITF_State_Idle)
        {
            // Operation is now complete. Clear both inprogress and cancel requested.
            workflow_clear_inprogress_and_cancelrequested(workflowData->WorkflowHandle);

            //
            // We are now ready to transition to the next step of the workflow.
            //
            ADUC_Workflow_AutoTransitionWorkflow(workflowData, true);
            goto done;
        }
    }
    else
    {
        // Operation (e.g. Download) failed or was cancelled - both are considered AducResult failure codes.

        if (workflow_get_operation_cancel_requested(workflowData->WorkflowHandle))
        {
            ADUC_WorkflowCancellationType cancellationType =
                workflow_get_cancellation_type(workflowData->WorkflowHandle);
            const char* cancellationTypeStr = ADUC_Workflow_CancellationTypeToString(cancellationType);

            Log_Warn("Handling cancel completion, cancellation type '%s'.", cancellationTypeStr);

            if (cancellationType == ADUC_WorkflowCancellationType_Replacement
                || cancellationType == ADUC_WorkflowCancellationType_Retry
                || cancellationType == ADUC_WorkflowCancellationType_ComponentChanged)
            {
                Log_Info("Starting process of deployment for '%s'", cancellationTypeStr);

                // Note: Must NOT call linux platform layer Idle method to reset cancellation request to false in the
                // platform layer because that would destroy and NULL out the WorkflowHandle in the workflowData.

                if (cancellationType == ADUC_WorkflowCancellationType_Replacement)
                {
                    // Cleanup the download sandbox for the current workflowId
                    // since it will not be transitioning to Idle state (where
                    // sandbox cleanup is normally done)

                    char* workflowId = ADUC_WorkflowData_GetWorkflowId(workflowData); // see workflow_free_string below
                    char* workFolder = ADUC_WorkflowData_GetWorkFolder(workflowData); // see workflow_free_string below

                    if (workflowId != NULL && workFolder != NULL)
                    {
                        Log_Info("Cleanup sandbox before replacement workflow");

                        const ADUC_UpdateActionCallbacks* updateActionCallbacks =
                            &(workflowData->UpdateActionCallbacks);

                        updateActionCallbacks->SandboxDestroyCallback(
                            updateActionCallbacks->PlatformLayerHandle, workflowId, workFolder);
                    }

                    workflow_free_string(workflowId);
                    workflow_free_string(workFolder);

                    // Reset workflow state to process deployment and transfer
                    // the deferred workflow to current.
                    workflow_update_for_replacement(workflowData->WorkflowHandle);

                }
                else
                {
                    // it's a retry. Reset workflow state to reprocess deployment.
                    workflow_update_for_retry(workflowData->WorkflowHandle);
                }

                ADUC_WorkflowData_SetLastReportedState(ADUCITF_State_Idle, workflowData);

                // ProcessDeployment's OperationFunc called by TransitionWorkflow is synchronous so it kicks off the
                // download worker thread after reporting DeploymentInProgress ACK for the replacement/retry, so we
                // return instead of goto done to avoid the redundant ADUC_Workflow_AutoTransitionWorkflow call.
                ADUC_Workflow_TransitionWorkflow(workflowData);

                goto done;
            }

            if (cancellationType != ADUC_WorkflowCancellationType_Normal)
            {
                Log_Error("Invalid cancellation Type '%s' when cancel requested.", cancellationTypeStr);
                goto done;
            }

            // Operation cancelled.
            //
            // We are now at the completion of the operation that was cancelled via a Cancel update action
            // and will just return to Idle state.
            //
            // Ignore the result of the operation, which most likely is cancelled, e.g. ADUC_Result_Failure_Cancelled.
            Log_Warn("Operation cancelled - returning to Idle state");

            const ADUC_Result result = { .ResultCode = ADUC_Result_Failure_Cancelled };
            ADUC_Workflow_SetUpdateStateWithResult(workflowData, ADUCITF_State_Idle, result);
        }
        else
        {
            // Operation failed.

            const ADUCITF_State nextUpdateStateOnFailure = entry->NextStateOnFailure;

            Log_Info(
                "WorkCompletionCallback: %s failed. Going to state %s",
                ADUCITF_WorkflowStepToString(entry->WorkflowStep),
                ADUCITF_StateToString(nextUpdateStateOnFailure));

            // Reset so that a Retry/Replacement avoids cancel and instead properly starts processing.
            workflow_set_operation_in_progress(workflowData->WorkflowHandle, false);

            ADUC_Workflow_SetUpdateState(workflowData, nextUpdateStateOnFailure);

            ADUC_Workflow_AutoTransitionWorkflow(workflowData, false);
        }
    }

done:
    // lifetime of methodCallData now ends as the operation work has completed.
    free(methodCallData);

    if (isAsync)
    {
        s_workflow_unlock();
    }
}

static const char* DownloadProgressStateToString(ADUC_DownloadProgressState state)
{
    switch (state)
    {
    case ADUC_DownloadProgressState_NotStarted:
        return "NotStarted";
    case ADUC_DownloadProgressState_InProgress:
        return "InProgress";
    case ADUC_DownloadProgressState_Completed:
        return "Completed";
    case ADUC_DownloadProgressState_Cancelled:
        return "Cancelled";
    case ADUC_DownloadProgressState_Error:
        return "Error";
    }

    return "<Unknown>";
}

//
// Download progress callback
//
void ADUC_Workflow_DefaultDownloadProgressCallback(
    const char* workflowId,
    const char* fileId,
    ADUC_DownloadProgressState state,
    uint64_t bytesTransferred,
    uint64_t bytesTotal)
{
    Log_Info(
        "ProgressCallback: workflowId: %s; Id %s; State: %s; Bytes: %" PRIu64 "/%" PRIu64,
        workflowId,
        fileId,
        DownloadProgressStateToString(state),
        bytesTransferred,
        bytesTotal);
}

/**
 * @brief Move state machine to a new stage.
 *
 * @param[in,out] workflowData Workflow data.
 * @param[in] updateState New update state to transition to.
 * @param[in] result Result to report (optional, can be NULL).
 */
static void ADUC_Workflow_SetUpdateStateHelper(
    ADUC_WorkflowData* workflowData, ADUCITF_State updateState, const ADUC_Result* result)
{
    Log_Info("Setting UpdateState to %s", ADUCITF_StateToString(updateState));
    ADUC_WorkflowHandle workflowHandle = workflowData->WorkflowHandle;

    // If we're transitioning from Apply_Started to Idle, we need to report InstalledUpdateId.
    //  if apply succeeded.
    // This is required by ADU service.
    if (updateState == ADUCITF_State_Idle)
    {
        if (ADUC_WorkflowData_GetLastReportedState(workflowData) == ADUCITF_State_ApplyStarted)
        {
            if (workflowData->SystemRebootState == ADUC_SystemRebootState_None
                && workflowData->AgentRestartState == ADUC_AgentRestartState_None)
            {
                // Apply completed, if no reboot or restart is needed, then report deployment succeeded
                // to the ADU service to complete the update workflow.
                char* updateId = workflow_get_expected_update_id_string(workflowHandle);
                ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle(workflowData, updateId);

                ADUC_WorkflowData_SetLastReportedState(updateState, workflowData);

                workflow_free_string(updateId);
                return;
            }

            if (workflowData->SystemRebootState == ADUC_SystemRebootState_InProgress)
            {
                // Reboot is required, and successfully initiated (device is shutting down and restarting).
                // We want to transition to an Idle state internally, but will not report the state to the ADU service,
                // since the InstallUpdateId will not be accurate until the device rebooted.
                //
                // Note: if we report Idle state and InstallUpdateId doesn't match ExpectedUpdateId,
                // ADU service will consider the update failed.
                ADUC_Workflow_MethodCall_Idle(workflowData);
                return;
            }

            if (workflowData->AgentRestartState == ADUC_AgentRestartState_InProgress)
            {
                // Agent restart is required, and successfully initiated.
                // We want to transition to an Idle state internally, but will not report the state to the ADU service,
                // until the agent restarted.
                //
                // Note: if we report Idle state and InstallUpdateId doesn't match ExpectedUpdateId,
                // ADU service will consider the update failed.
                ADUC_Workflow_MethodCall_Idle(workflowData);
                return;
            }

            // Device failed to reboot, or the agent failed to restart, consider update failed.
            // Fall through to report Idle without InstalledUpdateId.
        }

        if (!workflowData->ReportStateAndResultAsyncCallback(
                (ADUC_WorkflowDataToken)workflowData, updateState, result, NULL /* installedUpdateId */))
        {
            updateState = ADUCITF_State_Failed;
            workflow_set_state(workflowData->WorkflowHandle, ADUCITF_State_Failed);
        }
        else
        {
            ADUC_Workflow_MethodCall_Idle(workflowData);
        }
    }
    else // Not Idle state
    {
        if (!workflowData->ReportStateAndResultAsyncCallback(
                (ADUC_WorkflowDataToken)workflowData, updateState, result, NULL /* installedUpdateId */))
        {
            updateState = ADUCITF_State_Failed;
            workflow_set_state(workflowData->WorkflowHandle, ADUCITF_State_Failed);
        }
        else
        {
            workflow_set_state(workflowData->WorkflowHandle, updateState);
        }
    }

    ADUC_WorkflowData_SetLastReportedState(updateState, workflowData);
}

/**
 * @brief For each update payload that has a DownloadHandlerId, load the handler and call OnUpdateWorkflowCompleted.
 *
 * @param workflowHandle The workflow handle.
 * @details This function will not fail but if a download handler's OnUpdateWorkflowCompleted fails, side effects include logging the error result codes and saving the extended result code that can be reported along with a successful workflow deployment.
 */
static void CallDownloadHandlerOnUpdateWorkflowCompleted(const ADUC_WorkflowHandle workflowHandle)
{
    size_t payloadCount = workflow_get_update_files_count(workflowHandle);
    for (size_t i = 0; i < payloadCount; ++i)
    {
        ADUC_Result result = {};
        ADUC_FileEntity fileEntity;
        memset(&fileEntity, 0, sizeof(fileEntity));
        if (!workflow_get_update_file(workflowHandle, i, &fileEntity))
        {
            continue;
        }

        if (IsNullOrEmpty(fileEntity.DownloadHandlerId))
        {
            ADUC_FileEntity_Uninit(&fileEntity);
            continue;
        }

        // NOTE: do not free the handle as it is owned by the DownloadHandlerFactory.
        DownloadHandlerHandle* handle = ADUC_DownloadHandlerFactory_LoadDownloadHandler(fileEntity.DownloadHandlerId);
        if (handle == NULL)
        {
            Log_Error("Failed to load download handler.");
        }
        else
        {
            result = ADUC_DownloadHandlerPlugin_OnUpdateWorkflowCompleted(handle, workflowHandle);
            if (IsAducResultCodeFailure(result.ResultCode))
            {
                Log_Warn(
                    "OnupdateWorkflowCompleted, result 0x%08x, erc 0x%08x",
                    result.ResultCode,
                    result.ExtendedResultCode);

                workflow_set_success_erc(workflowHandle, result.ExtendedResultCode);
            }
        }

        ADUC_FileEntity_Uninit(&fileEntity);
    }
}

/**
 * @brief Set a new update state.
 *
 * @param[in,out] workflowData Workflow data object.
 * @param[in] updateState New update state.
 */
void ADUC_Workflow_SetUpdateState(ADUC_WorkflowData* workflowData, ADUCITF_State updateState)
{
    ADUC_Workflow_SetUpdateStateHelper(workflowData, updateState, NULL /*result*/);
}

/**
 * @brief Set a new update state and result.
 *
 * @param[in,out] workflowData Workflow data object.
 * @param[in] updateState New update state.
 * @param[in] result Result to report.
 */
void ADUC_Workflow_SetUpdateStateWithResult(
    ADUC_WorkflowData* workflowData, ADUCITF_State updateState, ADUC_Result result)
{
    ADUC_Workflow_SetUpdateStateHelper(workflowData, updateState, &result);
}

/**
 * @brief Sets installedUpdateId to the given update ID and sets state to Idle.
 *
 * @param[in,out] workflowData The workflow data.
 * @param[in] updateId The updateId for the installed content.
 */
void ADUC_Workflow_SetInstalledUpdateIdAndGoToIdle(ADUC_WorkflowData* workflowData, const char* updateId)
{
    ADUC_Result idleResult = { .ResultCode = ADUC_Result_Apply_Success, .ExtendedResultCode = 0 };

    if (!workflowData->ReportStateAndResultAsyncCallback(
            (ADUC_WorkflowDataToken)workflowData, ADUCITF_State_Idle, &idleResult, updateId))
    {
        Log_Error("Failed to report last installed updateId. Going to idle state.");
    }

    ADUC_WorkflowData_SetLastReportedState(ADUCITF_State_Idle, workflowData);

    if (!ADUC_WorkflowData_SetLastCompletedWorkflowId(workflow_peek_id(workflowData->WorkflowHandle), workflowData))
    {
        Log_Error("Failed to set last completed workflow id. Going to idle state.");
    }

    CallDownloadHandlerOnUpdateWorkflowCompleted(workflowData->WorkflowHandle);

    ADUC_Workflow_MethodCall_Idle(workflowData);

    workflowData->SystemRebootState = ADUC_SystemRebootState_None;
    workflowData->AgentRestartState = ADUC_AgentRestartState_None;
}

/**
 * @brief Called when entering Idle state.
 *
 * Idle state is the "ready for new workflow state"
 *
 * @param workflowData Workflow metadata.
 */
void ADUC_Workflow_MethodCall_Idle(ADUC_WorkflowData* workflowData)
{
    const ADUC_UpdateActionCallbacks* updateActionCallbacks = &(workflowData->UpdateActionCallbacks);

    ADUCITF_State lastReportedState = ADUC_WorkflowData_GetLastReportedState(workflowData);

    // Can reach Idle state from ApplyStarted as there isn't an ApplySucceeded state.
    if (lastReportedState != ADUCITF_State_Idle && lastReportedState != ADUCITF_State_ApplyStarted
        && lastReportedState != ADUCITF_State_Failed)
    {
        // Likely nothing we can do about this, but try setting Idle state again.
        Log_Warn("Idle UpdateAction called in unexpected state: %s!", ADUCITF_StateToString(lastReportedState));
    }

    //
    // Clean up the sandbox.  It will be re-created when download starts.
    //
    char* workflowId = ADUC_WorkflowData_GetWorkflowId(workflowData);
    char* workFolder = ADUC_WorkflowData_GetWorkFolder(workflowData);

    if (workflowId != NULL)
    {
        Log_Info("UpdateAction: Idle. Ending workflow with WorkflowId: %s", workflowId);
        if (workFolder != NULL)
        {
            Log_Info("Calling SandboxDestroyCallback");

            updateActionCallbacks->SandboxDestroyCallback(
                updateActionCallbacks->PlatformLayerHandle, workflowId, workFolder);
        }
    }
    else
    {
        Log_Info("UpdateAction: Idle. WorkFolder is not valid. Nothing to destroy.");
    }

    //
    // Notify callback that we're now back to idle.
    //

    Log_Info("Calling IdleCallback");

    updateActionCallbacks->IdleCallback(updateActionCallbacks->PlatformLayerHandle, workflowId);

    workflow_free_string(workflowId);
    workflow_free_string(workFolder);

    workflow_free(workflowData->WorkflowHandle);
    workflowData->WorkflowHandle = NULL;
}

/**
 * @brief Called to do ProcessDeployment.
 *
 * @param[in,out] methodCallData The metedata for the method call.
 * @return Result code.
 */
ADUC_Result ADUC_Workflow_MethodCall_ProcessDeployment(ADUC_MethodCall_Data* methodCallData)
{
    UNREFERENCED_PARAMETER(methodCallData);
    Log_Info("Workflow step: ProcessDeployment");

    ADUC_Result result = { ADUC_Result_Success };
    return result;
}

void ADUC_Workflow_MethodCall_ProcessDeployment_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result)
{
    UNREFERENCED_PARAMETER(methodCallData);
    UNREFERENCED_PARAMETER(result);
}

/**
 * @brief Called to do download.
 *
 * @param[in,out] methodCallData The metedata for the method call.
 * @return Result code.
 */
ADUC_Result ADUC_Workflow_MethodCall_Download(ADUC_MethodCall_Data* methodCallData)
{
    ADUC_WorkflowData* workflowData = methodCallData->WorkflowData;
    ADUC_WorkflowHandle* workflowHandle = workflowData->WorkflowHandle;
    const ADUC_UpdateActionCallbacks* updateActionCallbacks = &(workflowData->UpdateActionCallbacks);

    ADUCITF_State lastReportedState = ADUC_WorkflowData_GetLastReportedState(workflowData);

    ADUC_Result result = { ADUC_Result_Download_Success };
    char* workFolder = workflow_get_workfolder(workflowHandle);

    Log_Info("Workflow step: Download");

    if (lastReportedState != ADUCITF_State_DeploymentInProgress)
    {
        Log_Error("Download workflow step called in unexpected state: %s!", ADUCITF_StateToString(lastReportedState));

        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = ADUC_ERC_UPPERLEVEL_WORKFLOW_UPDATE_ACTION_UNEXPECTED_STATE;
        goto done;
    }

    Log_Info("Calling SandboxCreateCallback");

    // Note: It's okay for SandboxCreate to return NULL for the work folder.
    // NULL likely indicates an OS without a file system.
    result = updateActionCallbacks->SandboxCreateCallback(
        updateActionCallbacks->PlatformLayerHandle, workflow_peek_id(workflowData->WorkflowHandle), workFolder);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    Log_Info("Using sandbox %s", workFolder != NULL ? workFolder : "(null)");

    ADUC_Workflow_SetUpdateState(workflowData, ADUCITF_State_DownloadStarted);

    result = updateActionCallbacks->DownloadCallback(
        updateActionCallbacks->PlatformLayerHandle, &(methodCallData->WorkCompletionData), workflowData);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

done:
    workflow_free_string(workFolder);

    return result;
}

void ADUC_Workflow_MethodCall_Download_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result)
{
    UNREFERENCED_PARAMETER(methodCallData);
    UNREFERENCED_PARAMETER(result);
}

/**
 * @brief Called to do install.
 *
 * @param[in] methodCallData - the method call data.
 * @return Result code.
 */
ADUC_Result ADUC_Workflow_MethodCall_Install(ADUC_MethodCall_Data* methodCallData)
{
    ADUC_WorkflowData* workflowData = methodCallData->WorkflowData;
    const ADUC_UpdateActionCallbacks* updateActionCallbacks = &(workflowData->UpdateActionCallbacks);
    ADUC_Result result = {};

    Log_Info("Workflow step: Install");

    ADUCITF_State lastReportedState = ADUC_WorkflowData_GetLastReportedState(workflowData);
    if (lastReportedState != ADUCITF_State_BackupSucceeded)
    {
        Log_Error("Install Workflow step called in unexpected state: %s!", ADUCITF_StateToString(lastReportedState));
        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = ADUC_ERC_UPPERLEVEL_WORKFLOW_INSTALL_ACTION_IN_UNEXPECTED_STATE;
        goto done;
    }

    ADUC_Workflow_SetUpdateState(workflowData, ADUCITF_State_InstallStarted);

    Log_Info("Calling InstallCallback");

    result = updateActionCallbacks->InstallCallback(
        updateActionCallbacks->PlatformLayerHandle, &(methodCallData->WorkCompletionData), workflowData);

done:
    return result;
}

void ADUC_Workflow_MethodCall_Install_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result)
{
    if (workflow_is_immediate_reboot_requested(methodCallData->WorkflowData->WorkflowHandle)
        || workflow_is_reboot_requested(methodCallData->WorkflowData->WorkflowHandle))
    {
        // If 'install' indicated a reboot required result from apply, go ahead and reboot.
        Log_Info("Install indicated success with RebootRequired - rebooting system now");
        methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_Required;

        int success = ADUC_MethodCall_RebootSystem();
        if (success == 0)
        {
            methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_InProgress;
        }
        else
        {
            Log_Error("Reboot attempt failed.");
            workflow_set_operation_in_progress(methodCallData->WorkflowData->WorkflowHandle, false);
        }
    }
    else if (
        workflow_is_immediate_agent_restart_requested(methodCallData->WorkflowData->WorkflowHandle)
        || workflow_is_agent_restart_requested(methodCallData->WorkflowData->WorkflowHandle))
    {
        // If 'install' indicated a restart is required, go ahead and restart the agent.
        Log_Info("Install indicated success with AgentRestartRequired - restarting the agent now");
        methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_Required;

        int success = ADUC_MethodCall_RestartAgent();
        if (success == 0)
        {
            methodCallData->WorkflowData->AgentRestartState = ADUC_AgentRestartState_InProgress;
        }
        else
        {
            Log_Error("Agent restart attempt failed.");
            workflow_set_operation_in_progress(methodCallData->WorkflowData->WorkflowHandle, false);
        }
    }
}

/**
 * @brief Called to do backup.
 *
 * @param[in] methodCallData - the method call data.
 * @return Result code.
 */
ADUC_Result ADUC_Workflow_MethodCall_Backup(ADUC_MethodCall_Data* methodCallData)
{
    ADUC_WorkflowData* workflowData = methodCallData->WorkflowData;
    const ADUC_UpdateActionCallbacks* updateActionCallbacks = &(workflowData->UpdateActionCallbacks);
    ADUC_Result result = {};

    Log_Info("Workflow step: backup");

    ADUCITF_State lastReportedState = ADUC_WorkflowData_GetLastReportedState(workflowData);
    if (lastReportedState != ADUCITF_State_DownloadSucceeded)
    {
        Log_Error("Backup Workflow step called in unexpected state: %s!", ADUCITF_StateToString(lastReportedState));
        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = ADUC_ERC_UPPERLEVEL_WORKFLOW_UPDATE_ACTION_UNEXPECTED_STATE;
        goto done;
    }

    ADUC_Workflow_SetUpdateState(workflowData, ADUCITF_State_BackupStarted);

    Log_Info("Calling BackupCallback");

    result = updateActionCallbacks->BackupCallback(
        updateActionCallbacks->PlatformLayerHandle, &(methodCallData->WorkCompletionData), workflowData);

done:
    return result;
}

void ADUC_Workflow_MethodCall_Backup_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result)
{
    UNREFERENCED_PARAMETER(methodCallData);
    UNREFERENCED_PARAMETER(result);
}

/**
 * @brief Called to do apply.
 *
 * @param[in] methodCallData - the method call data.
 * @return Result code.
 */
ADUC_Result ADUC_Workflow_MethodCall_Apply(ADUC_MethodCall_Data* methodCallData)
{
    ADUC_WorkflowData* workflowData = methodCallData->WorkflowData;
    const ADUC_UpdateActionCallbacks* updateActionCallbacks = &(workflowData->UpdateActionCallbacks);
    ADUC_Result result = {};

    Log_Info("Workflow step: Apply");

    ADUCITF_State lastReportedState = ADUC_WorkflowData_GetLastReportedState(workflowData);
    if (lastReportedState != ADUCITF_State_InstallSucceeded)
    {
        Log_Error("Apply Workflow step called in unexpected state: %s!", ADUCITF_StateToString(lastReportedState));
        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOTPERMITTED;
        goto done;
    }

    ADUC_Workflow_SetUpdateState(workflowData, ADUCITF_State_ApplyStarted);

    Log_Info("Calling ApplyCallback");

    result = updateActionCallbacks->ApplyCallback(
        updateActionCallbacks->PlatformLayerHandle, &(methodCallData->WorkCompletionData), workflowData);

done:
    return result;
}

void ADUC_Workflow_MethodCall_Apply_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result)
{
    if (workflow_is_immediate_reboot_requested(methodCallData->WorkflowData->WorkflowHandle)
        || workflow_is_reboot_requested(methodCallData->WorkflowData->WorkflowHandle))
    {
        // If apply indicated a reboot required result from apply, go ahead and reboot.
        Log_Info("Apply indicated success with RebootRequired - rebooting system now");
        methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_Required;

        int success = ADUC_MethodCall_RebootSystem();
        if (success == 0)
        {
            methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_InProgress;
        }
        else
        {
            Log_Error("Reboot attempt failed.");
            workflow_set_operation_in_progress(methodCallData->WorkflowData->WorkflowHandle, false);
        }
    }
    else if (
        workflow_is_immediate_agent_restart_requested(methodCallData->WorkflowData->WorkflowHandle)
        || workflow_is_agent_restart_requested(methodCallData->WorkflowData->WorkflowHandle))
    {
        // If apply indicated a restart is required, go ahead and restart the agent.
        Log_Info("Apply indicated success with AgentRestartRequired - restarting the agent now");
        methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_Required;

        int success = ADUC_MethodCall_RestartAgent();
        if (success == 0)
        {
            methodCallData->WorkflowData->AgentRestartState = ADUC_AgentRestartState_InProgress;
        }
        else
        {
            Log_Error("Agent restart attempt failed.");
            workflow_set_operation_in_progress(methodCallData->WorkflowData->WorkflowHandle, false);
        }
    }
    else if (result.ResultCode == ADUC_Result_Apply_Success)
    {
        // An Apply action completed successfully. Continue to the next step.
        workflow_set_operation_in_progress(methodCallData->WorkflowData->WorkflowHandle, false);
    }
}

/**
 * @brief Called to do restore.
 *
 * @param[in] methodCallData - the method call data.
 * @return Result code.
 */
ADUC_Result ADUC_Workflow_MethodCall_Restore(ADUC_MethodCall_Data* methodCallData)
{
    ADUC_WorkflowData* workflowData = methodCallData->WorkflowData;
    const ADUC_UpdateActionCallbacks* updateActionCallbacks = &(workflowData->UpdateActionCallbacks);
    ADUC_Result result = {};

    Log_Info("Workflow step: Restore");

    ADUCITF_State lastReportedState = ADUC_WorkflowData_GetLastReportedState(workflowData);
    if (lastReportedState != ADUCITF_State_Failed)
    {
        Log_Error("Apply Workflow step called in unexpected state: %s!", ADUCITF_StateToString(lastReportedState));
        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOTPERMITTED;
        goto done;
    }

    workflow_set_current_workflowstep(workflowData->WorkflowHandle, ADUCITF_WorkflowStep_Restore);

    ADUC_Workflow_SetUpdateState(workflowData, ADUCITF_State_RestoreStarted);

    Log_Info("Calling RestoreCallback");

    result = updateActionCallbacks->RestoreCallback(
        updateActionCallbacks->PlatformLayerHandle, &(methodCallData->WorkCompletionData), workflowData);

done:
    return result;
}

void ADUC_Workflow_MethodCall_Restore_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result)
{
    if (result.ResultCode == ADUC_Result_Restore_RequiredReboot
        || result.ResultCode == ADUC_Result_Restore_RequiredImmediateReboot)
    {
        // If restore indicated a reboot required result from restore, go ahead and reboot.
        Log_Info("Restore indicated success with RebootRequired - rebooting system now");
        methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_Required;

        int success = ADUC_MethodCall_RebootSystem();
        if (success == 0)
        {
            methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_InProgress;
        }
        else
        {
            Log_Error("Reboot attempt failed.");
            workflow_set_operation_in_progress(methodCallData->WorkflowData->WorkflowHandle, false);
        }
    }
    else if (
        result.ResultCode == ADUC_Result_Restore_RequiredAgentRestart
        || result.ResultCode == ADUC_Result_Restore_RequiredImmediateAgentRestart)
    {
        // If restore indicated a restart is required, go ahead and restart the agent.
        Log_Info("Restore indicated success with AgentRestartRequired - restarting the agent now");
        methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_Required;

        int success = ADUC_MethodCall_RestartAgent();
        if (success == 0)
        {
            methodCallData->WorkflowData->AgentRestartState = ADUC_AgentRestartState_InProgress;
        }
        else
        {
            Log_Error("Agent restart attempt failed.");
            workflow_set_operation_in_progress(methodCallData->WorkflowData->WorkflowHandle, false);
        }
    }
    else if (
        result.ResultCode == ADUC_Result_Restore_Success
        || result.ResultCode == ADUC_Result_Restore_Success_Unsupported)
    {
        // An restore action completed successfully. Continue to the next step.
        workflow_set_operation_in_progress(methodCallData->WorkflowData->WorkflowHandle, false);
    }
}

/**
 * @brief Called to request platform-layer operation to cancel.
 *
 * This method should only be called while another MethodCall is currently active.
 *
 * @param[in] methodCallData - the method call data.
 */
void ADUC_Workflow_MethodCall_Cancel(const ADUC_WorkflowData* workflowData)
{
    const ADUC_UpdateActionCallbacks* updateActionCallbacks = &(workflowData->UpdateActionCallbacks);

    if (workflow_get_operation_in_progress(workflowData->WorkflowHandle))
    {
        Log_Info("Requesting cancel for ongoing operation.");
    }
    else
    {
        Log_Warn("Cancel requested without operation in progress - ignoring.");
        return;
    }

    updateActionCallbacks->CancelCallback(
        updateActionCallbacks->PlatformLayerHandle, (ADUC_WorkflowDataToken)workflowData);
}

/**
 * @brief Helper to call into the platform layer for IsInstalled.
 *
 * @param[in] workflowData The workflow data.
 *
 * @return ADUC_Result The result of the IsInstalled call.
 */
ADUC_Result ADUC_Workflow_MethodCall_IsInstalled(const ADUC_WorkflowData* workflowData)
{
    if (workflowData == NULL)
    {
        Log_Info("IsInstalled called before workflowData is initialized.");
        ADUC_Result result = { .ResultCode = ADUC_Result_IsInstalled_NotInstalled };
        return result;
    }

    const ADUC_UpdateActionCallbacks* updateActionCallbacks = &(workflowData->UpdateActionCallbacks);

    Log_Info("Calling IsInstalledCallback to check if content is installed.");
    return updateActionCallbacks->IsInstalledCallback(
        updateActionCallbacks->PlatformLayerHandle, (ADUC_WorkflowDataToken)workflowData);
}
