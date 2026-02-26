# adu_types

**Type:** Static C Library (`libadu_types`)

## Description

The `adu_types` module defines the **common data types, enumerations, structs, and callback signatures** used throughout the Azure IoT Hub Device Update agent codebase. It serves as a foundational shared type-definition layer that other agent components depend on.

## Key Components

- **`adu_types.h`** — Agent launch arguments (`ADUC_LaunchArguments`), IoT Hub connection info (`ADUC_ConnectionInfo`), connection/auth type enums (`ADUC_ConnType`, `ADUC_AuthType`), extension registration types, and PnP property update context.
- **`types/adu_core.h`** — Core update workflow types: callback function typedefs for each workflow phase (Download, Backup, Install, Apply, Restore, Cancel, Idle, IsInstalled, Sandbox create/destroy, DoWork), the `ADUC_UpdateActionCallbacks` struct, and the comprehensive `ADUC_ResultCode` enum covering all success/failure/in-progress codes.
- **`types/update_content.h`** — Update content data types: `ADUC_UpdateId` (Provider/Name/Version), `ADUC_FileEntity` (download URIs, hashes, related files), `ADUC_RelatedFile`, `ADUC_FileUrl`, and JSON field name constants for update manifests.

## Dependencies

Minimal external dependencies; primarily consumed by other agent modules.
