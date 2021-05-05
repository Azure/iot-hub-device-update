/**
 * @file agent_workflow.c
 * @brief Handles workflow requests coming in from the hub.
 *
 * The cloud-based orchestrator (CBO) holds the state machine, so the best we can do in this agent
 * is to react to the CBO update actions, and see if we think we're in the correct state.
 * If we are, we'll call an upper-level method to do the work.
 * If not, we'll fail the request.
 *
 *                    ┌───┐                                     ┌──────┐
 *                    │CBO│                                     │Client│
 *                    └─┬─┘                                     └──┬───┘
 *                      │          UpdateAction: Download          │
 *                      │ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─>
 *                      │                                          │
 *                      │                                          │────┐
 *                      │                                          │    │ DownloadStarted (Internal Client State)
 *                      │                                          │<───┘
 *                      │                                          │
 *                      │                                          │────┐
 *                      │                                          │    │ Download content
 *                      │                                          │<───┘
 *                      │                                          │
 *                      │                                          │
 *          ╔══════╤════╪══════════════════════════════════════════╪═════════════╗
 *          ║ ALT  │  Successful Download                          │             ║
 *          ╟──────┘    │                                          │             ║
 *          ║           │      UpdateState: DownloadSucceeded      │             ║
 *          ║           │<──────────────────────────────────────────             ║
 *          ╠═══════════╪══════════════════════════════════════════╪═════════════╣
 *          ║ [Failed Download]                                    │             ║
 *          ║           │           UpdateState: Failed            │             ║
 *          ║           │<──────────────────────────────────────────             ║
 *          ║           │                                          │             ║
 *          ║           │           UpdateAction: Cancel           │             ║
 *          ║           │ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─>             ║
 *          ║           │                                          │             ║
 *          ║           │            UpdateState: Idle             │             ║
 *          ║           │<──────────────────────────────────────────             ║
 *          ╠═══════════╪══════════════════════════════════════════╪═════════════╣
 *          ║ [Cancel received during "Download content"]          │             ║
 *          ║           │            UpdateState: Idle             │             ║
 *          ║           │<──────────────────────────────────────────             ║
 *          ╚═══════════╪══════════════════════════════════════════╪═════════════╝
 *                      │                                          │
 *                      │          UpdateAction: Install           │
 *                      │ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─>
 *                      │                                          │
 *                      │                                          │────┐
 *                      │                                          │    │ InstallStarted (Internal Client State)
 *                      │                                          │<───┘
 *                      │                                          │
 *                      │                                          │────┐
 *                      │                                          │    │ Install content
 *                      │                                          │<───┘
 *                      │                                          │
 *                      │UpdateState: InstallSucceeded (on success)│
 *                      │<──────────────────────────────────────────
 *                      │                                          │
 *                      │           UpdateAction: Apply            │
 *                      │ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─>
 *                      │                                          │
 *                      │                                          │────┐
 *                      │                                          │    │ ApplyStarted (Internal Client State)
 *                      │                                          │<───┘
 *                      │                                          │
 *                      │                                          │────┐
 *                      │                                          │    │ Apply content
 *                      │                                          │<───┘
 *                      │                                          │
 *                      │      UpdateState: Idle (on success)      │
 *                      │<──────────────────────────────────────────
 *                    ┌─┴─┐                                     ┌──┴───┐
 *                    │CBO│                                     │Client│
 *                    └───┘                                     └──────┘
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#include "aduc/agent_workflow.h"

#include <stdlib.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h> // PRIu64
#include <time.h>

#include "aduc/adu_core_export_helpers.h"
#include "aduc/adu_core_interface.h"
#include "aduc/c_utils.h"
#include "aduc/logging.h"
#include "aduc/result.h"

#include "agent_workflow_utils.h"

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
 * @brief Creates and initializes a new ADUC_ContentData object from the provided JSON.
 *
 * @param updateActionJson The JSON to use to populate fields in the ADUC_ContentData object.
 * @return ADUC_ContentData* The new ADUC_ContentData. Must be freed with ADUC_ContentData_Free by caller.
 */
ADUC_ContentData* ADUC_ContentData_AllocAndInit(const JSON_Value* updateActionJson)
{
    _Bool succeeded = false;
    ADUC_ContentData* newContentData = calloc(1, sizeof(ADUC_ContentData));
    if (newContentData == NULL)
    {
        goto done;
    }

    if (!ADUC_ContentData_Update(newContentData, updateActionJson, true))
    {
        goto done;
    }

    succeeded = true;

done:
    if (!succeeded)
    {
        ADUC_ContentData_Free(newContentData);
        newContentData = NULL;
    }

    return newContentData;
}

// TODO(Nic): 29650829: Need to investigate why ADUC_ContentData_Update doesn't requires all fields.

/**
 * @brief Updates the provided ADUC_ContentData object with values from the provided JSON.
 *
 * @param[in,out] contentData The ADUC_ContentData object to update.
 * @param[in] updateActionJson The JSON to use to update the ADUC_ContentData.
 * @param[in,opt] requiredAllData The boolean indicates whether all data are required. Default is 'false'.
 * @return _Boolean A boolean indicates whether the content is updated successfully.
 */
_Bool ADUC_ContentData_Update(ADUC_ContentData* contentData, const JSON_Value* updateActionJson, _Bool requiredAllData)
{
    _Bool succeeded = false;

    unsigned int desiredAction;

    if (!ADUC_Json_GetUpdateAction(updateActionJson, &desiredAction))
    {
        Log_Error("Missing Action property.");
        goto done;
    }

    // Cancel actions don't contain any ContentData related fields.
    if (desiredAction == ADUCITF_UpdateAction_Cancel)
    {
        succeeded = true;
        goto done;
    }

    ADUC_UpdateId* expectedUpdateId = NULL;
    if (ADUC_Json_GetUpdateId(updateActionJson, &expectedUpdateId))
    {
        ADUC_UpdateId_Free(contentData->ExpectedUpdateId);
        contentData->ExpectedUpdateId = expectedUpdateId;
    }
    else if (requiredAllData)
    {
        Log_Error("Missing UpdateId property.");
        goto done;
    }

    char* installedCriteria = NULL;
    if (ADUC_Json_GetInstalledCriteria(updateActionJson, &installedCriteria))
    {
        free(contentData->InstalledCriteria);
        contentData->InstalledCriteria = installedCriteria;
    }
    else if (requiredAllData)
    {
        Log_Error("Missing installedCriteria property.");
        goto done;
    }

    char* updateType = NULL;
    if (ADUC_Json_GetUpdateType(updateActionJson, &updateType))
    {
        free(contentData->UpdateType);
        contentData->UpdateType = updateType;
    }
    else if (requiredAllData)
    {
        Log_Error("Missing UpdateType property.");
        goto done;
    }

    succeeded = true;

done:
    return succeeded;
}

/**
 * @brief Frees the provided ADUC_ContentData object and its members.
 *
 * @param[in,out] contentData The ADUC_ContentData to free.
 */
void ADUC_ContentData_Free(ADUC_ContentData* contentData)
{
    if (contentData == NULL)
    {
        return;
    }

    ADUC_UpdateId_Free(contentData->ExpectedUpdateId);
    free(contentData->InstalledCriteria);
    free(contentData->UpdateType);
    free(contentData);
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

    workflowData->LastReportedState = ADUCITF_State_Idle;

    ADUC_Result result = ADUC_MethodCall_Register(&(workflowData->RegisterData), argc, (const char**)argv);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("ADUC_Register failed %d, %d", result.ResultCode, result.ExtendedResultCode);
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
 * @brief Initializes or updates the ADUC_WorkflowData#ContentData object with the provided JSON.
 *
 * @param[in,out] workflowData The ADUC_WorkflowData object to update.
 * @returns true if the contentData is successfully allocated and set, false if it fails to allocate the values
 * this function always returns true when it is updating a workflowData object with an existing (non-null) ContentData member
 */
_Bool ADUC_WorkflowData_UpdateContentData(ADUC_WorkflowData* workflowData)
{
    _Bool success = false;

    if (workflowData->ContentData == NULL)
    {
        workflowData->ContentData = ADUC_ContentData_AllocAndInit(workflowData->UpdateActionJson);

        if (workflowData->ContentData == NULL)
        {
            goto done;
        }
    }
    else
    {
        ADUC_ContentData_Update(workflowData->ContentData, workflowData->UpdateActionJson, false);
    }

    success = true;

done:

    return success;
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
    const ADUC_RegisterData* registerData = &(workflowData->RegisterData);

    registerData->DoWorkCallback(registerData->Token, workflowData->WorkflowId);
}

/**
 * @brief Free members of ADUC_WorkflowData object.
 *
 * @param workflowData Object whose members should be freed.
 */
void ADUC_WorkflowData_Free(ADUC_WorkflowData* workflowData)
{
    if (workflowData == NULL)
    {
        return;
    }

    free(workflowData->WorkFolder);

    ADUC_ContentData_Free(workflowData->ContentData);

    if (workflowData->IsRegistered)
    {
        ADUC_MethodCall_Unregister(&(workflowData->RegisterData));
    }

    memset(workflowData, 0, sizeof(*workflowData));
}

static void ADUC_Workflow_WorkCompletionCallback(const void* workCompletionToken, ADUC_Result result);

void ADUC_Workflow_HandleStartupWorkflowData(ADUC_WorkflowData* workflowData)
{
    // The default result for Idle state.
    // This will reset twin status code to 200 to indicate that we're successful (so far).
    const ADUC_Result result = { .ResultCode = ADUC_IdleResult_Success };

    if (workflowData == NULL || workflowData->ContentData == NULL)
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

    //
    // The following logic handles the case where the device (or adu-agent) restarted.
    //
    // If IsInstalled returns true, we will assume that the update was succeeded.
    //
    // In this case, we will update the 'InstalledContentId' to match 'ExpectedContentId'
    // and transition to Idle state.
    ADUC_Result isInstalledResult = ADUC_MethodCall_IsInstalled(workflowData);
    if (isInstalledResult.ResultCode == ADUC_IsInstalledResult_Installed)
    {
        Log_Info(
            "IsInstalled call was true - setting installedUpdateId to %s:%s:%s and setting state to Idle",
            workflowData->ContentData->ExpectedUpdateId->Provider,
            workflowData->ContentData->ExpectedUpdateId->Name,
            workflowData->ContentData->ExpectedUpdateId->Version);
        ADUC_SetInstalledUpdateIdAndGoToIdle(workflowData, workflowData->ContentData->ExpectedUpdateId);
        goto done;
    }

    if (IsAducResultCodeFailure(isInstalledResult.ResultCode))
    {
        Log_Warn(
            "IsInstalled call failed. ExtendedResultCode: %d - setting state to Idle",
            isInstalledResult.ExtendedResultCode);
    }
    else
    {
        Log_Info("The installed criteria is not met. The current update was not installed on the device.");

        unsigned int desiredAction;
        if (!ADUC_Json_GetUpdateAction(workflowData->UpdateActionJson, &desiredAction))
        {
            goto done;
        }

        if (desiredAction == ADUCITF_UpdateAction_Download)
        {
            Log_Info("There's a pending 'download' action request.");

            // There's a pending download request.
            // We need to make sure we don't change our state to 'idle'.
            workflowData->StartupIdleCallSent = true;

            ADUC_Workflow_HandleUpdateAction(workflowData);
            goto done;
        }
    }

    ADUC_SetUpdateStateWithResult(workflowData, ADUCITF_State_Idle, result);

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
    // char cast because incoming response might be binary data in the future?
    workflowData->UpdateActionJson = ADUC_Json_GetRoot((const char*)propertyUpdateValue);

    //
    // TODO(NIC): 29565671: Cancel messages don't send a signature. Need to add unit tests.
    //
    unsigned int desiredAction;
    if (!ADUC_Json_GetUpdateAction(workflowData->UpdateActionJson, &desiredAction))
    {
        Log_Error("Cannot determine the desired update action. Update data: (%s)", propertyUpdateValue);

        //
        // TODO(NIC): 31481186 Refactor reporting so failures are reported in line instead of waiting for the state change to happen
        //
        ADUC_Result result = { .ResultCode = ADUC_LowerLayerResult_Failure,
                               .ExtendedResultCode = ADUC_ERC_LOWERLEVEL_UPDATE_MANIFEST_VALIDATION_INVALID_HASH };
        ADUC_SetUpdateStateWithResult(workflowData, ADUCITF_State_Failed, result);
        return;
    }

    if (desiredAction != ADUCITF_UpdateAction_Cancel)
    {
        if (!ADUC_Json_ValidateManifest(workflowData->UpdateActionJson))
        {
            Log_Error("Invalid Manifest received, manifest str: %s", propertyUpdateValue);

            //
            // TODO(NIC): 31481186 Refactor reporting so failures are reported in line instead of waiting for the state change to happen
            //
            ADUC_Result result = { .ResultCode = ADUC_LowerLayerResult_Failure,
                                   .ExtendedResultCode = ADUC_ERC_LOWERLEVEL_INVALID_UPDATE_ACTION };
            ADUC_SetUpdateStateWithResult(workflowData, ADUCITF_State_Failed, result);
            return;
        }
    }

    ADUC_WorkflowData_UpdateContentData(workflowData);

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
}

/**
 * @brief Handle an incoming update action.
 *
 * @param workflowData Workflow metadata.
 */
void ADUC_Workflow_HandleUpdateAction(ADUC_WorkflowData* workflowData)
{
    if (workflowData->UpdateActionJson == NULL)
    {
        goto done;
    }

    //
    // Find the WorkflowHandlerMapEntry for the desired update action.
    //

    unsigned int desiredAction;
    if (!ADUC_Json_GetUpdateAction(workflowData->UpdateActionJson, &desiredAction))
    {
        goto done;
    }

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
        if (workflowData->OperationInProgress)
        {
            Log_Info("Cancel requested - notifying operation in progress.");

            // Set OperationCancelled so that when the operation in progress completes, it's clear
            // that it was due to a cancellation.
            // We will ignore the result of what the operation in progress returns when it completes
            // in cancelled state.
            workflowData->OperationCancelled = true;

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

            const ADUC_Result result = { .ResultCode = ADUC_IdleResult_Success };
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

    ADUCITF_State lastReportedState = workflowData->LastReportedState;
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
    if (workflowData->OperationInProgress)
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
    workflowData->OperationInProgress = true;

    // At the start of the workflow, that is before we download, ask the upper level handler
    // to verify the metadata of the update as part of a "prepare" step

    bool shouldCallOperationFunc = true;
    ADUC_Result result;

    if (entry->Action == ADUCITF_UpdateAction_Download)
    {
        // Generate workflowId when we start downloading.
        GenerateUniqueId(workflowData->WorkflowId, ARRAY_SIZE(workflowData->WorkflowId));
        Log_Info("Start the workflow - downloading, with WorkflowId %s", workflowData->WorkflowId);

        result = ADUC_MethodCall_Prepare(workflowData);
        shouldCallOperationFunc = IsAducResultCodeSuccess(result.ResultCode);
    }

    if (shouldCallOperationFunc)
    {
        result = entry->OperationFunc(methodCallData);
    }

    // Action is complete (i.e. we wont get a WorkCompletionCallback call from upper-layer) if:
    // * Upper-level did the work in a blocking manner.
    // * Method returned failure.
    // NOLINTNEXTLINE(misc-redundant-expression)
    if (!AducResultCodeIndicatesInProgress(result.ResultCode) || IsAducResultCodeFailure(result.ResultCode))
    {
        ADUC_Workflow_WorkCompletionCallback(methodCallData, result);
    }

done:
    if (workflowData->UpdateActionJson != NULL)
    {
        json_value_free(workflowData->UpdateActionJson);
        workflowData->UpdateActionJson = NULL;
    }
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
        "Action '%s' complete. Result: %d, %d",
        ADUCITF_UpdateActionToString(entry->Action),
        result.ResultCode,
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

        if (workflowData->OperationCancelled)
        {
            // Operation cancelled.
            //
            // We are now at the completion of the operation that was cancelled and will just return to Idle state,
            // Ignore the result of the operation, which most likely is cancelled, e.g. ADUC_DownloadResult_Cancelled.

            Log_Error("Operation cancelled - returning to Idle state");

            const ADUC_Result result = { .ResultCode = ADUC_IdleResult_Success };
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
                "%s failed. error %d, %d - Expecting service to send Cancel action",
                ADUCITF_UpdateActionToString(entry->Action),
                result.ResultCode,
                result.ExtendedResultCode);

            ADUC_SetUpdateStateWithResult(workflowData, ADUCITF_State_Failed, result);
        }
    }

done:
    // Operation is now complete.
    workflowData->OperationInProgress = false;
    workflowData->OperationCancelled = false;

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
