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

#include "aduc/adu_core_export_helpers.h"
#include "aduc/adu_core_interface.h"
#include "aduc/agent_orchestration.h"
#include "aduc/c_utils.h"
#include "aduc/logging.h"
#include "aduc/parser_utils.h"
#include "aduc/result.h"
#include "aduc/string_c_utils.h"
#include "aduc/system_utils.h"
#include "aduc/types/workflow.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_persistence_utils.h"
#include "aduc/workflow_utils.h"

#include <pthread.h>

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

// fwd decl
void ADUC_Workflow_WorkCompletionCallback(const void* workCompletionToken, ADUC_Result result, _Bool isAsync);

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
    case ADUCITF_WorkflowStep_Install:
        return "Install";
    case ADUCITF_WorkflowStep_Apply:
        return "Apply";
    case ADUCITF_WorkflowStep_Undefined:
        return "Undefined";
    }

    return "<Unknown>";
}

/**
 * @brief Generate a unique identifier.
 *
 * @param buffer Where to store identifier.
 * @param buffer_cch Number of characters in @p buffer.
 */
void GenerateUniqueId(char* buffer, size_t buffer_cch)
{
    const time_t timer = time(NULL);
    const struct tm* ptm = gmtime(&timer);
    (void)strftime(buffer, buffer_cch, "%y%m%d%H%M%S", ptm);
}

static void DownloadProgressCallback(
    const char* workflowId,
    const char* fileId,
    ADUC_DownloadProgressState state,
    uint64_t bytesTransferred,
    uint64_t bytesTotal);

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

    const ADUCITF_State NextState; /**< State to transition to on successful operation */

    /**< The next workflow step input to transition workflow after transitioning to above NextState when current
     *     workflow step is above WorkflowStep. Using ADUCITF_WorkflowStep_Undefined means it ends the workflow.
     */
    const ADUCITF_WorkflowStep AutoTransitionWorkflowStep;
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
        /* calls operation */                             ADUC_MethodCall_ProcessDeployment,
        /* and on completion calls */                     ADUC_MethodCall_ProcessDeployment_Complete,
        /* then on success, transitions to state */       ADUCITF_State_DeploymentInProgress,
        /* and then auto-transitions to workflow step */  ADUCITF_WorkflowStep_Download,
    },

    { ADUCITF_WorkflowStep_Download,
        /* calls operation */                             ADUC_MethodCall_Download,
        /* and on completion calls */                     ADUC_MethodCall_Download_Complete,
        /* then on success, transitions to state */       ADUCITF_State_DownloadSucceeded,
        /* and then auto-transitions to workflow step */  ADUCITF_WorkflowStep_Install,
    },

    { ADUCITF_WorkflowStep_Install,
        /* calls operation */                             ADUC_MethodCall_Install,
        /* and on completion calls */                     ADUC_MethodCall_Install_Complete,
        /* then on success, transitions to state */       ADUCITF_State_InstallSucceeded,
        /* and then auto-transitions to workflow step */  ADUCITF_WorkflowStep_Apply,
    },

    // Note: There's no "ApplySucceeded" state.  On success, we should return to Idle state.
    { ADUCITF_WorkflowStep_Apply,
        /* calls operation */                             ADUC_MethodCall_Apply,
        /* and on completion calls */                     ADUC_MethodCall_Apply_Complete,
        /* then on success, transition to state */        ADUCITF_State_Idle,
        /* and then auto-transitions to workflow step */  ADUCITF_WorkflowStep_Undefined, // Undefined means end of workflow
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
 * @brief Initialize a ADUC_WorkflowData object.
 *
 * @param[out] workflowData Workflow metadata.
 * @param argc Count of arguments in @p argv
 * @param argv Command line parameters.
 * @return _Bool True on success.
 */
_Bool ADUC_WorkflowData_Init(ADUC_WorkflowData* workflowData, int argc, char** argv)
{
    _Bool succeeded = false;

    memset(workflowData, 0, sizeof(*workflowData));

    ADUC_Result result = ADUC_MethodCall_Register(&(workflowData->UpdateActionCallbacks), argc, (const char**)argv);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ADUC_RegisterPlatformLayer failed %d, %d", result.ResultCode, result.ExtendedResultCode);
        goto done;
    }

    // Only call Unregister if register succeeded.
    workflowData->IsRegistered = true;

    workflowData->DownloadProgressCallback = DownloadProgressCallback;

    workflow_set_cancellation_type(workflowData->WorkflowHandle, ADUC_WorkflowCancellationType_None);

    succeeded = true;

done:
    return succeeded;
}

/**
 * @brief Called regularly to allow for cooperative multitasking during work.
 *
 * @param workflowData Workflow metadata.
 */
void ADUC_WorkflowData_DoWork(ADUC_WorkflowData* workflowData)
{
    // As this method will be called many times, rather than call into adu_core_export_helpers to call into upper-layer,
    // just call directly into upper-layer here.
    const ADUC_UpdateActionCallbacks* updateActionCallbacks = &(workflowData->UpdateActionCallbacks);

    updateActionCallbacks->DoWorkCallback(updateActionCallbacks->PlatformLayerHandle, workflowData);
}

/**
 * @brief Free members of ADUC_WorkflowData object.
 *
 * @param workflowData Object whose members should be freed.
 */
void ADUC_WorkflowData_Uninit(ADUC_WorkflowData* workflowData)
{
    if (workflowData == NULL)
    {
        return;
    }

    if (workflowData->IsRegistered)
    {
        ADUC_MethodCall_Unregister(&(workflowData->UpdateActionCallbacks));
    }

    memset(workflowData, 0, sizeof(*workflowData));
}

/**
 * @brief Attempts to rehydrate persisted workflow state, if it exists.
 *
 * @param currentWorkflowData The workflow data used for test hooks.
 */
static void HandleWorkflowPersistence(ADUC_WorkflowData* currentWorkflowData)
{
    WorkflowPersistenceState* persistenceState = WorkflowPersistence_Deserialize(currentWorkflowData);
    if (persistenceState == NULL)
    {
        Log_Info("Continuing...");
        return;
    }

    //
    // The following logic handles the case where the device (or adu-agent) restarted.
    //
    // If IsInstalled returns true, we will assume that the update was succeeded.
    //
    // In this case, we will update the 'InstalledUpdateId' to match 'UpdateId'
    // and transition to Idle state.
    //
    // Note: This applies to 'install' and 'apply' workflow step only, and persistence
    // state is saved only when restart/reboot occurred during install or apply.

    currentWorkflowData->persistenceState = persistenceState; // source from persistenceState instead of WorkflowHandle.

    ADUC_Result isInstalledResult = ADUC_MethodCall_IsInstalled(currentWorkflowData);

    if ((persistenceState->WorkflowStep == ADUCITF_WorkflowStep_Install || persistenceState->WorkflowStep == ADUCITF_WorkflowStep_Apply) &&
        (persistenceState->SystemRebootState == ADUC_SystemRebootState_InProgress || persistenceState->AgentRestartState == ADUC_AgentRestartState_InProgress))
    {
        if (isInstalledResult.ResultCode == ADUC_Result_IsInstalled_Installed)
        {
            const char* updateId = persistenceState->ExpectedUpdateId;
            Log_Info(
                "IsInstalled call was true - setting installedUpdateId to %s and setting state to Idle", updateId);

            ADUC_SetInstalledUpdateIdAndGoToIdle(currentWorkflowData, updateId);
        }
        else if (isInstalledResult.ResultCode == ADUC_Result_IsInstalled_NotInstalled)
        {
            // TODO(JeffW): Bug 36790018 Agent should continue processing after reboot/restart when contentHandler returns ADUC_Result_IsInstalled_NotInstalled
        }
        else if (IsAducResultCodeFailure(isInstalledResult.ResultCode))
        {
            Log_Warn(
                "IsInstalled call failed. ExtendedResultCode: %d (0x%x) - setting state to Idle",
                isInstalledResult.ExtendedResultCode,
                isInstalledResult.ExtendedResultCode);

            ADUC_Result persistenceResult =
            {
                .ResultCode = isInstalledResult.ResultCode,
                .ExtendedResultCode = isInstalledResult.ExtendedResultCode,
            };

            SetUpdateStateWithResultFunc setUpdateStateWithResultFunc = ADUC_WorkflowData_GetSetUpdateStateWithResultFunc(currentWorkflowData);

            (*setUpdateStateWithResultFunc)(currentWorkflowData, ADUCITF_State_Idle, persistenceResult);
        }

        goto done;
    }

    // TODO(JeffW): Task 36812133 Resume download on startup

done:
    WorkflowPersistence_Free(persistenceState);
    currentWorkflowData->persistenceState = NULL; // reset to default of sourcing from WorkflowHandle

    WorkflowPersistence_Delete(currentWorkflowData);
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

    // There are 2 call patterns for startup:
    //     (1) Called from AzureDeviceUpdateCoreInterface_Connected with a NULL WorkflowHandle in the workflowData
    //     (2) Called from ADUC_Workflow_HandlePropertyUpdate (instead of calling ADUC_Workflow_HandleUpdateAction) when StartupIdleCallSent is false.
    // In both cases, we must first try to complete workflow processing using the persisted workflow state.
    HandleWorkflowPersistence(currentWorkflowData);

    // NOTE: WorkflowHandle can be NULL when device first connected to the hub (no desired property).
    if (currentWorkflowData->WorkflowHandle == NULL)
    {
        Log_Info("There's no update actions in current workflow (first time connected to IoT Hub).");
    }
    else
    {
        ADUC_Result isInstalledResult = ADUC_MethodCall_IsInstalled(currentWorkflowData);
        if (isInstalledResult.ResultCode == ADUC_Result_IsInstalled_Installed)
        {
            // If it's already installed, then service either is misbehaving or failed to receive reporting.
            // Try sending reporting now.
            char* updateId = workflow_get_expected_update_id_string(currentWorkflowData->WorkflowHandle);
            ADUC_SetInstalledUpdateIdAndGoToIdle(currentWorkflowData, updateId);
            free(updateId);
            goto done;
        }

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

            SetUpdateStateWithResultFunc setUpdateStateWithResultFunc = ADUC_WorkflowData_GetSetUpdateStateWithResultFunc(currentWorkflowData);
            (*setUpdateStateWithResultFunc)(currentWorkflowData, ADUCITF_State_Idle, result);

            goto done;
        }

        Log_Info("There's a pending '%s' action", ADUCITF_UpdateActionToString(desiredAction));
    }

    // There's a pending ProcessDeployment action in the twin.
    // We need to make sure we don't report an 'idle' state, if we can resume or retry the action.
    // In this case, we will set last reportedState to 'idle', so that we can continue.
    ADUC_WorkflowData_SetLastReportedState(ADUCITF_State_Idle, currentWorkflowData);

    HandleUpdateActionFunc handleUpdateActionFunc = ADUC_WorkflowData_GetHandleUpdateActionFunc(currentWorkflowData);
    (*handleUpdateActionFunc)(currentWorkflowData);

done:

    // Once we set Idle state to the orchestrator we can start receiving update actions.
    currentWorkflowData->StartupIdleCallSent = true;
}

/**
 * @brief Handles updates to a 1 or more PnP Properties in the ADU Core interface.
 *
 * @param[in,out] currentWorkflowData The current ADUC_WorkflowData object.
 * @param[in] propertyUpdateValue The updated property value.
 */
void ADUC_Workflow_HandlePropertyUpdate(
    ADUC_WorkflowData* currentWorkflowData, const unsigned char* propertyUpdateValue)
{
    ADUC_WorkflowHandle nextWorkflow;
    ADUC_Result result = workflow_init((const char*)propertyUpdateValue, true, &nextWorkflow);

    // There are 2 call patterns for startup:
    //     (1) Called from AzureDeviceUpdateCoreInterface_Connected with a NULL WorkflowHandle in the workflowData
    //     (2) Called from ADUC_Workflow_HandlePropertyUpdate (instead of calling ADUC_Workflow_HandleUpdateAction) when StartupIdleCallSent is false.
    // In both cases, we must first try to complete workflow processing using the persisted workflow state.
    HandleWorkflowPersistence(currentWorkflowData);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("Invalid desired update action data. Update data: (%s)", propertyUpdateValue);

        //
        // TODO(NIC): 31481186 Refactor reporting so failures are reported in line instead of waiting for the state change to happen
        //
        ADUC_SetUpdateStateWithResult(currentWorkflowData, ADUCITF_State_Failed, result);
        return;
    }

    ADUCITF_UpdateAction nextUpdateAction = workflow_get_action(nextWorkflow);

    if (nextUpdateAction == ADUCITF_UpdateAction_ProcessDeployment)
    {
        ADUC_WorkflowData tempWorkflowData = *currentWorkflowData;
        tempWorkflowData.WorkflowHandle = nextWorkflow;

        ADUC_Result isInstalledResult = ADUC_MethodCall_IsInstalled(&tempWorkflowData);
        if (isInstalledResult.ResultCode == ADUC_Result_IsInstalled_Installed)
        {
            Log_Debug("IsInstalled call was true for desired workflow id - skipping");

            // However, if currentWorkflowData is empty, this is the first workflow data
            // received from the twin. We should keep it around.
            if (currentWorkflowData->WorkflowHandle == NULL)
            {
                currentWorkflowData->WorkflowHandle = nextWorkflow;
                nextWorkflow = NULL;
            }

            goto done;
        }
    }

    //
    // Take lock until goto done.
    //
    // N.B.
    // Lock must *NOT* be taken in HandleStartupWorkflowData and HandleUpdateAction or any functions they call
    //
    s_workflow_lock();

    HandleUpdateActionFunc handleUpdateActionFunc = ADUC_WorkflowData_GetHandleUpdateActionFunc(currentWorkflowData);

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

                // call into handle update action for cancellation logic to invoke ADUC_MethodCall_Cancel
                (*handleUpdateActionFunc)(currentWorkflowData);

                goto done;
            }
            else
            {
                Log_Info(
                    "Ignoring duplicate '%s' action. Current cancellation type is already '%s'.",
                    ADUCITF_UpdateActionToString(nextUpdateAction),
                    ADUC_WorkflowCancellationTypeToString(currentCancellationType));
                goto done;
            }
        }
        else if (nextUpdateAction == ADUCITF_UpdateAction_ProcessDeployment)
        {
            if (workflow_id_compare(currentWorkflowData->WorkflowHandle, nextWorkflow) == 0)
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

                // This atomically sets both cancellation type to Retry and updates the current retry token on the workflow handle
                workflow_update_retry_deployment(currentWorkflowData->WorkflowHandle, currentRetryToken);

                // call into handle update action for cancellation logic to invoke ADUC_MethodCall_Cancel
                (*handleUpdateActionFunc)(currentWorkflowData);
                goto done;
            }
            else
            {
                // Possible replacement with a new workflow.
                ADUCITF_State currentState = ADUC_WorkflowData_GetLastReportedState(currentWorkflowData);
                ADUCITF_WorkflowStep currentWorkflowStep = workflow_get_current_workflowstep(currentWorkflowData->WorkflowHandle);

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
                    _Bool deferredReplacement =
                        workflow_update_replacement_deployment(currentWorkflowData->WorkflowHandle, nextWorkflow);
                    if (deferredReplacement)
                    {
                        Log_Info(
                            "Deferred Replacement workflow id [%s] since current workflow id [%s] was still in progress.",
                            workflow_peek_id(nextWorkflow),
                            workflow_peek_id(currentWorkflowData->WorkflowHandle));

                        // Ownership was transferred to current workflow so ensure it doesn't get freed.
                        nextWorkflow = NULL;

                        // call into handle update action for cancellation logic to invoke ADUC_MethodCall_Cancel
                        (*handleUpdateActionFunc)(currentWorkflowData);
                        goto done;
                    }

                    workflow_transfer_data(
                        currentWorkflowData->WorkflowHandle /* wfTarget */, nextWorkflow /* wfSource */);

                    (*handleUpdateActionFunc)(currentWorkflowData);
                    goto done;
                }

                // Fall through to handle new workflow
            }
        }
    }
    else
    {
        // This is a top level workflow, make sure that we set the working folder correctly.
        STRING_HANDLE workFolder =
            STRING_construct_sprintf("%s/%s", ADUC_DOWNLOADS_FOLDER, workflow_peek_id(nextWorkflow));
        workflow_set_workfolder(nextWorkflow, STRING_c_str(workFolder));
        STRING_delete(workFolder);
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
        (*handleUpdateActionFunc)(currentWorkflowData);
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
    Log_Debug("cancellationType(%d) => %s", cancellationType, ADUC_WorkflowCancellationTypeToString(cancellationType));

    _Bool isReplaceOrRetry = (cancellationType == ADUC_WorkflowCancellationType_Replacement)
        || (cancellationType == ADUC_WorkflowCancellationType_Retry);

    if (desiredAction == ADUCITF_UpdateAction_Cancel || cancellationType == ADUC_WorkflowCancellationType_Normal
        || ((desiredAction == ADUCITF_UpdateAction_ProcessDeployment) && isReplaceOrRetry))
    {
        if (workflow_get_operation_in_progress(workflowData->WorkflowHandle))
        {
            Log_Info(
                "Canceling request for in-progress operation. desiredAction: %s, cancelationType: %s",
                ADUCITF_UpdateActionToString(desiredAction),
                ADUC_WorkflowCancellationTypeToString(cancellationType));

            // This sets a marker that cancelation has been requested.
            workflow_set_operation_cancel_requested(workflowData->WorkflowHandle, true);

            // Call upper-layer to notify of cancel
            ADUC_MethodCall_Cancel(workflowData);
        }
        else if (
            desiredAction == ADUCITF_UpdateAction_Cancel || cancellationType == ADUC_WorkflowCancellationType_Normal)
        {
            // Cancel without an operation in progress means return to Idle state.
            Log_Info("Cancel received with no operation in progress - returning to Idle state");
            goto done;
        }

        goto done;
    }

    //
    // Save the original action to the workflow data
    //
    ADUC_WorkflowData_SetCurrentAction(desiredAction, workflowData);

    //
    // Determine the current workflow step
    //
    ADUCITF_WorkflowStep nextStep = AgentOrchestration_GetWorkflowStep(desiredAction);
    workflow_set_current_workflowstep(workflowData->WorkflowHandle, nextStep);

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
 */
void ADUC_Workflow_AutoTransitionWorkflow(ADUC_WorkflowData* workflowData)
{
    if (ADUC_WorkflowData_GetLastReportedState(workflowData) == ADUCITF_State_Failed)
    {
        Log_Debug("Skipping transition for Failed state.");
        return;
    }

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

    if (AgentOrchestration_IsWorkflowComplete(postCompleteEntry->AutoTransitionWorkflowStep))
    {
        Log_Info("Workflow is Complete.");
    }
    else
    {
        workflow_set_current_workflowstep(workflowData->WorkflowHandle, postCompleteEntry->AutoTransitionWorkflowStep);

        Log_Info(
            "workflow is not completed. AutoTransition to step: %s",
            ADUCITF_WorkflowStepToString(postCompleteEntry->AutoTransitionWorkflowStep));

        ADUC_Workflow_TransitionWorkflow(workflowData);
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
    WorkCompletionCallbackFunc workCompletionCallbackFunc = ADUC_Workflow_WorkCompletionCallback;

#ifdef ADUC_BUILD_UNIT_TESTS
    if (workflowData->TestOverrides && workflowData->TestOverrides->WorkCompletionCallbackFunc_TestOverride)
    {
        workCompletionCallbackFunc = workflowData->TestOverrides->WorkCompletionCallbackFunc_TestOverride;
    }
#endif

    methodCallData->WorkCompletionData.WorkCompletionCallback = workCompletionCallbackFunc;
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
        (*workCompletionCallbackFunc)(methodCallData, result, false /* isAsync */);
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
void ADUC_Workflow_WorkCompletionCallback(const void* workCompletionToken, ADUC_Result result, _Bool isAsync)
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

        const ADUCITF_State nextUpdateState = entry->NextState;

        Log_Info(
            "WorkCompletionCallback: %s succeeded. Going to state %s",
            ADUCITF_WorkflowStepToString(entry->WorkflowStep),
            ADUCITF_StateToString(nextUpdateState));

        ADUC_SetUpdateState(workflowData, nextUpdateState);

        // Transitioning to idle (or failed) state frees and nulls-out the WorkflowHandle as a side-effect of
        // setting the update state.
        if (ADUC_WorkflowData_GetLastReportedState(workflowData) != ADUCITF_State_Idle)
        {
            // Operation is now complete. Clear both inprogress and cancel requested.
            workflow_clear_inprogress_and_cancelrequested(workflowData->WorkflowHandle);

            //
            // We are now ready to transition to the next step of the workflow.
            //
            ADUC_Workflow_AutoTransitionWorkflow(workflowData);
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
            const char* cancellationTypeStr = ADUC_WorkflowCancellationTypeToString(cancellationType);

            Log_Warn("Handling cancel completion, cancellation type '%s'.", cancellationTypeStr);

            if (cancellationType == ADUC_WorkflowCancellationType_Replacement
                || cancellationType == ADUC_WorkflowCancellationType_Retry)
            {
                Log_Info("Starting process of deployment for '%s'", cancellationTypeStr);

                // Note: Must NOT call linux platform layer Idle method to reset cancellation request to false in the
                // platform layer because that would destroy and NULL out the WorkflowHandle in the workflowData.

                if (cancellationType == ADUC_WorkflowCancellationType_Replacement)
                {
                    // Reset workflow state to process deployment and transfer the deferred workflow to current.
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
            ADUC_SetUpdateStateWithResult(workflowData, ADUCITF_State_Idle, result);
        }
        else
        {
            // Operation failed.
            //
            // Report back the result and set state to "Failed".
            // It's expected that the service will call us again with a "Cancel" action,
            // to indicate that it's received the operation result and state, at which time
            // we'll return back to idle state.

            Log_Error(
                "%s failed. error %d, %d (0x%X) - Expecting service to send Cancel action.",
                ADUCITF_WorkflowStepToString(entry->WorkflowStep),
                result.ResultCode,
                result.ExtendedResultCode,
                result.ExtendedResultCode);

            ADUC_SetUpdateStateWithResult(workflowData, ADUCITF_State_Failed, result);
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

//
// Download progress callback
//

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

static void DownloadProgressCallback(
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
