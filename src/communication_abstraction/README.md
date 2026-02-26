# communication_abstraction

**Type:** Static C Library (`communication_abstraction`)

## Description

Provides an **abstract interface layer over the Azure IoT Hub Device Client and Module Client SDKs**. It defines wrapper functions that allow the rest of the codebase to work uniformly regardless of whether the agent connects as an IoT Hub device or as an IoT Edge module.

## Key Functions

- `ClientHandle_CreateFromConnectionString` — Create a client connection from a connection string
- `ClientHandle_SetConnectionStatusCallback` — Register connection status change callbacks
- `ClientHandle_SetDeviceTwinCallback` — Register device twin change callbacks
- `ClientHandle_SetDeviceMethodCallback` — Register direct method callbacks
- `ClientHandle_SendEventAsync` — Send device-to-cloud events
- `ClientHandle_SendReportedState` — Report device state to IoT Hub
- `ClientHandle_SetOption` — Set client options
- `ClientHandle_Destroy` — Clean up and destroy the client handle

## Purpose

By abstracting the IoT Hub SDK client handles, this module decouples the agent's business logic from the specifics of the underlying transport and connection type (device vs. module identity).

## Dependencies

Depends on the Azure IoT Hub C SDK (`IotHubClient`, `umqtt`, MQTT transport).
