# adu_workflow

**Type:** Static C Library (`agent_workflow`)

## Description

Implements the **Device Update agent workflow state machine**. This is the core orchestration logic that handles update actions received from Azure IoT Hub. It processes property updates (desired twin changes), transitions through update workflow states, and reports state back to the service.

## Key Workflow Phases

1. **Download** — Fetches update payloads from the cloud
2. **Backup** — Creates a backup of the current state before applying changes
3. **Install** — Installs the update content on the device
4. **Apply** — Applies/activates the installed update
5. **Restore** — Rolls back to backup if an error occurs
6. **Cancel** — Cancels an in-progress update

## Key Functions

- `ADUC_Workflow_HandlePropertyUpdate` — Entry point for processing desired property changes from the IoT Hub device twin
- `ADUC_Workflow_TransitionWorkflow` — Advances the workflow through its state transitions
- `MethodCall_Download`, `MethodCall_Install`, `MethodCall_Apply`, etc. — Per-phase dispatch methods

## Dependencies

Depends on `adu_types`, `workflow_utils`, and the extension framework for dispatching to update-type-specific handlers.
