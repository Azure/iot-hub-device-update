# diagnostics_component

**Type:** Collection of Static Libraries

## Description

Implements the **diagnostics subsystem** for the Device Update agent. This component enables remote diagnostic data collection from devices, allowing operators to troubleshoot issues by requesting diagnostic logs and device information through IoT Hub.

## Submodules

- **`diagnostics_interface/`** — PnP interface implementation that handles diagnostic direct method calls from IoT Hub (e.g., log upload requests) and reports diagnostic state via device twin properties.
- **`diagnostics_workflow/`** — Orchestrates the diagnostic data collection workflow, coordinating log gathering, upload, and status reporting.
- **`diagnostics_async_helper/`** — Provides asynchronous execution support for diagnostic operations to avoid blocking the main agent loop.
- **`diagnostics_devicename/`** — Utility for resolving and providing the device name/hostname for diagnostic reports.
- **`utils/`** — Shared utility functions used by the diagnostics subsystem.

## Key Capabilities

- Receive diagnostic requests via IoT Hub direct methods
- Collect device logs and diagnostic information
- Upload diagnostic data to Azure Blob Storage
- Report diagnostic operation status back to IoT Hub

## Dependencies

Depends on `adu_types`, `communication_abstraction`, logging utilities, and Azure Storage SDK for blob uploads.
