# ADU Gen2 Agent Architecture

## Overview

The Gen2 agent is a modular, plugin-based Device Update agent built in C for embedded Linux systems. It uses a DAG-based workflow engine to execute multi-step update deployments with dependency resolution and parallel execution.

```
┌─────────────────────────────────────────────────────┐
│                   ADU Gen2 Agent                     │
├─────────────────────────────────────────────────────┤
│  Config (TOML) → Agent Core → Workflow Engine (DAG) │
│                        │                            │
│              ┌─────────┼─────────┐                  │
│              ▼         ▼         ▼                  │
│        Comm Provider  Extension  Download           │
│        (adu_direct)   Loader     Manager            │
│              │         │                            │
│              ▼         ▼                            │
│        ADU Service   Step Handlers (.so)            │
│                      ├─ script:2                    │
│                      ├─ apt:2                       │
│                      └─ swupdate:2                  │
└─────────────────────────────────────────────────────┘
```

## Core Components

### Agent Main (`src/adu_gen2_agent/`)

The agent binary (`adu_gen2_agent`) is a config-driven process that:

1. Reads TOML configuration
2. Loads extensions from configured search paths (dlopen)
3. Polls the ADU service for deployments
4. Downloads update payloads
5. Executes workflow steps via the DAG engine
6. Reports results back to the service

```bash
adu_gen2_agent --config /etc/adu/adu-agent.conf --once --log-level 6
```

### Extension SDK (`src/extension_sdk/`)

The Extension Development Kit (EDK) provides the core libraries and APIs for building extensions:

| Source | Purpose |
|--------|---------|
| `workflow_engine.c` | Step orchestration and lifecycle |
| `dag_engine.c` | DAG dependency resolution and parallel scheduling |
| `extension_loader.c` | dlopen-based plugin loading with capability matching |
| `download_manager.c` | Content pipeline and file retrieval |
| `comm_dispatcher.c` | Communication provider abstraction |
| `manifest_parser.c` | Update manifest JSON parsing |
| `config_reader.c` | TOML configuration loading |
| `log_writer.c` | Binary/text log output |

**Dependencies:** Parson (JSON), CURL (HTTP), OpenSSL (Crypto), pthread, dl

### Extension Loader

Extensions are shared libraries (`.so`) loaded at runtime via dlopen. The loader:

- Auto-scans configured directories for extension libraries
- Matches capabilities declared in the manifest to registered handlers
- Supports hot-reload and version negotiation

---

## Extension Development Kit (EDK)

### Vtable-Based Extension Model

Every extension implements a vtable interface. Step handlers implement these operations:

```c
typedef struct ADUC_StepHandler_VTable {
    ADUC_Result (*Evaluate)(ADUC_WorkflowHandle, ADUC_StepHandle);
    ADUC_Result (*Acquire)(ADUC_WorkflowHandle, ADUC_StepHandle);
    ADUC_Result (*Preprocess)(ADUC_WorkflowHandle, ADUC_StepHandle);
    ADUC_Result (*Execute)(ADUC_WorkflowHandle, ADUC_StepHandle);
    ADUC_Result (*Validate)(ADUC_WorkflowHandle, ADUC_StepHandle);
    ADUC_Result (*Postprocess)(ADUC_WorkflowHandle, ADUC_StepHandle);
    ADUC_Result (*Cancel)(ADUC_WorkflowHandle, ADUC_StepHandle);
    ADUC_Result (*Cleanup)(ADUC_WorkflowHandle, ADUC_StepHandle);
} ADUC_StepHandler_VTable;
```

### Step Handler Lifecycle

Each step passes through phases in order:

```
Evaluate → Acquire → Preprocess → Execute → Validate → Postprocess
                                                              │
                                                         (Cleanup)
```

| Phase | Purpose |
|-------|---------|
| **Evaluate** | Check if update is needed (already-installed detection) |
| **Acquire** | Download/prepare content files |
| **Preprocess** | Validate content, check prerequisites |
| **Execute** | Apply the update |
| **Validate** | Verify successful application |
| **Postprocess** | Finalize (commit, reboot scheduling) |
| **Cancel** | Abort in-progress operation |
| **Cleanup** | Release resources on completion or failure |

### Writing a Custom Extension

1. Implement the vtable functions
2. Export a registration function:

```c
#include <aduc/extension_sdk.h>

static ADUC_Result MyHandler_Execute(ADUC_WorkflowHandle wf, ADUC_StepHandle step) {
    // Your update logic here
    return ADUC_Result_Success;
}

static ADUC_StepHandler_VTable my_vtable = {
    .Evaluate = MyHandler_Evaluate,
    .Acquire = MyHandler_Acquire,
    .Preprocess = MyHandler_Preprocess,
    .Execute = MyHandler_Execute,
    .Validate = MyHandler_Validate,
    .Postprocess = MyHandler_Postprocess,
    .Cancel = MyHandler_Cancel,
    .Cleanup = MyHandler_Cleanup,
};

ADUC_EXTENSION_EXPORT ADUC_Result ADUC_Extension_Register(ADUC_ExtensionRegistration* reg) {
    reg->name = "myorg/myhandler";
    reg->version = 1;
    reg->type = ADUC_EXTENSION_TYPE_STEP_HANDLER;
    reg->vtable = &my_vtable;
    return ADUC_Result_Success;
}
```

3. Build as a shared library and place in the extensions search path.

---

## DAG-Based Workflow Engine

### Dependency Model

Steps in an update manifest declare dependencies via `handlerProperties` — no schema changes to the manifest format are needed.

**Dependency Keys:**

| Key | Type | Description |
|-----|------|-------------|
| `id` | string | Unique step identifier |
| `requires` | string | Comma-separated list of step IDs this step depends on |
| `skipOnFailed` | string | Comma-separated IDs — skip this step if any listed step failed |
| `runOnFailed` | string | Comma-separated IDs — only run this step if a listed step failed |

### Example Manifest (DAG)

```json
{
  "updateId": { "provider": "Contoso", "name": "Vacuum", "version": "2.0" },
  "instructions": {
    "steps": [
      {
        "handler": "microsoft/script:2",
        "handlerProperties": { "id": "pre-check" },
        "files": ["check.sh"]
      },
      {
        "handler": "microsoft/swupdate:2",
        "handlerProperties": { "id": "firmware", "requires": "pre-check" },
        "files": ["firmware.swu"]
      },
      {
        "handler": "microsoft/apt:2",
        "handlerProperties": { "id": "packages", "requires": "pre-check" },
        "files": ["packages.list"]
      },
      {
        "handler": "microsoft/script:2",
        "handlerProperties": { "id": "config", "requires": "firmware,packages" },
        "files": ["configure.sh"]
      },
      {
        "handler": "microsoft/script:2",
        "handlerProperties": { "id": "verify", "requires": "config" },
        "files": ["verify.sh"]
      },
      {
        "handler": "microsoft/script:2",
        "handlerProperties": { "id": "rollback", "runOnFailed": "firmware,packages,config" },
        "files": ["rollback.sh"]
      }
    ]
  }
}
```

### Execution Behavior

```
pre-check
  ├──→ firmware  (parallel with packages)
  ├──→ packages  (parallel with firmware)
  │         │
  └─────────┴──→ config (waits for both)
                    │
                    └──→ verify
                    
rollback ──── (only runs if firmware, packages, or config fails)
```

- **Parallel execution**: Steps with independent deps run concurrently
- **Failure cascade**: Downstream steps are skipped when an upstream dependency fails
- **Conditional execution**: `runOnFailed` steps only execute on upstream failure (rollback patterns)
- **Skip logic**: `skipOnFailed` steps are skipped (but not failed) when named steps fail

### Test Coverage

52 test cases cover all dependency scenarios including cycles, diamond dependencies, failure cascades, and rollback patterns.

---

## Communication Provider Model

Communication providers are extensions that handle transport between the agent and the cloud service.

### Built-in Providers

| Provider | Description |
|----------|-------------|
| `adu_direct_comm` | Direct REST API communication with ADU service (mTLS) |
| `iothub_adapter` | Azure IoT Hub device twin + direct methods |

### Provider Interface

```c
typedef struct ADUC_CommProvider_VTable {
    ADUC_Result (*Connect)(ADUC_CommHandle);
    ADUC_Result (*Poll)(ADUC_CommHandle, ADUC_DeploymentInfo*);
    ADUC_Result (*ReportStatus)(ADUC_CommHandle, ADUC_WorkflowStatus*);
    ADUC_Result (*Disconnect)(ADUC_CommHandle);
} ADUC_CommProvider_VTable;
```

The agent uses the configured communication provider to:
1. Poll for pending deployments
2. Download update manifests
3. Report step-level and workflow-level status
4. Acknowledge deployment completion

---

## First-Party Step Handlers

### `microsoft/script:2` (`src/extensions/step_handlers/script_handler_v2/`)

Executes customer-provided shell scripts with a clean interface:

- Scripts are plain bash — no SDK boilerplate required
- Exit codes map to results: `0`=success, `1`=fail, `2`=retry, `3`=reboot, `100`=already-installed
- Environment variables provided to scripts:

| Variable | Description |
|----------|-------------|
| `ADU_WORK_FOLDER` | Temporary working directory |
| `ADU_CONTENT_DIR` | Directory with downloaded content files |
| `ADU_RESULT_FILE` | Path to write structured result JSON |
| `ADU_STEP_ID` | Current step identifier |

### `microsoft/apt:2` (`src/extensions/step_handlers/apt_handler_v2/`)

Manages Debian packages via APT — installs, upgrades, and removes packages specified in the manifest.

### `microsoft/swupdate:2` (`src/extensions/step_handlers/swupdate_handler_v2/`)

Applies full firmware images using the [SWUpdate](https://sbabic.github.io/swupdate/) framework for A/B partition updates.

---

## Build Infrastructure

### Code Coverage (`cmake/code-coverage.cmake`)

```bash
cmake -DADUC_ENABLE_COVERAGE=ON -B build -G Ninja
ninja -C build
ctest --test-dir build
ninja -C build coverage        # Generate HTML report → build/coverage_report/
ninja -C build coverage_check  # Fail if below 85% threshold
```

**Requirements:** gcov, lcov, genhtml

### Debug Symbols (`cmake/debug-symbols.cmake`)

Strips binaries and produces separate `.debug` files for symbol servers.

### Scripts

| Script | Purpose |
|--------|---------|
| `scripts/run_coverage.sh` | Run full coverage pipeline |
| `scripts/upload_symbols.sh` | Upload .debug files to symbol server |
| `tools/test_packages/build_test_deb.sh` | Build test .deb packages |
