# Device Update Agent — Architecture Overview

This document describes how the modules under `src/` fit together to form the Azure IoT Hub Device Update agent.

---

## High-Level Architecture

```
                        ┌──────────────────────────────┐
                        │        Azure IoT Hub         │
                        │  (Device Twin / Direct Methods)│
                        └──────────────┬───────────────┘
                                       │
                        ┌──────────────▼───────────────┐
                        │  communication_managers/     │  Connection lifecycle
                        │  (iothub_communication_mgr)  │  (connect, reconnect, auth)
                        └──────────────┬───────────────┘
                                       │
                        ┌──────────────▼───────────────┐
                        │  communication_abstraction/  │  SDK wrapper layer
                        │  (ClientHandle_* API)        │  (Device vs Module client)
                        └──────────────┬───────────────┘
                                       │
               ┌───────────────────────▼───────────────────────┐
               │                   agent/                       │
               │              (Main Executable)                 │
               │                                                │
               │  ┌─────────────────┐  ┌─────────────────────┐ │
               │  │adu_core_interface│  │device_info_interface│ │  PnP Interfaces
               │  └────────┬────────┘  └─────────────────────┘ │
               │           │           ┌─────────────────────┐ │
               │           │           │   pnp_helper/       │ │
               │           │           └─────────────────────┘ │
               │           │           ┌─────────────────────┐ │
               │           │           │  shutdown_service/   │ │
               │           │           └─────────────────────┘ │
               └───────────┼───────────────────────────────────┘
                           │
          ┌────────────────▼────────────────┐
          │      agent_orchestration/       │  Maps cloud actions
          │  (action → workflow mapping)    │  to internal steps
          └────────────────┬────────────────┘
                           │
          ┌────────────────▼────────────────┐
          │         adu_workflow/           │  State machine
          │  (Download → Backup → Install   │  driving the update
          │   → Apply, Cancel, Restore)     │  lifecycle
          └───────┬────────────────┬────────┘
                  │                │
    ┌─────────────▼──────┐  ┌─────▼──────────────────┐
    │    extensions/      │  │   rootkey_workflow/    │
    │                     │  │  (root key validation  │
    │  ┌───────────────┐  │  │   & secure storage)   │
    │  │step_handlers/ │  │  └────────────────────────┘
    │  │ APT, Script,  │  │
    │  │ SWUpdate, etc.│  │
    │  └───────┬───────┘  │
    │          │          │
    │  ┌───────────────┐  │
    │  │content_       │  │
    │  │downloaders/   │  │
    │  │ curl, DO      │  │
    │  └───────────────┘  │
    │          │          │
    │  ┌───────────────┐  │
    │  │update_manifest│  │
    │  │_handlers/     │  │
    │  └───────────────┘  │
    │          │          │
    │  ┌───────────────┐  │
    │  │component_     │  │
    │  │enumerators/   │  │
    │  └───────────────┘  │
    │          │          │
    │  ┌───────────────┐  │
    │  │download_      │  │
    │  │handlers/      │  │
    │  └───────────────┘  │
    └──────────┬──────────┘
               │
    ┌──────────▼──────────┐
    │     adu-shell/      │  Privileged operations
    │  (setuid root helper │  (apt-get, reboot,
    │   for elevated ops)  │   script execution)
    └─────────────────────┘

  ┌─────────────────────────────────────────────────┐
  │          diagnostics_component/                  │
  │  (Parallel subsystem: log collection & upload    │
  │   triggered via IoT Hub direct methods)          │
  └─────────────────────────────────────────────────┘
```

---

## How They Tie Together

### 1. Agent Startup & Connection

The **`agent/`** module is the entry point. On startup it:
- Parses CLI arguments and loads configuration
- Uses **`communication_managers/`** to establish an IoT Hub connection
- **`communication_managers/`** delegates to **`communication_abstraction/`**, which wraps the Azure IoT Hub C SDK and abstracts whether the agent runs as a device or an IoT Edge module
- Registers PnP interfaces: **ADU Core** (for updates), **Device Info** (for device metadata), and **Diagnostics** (for remote log collection)

### 2. Receiving an Update

When Azure IoT Hub sends a desired property update (via the device twin):
1. **`agent/adu_core_interface/`** receives the twin change callback
2. It passes the payload to **`agent_orchestration/`**, which maps the desired action (e.g., "ProcessDeployment") into an internal workflow state
3. **`adu_workflow/`** takes over as the state machine, transitioning through phases: **Download → Backup → Install → Apply**

### 3. Executing Update Steps

**`adu_workflow/`** dispatches each phase to the **`extensions/`** framework:
- The **`extension_manager/`** dynamically loads the appropriate plugin based on the update type
- **`step_handlers/`** (APT, Script, SWUpdate, etc.) implement the actual update logic for each phase
- **`content_downloaders/`** (curl or Delivery Optimization) handle fetching update payloads
- **`update_manifest_handlers/`** parse and orchestrate multi-step update manifests
- **`download_handlers/`** provide custom download processing (e.g., delta downloads)
- **`component_enumerators/`** identify device sub-components for targeted updates

### 4. Privileged Operations

When an extension needs elevated permissions (e.g., installing a package, rebooting):
- It invokes **`adu-shell/`**, the setuid-root helper, via a subprocess call
- `adu-shell` validates the request and executes the privileged operation

### 5. Security — Root Key Validation

Before applying updates, **`rootkey_workflow/`** ensures the agent's root key store is current:
- Downloads and validates root key packages against hardcoded trusted keys
- Atomically updates the local key store
- These root keys are used by **`utils/jws_utils/`** and **`utils/crypto_utils/`** to verify update manifest signatures

### 6. Diagnostics (Parallel Subsystem)

**`diagnostics_component/`** operates as a parallel subsystem:
- Receives diagnostic requests via IoT Hub direct methods
- Collects device logs asynchronously (via `diagnostics_async_helper/`)
- Uploads diagnostic data to Azure Blob Storage
- Reports status back through the device twin

### 7. Cross-Cutting Foundations

Several modules provide foundational services used by all layers:

| Module | Role |
|---|---|
| **`adu_types/`** | Shared type definitions (structs, enums, callbacks) — the common vocabulary |
| **`utils/`** | ~30 utility libraries (crypto, config, hashing, JWS, workflow helpers, etc.) |
| **`logging/`** | Centralized logging framework (zlog backend) used by every module |
| **`libaducpal/`** | Platform Abstraction Layer — OS-agnostic wrappers for system calls |
| **`platform_layers/`** | OS-specific implementations (Linux, Windows) selected at build time |
| **`inc/`** | Top-level shared headers |
| **`deps/`** | Third-party build configuration (SWUpdate) |
| **`docs/`** | Build-time documentation generation |

---

## Dependency Flow (Simplified)

```
agent  ──►  communication_managers  ──►  communication_abstraction  ──►  IoT Hub SDK
  │
  ├──►  agent_orchestration  ──►  adu_workflow  ──►  extensions (plugins)
  │                                     │                   │
  │                                     │                   └──►  adu-shell (privileged ops)
  │                                     │
  │                                     └──►  rootkey_workflow
  │
  ├──►  diagnostics_component
  │
  └──►  [Cross-cutting: adu_types, utils, logging, libaducpal, platform_layers]
```

Every module depends downward on `adu_types` for shared types, `logging` for log output, and `utils` for common operations. Platform-specific behavior is isolated in `platform_layers` and `libaducpal`.
