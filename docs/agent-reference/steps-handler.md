# Steps Handler (a.k.a. Update Manifest Handler)

This document describes how the **Steps Handler** — the agent's default **Update Manifest Handler** — processes a multi-step update manifest **as currently implemented**. It is intended as a precise reference for agent maintainers and step-handler authors. For a higher-level conceptual introduction, see [Multi Step Ordered Execution (MSOE)](./update-manifest-v5-schema.md#multi-step-ordered-execution-msoe-support) and the public [Microsoft Learn doc on multistep ordered execution](https://learn.microsoft.com/en-us/azure/iot-hub-device-update/device-update-multi-step-updates).

**Source of truth:** [`src/extensions/update_manifest_handlers/steps_handler/src/steps_handler.cpp`](../../src/extensions/update_manifest_handlers/steps_handler/src/steps_handler.cpp).
The compiled artifact is `libmicrosoft_steps_1.so`.

## What it is

When the agent receives an update with manifest version `4.0` or `5.0`, the top-level manifest does not have an `updateType`; it has `instructions.steps`. The agent implicitly assigns the type `microsoft/steps:1` to the top-level workflow and loads the Steps Handler to process it. The Steps Handler is registered for the following `extension-id`s in the deviceupdate-agent Debian post-install (see [registering-device-update-extensions.md](./registering-device-update-extensions.md)):

- `microsoft/update-manifest`
- `microsoft/update-manifest:4`
- `microsoft/update-manifest:5`

For **reference steps**, the child workflow is also processed by the Steps Handler — it always uses the default reference-step handler `microsoft/steps:1` regardless of the child manifest's own type information (`DEFAULT_REF_STEP_HANDLER` in `steps_handler.cpp`).

## Workflow data model

Every Steps Handler operation works against an `ADUC_WorkflowHandle` tree:

```
parent (level 0)              ← top-level update manifest
 ├─ child step 0  (level 1)   ← inline or reference step
 ├─ child step 1  (level 1)
 │   ├─ grandchild step 0     ← only when the level-1 step is a *reference step*
 │   ├─ grandchild step 1     ←   (the referenced child manifest's inline steps)
 │   └─ ...
 └─ child step N
```

- `workflow_get_level(handle)` returns `0` for the top-level workflow, `1` for steps of the top-level manifest, and so on.
- `workflow_get_step_index(handle)` is the step's index within its parent.
- Child workflows are created lazily by `PrepareStepsWorkflowDataObject` (called at the top of every phase). For inline steps it copies file entities and selected components from the parent. For reference steps it downloads the **detached child manifest**, parses it, and creates the child workflow from that file.
- Children are kept alive for the entire workflow so subsequent phases can reuse parsed state. They are only freed when the top-level workflow ends.

### Level-0 vs. level-1 semantics

| | level 0 (parent) | level 1 (under a reference step) |
| --- | --- | --- |
| Target | Host device | Component(s) selected via `compatibility` of the child manifest, using the registered Component Enumerator |
| `selectedComponents` set on step workflow? | No (host) | Yes, one component at a time |
| If no Component Enumerator registered | n/a | The level-1 reference step is treated as targeting the host (single iteration, no component data) |
| If components match `compatibility` but `count == 0` | n/a | Step is treated as **optional** and short-circuits with `ADUC_Result_Install_Skipped_NoMatchingComponents` (or `ADUC_Result_Download_Skipped_NoMatchingComponents` during Download) |

A **child update may not contain reference steps** — only one level of indirection is allowed. The service validates this at import time.

## Phase-by-phase behavior

The agent's core orchestrator calls the eight `ContentHandler` functions in the order: `IsInstalled` → `Download` → `Backup` → `Install` → `Apply` → (on failure) `Restore` → `Cancel` (asynchronous). The Steps Handler implements them as follows.

### `Download`

`StepsHandler_Download` (lines ~534–710 of `steps_handler.cpp`):

1. If cancel was requested before entry, return `ADUC_Result_Failure_Cancelled` immediately.
2. Create the sandbox work folder (`ADUC_ERC_STEPS_HANDLER_CREATE_SANDBOX_FAILURE` on error).
3. `PrepareStepsWorkflowDataObject` — ensure all child workflows exist (downloads detached manifests for reference steps).
4. Determine `selectedComponentsCount` (1 for level 0 or when no enumerator; otherwise the count of matched components — 0 means the step is treated as optional and is skipped via `ADUC_Result_Download_Skipped_NoMatchingComponents`).
5. **For each (component × step):**
   1. Set per-step `selectedComponents` (inline steps only).
   2. Pick handler: inline step → step's `handler` property; reference step → `microsoft/steps:1`.
   3. `LoadUpdateContentHandlerExtension` — fail fast on load error.
   4. Call the step handler's `IsInstalled`. If it returns `ADUC_Result_IsInstalled_Installed`, store `ADUC_Result_Install_Skipped_UpdateAlreadyInstalled` on the step and **continue to the next step** (the workflow is not aborted).
   5. Otherwise call the step handler's `Download`. Any failure aborts the whole Download phase (`goto done`) and the parent's `result_details` is populated from the failing step.
6. On overall success the parent state becomes `ADUCITF_State_DownloadSucceeded` and result is `ADUC_Result_Download_Success`.

> **Note:** Existing inline-step "already installed" → skip behavior applies *per step*. The Learn doc's claim that the agent "skips future steps if an update is already installed" is not what the implementation does.

### `Install` (and implicitly Apply per step)

`StepsHandler_Install` (lines ~768–1165). The Install phase is by far the most behavior-rich:

1. Cancel pre-check, sandbox creation, `PrepareStepsWorkflowDataObject`, component selection — same as Download.
2. **Outer loop: each selected component.** For level-0 or no enumerator, this loop iterates exactly once with no component data set on the step workflow.
3. **Inner loop: each step (child workflow).**
   1. Set component data (inline steps only).
   2. Load the step handler.
   3. Call `IsInstalled`; if `Installed`, mark the step with `ADUC_Result_Install_Skipped_UpdateAlreadyInstalled` and `goto instanceDone` (skip Backup/Install/Apply, but **continue to the next step**).
   4. Call `Backup` (handler-defined; may be a no-op). Failure ⇒ propagate details and `goto done` (abort whole Install phase).
   5. Call `Install`. The handler may request reboot/restart via the workflow API:
      - **Immediate reboot/restart requested** — propagate to parent (`workflow_request_immediate_reboot` / `workflow_request_immediate_agent_restart`) and `goto done` to skip all remaining components and steps.
      - **Deferred reboot/restart requested** — propagate to parent and `break` out of the inner step loop (skip remaining steps for *this* component but continue with the next component). This is intentional: deferred requests group at the component boundary.
   6. If the handler returned `Install_Skipped_UpdateAlreadyInstalled` or `Install_Skipped_NoMatchingComponents`, `goto instanceDone` (skip Apply).
   7. On Install failure, call the handler's `Restore` (best-effort, result discarded), propagate `result_details` to parent, and `goto done`.
   8. Call `Apply`. On failure, call `Restore` (best-effort) and **fall through** to `instanceDone`. The failing apply's result code stays on `result`, so the post-loop check `if (IsAducResultCodeFailure(result.ResultCode)) goto componentDone; ⇒ goto done` aborts the whole phase. (See *Concerns* below — this control flow is subtle.)
4. On overall success, return `ADUC_Result_Install_Success` and set state `ADUCITF_State_InstallSucceeded`.

### `Apply`

The parent-level `Apply` is a **no-op** (returns `ADUC_Result_Apply_Success`). The actual per-step Apply was already invoked inside `Install`. This is by design but is non-obvious; preserve this behavior when implementing custom Update Manifest Handlers.

### `Cancel`

Sets `WORKFLOW_PROPERTY_FIELD_CANCEL_REQUESTED = true` on the workflow handle (`workflow_request_cancel`). Each step handler is responsible for checking this flag during its own work and short-circuiting with `ADUC_Result_Failure_Cancelled`. The Steps Handler itself only checks the flag:

- At the top of Download / Install / Apply / Backup (returns immediately).
- At the bottom of Install (after all components/steps run — to overwrite a `Install_Success` with `Failure_Cancelled` when cancel raced with completion).

A cancel raised mid-Install will *not* be honored at the parent level until a step handler returns control. Step handlers must poll the flag.

### `IsInstalled`

`StepsHandler_IsInstalled` (lines ~1293–1517) returns `ADUC_Result_IsInstalled_Installed` only when **every** (component × step) pair returns `Installed`. Any single non-installed step ⇒ `ADUC_Result_IsInstalled_NotInstalled`. Failures inside a step's `IsInstalled` are converted to `NotInstalled` (the workflow is processed). A `selectedComponentsCount == 0` (no matching components) returns `Installed` so the optional step is silently met.

### `Backup` and `Restore`

Top-level `Backup` and `Restore` are **no-ops**. The Steps Handler invokes per-step `Backup`/`Restore` from inside `Install` as described above.

## Reboot and agent-restart propagation summary

| Request flag set by step handler | Action by Steps Handler |
| --- | --- |
| `workflow_request_immediate_reboot` | Propagate to parent, `goto done` — abort all remaining components & steps |
| `workflow_request_immediate_agent_restart` | Propagate to parent, `goto done` |
| `workflow_request_reboot` (deferred) | Propagate to parent, `break` inner step loop; continue with next component |
| `workflow_request_agent_restart` (deferred) | Propagate to parent, `break` inner step loop; continue with next component |

The top-level orchestrator inspects these flags after Install returns and performs the actual reboot / restart through the agent's reboot synchronization protocol (see [architecture-overview.md → Graceful Reboot Flow](./architecture-overview.md)).

## Result-code conventions

Steps Handler ⇒ parent workflow result codes:

| Phase result | Meaning |
| --- | --- |
| `500 ADUC_Result_Download_Success` | Every step downloaded (or was already installed / had no matching components). |
| `502/503/504` | Forwarded from a step handler's `Download` skip cases. |
| `600 ADUC_Result_Install_Success` | Every step installed and applied. |
| `603 ADUC_Result_Install_Skipped_UpdateAlreadyInstalled` | Set on a step when its handler reports `IsInstalled_Installed`. |
| `604 ADUC_Result_Install_Skipped_NoMatchingComponents` | Set on a step when 0 components matched. |
| `700 ADUC_Result_Apply_Success` | Always (parent-level `Apply` is a no-op). |
| `800 ADUC_Result_Cancel_Success` / `801 ADUC_Result_Cancel_UnableToCancel` | From `workflow_request_cancel`. |
| `900 / 901` | Aggregate `IsInstalled` result. |
| `1000 / 1100` | Backup/Restore — always success at parent level. |
| `0 ADUC_Result_Failure` | Generic failure; check `ExtendedResultCode`. Common Steps-Handler ERCs: `ADUC_ERC_STEPS_HANDLER_CREATE_SANDBOX_FAILURE`, `ADUC_ERC_STEPS_HANDLER_INSTALL_FAILURE_MISSING_CHILD_WORKFLOW`, `ADUC_ERC_STEPS_HANDLER_SET_SELECTED_COMPONENTS_FAILURE`, `ADUC_ERC_STEPS_HANDLER_INSTALL_UNKNOWN_EXCEPTION_*`. |
| `-1 ADUC_Result_Failure_Cancelled` | Cancel was requested before/during the phase. |

For the full enumeration see [device-update-agent-extended-result-codes.md](./device-update-agent-extended-result-codes.md).

## Diagnostics and debug logging

Set the environment variable `DU_AGENT_ENABLE_STEPS_HANDLER_EXTRA_DEBUG_LOGS` to any non-empty value to enable verbose per-step / per-component traces. Useful when reproducing component-selection issues with the Contoso example component enumerator.

## Implementation files

| File | Role |
| --- | --- |
| `src/extensions/update_manifest_handlers/steps_handler/src/steps_handler.cpp` | All phase logic |
| `src/extensions/update_manifest_handlers/steps_handler/src/handler_create.cpp` | Exported `CreateUpdateContentHandlerExtension` factory |
| `src/extensions/update_manifest_handlers/steps_handler/inc/aduc/steps_handler.hpp` | `StepsHandlerImpl` class declaration |
| `src/extensions/update_manifest_handlers/steps_handler/README.md` | Examples + sequence diagrams |
| `src/extensions/update_manifest_handlers/steps_handler/tests/steps_handler_ut.cpp` | Catch2 unit tests |

## Related documentation

- [Update Manifest v5 Schema](./update-manifest-v5-schema.md) — manifest structure consumed by this handler
- [Device Update Agent Extensibility Points](./device-update-agent-extensibility-points.md) — where Steps Handler fits in the extension model
- [Multi Component Updating](./multi-component-updating.md) — how reference steps target components
- [How To Implement Custom Update Handler](./how-to-implement-custom-update-handler.md) — for authoring step handlers that the Steps Handler will load
- [Steps Handler README (with sequence diagrams)](../../src/extensions/update_manifest_handlers/steps_handler/README.md)
- [Extended Result Codes](./device-update-agent-extended-result-codes.md)
