/**
 * @file agent_workflow.c
 * @brief Handles workflow requests coming in from the hub.
 *
 * The cloud-based orchestrator (CBO) holds the state machine, so the best we can do in this agent
 * is to react to the CBO update actions, and see if we think we're in the correct state.
 * If we are, we'll call an upper-level method to do the work.
 * If not, we'll fail the request.
 *
 * @copyright Copyright (c) 2021, Microsoft Corp.
 */
#include "aduc/agent_workflow.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h> // PRIu64
#include <stdlib.h>
#include <time.h>

#include "aduc/adu_core_export_helpers.h"
#include "aduc/adu_core_interface.h"
#include "aduc/c_utils.h"
#include "aduc/logging.h"
#include "aduc/parser_utils.h"
#include "aduc/result.h"
#include "aduc/string_c_utils.h"
#include "aduc/system_utils.h"
#include "aduc/workflow_utils.h"

#include "aduc/agent_workflow_utils.h"

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
 * @brief Map from an UpdateAction to a method that performs that action, and the UpdateState to transition to
 * if that method is successful.
 */
typedef struct tagADUC_WorkflowHandlerMapEntry
{
    const ADUCITF_UpdateAction Action; /**< Requested action. */

    const ADUC_Workflow_OperationFunc OperationFunc; /**< Calls upper-level operation */

    const ADUC_Workflow_OperationCompleteFunc OperationCompleteFunc;

    const ADUCITF_State NextState; /**< State to transition to on successful operation */
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
 */
const ADUC_WorkflowHandlerMapEntry workflowHandlerMap[] = {
    { ADUCITF_UpdateAction_Download, ADUC_MethodCall_Download, ADUC_MethodCall_Download_Complete, ADUCITF_State_DownloadSucceeded },
    { ADUCITF_UpdateAction_Install,  ADUC_MethodCall_Install,  ADUC_MethodCall_Install_Complete,  ADUCITF_State_InstallSucceeded },
    // Note: There's no "ApplySucceeded" state.  On success, we should return to Idle state.
    { ADUCITF_UpdateAction_Apply,    ADUC_MethodCall_Apply,    ADUC_MethodCall_Apply_Complete,    ADUCITF_State_Idle },
};
// clang-format on

/**
 * @brief Get the Workflow Handler Map Entry For Action object
 *
 * @param action Action to find.
 * @return ADUC_WorkflowHandlerMapEntry* NULL if @p action not found.
 */
const ADUC_WorkflowHandlerMapEntry* GetWorkflowHandlerMapEntryForAction(unsigned int action)
{
    const ADUC_WorkflowHandlerMapEntry* entry;
    const unsigned int map_count = ARRAY_SIZE(workflowHandlerMap);
    unsigned index;
    for (index = 0; index < map_count; ++index)
    {
        entry = workflowHandlerMap + index;
        if (entry->Action == action)
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

static void ADUC_Workflow_WorkCompletionCallback(const void* workCompletionToken, ADUC_Result result);

void ADUC_Workflow_HandleStartupWorkflowData(ADUC_WorkflowData* workflowData)
{
    // The default result for Idle state.
    // This will reset twin status code to 200 to indicate that we're successful (so far).
    const ADUC_Result result = { .ResultCode = ADUC_Result_Idle_Success };

    if (workflowData == NULL)
    {
        Log_Info("No update content. Reporting Idle state.");
        ADUC_SetUpdateStateWithResult(workflowData, ADUCITF_State_Idle, result);
        goto done;
    }

    if (workflowData->StartupIdleCallSent)
    {
        goto done;
    }

    Log_Info("Perform startup tasks.");

    int desiredAction = workflow_get_action(workflowData->WorkflowHandle);
    if (desiredAction == ADUCITF_UpdateAction_Cancel)
    {
        Log_Info("Received 'cancel' action on startup, reporting Idle state.");
        ADUC_SetUpdateStateWithResult(workflowData, ADUCITF_State_Idle, result);
        goto done;
    }

    //
    // The following logic handles the case where the device (or adu-agent) restarted.
    //
    // If IsInstalled returns true, we will assume that the update was succeeded.
    //
    // In this case, we will update the 'InstalledUpdateId' to match 'UpdateId'
    // and transition to Idle state.
    //
    // Note: This apply to 'install' and 'apply' actions only.
    //
    if (desiredAction == ADUCITF_UpdateAction_Install || desiredAction == ADUCITF_UpdateAction_Apply)
    {
        ADUC_Result isInstalledResult = ADUC_MethodCall_IsInstalled(workflowData);
        if (isInstalledResult.ResultCode == ADUC_Result_IsInstalled_Installed)
        {
            char* updateId = workflow_get_expected_update_id_string(workflowData->WorkflowHandle);
            Log_Info(
                "IsInstalled call was true - setting installedUpdateId to %s and setting state to Idle", updateId);
            ADUC_SetInstalledUpdateIdAndGoToIdle(workflowData, updateId);
            workflow_free_string(updateId);
            goto done;
        }

        if (IsAducResultCodeFailure(isInstalledResult.ResultCode))
        {
            Log_Warn(
                "IsInstalled call failed. ExtendedResultCode: %d (0x%x) - setting state to Idle",
                isInstalledResult.ExtendedResultCode,
                isInstalledResult.ExtendedResultCode);

            ADUC_SetUpdateStateWithResult(workflowData, ADUCITF_State_Idle, result);
            goto done;
        }
    }

    // Allows retrying or resuming both 'download' and 'install' phases.
    if (desiredAction == ADUCITF_UpdateAction_Download || desiredAction == ADUCITF_UpdateAction_Install)
    {
        Log_Info("There's a pending '%s' action request.", ADUCITF_UpdateActionToString(desiredAction));

        // There's a pending download or install action in the twin.
        // We need to make sure we don't report an 'idle' state, if we can resume or retry the action.
        // In this case, we will set last reportedState to 'idle', so that we can continue 'download' or 'install'
        // phase as appropriate.
        workflowData->StartupIdleCallSent = true;
        ADUCITF_State lastReportState =
            (desiredAction == ADUCITF_UpdateAction_Install ? ADUCITF_State_DownloadSucceeded : ADUCITF_State_Idle);

        workflow_set_last_reported_state(lastReportState);
        ADUC_Workflow_HandleUpdateAction(workflowData);
        goto done;
    }

    ADUC_SetUpdateStateWithResult(
        workflowData, ADUCITF_State_Idle, result);

done:

    // Once we set Idle state to the orchestrator we can start receiving update actions.
    workflowData->StartupIdleCallSent = true;
}

/**
 * @brief Handles updates to a 1 or more PnP Properties in the ADU Core interface.
 *
 * @param[in,out] workflowData The current ADUC_WorkflowData object.
 * @param[in] propertyUpdateValue The updated property value.
 */
void ADUC_Workflow_HandlePropertyUpdate(ADUC_WorkflowData* workflowData, const unsigned char* propertyUpdateValue)
{
    ADUC_WorkflowHandle nextWorkflow;
    ADUC_Result result = workflow_init((const char*)propertyUpdateValue, true, &nextWorkflow);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("Invalid desired update action data. Update data: (%s)", propertyUpdateValue);

        //
        // TODO(NIC): 31481186 Refactor reporting so failures are reported in line instead of waiting for the state change to happen
        //
        ADUC_SetUpdateStateWithResult(workflowData, ADUCITF_State_Failed, result);
        return;
    }

    //
    if (workflowData->WorkflowHandle != NULL)
    {
        if (workflow_id_compare(nextWorkflow, workflowData->WorkflowHandle) == 0)
        {
            // This desired action is part of an existing workflow, let's process it.
            ADUCITF_UpdateAction nextAction = workflow_get_action(nextWorkflow);

            if (workflow_get_operation_in_progress(workflowData->WorkflowHandle)
                && nextAction != ADUCITF_UpdateAction_Cancel)
            {
                Log_Warn("Received unexpected action '%s'.", ADUCITF_UpdateActionToString(nextAction));
            }
            else
            {
                workflow_update_action_data(workflowData->WorkflowHandle, nextWorkflow);
                workflow_free(nextWorkflow);
                nextWorkflow = NULL;

                ADUC_Workflow_HandleUpdateAction(workflowData);
            }

            goto done;
        }

        // Not the same workflow.

        // If existing workflow in progress? (Unlikely.)
        if (workflow_get_operation_in_progress(workflowData->WorkflowHandle))
        {
            Log_Warn("Received a new desired action while another workflow is in progress.");
            workflow_set_operation_cancel_requested(workflowData->WorkflowHandle, true);
            goto done;
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
    workflow_free(workflowData->WorkflowHandle);
    workflowData->WorkflowHandle = nextWorkflow;
    nextWorkflow = NULL;

    // If the agent has just started up but we have yet to report the installedUpdateId along with a state of 'Idle'
    // we want to ignore any further action received so we don't confuse the workflow which would interpret a state of 'Idle'
    // not accompanied with an installedUpdateId as a failed end state in some cases.
    // In this case we will go through our startup logic which would report the installedUpdateId with a state of 'Idle',
    // if we can determine that the update has been installed successfully (by calling IsInstalled()).
    // Otherwise we will honor and process the action requested.
    if (!workflowData->StartupIdleCallSent)
    {
        ADUC_Workflow_HandleStartupWorkflowData(workflowData);
    }
    else
    {
        ADUC_Workflow_HandleUpdateAction(workflowData);
    }

done:
    workflow_free(nextWorkflow);
    Log_Debug("PropertyUpdated event handler completed.");
}

/**
 * @brief Handle an incoming update action.
 *
 * @param workflowData Workflow metadata.
 */
void ADUC_Workflow_HandleUpdateAction(ADUC_WorkflowData* workflowData)
{
    ADUC_Result result;

    //
    // Find the WorkflowHandlerMapEntry for the desired update action.
    //
    unsigned int desiredAction = workflow_get_action(workflowData->WorkflowHandle);

    //
    // Special case: Cancel is handled here.
    //
    // If Cancel action is received while another action (e.g. download) is in progress the agent should cancel
    // the in progress action and the agent should set Idle state.
    //
    // If an operation completes with a failed state, the error should be reported to the service, and the agent
    // should set Failed state.  The CBO once it receives the Failed state will send the agent a Cancel action to
    // indicate that it should return to Idle.  It's assumed that there is no action in progress on the agent at this
    // point and as such, the agent should set Idle state and return a success result code to the service.
    //
    //  Cancel should only be sent from the CBO when:
    // * An operation is in progress, to cancel the operation.
    // * After an operation fails to return the agent back to Idle state.
    // * A rollout end time has passed & the device has been offline and did not receive the previous command.
    //

    if (desiredAction == ADUCITF_UpdateAction_Cancel)
    {
        if (workflow_get_operation_in_progress(workflowData->WorkflowHandle))
        {
            Log_Info("Cancel requested - notifying operation in progress.");

            // Set OperationCancelled so that when the operation in progress completes, it's clear
            // that it was due to a cancellation.
            // We will ignore the result of what the operation in progress returns when it completes
            // in cancelled state.
            workflow_set_operation_cancel_requested(workflowData->WorkflowHandle, true);

            // Call upper-layer to notify of cancel request.
            ADUC_MethodCall_Cancel(workflowData);
        }
        else if (workflowData->SystemRebootState == ADUC_SystemRebootState_InProgress)
        {
            // It may not be possible, and unsafe, to try to cancel the reboot.
            // At the moment, we will ignore the cancel request.
            Log_Info("Cancel requested while system restarting - ignored");
        }
        else if (workflowData->AgentRestartState == ADUC_AgentRestartState_InProgress)
        {
            // It may not be possible, and unsafe, to try to cancel.
            // At the moment, we will ignore the cancel request.
            Log_Info("Cancel requested while agent is restarting - ignored");
        }
        else
        {
            // Cancel without an operation in progress means return to Idle state.
            Log_Info("Cancel received with no operation in progress - returning to Idle state");

            const ADUC_Result result = { .ResultCode = ADUC_Result_Idle_Success };
            ADUC_SetUpdateStateWithResult(workflowData, ADUCITF_State_Idle, result);
        }

        goto done;
    }

    //
    // Not a cancel action.
    //
    const ADUC_WorkflowHandlerMapEntry* entry = GetWorkflowHandlerMapEntryForAction(desiredAction);
    if (entry == NULL)
    {
        Log_Error("Invalid UpdateAction %u -- ignoring", desiredAction);
        goto done;
    }

    //
    // Workaround:
    // Connections to the service may disconnect after a period of time (e.g. 40 minutes)
    // due to the need to refresh a token. When the reconnection occurs, all properties are re-sent
    // to the client, and as such the client might see a duplicate request, for example, another download request
    // after already processing a downloadrequest.
    // We ignore these requests because they have been handled, are currently being handled, or would be a no-op.
    //

    ADUCITF_State lastReportedState = workflow_get_last_reported_state();
    _Bool isDuplicateRequest = IsDuplicateRequest(entry->Action, lastReportedState);
    if (isDuplicateRequest)
    {
        Log_Info(
            "'%s' action received. Last reported state: '%s'. Ignoring this action.",
            ADUCITF_UpdateActionToString(entry->Action),
            ADUCITF_StateToString(lastReportedState));
        goto done;
    }

    // Fail if we have already have an operation in progress.
    // This check happens after the check for duplicates, so we don't log a warning in our logs for an operation
    // that is currently being processed.
    if (workflow_get_operation_in_progress(workflowData->WorkflowHandle))
    {
        Log_Error(
            "Cannot process action '%s' - async operation already in progress.",
            ADUCITF_UpdateActionToString(entry->Action));
        goto done;
    }

    Log_Info("Processing '%s' action", ADUCITF_UpdateActionToString(entry->Action));

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

    workflowData->CurrentAction = entry->Action;

    // Call into the upper-layer method to perform operation..
    workflow_set_operation_in_progress(workflowData->WorkflowHandle, true);

    // Perform an update action.
    result = entry->OperationFunc(methodCallData);

    // Action is complete (i.e. we wont get a WorkCompletionCallback call from upper-layer) if:
    // * Upper-level did the work in a blocking manner.
    // * Method returned failure.
    // NOLINTNEXTLINE(misc-redundant-expression)
    if (!AducResultCodeIndicatesInProgress(result.ResultCode) || IsAducResultCodeFailure(result.ResultCode))
    {
        ADUC_Workflow_WorkCompletionCallback(methodCallData, result);
    }

done:
    return;
}

/**
 * @brief Called when work is complete.
 *
 * @param workCompletionToken ADUC_MethodCall_Data pointer.
 * @param result Result of work.
 */
static void ADUC_Workflow_WorkCompletionCallback(const void* workCompletionToken, ADUC_Result result)
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

    const ADUC_WorkflowHandlerMapEntry* entry = GetWorkflowHandlerMapEntryForAction(workflowData->CurrentAction);
    if (entry == NULL)
    {
        Log_Error("Invalid UpdateAction %u -- ignoring", workflowData->CurrentAction);
        goto done;
    }

    if (entry->Action == ADUCITF_UpdateAction_Cancel)
    {
        Log_Error("Cancel received in completion callback - should not happen!");
        goto done;
    }

    Log_Info(
        "Action '%s' complete. Result: %d (%s), %d (0x%x)",
        ADUCITF_UpdateActionToString(entry->Action),
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
            ADUCITF_UpdateActionToString(entry->Action),
            ADUCITF_StateToString(nextUpdateState));

        ADUC_SetUpdateState(workflowData, nextUpdateState);
    }
    else
    {
        // Operation (e.g. Download) failed or was cancelled - both are considered AducResult failure codes.

        if (workflow_get_operation_cancel_requested(workflowData->WorkflowHandle))
        {
            // Operation cancelled.
            //
            // We are now at the completion of the operation that was cancelled and will just return to Idle state,
            // Ignore the result of the operation, which most likely is cancelled, e.g. ADUC_Result_Failure_Cancelled.
            Log_Error("Operation cancelled - returning to Idle state");

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
                ADUCITF_UpdateActionToString(entry->Action),
                result.ResultCode,
                result.ExtendedResultCode,
                result.ExtendedResultCode);

            ADUC_SetUpdateStateWithResult(
                workflowData,
                ADUCITF_State_Failed,
                result);
        }
    }

done:
    // Operation is now complete.
    workflow_set_operation_in_progress(workflowData->WorkflowHandle, false);
    workflow_set_operation_cancel_requested(workflowData->WorkflowHandle, false);

    free(methodCallData);
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

_Bool IsDuplicateRequest(ADUCITF_UpdateAction action, ADUCITF_State lastReportedState)
{
    _Bool isDuplicateRequest = false;
    switch (action)
    {
    case ADUCITF_UpdateAction_Download:
        isDuplicateRequest =
            (lastReportedState == ADUCITF_State_DownloadStarted
             || lastReportedState == ADUCITF_State_DownloadSucceeded);

        break;
    case ADUCITF_UpdateAction_Install:
        isDuplicateRequest =
            (lastReportedState == ADUCITF_State_InstallStarted || lastReportedState == ADUCITF_State_InstallSucceeded);

        break;
    case ADUCITF_UpdateAction_Apply:
        isDuplicateRequest =
            (lastReportedState == ADUCITF_State_ApplyStarted || lastReportedState == ADUCITF_State_Idle);
        break;
    case ADUCITF_UpdateAction_Cancel:
        // Cancel is considered a duplicate action when in the Idle state.
        // This is because one of the purposes of cancel is to get
        // the client back to the idle state. If the client is already
        // in the idle state, there is no operation to cancel.
        // Also, if a cancel is processed, the client will be in the idle state.
        // If the same cancel is sent/received again, the device would already be in the
        // idle state and the client should take no action on the duplicate cancel.
        isDuplicateRequest = lastReportedState == ADUCITF_State_Idle;
        break;
    default:
        break;
    }

    return isDuplicateRequest;
}
