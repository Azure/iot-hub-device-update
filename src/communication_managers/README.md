# communication_managers

**Type:** Static C++ Library (`iothub_communication_manager`)

## Description

Implements the **concrete IoT Hub communication manager** that manages the agent's connection lifecycle with Azure IoT Hub. This module builds on top of the `communication_abstraction` layer to provide higher-level connection management capabilities.

## Key Responsibilities

- Manage the IoT Hub client connection lifecycle (connect, reconnect, disconnect)
- Handle authentication and connection string provisioning
- Coordinate device twin and direct method callback registration
- Manage reported property updates and device-to-cloud messaging
- Handle connection status monitoring and error recovery

## Submodules

- **`iothub_communication_manager/`** — The primary IoT Hub communication manager implementation

## Dependencies

Depends on `communication_abstraction`, `adu_types`, and the Azure IoT Hub C SDK.
