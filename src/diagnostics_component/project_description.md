# Diagnostics Component Overview

The `diagnostics_component` directory implements a **remote log collection and upload system** for Azure IoT Hub Device Update. It allows the cloud service to request diagnostic logs from devices, which are then uploaded to Azure Blob Storage.

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           diagnostics_component                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────┐     ┌──────────────────────────────────────┐  │
│  │  diagnostics_interface  │────▶│      diagnostics_async_helper        │  │
│  │  (PnP Interface)        │     │  (Async Thread Management)           │  │
│  └─────────────────────────┘     └──────────────────────────────────────┘  │
│           │                                       │                         │
│           ▼                                       ▼                         │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      diagnostics_workflow                            │   │
│  │  (Discovery + Upload Orchestration)                                  │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│           │                    │                    │                       │
│           ▼                    ▼                    ▼                       │
│  ┌───────────────┐   ┌─────────────────┐   ┌──────────────────────────┐   │
│  │diagnostics_   │   │     utils/      │   │     utils/               │   │
│  │devicename     │   │  config_utils   │   │  file_info_utils         │   │
│  └───────────────┘   └─────────────────┘   └──────────────────────────┘   │
│                                                     │                       │
│                      ┌─────────────────┐   ┌──────────────────────────┐   │
│                      │     utils/      │   │     utils/               │   │
│                      │operation_id_utils│   │  file_upload_utils      │   │
│                      └─────────────────┘   └──────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Module Descriptions

### 1. **diagnostics_interface** (PnP Interface Layer)
- **Purpose**: Implements the Azure IoT PnP interface `dtmi:azure:iot:deviceUpdateDiagnosticModel;1`
- **Key Functions**:
  - `DiagnosticsInterface_Create()`: Initializes the interface and loads config from file
  - `DiagnosticsInterface_PropertyUpdateCallback()`: Handles cloud-to-device property updates (log upload requests)
  - `DiagnosticsInterface_ReportStateAndResultAsync()`: Reports upload results back to the cloud
- **Flow**: Receives requests from IoT Hub → triggers async workflow → reports results

### 2. **diagnostics_async_helper** (C++ Async Wrapper)
- **Purpose**: Provides asynchronous execution of the diagnostics workflow
- **Key Class**: `DiagnosticsWorkflowManager` - manages a single worker thread
- **Key Function**: `DiagnosticsWorkflow_DiscoverAndUploadLogsAsync()` - spawns async log upload
- **Features**: Deduplication of requests using operation IDs, thread-safe execution

### 3. **diagnostics_workflow** (Core Business Logic)
- **Purpose**: Orchestrates the complete log discovery and upload process
- **Key Functions**:
  - `DiagnosticsWorkflow_DiscoverAndUploadLogs()`: Main entry point
  - `DiagnosticsWorkflow_GetFilesForComponent()`: Discovers log files for a component
  - `DiagnosticsWorkflow_UploadFilesForComponent()`: Uploads files to Azure Blob Storage
- **Result Codes** (defined in `diagnostics_result.h`):
  - `Diagnostics_Result_Success` (200)
  - `Diagnostics_Result_NoLogsFound` (-1)
  - `Diagnostics_Result_UploadFailed` (-2)
  - `Diagnostics_Result_NoSasCredential` (-7)
  - etc.

### 4. **diagnostics_devicename** (Device Identity)
- **Purpose**: Manages device name for blob storage virtual directory paths
- **Key Functions**:
  - `DiagnosticsComponent_SetDeviceName()`: Sets device name as `<deviceId>/<moduleId>`
  - `DiagnosticsComponent_GetDeviceName()`: Retrieves the stored device name

### 5. **utils/** (Utility Libraries)

| Module | Purpose |
|--------|---------|
| **config_utils** | Parses `diagnostics-config.json` into `DiagnosticsWorkflowData` struct |
| **file_info_utils** | Scans directories for newest log files under size limit |
| **file_upload_utils** | Uploads files to Azure Blob Storage using SAS credentials |
| **operation_id_utils** | Tracks completed operation IDs to prevent duplicate uploads |

## Data Flow

1. **Cloud sends request** via PnP property update containing:
   - `storageSasUrl`: Azure Blob Storage SAS URL
   - `operationId`: Unique identifier for this request

2. **Interface layer** validates and forwards to async helper

3. **Async helper** checks for duplicate operations, spawns worker thread

4. **Workflow**:
   - Parses cloud message for credentials
   - Gets device name for blob path
   - For each configured log component:
     - Discovers newest files under size limit
     - Uploads to blob storage at path: `<device-name>/<operation-id>/<component-name>/<files>`

5. **Reports result** back to IoT Hub with result code and operation ID

## Existing Test Coverage

| Module | Test File | Coverage |
|--------|-----------|----------|
| config_utils | `diagnostics_config_utils_ut.cpp` | Init from JSON, missing fields |
| file_info_utils | `file_info_utils_ut.cpp` | Insert file info, array limits |
| **diagnostics_interface** | **None** | **No tests** |
| **diagnostics_workflow** | **None** | **No tests** |
| **diagnostics_devicename** | **None** | **No tests** |
| **operation_id_utils** | **None** | **No tests** |
| **file_upload_utils** | **None** | **No tests** |

## Key Testable Units (Missing Coverage)

1. **diagnostics_devicename**: 
   - `SetDeviceName` with various input combinations
   - `GetDeviceName` before/after setting
   - Memory management

2. **diagnostics_workflow**:
   - `DiagnosticsWorkflow_GetFilesForComponent()` - file discovery logic
   - `DiagnosticsWorkflow_UploadFilesForComponent()` - upload orchestration
   - `DiagnosticsComponent_CreateSasCredential()` / `SecurelyFreeSasCredential()` - secure memory handling
   - Error path handling for null parameters

3. **operation_id_utils**:
   - `OperationIdUtils_OperationIsComplete()` - duplicate detection
   - `OperationIdUtils_StoreCompletedOperationId()` - persistence

4. **diagnostics_interface**:
   - `DiagnosticsInterface_Create()` / `Destroy()` lifecycle
   - JSON serialization in `ReportStateAndResultAsync()`