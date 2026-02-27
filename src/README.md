# Device Update Agent — Source Modules Reference

A consolidated reference for every module under `src/`, followed by an architecture overview showing how they all connect.

---

## Table of Contents

- [adu_types](#adu_types)
- [adu_workflow](#adu_workflow)
- [adu-shell](#adu-shell)
- [agent](#agent)
- [agent_orchestration](#agent_orchestration)
- [communication_abstraction](#communication_abstraction)
- [communication_managers](#communication_managers)
- [deps](#deps)
- [diagnostics_component](#diagnostics_component)
- [docs](#docs)
- [extensions](#extensions)
- [inc](#inc)
- [libaducpal](#libaducpal)
- [logging](#logging)
- [platform_layers](#platform_layers)
- [rootkey_workflow](#rootkey_workflow)
- [utils](#utils)
- [Architecture Overview](#architecture-overview)

---

## adu_types

**Type:** Static C Library (`libadu_types`)

The `adu_types` module defines the **common data types, enumerations, structs, and callback signatures** used throughout the Azure IoT Hub Device Update agent codebase. It serves as a foundational shared type-definition layer that other agent components depend on.

### Key Components

- **`adu_types.h`** — Agent launch arguments (`ADUC_LaunchArguments`), IoT Hub connection info (`ADUC_ConnectionInfo`), connection/auth type enums (`ADUC_ConnType`, `ADUC_AuthType`), extension registration types, and PnP property update context.
- **`types/adu_core.h`** — Core update workflow types: callback function typedefs for each workflow phase (Download, Backup, Install, Apply, Restore, Cancel, Idle, IsInstalled, Sandbox create/destroy, DoWork), the `ADUC_UpdateActionCallbacks` struct, and the comprehensive `ADUC_ResultCode` enum covering all success/failure/in-progress codes.
- **`types/update_content.h`** — Update content data types: `ADUC_UpdateId` (Provider/Name/Version), `ADUC_FileEntity` (download URIs, hashes, related files), `ADUC_RelatedFile`, `ADUC_FileUrl`, and JSON field name constants for update manifests.

### Dependencies

Minimal external dependencies; primarily consumed by other agent modules.

---

## adu_workflow

**Type:** Static C Library (`agent_workflow`)

Implements the **Device Update agent workflow state machine**. This is the core orchestration logic that handles update actions received from Azure IoT Hub. It processes property updates (desired twin changes), transitions through update workflow states, and reports state back to the service.

### Key Workflow Phases

1. **Download** — Fetches update payloads from the cloud
2. **Backup** — Creates a backup of the current state before applying changes
3. **Install** — Installs the update content on the device
4. **Apply** — Applies/activates the installed update
5. **Restore** — Rolls back to backup if an error occurs
6. **Cancel** — Cancels an in-progress update

### Key Functions

- `ADUC_Workflow_HandlePropertyUpdate` — Entry point for processing desired property changes from the IoT Hub device twin
- `ADUC_Workflow_TransitionWorkflow` — Advances the workflow through its state transitions
- `MethodCall_Download`, `MethodCall_Install`, `MethodCall_Apply`, etc. — Per-phase dispatch methods

### Dependencies

Depends on `adu_types`, `workflow_utils`, and the extension framework for dispatching to update-type-specific handlers.

---

## adu-shell

**Type:** Privileged Helper Executable (`adu-shell`)

A **setuid-root helper executable** that performs operations requiring elevated permissions on behalf of the Device Update agent. The main agent process runs as a non-root user and delegates privileged operations (package installation, system reboots, script execution) to `adu-shell` via CLI invocations.

### How It Works

`adu-shell` receives commands via CLI arguments specifying:
- **Update type** — e.g., `microsoft/apt`, `microsoft/script`, or `common`
- **Action** — e.g., `download`, `install`, `apply`, `cancel`, `rollback`, `reboot`

It then dispatches to the appropriate task handler based on the update type and action.

### Supported Task Handlers

- **APT package operations** — Install/remove Debian packages via `apt-get`
- **Script execution** — Run update scripts with elevated privileges
- **Common tasks** — System reboot and other shared privileged operations

### Security

Runs as a setuid binary to bridge the privilege gap between the unprivileged agent process and operations that require root access. Only a well-defined set of operations is supported.

### Dependencies

Depends on `adu_types`, platform layer utilities, and system package management tools.

---

## agent

**Type:** Main Executable (`AducIotAgent`)

The **main entry point** for the Azure IoT Hub Device Update agent. This module contains the `main()` function, command-line argument parsing, agent initialization, and the primary run loop. It connects to Azure IoT Hub, registers PnP (Plug and Play) interfaces, and coordinates all agent subsystems.

### Key Submodules

- **`src/`** — Contains the `main()` entry point, argument parsing, and agent lifecycle management.
- **`adu_core_interface/`** — Implements the ADU Core PnP interface for communicating update state with IoT Hub via device twin properties and direct methods.
- **`device_info_interface/`** — Reports device information (manufacturer, model, OS version, etc.) to IoT Hub as reported properties.
- **`pnp_helper/`** — Utility functions for PnP (IoT Plug and Play) operations such as creating PnP components and handling property callbacks.
- **`command_helper/`** — Helpers for processing agent commands and CLI options.
- **`shutdown_service/`** — Manages graceful shutdown of the agent process.
- **`adu_core_export_helpers/`** — Export helper functions for the ADU core interface callbacks.

### Responsibilities

- Parse CLI arguments and configuration
- Establish and manage the IoT Hub connection
- Register PnP interfaces (ADU Core, Device Info, Diagnostics)
- Run the main event/work loop
- Coordinate graceful shutdown

### Dependencies

Depends on `adu_workflow`, `communication_abstraction`, `communication_managers`, `extensions`, `diagnostics_component`, `adu_types`, and various utility libraries.

---

## agent_orchestration

**Type:** Static C Library (`agent_orchestration`)

Contains the **business logic for agent-driven workflow orchestration processing**. This module maps desired update actions received from the Azure IoT Hub device twin into internal workflow steps and manages the decision-making around workflow progression.

### Key Responsibilities

- Map desired update actions from the cloud into internal workflow states
- Determine whether a workflow is complete
- Decide whether state should be reported back to the cloud
- Evaluate whether a retry is applicable based on timestamp tokens

### Dependencies

Depends on `adu_types`, `c_utils`, and `workflow_utils`.

---

## communication_abstraction

**Type:** Static C Library (`communication_abstraction`)

Provides an **abstract interface layer over the Azure IoT Hub Device Client and Module Client SDKs**. It defines wrapper functions that allow the rest of the codebase to work uniformly regardless of whether the agent connects as an IoT Hub device or as an IoT Edge module.

### Key Functions

- `ClientHandle_CreateFromConnectionString` — Create a client connection from a connection string
- `ClientHandle_SetConnectionStatusCallback` — Register connection status change callbacks
- `ClientHandle_SetDeviceTwinCallback` — Register device twin change callbacks
- `ClientHandle_SetDeviceMethodCallback` — Register direct method callbacks
- `ClientHandle_SendEventAsync` — Send device-to-cloud events
- `ClientHandle_SendReportedState` — Report device state to IoT Hub
- `ClientHandle_SetOption` — Set client options
- `ClientHandle_Destroy` — Clean up and destroy the client handle

### Purpose

By abstracting the IoT Hub SDK client handles, this module decouples the agent's business logic from the specifics of the underlying transport and connection type (device vs. module identity).

### Dependencies

Depends on the Azure IoT Hub C SDK (`IotHubClient`, `umqtt`, MQTT transport).

---

## communication_managers

**Type:** Static C++ Library (`iothub_communication_manager`)

Implements the **concrete IoT Hub communication manager** that manages the agent's connection lifecycle with Azure IoT Hub. This module builds on top of the `communication_abstraction` layer to provide higher-level connection management capabilities.

### Key Responsibilities

- Manage the IoT Hub client connection lifecycle (connect, reconnect, disconnect)
- Handle authentication and connection string provisioning
- Coordinate device twin and direct method callback registration
- Manage reported property updates and device-to-cloud messaging
- Handle connection status monitoring and error recovery

### Submodules

- **`iothub_communication_manager/`** — The primary IoT Hub communication manager implementation

### Dependencies

Depends on `communication_abstraction`, `adu_types`, and the Azure IoT Hub C SDK.

---

## deps

**Type:** External Dependencies / Third-Party Configuration

Contains **build configuration files for external dependencies** used by the Device Update agent. Currently holds the build configuration for SWUpdate, an open-source software update framework for embedded Linux.

### Contents

- **`swupdate/.config`** — Build configuration file for the SWUpdate dependency, used by the SWUpdate v2 step handler extension.

---

## diagnostics_component

**Type:** Collection of Static Libraries

Implements the **diagnostics subsystem** for the Device Update agent. This component enables remote diagnostic data collection from devices, allowing operators to troubleshoot issues by requesting diagnostic logs and device information through IoT Hub.

### Submodules

- **`diagnostics_interface/`** — PnP interface implementation that handles diagnostic direct method calls from IoT Hub (e.g., log upload requests) and reports diagnostic state via device twin properties.
- **`diagnostics_workflow/`** — Orchestrates the diagnostic data collection workflow, coordinating log gathering, upload, and status reporting.
- **`diagnostics_async_helper/`** — Provides asynchronous execution support for diagnostic operations to avoid blocking the main agent loop.
- **`diagnostics_devicename/`** — Utility for resolving and providing the device name/hostname for diagnostic reports.
- **`utils/`** — Shared utility functions used by the diagnostics subsystem.

### Key Capabilities

- Receive diagnostic requests via IoT Hub direct methods
- Collect device logs and diagnostic information
- Upload diagnostic data to Azure Blob Storage
- Report diagnostic operation status back to IoT Hub

### Dependencies

Depends on `adu_types`, `communication_abstraction`, logging utilities, and Azure Storage SDK for blob uploads.

---

## docs

**Type:** Build Documentation / CMake Configuration

Contains **CMake build configuration for documentation generation**. This directory is part of the build system and provides targets for generating project documentation as part of the CMake build process.

### Contents

- **`CMakeLists.txt`** — CMake configuration for documentation build targets.

---

## extensions

**Type:** Plugin/Extension Framework (Shared Libraries + Static Libraries)

Implements the **dynamically-loadable extension architecture** for the Device Update agent. This framework allows the agent to support multiple update types, download methods, and device component enumeration strategies through a plugin-based system.

### Core Framework

- **`extension_manager/`** — Central `ExtensionManager` class that loads, registers, and dispatches to extension shared libraries at runtime. Manages the lifecycle of all extension types.
- **`shared_lib/`** — Shared library loading utilities used by the extension manager.
- **`inc/`** — Shared header files and interface definitions for extensions.

### Content Downloaders

- **`content_downloaders/`** — Plugins for downloading update content from the cloud:
  - **curl-based downloader** — Uses libcurl for HTTP/HTTPS downloads
  - **Delivery Optimization downloader** — Uses Microsoft Delivery Optimization for bandwidth-efficient downloads

### Step Handlers

- **`step_handlers/`** — Update-type-specific handlers that implement the `ContentHandler` interface (`Download`, `Install`, `Apply`, `Backup`, `Restore`, `Cancel`, `IsInstalled`):
  - **APT handler** — Installs/removes Debian packages via `apt-get`
  - **Script handler** — Executes custom update scripts
  - **SWUpdate v2 handler** — Integrates with the SWUpdate framework for embedded Linux updates
  - **Simulator handler** — Simulates update operations for testing
  - **WIM handler** — Windows Imaging format handler

### Update Manifest Handlers

- **`update_manifest_handlers/`** — Handlers for processing update manifests:
  - **Steps handler** — Orchestrates multi-step updates with ordered execution

### Download Handlers

- **`download_handlers/`** — Plugin system for custom download processing (e.g., delta/diff downloads). Includes a factory and plugin loader.

### Component Enumerators

- **`component_enumerators/`** — Plugins that enumerate device sub-components for component-targeted updates (e.g., enumerating firmware components on a multi-component device).

### Dependencies

Depends on `adu_types`, `c_utils`, `contract_utils`, and platform-specific dynamic library loading (`dlopen`/`dlsym`).

---

## inc

**Type:** Shared Header Files

Contains **top-level shared header files** used across the Device Update agent codebase. These headers define common interfaces, macros, and declarations that are included by multiple modules throughout the project.

### Contents

- **`aduc/`** — Subdirectory containing ADUC (Azure Device Update Client) shared headers, including common definitions, result codes, and interface declarations used by the agent and its components.

---

## libaducpal

**Type:** Static C Library (`libaducpal` — ADUC Platform Abstraction Layer)

The **Platform Abstraction Layer (PAL)** for the Device Update agent. This library provides OS-abstracted wrappers around platform-specific system calls, enabling the agent codebase to be portable across different operating systems.

### Key Abstractions

- **File system operations** — Directory creation, file permissions, path manipulation
- **Process management** — Process spawning, signal handling
- **User/group operations** — User ID lookups, permission checks
- **System information** — OS version, hostname, hardware info
- **Time operations** — Monotonic clocks, timestamps

### Purpose

By isolating platform-specific code behind a consistent API, `libaducpal` allows the rest of the agent to be written in a platform-independent manner. Platform-specific implementations are provided for each supported OS.

### Dependencies

Minimal dependencies; wraps native OS APIs (POSIX/Linux system calls).

---

## logging

**Type:** Static C Library (`logging`)

Provides the **centralized logging framework** for the Device Update agent. This module defines the logging API used throughout the codebase and provides a pluggable backend architecture for log output.

### Key Components

- **`inc/`** — Public logging API headers defining log macros and severity levels (e.g., `Log_Debug`, `Log_Info`, `Log_Warn`, `Log_Error`) used by all agent modules.
- **`zlog/`** — Implementation of the logging backend using the **zlog** library, a reliable, high-performance, thread-safe logging library for C. Handles log formatting, rotation, and output to files, console, or syslog.

### Features

- Multiple log severity levels
- Thread-safe logging
- Configurable log output destinations (file, console, syslog)
- Log rotation support
- Consistent log formatting across the agent

### Dependencies

Depends on the `zlog` third-party library for the logging backend implementation.

---

## platform_layers

**Type:** Static C Libraries (per-platform implementations)

Contains **platform-specific implementations** of the Device Update agent's platform layer interface. Each subdirectory provides the concrete implementation of platform-dependent operations for a specific operating system.

### Submodules

- **`linux_platform_layer/`** — Linux-specific implementation providing:
  - Device information retrieval (manufacturer, model, OS version, etc.)
  - File system operations specific to Linux
  - System reboot and shutdown commands
  - Package management integration
  - Linux-specific path conventions and permissions

- **`windows_platform_layer/`** — Windows-specific implementation providing equivalent functionality for Windows-based devices.

### Purpose

The platform layer pattern separates OS-specific code from the core agent logic. The appropriate platform layer is selected at build time via CMake configuration, allowing the same agent codebase to target multiple operating systems.

### Dependencies

Depends on `adu_types`, `libaducpal`, and OS-specific system APIs.

---

## rootkey_workflow

**Type:** Static C Library (`rootkey_workflow`)

Implements the **root key update workflow** for the Device Update agent. Root keys are used to verify the authenticity and integrity of update packages. This module handles the secure lifecycle of downloading, validating, and storing root key packages.

### Key Function

- `RootKeyWorkflow_UpdateRootKeys()` — Main entry point that orchestrates the entire root key update process.

### Workflow Steps

1. **Download** the root key package from a specified URL (supports both curl and Delivery Optimization backends)
2. **Parse** the downloaded root key package
3. **Validate** the package against hardcoded trusted keys
4. **Compare** with the existing local root key store to determine if an update is needed
5. **Atomically write** the new root key package to disk if an update is required

### Security

- Validates root key packages against hardcoded keys built into the agent
- Enforces test vs. production package separation
- Uses atomic file writes to prevent corruption during updates

### Dependencies

Depends on `root_key_utils`, `rootkeypackage_utils`, `crypto_utils`, `c_utils`, and download backends (curl/Delivery Optimization).

---

## utils

**Type:** Collection of ~30 Static C/C++ Utility Libraries

A large collection of **reusable utility libraries**, each in its own subdirectory. These provide foundational capabilities used across the entire Device Update agent codebase.

### Utility Libraries

| Directory | Description |
|---|---|
| **`c_utils/`** | Core C string/bit operations, connection string parsing, memory helpers |
| **`crypto_utils/`** | Cryptographic operations, Base64 encoding/decoding |
| **`config_utils/`** | Configuration file parsing and management |
| **`hash_utils/`** | File and data hash computation and verification |
| **`jws_utils/`** | JSON Web Signature (JWS) parsing and validation |
| **`root_key_utils/`** | Root key management and validation |
| **`rootkeypackage_utils/`** | Root key package parsing and handling |
| **`workflow_utils/`** | Update workflow data structures and helper functions |
| **`workflow_data_utils/`** | Workflow data serialization and deserialization |
| **`entity_utils/`** | Update entity (file, update ID) management |
| **`contract_utils/`** | Extension contract version checking and validation |
| **`eis_utils/`** | Edge Identity Service (EIS) integration utilities |
| **`d2c_messaging/`** | Device-to-cloud messaging helpers |
| **`auto_utils/`** | RAII/auto-cleanup wrappers for C resources |
| **`apiproto_utils/`** | API protocol utilities |

### Purpose

By centralizing common functionality into small, focused libraries, the `utils` directory promotes code reuse and consistency across the agent's modules. Each utility library is independently buildable and testable.

### Dependencies

Varies by library; common dependencies include `adu_types`, OpenSSL, Parson (JSON), and standard C libraries.

---

## Architecture Overview

This section describes how the modules above fit together to form the Azure IoT Hub Device Update agent.

### High-Level Architecture

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

### How They Tie Together

#### 1. Agent Startup & Connection

The **`agent/`** module is the entry point. On startup it:
- Parses CLI arguments and loads configuration
- Uses **`communication_managers/`** to establish an IoT Hub connection
- **`communication_managers/`** delegates to **`communication_abstraction/`**, which wraps the Azure IoT Hub C SDK and abstracts whether the agent runs as a device or an IoT Edge module
- Registers PnP interfaces: **ADU Core** (for updates), **Device Info** (for device metadata), and **Diagnostics** (for remote log collection)

#### 2. Receiving an Update

When Azure IoT Hub sends a desired property update (via the device twin):
1. **`agent/adu_core_interface/`** receives the twin change callback
2. It passes the payload to **`agent_orchestration/`**, which maps the desired action (e.g., "ProcessDeployment") into an internal workflow state
3. **`adu_workflow/`** takes over as the state machine, transitioning through phases: **Download → Backup → Install → Apply**

#### 3. Executing Update Steps

**`adu_workflow/`** dispatches each phase to the **`extensions/`** framework:
- The **`extension_manager/`** dynamically loads the appropriate plugin based on the update type
- **`step_handlers/`** (APT, Script, SWUpdate, etc.) implement the actual update logic for each phase
- **`content_downloaders/`** (curl or Delivery Optimization) handle fetching update payloads
- **`update_manifest_handlers/`** parse and orchestrate multi-step update manifests
- **`download_handlers/`** provide custom download processing (e.g., delta downloads)
- **`component_enumerators/`** identify device sub-components for targeted updates

#### 4. Privileged Operations

When an extension needs elevated permissions (e.g., installing a package, rebooting):
- It invokes **`adu-shell/`**, the setuid-root helper, via a subprocess call
- `adu-shell` validates the request and executes the privileged operation

#### 5. Security — Root Key Validation

Before applying updates, **`rootkey_workflow/`** ensures the agent's root key store is current:
- Downloads and validates root key packages against hardcoded trusted keys
- Atomically updates the local key store
- These root keys are used by **`utils/jws_utils/`** and **`utils/crypto_utils/`** to verify update manifest signatures

#### 6. Diagnostics (Parallel Subsystem)

**`diagnostics_component/`** operates as a parallel subsystem:
- Receives diagnostic requests via IoT Hub direct methods
- Collects device logs asynchronously (via `diagnostics_async_helper/`)
- Uploads diagnostic data to Azure Blob Storage
- Reports status back through the device twin

#### 7. Cross-Cutting Foundations

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

### Dependency Flow (Simplified)

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
