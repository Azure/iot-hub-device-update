# Goal State Deployment Support in Device Update Agent

## What's **Goal State**

Goal state is the new update workflow processing protocol for dynamic deployments.
Devices belong to a device group and each device group has an update goal shared across all devices in the group.

As devices
are added to the group, the service will push an UpdateAction of ProcessDeployment(3) to the DU Agent on the device along with data required for all phases (download, install, apply) for processing the update.

The agent does not report the final update status until all the update workflow
steps (download, install, apply) have completed, or a failure has occurred.

Note:

With previous, pre-public-preview protocol, the cloud would push UpdateAction for each individual step (e.g. download, install) and the agent would report to the cloud after every workflow step completion.

## Agent's Goal State Workflow


Processing of the goal state update workflow starts with the service updating the digital twin's desired section
with ProcessWorkflow UpdateAction. See `ADUCITF_UpdateAction` enum in [update_content.h](../../src/adu_types/inc/aduc/types/update_content.h).

The Agent state machine will go through the different steps locally without further communication with the cloud.

Either on failure, or once the entire workflow processing is done and agent is in Idle state again, the Agent will report the results of the workflow processing to the cloud. Until this occurs, the UX in the portal will show the workflow as "In-Progress".

## Workflow Processing State Machine

### Workflow Steps
There are logical steps of the workflow processing.  The update workflow processing state machine steps are defined in the `ADUCITF_WorkflowStep` enum in [update_content.h](../../src/adu_types/inc/aduc/types/update_content.h).

### Workflow Step High-level Details
Here is each workflow step and what it does at a high-level:
- `ADUCITF_WorkflowStep_ProcessDeployment`
    - Sends cloud an acknowledgement of `ADUCITF_State_DeploymentInProgress(6)` along with the workflowId that was sent by the cloud in the desired section of twin
    - Transitions to `ADUCITF_State_DeploymentInProgress` state
    - Auto-Transitions to `ADUCITF_WorkflowStep_Download`
- `ADUCITF_WorkflowStep_Download`
    - Sets current state to `ADUCITF_State_DownloadStarted`
    - Kicks off Download worker thread, which will call the content handler's `Download()` method to download the update content to the work folder download sandbox.
    - On success, it will set the state to `ADUCITF_State_DownloadCompleted` and auto-transitions to `ADUCITF_WorkflowStep_InstallStarted`
    - On failure, it sets the state to `ADUCITF_State_Failed` and reports the failure.
- `ADUCITF_WorkflowStep_Install`
    - Sets the current state to `ADUCITF_State_InstallStarted`
    - Kicks off Install worker thread, which will call the content handler's `Install()` method to install the content that is in the work folder
    - On success, it will:
        - Reboot the system if the result code is either  
            - `ADUC_Result_Install_RequiredImmediateReboot` or
            - `ADUC_Result_Install_RequiredReboot`
        - Restart the agent if the result code is either
            - `ADUC_Result_Install_RequiredImmediateAgentRestart` or
            - `ADUC_Result_Install_RequiredAgentRestart`
        - Otherwise, it will Auto-transition to `ADUCITF_WorkflowStep_Apply`
- `ADUCITF_WorkflowStep_Apply`
    - Sets the current state to `ADUCITF_State_ApplyStarted`
    - Kicks off Apply worker thread, which will call the content handler's `Apply()` method
    - On success, it will:
        - Reboot the system if the result code is either  
            - `ADUC_Result_Apply_RequiredImmediateReboot` or
            - `ADUC_Result_Apply_RequiredReboot`
        - Restart the agent if the result code is either
            - `ADUC_Result_Apply_RequiredImmediateAgentRestart` or
            - `ADUC_Result_Apply_RequiredAgentRestart`
        - Otherwise, it will auto-transition to idle state

When transitioning to Idle from ApplyStarted state, the agent will report the result and installedUpdateId.

Prior to restart or reboot, the agent persists state to be able to report to the cloud after transitioning to idle.

After restart or reboot, the agent will call the content handler's `IsInstalled()` method to get the result of `Install()` or `Apply()`.

If result is `ADUC_Result_IsInstalled_NotInstalled`, then the agent will continue processing the workflow at the `ADUCITF_WorkflowStep_Install` workflow step.

If result is `ADUC_Result_IsInstalled_Installed`, then the agent will transition to Idle state, which will report the result of the install/apply and the installed UpdateId to the cloud.

### State Machine States
The states of the state machine are defined in the `ADUCITF_State` enum in [update_content.h](../../src/adu_types/inc/aduc/types/update_content.h).

### State Transitions

A state transition is describe by the `ADUC_WorkflowHandlerMapEntry` struct in [agent_workflow.c](../../src/agent/adu_core_interface/src/agent_workflow.c).

The state transitions are defined in the `workflowHandlerMap` array in [agent_workflow.c](../../src/agent/adu_core_interface/src/agent_workflow.c).

### State Machine Diagram
![State Machine Diagram](images/goalstate-state-diagram.png)

## Handling Service-Initiated Replacement

If the service operator initiates a new goal state for the device group before a device completes processing the current workflow, the agent will try to cancel the current processing.

Once processing is done, or cancel completes, it will start processing the new workflow.

Note:

The content handler's Cancel method should be able to cancel processing of other methods such as Download.

## Handling Service-Initiated Retry

Similarly, if the service operator issues a retry, the agent will cancel any current workflow processing and restart the processing.

## Handling Service-Initiated Cancel Request

When a service operator issues a cancellation of the current workflow processing, the agent will attempt to cancel it. This leads to a call to the Cancel method of the ContentHandler that is registered for the update's update type.

The content handler should interupt the current in-progress operation (e.g. Download) so that the operation exits and calls WorkCompletionCallback to complete the current operation.

The agent will then report to the cloud a result code of ADUC_Result_Failure_Cancelled(-1) and transition to Idle state.
