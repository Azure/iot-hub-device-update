# IoT Hub Communication Architecture

This document provides a detailed overview of how the Azure Device Update Agent communicates with Azure IoT Hub, including the underlying SDKs, protocols, and communication patterns used.

## Overview

The Azure Device Update Agent uses the **Azure IoT C SDK** as its primary communication library to establish secure, bi-directional communication with Azure IoT Hub. The architecture follows Azure IoT's standard Device Twin pattern for reliable message exchange.

## Core Dependencies

### Azure IoT C SDK

- **Primary Library**: Azure IoT C SDK
- **Core Headers**:
  - `iothub.h` - Main IoT Hub client functionality
  - `iothub_client_options.h` - Client configuration options
- **Purpose**: Provides device and module client abstractions for IoT Hub communication

### Transport Libraries

The agent supports multiple transport protocols through these libraries:

- **`iothubtransportmqtt.h`** - MQTT protocol support
- **`iothubtransportmqtt_websockets.h`** - MQTT over WebSockets support

## Supported Communication Protocols

The agent supports two transport protocols, configurable via the `iotHubProtocol` property in `du-config.json`:

| Protocol | Configuration Value | Transport Provider | Use Case |
|----------|-------------------|-------------------|----------|
| MQTT | `"mqtt"` | `MQTT_Protocol` | Standard IoT communication |
| MQTT over WebSockets | `"mqtt/ws"` | `MQTT_WebSocket_Protocol` | Firewall-friendly environments |

### Protocol Selection

```json
{
  "iotHubProtocol": "mqtt"  // or "mqtt/ws"
}
```

If no protocol is specified, the agent defaults to MQTT.

## Client Architecture

### Dual Client Support
The agent uses an abstraction layer that supports both connection types:

- **Device Client** (`IOTHUB_DEVICE_CLIENT_LL_HANDLE`)
  - For standalone devices connecting directly to IoT Hub
  - Uses device-level authentication and twin properties

- **Module Client** (`IOTHUB_MODULE_CLIENT_LL_HANDLE`)
  - For IoT Edge modules running on edge devices
  - Inherits authentication from IoT Edge runtime

### Client Handle Abstraction
The `client_handle_helper` component provides unified APIs that work with either client type:

```c
// Unified client handle type
typedef void* ADUC_ClientHandle;

// Abstract connection type
typedef enum tagADUC_ConnType
{
    ADUC_ConnType_NotSet = 0,
    ADUC_ConnType_Device = 1,
    ADUC_ConnType_Module = 2
} ADUC_ConnType;
```

## Communication Patterns

### Receiving Data from IoT Hub

#### 1. Device Twin Synchronization
The agent uses Device Twin callbacks to receive configuration and deployment instructions:

- **Initial Sync**: `ClientHandle_GetTwinAsync()`
  - Retrieves current desired properties on startup
  - Ensures agent has latest configuration

- **Ongoing Updates**: `ClientHandle_SetClientTwinCallback()`
  - Receives real-time updates to desired properties
  - Triggers deployment workflows when new updates are available

- **Callback Type**: `IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK`

#### 2. Direct Methods
For immediate command execution:

- **Setup**: `ClientHandle_SetDeviceMethodCallback()`
- **Callback Type**: `IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC`
- **Use Cases**: 
  - Immediate actions (restart, cancel update)
  - Diagnostic commands
  - Manual deployment triggers

### Sending Data to IoT Hub

#### 1. Reported Properties
Used to communicate agent status and update progress:

- **API**: `ClientHandle_SendReportedState()`
- **Content**: JSON payload with current state
- **Examples**:
  - Agent version and capabilities
  - Current update status and progress
  - Error conditions and diagnostics
  - Installation results

#### 2. Telemetry Messages
For diagnostic information and monitoring:

- **API**: `ClientHandle_SendEventAsync()`
- **Content**: Structured telemetry data
- **Use Cases**:
  - Performance metrics
  - Diagnostic events
  - Operational status updates

## Authentication Methods

The agent supports multiple authentication mechanisms:

### 1. Connection Strings
```json
{
  "connectionData": {
    "connectionString": "HostName=hub.azure-devices.net;DeviceId=device1;SharedAccessKey=..."
  }
}
```

### 2. X.509 Certificates
```json
{
  "connectionData": {
    "HostName": "hub.azure-devices.net",
    "DeviceId": "device1"
  },
  "certificates": {
    "device": "/path/to/device-cert.pem",
    "devicePrivateKey": "/path/to/device-key.pem"
  }
}
```

### 3. Azure Identity Service (AIS)
For managed identity scenarios in Azure environments:
```json
{
  "connectionData": {
    "authType": "AIS",
    "HostName": "hub.azure-devices.net",
    "DeviceId": "device1"
  }
}
```

## Key Communication APIs

### Connection Management
```c
// Establish connection
bool ClientHandle_CreateFromConnectionString(
    ADUC_ClientHandle* handle,
    ADUC_ConnType type,
    const char* connectionString,
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol
);

// Monitor connection health
IOTHUB_CLIENT_RESULT ClientHandle_SetConnectionStatusCallback(
    ADUC_ClientHandle handle,
    IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK callback,
    void* userContext
);

// Process messages (must be called regularly)
void ClientHandle_DoWork(ADUC_ClientHandle handle);
```

### Device Twin Operations
```c
// Get current twin state
IOTHUB_CLIENT_RESULT ClientHandle_GetTwinAsync(
    ADUC_ClientHandle handle,
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK callback,
    void* userContext
);

// Register for twin updates
IOTHUB_CLIENT_RESULT ClientHandle_SetClientTwinCallback(
    ADUC_ClientHandle handle,
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK callback,
    void* userContext
);

// Send reported properties
IOTHUB_CLIENT_RESULT ClientHandle_SendReportedState(
    ADUC_ClientHandle handle,
    const unsigned char* reportedState,
    size_t size,
    IOTHUB_CLIENT_REPORTED_STATE_CALLBACK callback,
    void* userContext
);
```

### Message Operations
```c
// Send telemetry
IOTHUB_CLIENT_RESULT ClientHandle_SendEventAsync(
    ADUC_ClientHandle handle,
    IOTHUB_MESSAGE_HANDLE message,
    IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK callback,
    void* userContext
);

// Register direct method handler
IOTHUB_CLIENT_RESULT ClientHandle_SetDeviceMethodCallback(
    ADUC_ClientHandle handle,
    IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC callback,
    void* userContext
);
```

## Communication Flow

### 1. Initialization Phase
```
Agent Startup
    ↓
Create Client Handle (Device or Module)
    ↓
Establish Connection (with auth)
    ↓
Set Connection Status Callback
    ↓
Register Twin and Method Callbacks
```

### 2. Synchronization Phase
```
Get Initial Device Twin
    ↓
Parse Desired Properties
    ↓
Send Current Reported Properties
    ↓
Ready for Operations
```

### 3. Operational Phase
```
Continuous DoWork() Loop
    ↓
Process Incoming Messages
    ├── Twin Updates → Trigger Deployments
    ├── Direct Methods → Execute Commands
    └── Connection Events → Handle Reconnection
    ↓
Send Outgoing Messages
    ├── Reported Properties → Status Updates
    └── Telemetry → Diagnostics
```

## Message Processing

### Device Twin Message Format
**Desired Properties** (from IoT Hub):
```json
{
  "deviceUpdateAgent": {
    "service": {
      "workflow": {
        "action": 3,
        "id": "update-workflow-123"
      },
      "updateManifest": "{ ... update manifest JSON ... }",
      "updateManifestSignature": "signature-data"
    }
  }
}
```

**Reported Properties** (to IoT Hub):
```json
{
  "deviceUpdateAgent": {
    "client": {
      "state": 0,
      "workflow": {
        "action": 3,
        "id": "update-workflow-123",
        "retryTimestamp": "2025-10-22T10:30:00Z"
      },
      "installedUpdateId": {
        "provider": "Microsoft",
        "name": "Ubuntu",
        "version": "20.04.1"
      }
    }
  }
}
```

## Error Handling and Reliability

### Connection Resilience
- Automatic reconnection on network failures
- Configurable retry policies and timeouts
- Connection status monitoring and logging

### Message Reliability
- Confirmed delivery for reported properties
- Event confirmation callbacks for telemetry
- Persistent twin state synchronization

### Configuration Options
```c
// Set client options for reliability
ClientHandle_SetOption(handle, OPTION_KEEP_ALIVE, &keepAlive);
ClientHandle_SetOption(handle, OPTION_CONNECTION_TIMEOUT, &timeout);
ClientHandle_SetOption(handle, OPTION_RETRY_POLICY, &retryPolicy);
```

## Implementation Files

Key source files implementing IoT Hub communication:

- **`src/communication_managers/iothub_communication_manager/`** - Main communication logic
- **`src/communication_abstraction/`** - Client handle abstractions
- **`src/adu_workflow/`** - Device Twin message processing
- **`src/agent/`** - Main agent communication loop

## See Also

- [Configuration Guide](configuration-guide.md) - IoT Hub connection settings
- [Security and Authentication](security-authentication.md) - Certificate and key management
- [Troubleshooting Guide](../how-to-troubleshoot-guide.md) - Communication issues
- [Azure IoT Hub Documentation](https://docs.microsoft.com/azure/iot-hub/) - IoT Hub service details