# agent

**Type:** Main Executable (`AducIotAgent`)

## Description

The **main entry point** for the Azure IoT Hub Device Update agent. This module contains the `main()` function, command-line argument parsing, agent initialization, and the primary run loop. It connects to Azure IoT Hub, registers PnP (Plug and Play) interfaces, and coordinates all agent subsystems.

## Key Submodules

- **`src/`** — Contains the `main()` entry point, argument parsing, and agent lifecycle management.
- **`adu_core_interface/`** — Implements the ADU Core PnP interface for communicating update state with IoT Hub via device twin properties and direct methods.
- **`device_info_interface/`** — Reports device information (manufacturer, model, OS version, etc.) to IoT Hub as reported properties.
- **`pnp_helper/`** — Utility functions for PnP (IoT Plug and Play) operations such as creating PnP components and handling property callbacks.
- **`command_helper/`** — Helpers for processing agent commands and CLI options.
- **`shutdown_service/`** — Manages graceful shutdown of the agent process.
- **`adu_core_export_helpers/`** — Export helper functions for the ADU core interface callbacks.

## Responsibilities

- Parse CLI arguments and configuration
- Establish and manage the IoT Hub connection
- Register PnP interfaces (ADU Core, Device Info, Diagnostics)
- Run the main event/work loop
- Coordinate graceful shutdown

## Dependencies

Depends on `adu_workflow`, `communication_abstraction`, `communication_managers`, `extensions`, `diagnostics_component`, `adu_types`, and various utility libraries.
