/**
 * @file adu_core_export_helpers.c
 * @brief Provides set of helpers for creating objects defined in adu_core_exports.h
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/adu_core_export_helpers.h"
#include "aduc/adu_core_interface.h"
#include "aduc/c_utils.h"
#include "aduc/extension_manager.h"
#include "aduc/hash_utils.h"
#include "aduc/logging.h"
#include "aduc/parser_utils.h"
#include "aduc/string_c_utils.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_utils.h"

#include <parson.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sys/wait.h> // for waitpid
#include <unistd.h>

/**
 * @brief Move state machine to a new stage.
 *
 * @param[in,out] workflowData Workflow data.
 * @param[in] updateState New update state to transition to.
 * @param[in] result Result to report (optional, can be NULL).
 */
static void ADUC_SetUpdateStateHelper(
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
                ADUC_SetInstalledUpdateIdAndGoToIdle(workflowData, updateId);

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
                ADUC_MethodCall_Idle(workflowData);
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
                ADUC_MethodCall_Idle(workflowData);
                return;
            }

            // Device failed to reboot, or the agent failed to restart, consider update failed.
            // Fall through to report Idle without InstalledUpdateId.
        }

        if (!AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync(workflowData, updateState, result, NULL /* installedUpdateId */))
        {
            updateState = ADUCITF_State_Failed;
            workflow_set_state(workflowData->WorkflowHandle, ADUCITF_State_Failed);
        }
        else
        {
            ADUC_MethodCall_Idle(workflowData);
        }
    }
    else // Not Idle state
    {
        if (!AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync(workflowData, updateState, result, NULL /* installedUpdateId */))
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
    Log_RequestFlush();
}

/**
 * @brief Set a new update state.
 *
 * @param[in,out] workflowData Workflow data object.
 * @param[in] updateState New update state.
 */
void ADUC_SetUpdateState(ADUC_WorkflowData* workflowData, ADUCITF_State updateState)
{
    ADUC_SetUpdateStateHelper(workflowData, updateState, NULL /*result*/);
}

/**
 * @brief Set a new update state and result.
 *
 * @param[in,out] workflowData Workflow data object.
 * @param[in] updateState New update state.
 * @param[in] result Result to report.
 */
void ADUC_SetUpdateStateWithResult(
    ADUC_WorkflowData* workflowData, ADUCITF_State updateState, ADUC_Result result)
{
    ADUC_SetUpdateStateHelper(workflowData, updateState, &result);
    Log_RequestFlush();
}

/**
 * @brief Sets installedUpdateId to the given update ID and sets state to Idle.
 *
 * @param[in,out] workflowData The workflow data.
 * @param[in] updateId The updateId for the installed content.
 */
void ADUC_SetInstalledUpdateIdAndGoToIdle(ADUC_WorkflowData* workflowData, const char* updateId)
{
    if (!AzureDeviceUpdateCoreInterface_ReportUpdateIdAndIdleAsync(workflowData, updateId))
    {
        Log_Error("Failed to report last installed updateId. Going to idle state.");
    }

    ADUC_WorkflowData_SetLastReportedState(ADUCITF_State_Idle, workflowData);

    ADUC_MethodCall_Idle(workflowData);

    workflowData->SystemRebootState = ADUC_SystemRebootState_None;
    workflowData->AgentRestartState = ADUC_AgentRestartState_None;
}

//
// ADUC_UpdateActionCallbacks helpers.
//

/**
 * @brief Check to see if a ADUC_UpdateActionCallbacks object is valid.
 *
 * @param updateActionCallbacks Object to verify.
 * @return _Bool True if valid.
 */
static _Bool ADUC_UpdateActionCallbacks_VerifyData(const ADUC_UpdateActionCallbacks* updateActionCallbacks)
{
    // Note: Okay for updateActionCallbacks->PlatformLayerHandle to be NULL.

    if (updateActionCallbacks->IdleCallback == NULL
        || updateActionCallbacks->DownloadCallback == NULL
        || updateActionCallbacks->InstallCallback == NULL
        || updateActionCallbacks->ApplyCallback == NULL
        || updateActionCallbacks->SandboxCreateCallback == NULL
        || updateActionCallbacks->SandboxDestroyCallback == NULL
        || updateActionCallbacks->DoWorkCallback == NULL
        || updateActionCallbacks->IsInstalledCallback == NULL)
    {
        Log_Error("Invalid ADUC_UpdateActionCallbacks object");
        return false;
    }

    return true;
}

//
// Register/Unregister methods.
//

/**
 * @brief Call into upper layer ADUC_RegisterPlatformLayer() method.
 *
 * @param updateActionCallbacks Metadata for call.
 * @param argc Count of arguments in @p argv
 * @param argv Arguments to pass to ADUC_RegisterPlatformLayer().
 * @return ADUC_Result Result code.
 */
ADUC_Result
ADUC_MethodCall_Register(ADUC_UpdateActionCallbacks* updateActionCallbacks, unsigned int argc, const char** argv)
{
    Log_Info("Calling ADUC_RegisterPlatformLayer");

    ADUC_Result result = ADUC_RegisterPlatformLayer(updateActionCallbacks, argc, argv);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        return result;
    }

    if (!ADUC_UpdateActionCallbacks_VerifyData(updateActionCallbacks))
    {
        Log_Error("Invalid ADUC_UpdateActionCallbacks structure");

        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOTRECOVERABLE;
        return result;
    }

    result.ResultCode = ADUC_Result_Register_Success;
    return result;
}

/**
 * @brief Call into upper layer ADUC_Unregister() method.
 *
 * @param updateActionCallbacks Metadata for call.
 */
void ADUC_MethodCall_Unregister(const ADUC_UpdateActionCallbacks* updateActionCallbacks)
{
    Log_Info("Calling ADUC_Unregister");

    ADUC_Unregister(updateActionCallbacks->PlatformLayerHandle);
}

/**
 * @brief Call into upper layer ADUC_RebootSystem() method.
 *
 * @param workflowData Metadata for call.
 *
 * @returns int errno, 0 if success.
 */
int ADUC_MethodCall_RebootSystem()
{
    Log_Info("Calling ADUC_RebootSystem");

    return ADUC_RebootSystem();
}

/**
 * @brief Call into upper layer ADUC_RestartAgent() method.
 *
 * @param workflowData Metadata for call.
 *
 * @returns int errno, 0 if success.
 */
int ADUC_MethodCall_RestartAgent()
{
    Log_Info("Calling ADUC_RestartAgent");

    return ADUC_RestartAgent();
}

//
// ADU Core Interface methods.
//

/**
 * @brief Called when entering Idle state.
 *
 * Idle state is the "ready for new workflow state"
 *
 * @param workflowData Workflow metadata.
 */
void ADUC_MethodCall_Idle(ADUC_WorkflowData* workflowData)
{
    const ADUC_UpdateActionCallbacks* updateActionCallbacks = &(workflowData->UpdateActionCallbacks);

    ADUCITF_State lastReportedState = ADUC_WorkflowData_GetLastReportedState(workflowData);

    // Can reach Idle state from ApplyStarted as there isn't an ApplySucceeded state.
    if (lastReportedState != ADUCITF_State_Idle &&
        lastReportedState != ADUCITF_State_ApplyStarted &&
        lastReportedState != ADUCITF_State_Failed)
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
ADUC_Result ADUC_MethodCall_ProcessDeployment(ADUC_MethodCall_Data* methodCallData)
{
    UNREFERENCED_PARAMETER(methodCallData);
    Log_Info("Workflow step: ProcessDeployment");

    ADUC_Result result = { ADUC_Result_Success };
    return result;
}

void ADUC_MethodCall_ProcessDeployment_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result)
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
ADUC_Result ADUC_MethodCall_Download(ADUC_MethodCall_Data* methodCallData)
{
    ADUC_WorkflowData* workflowData = methodCallData->WorkflowData;
    ADUC_WorkflowHandle* workflowHandle = workflowData->WorkflowHandle;
    const ADUC_UpdateActionCallbacks* updateActionCallbacks = &(workflowData->UpdateActionCallbacks);

    ADUCITF_State lastReportedState = ADUC_WorkflowData_GetLastReportedState(workflowData);

    ADUC_Result result = { ADUC_Result_Download_Success };
    char* workFolder = workflow_get_workfolder(workflowHandle);
    char* workflowId = workflow_get_id(workflowHandle);

    Log_Info("Workflow step: Download");

    if (lastReportedState != ADUCITF_State_DeploymentInProgress)
    {
        Log_Error(
            "Download workflow step called in unexpected state: %s!",
            ADUCITF_StateToString(lastReportedState));

        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = ADUC_ERC_UPPERLEVEL_WORKFLOW_UPDATE_ACTION_UNEXPECTED_STATE;
        goto done;
    }

    Log_Info("Calling SandboxCreateCallback");

    // Note: It's okay for SandboxCreate to return NULL for the work folder.
    // NULL likely indicates an OS without a file system.
    result = updateActionCallbacks->SandboxCreateCallback(
        updateActionCallbacks->PlatformLayerHandle, workflowId, workFolder);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    Log_Info("Using sandbox %s", workFolder);

    ADUC_SetUpdateState(workflowData, ADUCITF_State_DownloadStarted);

    result = updateActionCallbacks->DownloadCallback(
        updateActionCallbacks->PlatformLayerHandle, &(methodCallData->WorkCompletionData), workflowData);
    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

done:
    workflow_free_string(workflowId);
    workflow_free_string(workFolder);

    return result;
}

void ADUC_MethodCall_Download_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result)
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
ADUC_Result ADUC_MethodCall_Install(ADUC_MethodCall_Data* methodCallData)
{
    ADUC_WorkflowData* workflowData = methodCallData->WorkflowData;
    const ADUC_UpdateActionCallbacks* updateActionCallbacks = &(workflowData->UpdateActionCallbacks);
    ADUC_Result result = {};

    Log_Info("Workflow step: Install");

    ADUCITF_State lastReportedState = ADUC_WorkflowData_GetLastReportedState(workflowData);
    if (lastReportedState != ADUCITF_State_DownloadSucceeded)
    {
        Log_Error("Install Workflow step called in unexpected state: %s!", ADUCITF_StateToString(lastReportedState));
        result.ResultCode = ADUC_Result_Failure;
        result.ExtendedResultCode = ADUC_ERC_UPPERLEVEL_WORKFLOW_INSTALL_ACTION_IN_UNEXPECTED_STATE;
        goto done;
    }

    ADUC_SetUpdateState(workflowData, ADUCITF_State_InstallStarted);

    Log_Info("Calling InstallCallback");

    result = updateActionCallbacks->InstallCallback(
        updateActionCallbacks->PlatformLayerHandle, &(methodCallData->WorkCompletionData), workflowData);

done:
    return result;
}

void ADUC_MethodCall_Install_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result)
{
    if (result.ResultCode == ADUC_Result_Install_RequiredImmediateReboot
        || result.ResultCode == ADUC_Result_Install_RequiredReboot)
    {
        // If 'install' indicated a reboot required result from apply, go ahead and reboot.
        Log_Info("Install indicated success with RebootRequired - rebooting system now");
        methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_Required;

        RebootSystemFunc rebootFn = ADUC_WorkflowData_GetRebootSystemFunc(methodCallData->WorkflowData);

        int success = (*rebootFn)();
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
        result.ResultCode == ADUC_Result_Install_RequiredImmediateAgentRestart
        || result.ResultCode == ADUC_Result_Install_RequiredAgentRestart)
    {
        // If 'install' indicated a restart is required, go ahead and restart the agent.
        Log_Info("Install indicated success with AgentRestartRequired - restarting the agent now");
        methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_Required;

        RestartAgentFunc restartAgentFn = ADUC_WorkflowData_GetRestartAgentFunc(methodCallData->WorkflowData);

        int success = (*restartAgentFn)();
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
 * @brief Called to do apply.
 *
 * @param[in] methodCallData - the method call data.
 * @return Result code.
 */
ADUC_Result ADUC_MethodCall_Apply(ADUC_MethodCall_Data* methodCallData)
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

    ADUC_SetUpdateState(workflowData, ADUCITF_State_ApplyStarted);

    Log_Info("Calling ApplyCallback");

    result = updateActionCallbacks->ApplyCallback(
        updateActionCallbacks->PlatformLayerHandle, &(methodCallData->WorkCompletionData), workflowData);

done:
    return result;
}

void ADUC_MethodCall_Apply_Complete(ADUC_MethodCall_Data* methodCallData, ADUC_Result result)
{
    if (result.ResultCode == ADUC_Result_Apply_RequiredReboot ||
        result.ResultCode == ADUC_Result_Apply_RequiredImmediateReboot)
    {
        // If apply indicated a reboot required result from apply, go ahead and reboot.
        Log_Info("Apply indicated success with RebootRequired - rebooting system now");
        methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_Required;

        RebootSystemFunc rebootFn = ADUC_WorkflowData_GetRebootSystemFunc(methodCallData->WorkflowData);

        int success = (*rebootFn)();
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
    else if (result.ResultCode == ADUC_Result_Apply_RequiredAgentRestart ||
        result.ResultCode == ADUC_Result_Apply_RequiredImmediateAgentRestart)
    {
        // If apply indicated a restart is required, go ahead and restart the agent.
        Log_Info("Apply indicated success with AgentRestartRequired - restarting the agent now");
        methodCallData->WorkflowData->SystemRebootState = ADUC_SystemRebootState_Required;

        RestartAgentFunc restartAgentFn = ADUC_WorkflowData_GetRestartAgentFunc(methodCallData->WorkflowData);

        int success = (*restartAgentFn)();
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
 * @brief Called to request platform-layer operation to cancel.
 *
 * This method should only be called while another MethodCall is currently active.
 *
 * @param[in] methodCallData - the method call data.
 */
void ADUC_MethodCall_Cancel(const ADUC_WorkflowData* workflowData)
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
ADUC_Result ADUC_MethodCall_IsInstalled(const ADUC_WorkflowData* workflowData)
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
