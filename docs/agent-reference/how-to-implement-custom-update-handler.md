# How To Implement A Custom Step Handler Extension

## What's A Step Handler Extension

Device Update agent supports a Step Handler (a.k.a, Step Handler) extension, which enables the Agent to support multiple Handler Types at the same time.

Device builders can define a custom Update Type, implement the associated custom Step Handler, and register it, if needed for their additional device updates and deployment scenarios.

## Requirements

1. An extension must be a Linux shared-library that implements and exports following C function:

```c

/**
 * @brief Instantiates a Step Handler.
 * @return A pointer to an instantiated Step Handler object.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel);

```

This function must return a C++ class object that derived from **ContentHandler** abstract class.

See **ContentHandler** class definition in  [content_handler.hpp](../../src/extensions/inc/aduc/content_handler.hpp) for details.

2. The custom step handler must implement all virtual functions. See the details of each functions below:

|Function| Purpose| Return Values|
|----------|----------|----------|
| Download | Handles a 'download' task invoked by DU Agent workflow.<br/><br/>The downloaded file must be stored in a work folder specified in [**ADUC_WorkflowData**](../../src/adu_types/inc/aduc/types/workflow.h) | For success cases, the **ResultCode** field of the **ADUC_Result** struct can be one of the following values:<br/><br/>    ADUC_Result_Download_Success<br/>ADUC_Result_Download_InProgress<br/>ADUC_Result_Download_Skipped_FileExists<br/>ADUC_Result_Download_Skipped_UpdateAlreadyInstalled<br/>ADUC_Result_Download_Skipped_NoMatchingComponents<br/><br/>See [adu_core.h](../../src/adu_types/inc/aduc/types/adu_core.h) for more details |
| Install | Handles an 'install' task invoked by DU Agent workflow.<br/><br/>The install task usually includes a process where the handler invokes external tool or command to install the downloaded update payload file(s) to the desired target.<br/>For example, for 'microsoft/apt' update, the handler could invoke following command:<br/><br/> <b>`apt-get install <options> <list of packages to install>`</b> | For success cases, the **ResultCode** field of the **ADUC_Result** struct can be one of the following values:<br/><br/>    ADUC_Result_Install_Success<br/>ADUC_Result_Install_InProgress<br/>ADUC_Result_Install_Skipped_UpdateAlreadyInstalled<br/>ADUC_Result_Install_Skipped_NoMatchingComponents <br/>ADUC_Result_Install_RequiredImmediateReboot <br/>ADUC_Result_Install_RequiredReboot<br/>ADUC_Result_Install_RequiredImmediateAgentRestart<br/>ADUC_Result_Install_RequiredAgentRestart<br/><br/>See [adu_core.h](../../src/adu_types/inc/aduc/types/adu_core.h) for more details |
| Apply | Handles an 'apply' task invoked by DU Agent workflow.<br/><br/>The apply task usually includes one or more additional steps after an 'install' task has completed. Such as, validating installed items, restart system service, update configuration files, persist some meta data related to the update process.<br/><br/> The DU Agent workflow will consider the update complete successfully only when an apply task returns ADUC_Result_Apply_Success | For success cases, the **ResultCode** field of the **ADUC_Result** struct can be one of the following values:<br/><br/>    ADUC_Result_Apply_Success <br/>ADUC_Result_Apply_InProgress<br/>ADUC_Result_Apply_RequiredImmediateReboot<br/>ADUC_Result_Apply_RequiredReboot <br/>ADUC_Result_Apply_RequiredImmediateAgentRestart <br/>ADUC_Result_Apply_RequiredAgentRestart<br/><br/>See [adu_core.h](../../src/adu_types/inc/aduc/types/adu_core.h) for more details |
| Cancel | Handles a 'cancel' task invoked by DU Agent workflow.<br/><br/>The cancel task usually initiated by the Device Update Service. When an Agent received a cancel request, the Agent workflow will relay this request to an active Step Handler that's currently processing the deployment. The handler should try to gracefully cancelling current task, and return appropriate result. | For success cases, the **ResultCode** field of the **ADUC_Result** struct can be one of the following values:<br/><br/>    ADUC_Result_Cancel_Success <br/>ADUC_Result_Cancel_UnableToCancel<br/<br/>See [adu_core.h](../../src/adu_types/inc/aduc/types/adu_core.h) for more details |
| IsInstalled | Probe that lets the orchestrator (and the Steps Handler, when this handler is used as a step) decide whether the update described by [**ADUC_WorkflowData**](../../src/adu_types/inc/aduc/types/workflow.h) is already present on the target device or component, so it can skip work that has already been done. **Called multiple times per deployment** (see [IsInstalled call sites and authoring guidance](#isinstalled-call-sites-and-authoring-guidance) below) — must be cheap, side-effect-free, and idempotent. The handler compares the workflow's `installedCriteria` against handler-scoped state (e.g., installed package version for `microsoft/apt:1`, file hash on disk for `microsoft/swupdate:2`) and returns the result; it must **not** consult system-wide state outside the scope of this handler's content. | The **ResultCode** field of the **ADUC_Result** struct can be one of the following values:<br/><br/>    ADUC_Result_IsInstalled_Installed (`900`)<br/>ADUC_Result_IsInstalled_NotInstalled (`901`)<br/><br/>A `ResultCode` of `0` is `ADUC_Result_Failure`. At each per-step call site the Steps Handler only proceeds when the return is exactly `ADUC_Result_IsInstalled_Installed`, so any other value (including a failure) causes the workflow to fall through and perform the step. See [adu_core.h](../../src/adu_types/inc/aduc/types/adu_core.h) for more details. |

## Consuming ADUC_WorkflowData

As noted above, DU Agent Core passes an [**ADUC_WorkflowData**](../../src/adu_types/inc/aduc/types/workflow.h) object when invoking a function provided by the handler. A '**WorkflowHandle**' field of the  [**ADUC_WorkflowData**](../../src/adu_types/inc/aduc/types/workflow.h) object contains all data needed to perform each task.

A group of helper functions declared in workflow_utils.h can be used to query workflow data from the WorkflowHandle.

Below, you can find an example of useful helper functions used in most handlers provided in this project.

| Function | Purpose |
|:---|:---|
|workflow_get_update_id|Get the id of an update currently being processed
|workflow_get_update_type| Get the type of an update being processed |
|workflow_get_update_files_count|Get total update payload files count|
|workflow_get_update_file| Get a file entity information.<br/>See [ADUC_FileEntity](../../src/adu_types/inc/aduc/types/update_content.h) for more info.
workflow_get_installed_criteria|Get the 'installed' criteria string|

> See [workflow_utils.h](../../src/utils/workflow_utils/inc/aduc/workflow_utils.h) for full list of the helper functions.

## Implementing A 'Component-Aware' Step Handler

Usually, a step handler is designed to install an update content on a Host Device. An example of this is the [APT Update Handler](../../src/extensions/step_handlers/apt_handler/README.md) provided in this project, which installs one or more Debian packages on the host device.

## Contract Enforced By The Steps Handler

For multi-step manifests, the **Steps Handler** (the default Update Manifest Handler — see [steps-handler.md](steps-handler.md)) drives your step handler through the per-step phases. Custom step handlers that participate in MSOE must honor the following contract; failure to do so will produce surprising behavior in the parent workflow.

### Cancel polling

The Steps Handler only checks `workflow_is_cancel_requested` at phase boundaries (entry into Download / Install / Apply, and after the entire (component × step) loop completes). Once your step's `Install` returns control, the parent re-checks; **but while your step is executing, no parent-level preemption happens.** Long-running operations must poll cancel themselves:

```c
#include <aduc/workflow_utils.h>

if (workflow_is_cancel_requested(workflowHandle)) {
    return ADUC_Result{ ADUC_Result_Cancel_Success, 0 };
}
```

If your handler cannot interrupt the in-flight operation, it must return `ADUC_Result_Cancel_UnableToCancel` from `Cancel`.

### Reboot and agent-restart result codes

When your step's `Install` or `Apply` returns one of the reboot / restart result codes, the Steps Handler interprets them as follows. Your handler must **not** itself call `reboot()` or restart the agent — that is the orchestrator's responsibility.

| Result code | Steps Handler behavior |
|---|---|
| `ADUC_Result_Install_RequiredImmediateReboot` <br/> `ADUC_Result_Apply_RequiredImmediateReboot` | Aborts all remaining steps and components; the agent reboots after the Apply phase reports state. |
| `ADUC_Result_Install_RequiredReboot` <br/> `ADUC_Result_Apply_RequiredReboot` | Marks the workflow as reboot-pending, **breaks the inner step loop for the current component**, and continues with the next component. The reboot is deferred until the workflow completes. |
| `ADUC_Result_Install_RequiredImmediateAgentRestart` <br/> `ADUC_Result_Apply_RequiredImmediateAgentRestart` | Aborts all remaining steps; agent restart is requested after the Apply phase reports state. |
| `ADUC_Result_Install_RequiredAgentRestart` <br/> `ADUC_Result_Apply_RequiredAgentRestart` | Deferred restart, same per-component break semantics as deferred reboot. |

### "Already installed" semantics

If your `Install` returns `ADUC_Result_Install_Skipped_UpdateAlreadyInstalled`, the Steps Handler treats it as success **for that step only** and continues with the next step. It does **not** short-circuit the rest of the workflow.

Similarly, `ADUC_Result_Install_Skipped_NoMatchingComponents` (returned when the target component isn't present) is treated as success for that step.

### Result-code propagation

Result codes use the `ADUC_Result.ResultCode` enum from [adu_core.h](../../src/adu_types/inc/aduc/types/adu_core.h). **A `ResultCode` value of `0` is `ADUC_Result_Failure` — it does *not* mean success.** Each phase has its own success codes (e.g., `ADUC_Result_Install_Success = 600`); see the table earlier in this document for the per-phase set.

If your handler fails, populate `ADUC_Result.ExtendedResultCode` with a meaningful ERC (4-byte facility/component/code) so failures can be triaged from the cloud. See [device-update-agent-extended-result-codes.md](device-update-agent-extended-result-codes.md).

### Apply happens inside Install

For the parent workflow, the Steps Handler's `Apply` is a no-op — Apply for each individual step is invoked **inside** the parent's `Install` phase, immediately after that step's `Install` succeeds. If you implement a custom Update Manifest Handler (rare), you must replicate this convention or steps will never be applied.

### Per-component invocation

When a step targets selected components, the Steps Handler invokes your handler **once per component** with `selectedComponents` containing a single component (not a list). Iterate through your work for that one component and return; the parent loops to the next component.

If `selectedComponentsCount == 0` for an inline step at the parent level, the step is treated as **optional** and skipped (not failed).

### IsInstalled call sites and authoring guidance

`IsInstalled` is **not** invoked once per deployment. The agent core and the Steps Handler call it several times to decide whether the update — or any individual step — can be skipped. Authors must therefore treat it as a **cheap, side-effect-free, idempotent probe**. In the current `develop` code the call sites are:

| Call site | Code reference | Why |
|---|---|---|
| Agent core, on startup when a `ProcessDeployment` action is already pending in the twin | `ADUC_Workflow_HandleStartupWorkflowData` &rarr; `ADUC_Workflow_MethodCall_IsInstalled` in [`agent_workflow.c`](../../src/adu_workflow/src/agent_workflow.c) | Avoids re-running an update that is already installed after an agent or host restart. |
| Agent core, during normal `ProcessDeployment` handling — after the duplicate-deployment / already-completed-workflow guards in `ADUC_Workflow_HandleUpdateAction` | `ADUC_Workflow_HandleUpdateAction` &rarr; `ADUC_Workflow_MethodCall_IsInstalled` in [`agent_workflow.c`](../../src/adu_workflow/src/agent_workflow.c) | If the desired update is already installed, the agent reports the installed update id and goes back to Idle without downloading anything. (A `ProcessDeployment` redelivery that is detected as a duplicate of `LastCompletedWorkflowId` returns earlier and never reaches this call.) |
| Steps Handler, before each step's `Download` | `DoV1DownloadWork` &rarr; `contentHandler->IsInstalled` in [`steps_handler.cpp`](../../src/extensions/update_manifest_handlers/steps_handler/src/steps_handler.cpp) | If the step's content is already installed, the step's `Download` is skipped (the step is marked `ADUC_Result_Install_Skipped_UpdateAlreadyInstalled`). |
| Steps Handler, before each step's `Backup` / `Install` / `Apply` | install/backup/apply loop &rarr; `contentHandler->IsInstalled` in [`steps_handler.cpp`](../../src/extensions/update_manifest_handlers/steps_handler/src/steps_handler.cpp) | If the step is already installed, `Backup`, `Install` and `Apply` are all skipped for that step and the workflow continues with the next step. The rest of the workflow is **not** short-circuited. |
| Steps Handler aggregate — invoked by the agent-core call sites above whenever the registered update-manifest handler is the Steps Handler (the default for multi-step manifests) | `StepsHandler_IsInstalled` in [`steps_handler.cpp`](../../src/extensions/update_manifest_handlers/steps_handler/src/steps_handler.cpp) | Iterates every (selected-component × child-step) pair and calls each sub-handler's `IsInstalled`. The loop short-circuits on the first sub-handler that returns `ADUC_Result_IsInstalled_NotInstalled` (aggregate becomes `NotInstalled`) or that returns a failure code (aggregate is that failure, returned unchanged); a thrown exception from a sub-handler is caught and treated as `NotInstalled`. If the loop completes without short-circuiting the aggregate is `ADUC_Result_IsInstalled_Installed`. A reference step with `selectedComponentsCount == 0` is treated as an optional/no-op step and the aggregate is reported as `Installed` for that step. |

> The agent does **not** re-invoke `IsInstalled` after a successful `Apply`; success is reported based on the workflow result itself. The next `IsInstalled` call typically happens only on the next deployment attempt or on the next agent startup.

Authoring rules:

1. **Be cheap.** It runs many times. Avoid network calls, package-manager refreshes, or anything that mutates state. If you cache results inside the handler, key the cache by workflow id, the selected component, and `installedCriteria`, and invalidate it whenever any of those change or after a successful Install/Apply — handler instances are reused across deployments by `updateType` (see [`extension_manager.cpp`](../../src/extensions/extension_manager/src/extension_manager.cpp)), so an unkeyed cache will go stale.
2. **Be scoped.** A step's `IsInstalled` must reflect the installed state of *this step's content*, not system-wide state. A `microsoft/apt:1` step asks "is this package at this version installed?", not "is the device fully up to date?".
3. **Be deterministic.** Given the same `ADUC_WorkflowData`, return the same result regardless of how many times you are called or in what order.
4. **Use `installedCriteria` honestly.** Read it via `workflow_get_installed_criteria` and compare it against the handler-scoped state. Don't hard-code "always installed" or "always not installed" — both will break MSOE skip semantics.
5. **Return only `900`, `901`, or a failure code.** `0` is `ADUC_Result_Failure`. Use `ADUC_Result_IsInstalled_Installed` (`900`) or `ADUC_Result_IsInstalled_NotInstalled` (`901`); any other positive `ResultCode` is undefined for `IsInstalled` and may be misinterpreted by the aggregate (e.g. it will not short-circuit the per-step loop, leaving the aggregate `Installed`). At the per-step call sites the Steps Handler only treats `ADUC_Result_IsInstalled_Installed` as "skip"; anything else (including a `0`/failure or an uncaught exception) falls through and the step is executed.
6. **No reboots, no restarts.** `IsInstalled` is a query. It must never schedule a reboot, restart the agent, or modify the device.

When this handler is used as a step inside a multi-step manifest, the Steps Handler's aggregate `IsInstalled` is the **conjunction** of each step's per-component `IsInstalled` results. See [steps-handler.md](steps-handler.md) for the surrounding workflow and the `selectedComponentsCount == 0` (no matching components) edge case.



In some case, a Device Builder may want to install an update content on one or more component(s) that connected to the Host Device instead.

In this case, if the Update has been authored and imported correctly, the DU Agent workflow will include a 'Selected Components' data in the ADUC_WorkflowHandle object that is passed to the handler's function.

When implementing a handler that handles an update intended for connected-components, the following helper functions can be used to get selected components properties:

|Function| Purpose|
|---|---|
|workflow_get_selected_components|Get a serialized JSON string containing a collection of components that the update should be installed. Caller must free with workflow_free_string().|

The component data is provided by Component Enumerator Extension, which usually implemented by Device Builder and registered on the Host Device.

> See [Contoso Virtual Vacuum Component Enumerator](../../src/extensions/component_enumerators/examples/contoso_component_enumerator/README.md) example for more details

## How To Build A Step Handler Extension

To get started, take a look at existing Step Handler Extensions in [src/extensions/step_handlers](../../src/extensions/step_handlers) folder for reference.

## How To Register A Step Handler Extension

To register a Step handler, run following command on the device:

```sh
sudo /usr/bin/AducIotAgent --extension-type updateContentHandler --register-extension <full path to the handler file> --extension-id <update type name>

# For example
# sudo /usr/bin/AducIotAgent --extension-type updateContentHandler --register-extension /var/lib/adu/extensions/sources/libmicrosoft_apt_1.so --extension-id 'microsoft/apt:1'
```

### Return Value

AducIotAgent will return 0 if it succeeded.
