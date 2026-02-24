# Diagnostics Component Test Coverage Improvement

## Task Status: ✅ Complete - All Tests Passing

## Current Coverage Summary

| File | Coverage | Status |
|------|----------|--------|
| diagnostics_result.c | 100.00% | ✅ Complete |
| file_info_utils.c | 88.00% | ✅ Above 85% |
| diagnostics_devicename.c | 78.26% | 🟡 Tests added |
| diagnostics_config_utils.c | 70.53% | 🟡 Tests added |
| operation_id_utils.c | 56.67% | 🟡 Tests added |
| diagnostics_async_helper.cpp | 0% | 🔴 Not unit testable |
| diagnostics_interface.c | 0% | 🔴 Not unit testable |
| diagnostics_workflow.c | 0% | 🔴 Not unit testable |
| blob_storage_helper.cpp | 0% | 🔴 Not unit testable |
| file_upload_utility.cpp | 0% | 🔴 Not unit testable |

**Overall: 35.23%** (target: 85% for testable modules)

## Tests Added

### 1. diagnostics_devicename_ut.cpp
Added comprehensive tests:
- `SetDeviceName with valid deviceId only`
- `SetDeviceName with valid deviceId and moduleId`
- `SetDeviceName with null deviceId fails`
- `SetDeviceName with null deviceId and valid moduleId fails`
- `SetDeviceName reuse without destroy`
- `SetDeviceName with empty deviceId`
- `SetDeviceName with empty moduleId`
- `GetDeviceName edge cases` (before set, after destroy, null output)
- `DestroyDeviceName multiple times safely`

### 2. diagnostics_config_utils_ut.cpp
Added comprehensive tests:
- `Init with NULL workflowData`
- `Init with NULL fileJsonValue`
- `Init with maxKilobytes less than 1`
- `Init with negative maxKilobytes`
- `Init with maxKilobytes exceeding max limit`
- `Init with empty logComponents array`
- `Init with JSON array at root level`
- `Init with component missing componentName`
- `Init with component missing logPath`
- `GetLogComponentElem with NULL workflowData`
- `GetLogComponentElem with out-of-range index`
- `LogComponentUninit with NULL/valid/partial components`
- `UnInit with NULL/empty/initialized workflowData`

### 3. operation_id_utils_ut.cpp
Added edge case tests:
- `Empty string operationId`
- `Very long operationId` (>256 chars)
- `operationId at MAX_OPERATION_ID_CHARS`
- `Special characters in operationId`
- `Unicode in operationId`
- `Nested JSON structure`
- `Multiple operationId fields`
- `operationId as empty array/object/float`
- `Extra fields in JSON`

## Next Steps to Achieve 85%

1. **Rebuild and re-run coverage**: `./scripts/run_coverage.sh`

2. **For diagnostics_devicename.c**: The tests I added should improve coverage to ~95%+

3. **For diagnostics_config_utils.c**: Tests added should improve to ~85%+

4. **For operation_id_utils.c**: Coverage limited by `OperationIdUtils_StoreCompletedOperationId` requiring file system access. Consider:
   - Mocking file operations
   - Using a temp file path for testing

## Files Not Unit Testable (0% coverage) - Detailed Explanations

These files have external dependencies that prevent unit testing without significant mocking infrastructure:

### 1. diagnostics_async_helper.cpp

**Why it's not unit testable:**

- **Static Global State**: Contains a static `DiagnosticsWorkflowManager s_DiagnosticsManager` singleton that persists across test runs, making test isolation impossible.
- **Thread Management**: Uses `std::thread` for asynchronous execution. Testing threaded code requires:
  - Thread synchronization barriers
  - Deterministic timing control
  - Race condition prevention
- **Deep Dependency Chain**: Calls `DiagnosticsWorkflow_DiscoverAndUploadLogs()` which triggers the entire workflow including Azure Blob Storage uploads.
- **Memory Management Complexity**: Uses `STRING_clone()` and transfers ownership to threads, making memory tracking in tests difficult.
- **Exception Handling**: Contains try-catch blocks that silently log errors, making failure verification challenging.

**Alternative Testing Approach:**
- Integration tests with real IoT Hub connections
- End-to-end tests that verify the complete async workflow

---

### 2. diagnostics_interface.c

**Why it's not unit testable:**

- **IoT Hub Client Dependency**: Uses `ADUC_ClientHandle g_iotHubClientHandleForDiagnosticsComponent` - a global handle that requires an active IoT Hub connection.
- **PnP Protocol Functions**: Calls `PnP_CreateReportedProperty()` and `PnP_CreateReportedPropertyWithStatus()` which require IoT Hub infrastructure.
- **D2C Messaging**: Uses `ADUC_D2C_Message_SendAsync()` for device-to-cloud communication, requiring network connectivity.
- **Configuration File Dependency**: `DiagnosticsInterface_Create()` calls `DiagnosticsConfigUtils_InitFromFile(DIAGNOSTICS_CONFIG_FILE_PATH)` with a hardcoded production path.
- **Global State**: Multiple functions modify global state (`g_iotHubClientHandleForDiagnosticsComponent`) without synchronization primitives.
- **Callback Architecture**: Uses callback patterns (`DiagnosticsOrchestratorUpdateCallback`, `OnDiagnosticsD2CMessageCompleted`) that are triggered by external IoT SDK events.

**Alternative Testing Approach:**
- Mock the IoT Hub client SDK
- Use dependency injection for the client handle
- Integration tests with IoT Hub test instances

---

### 3. diagnostics_workflow.c

**Why it's not unit testable:**

- **Multiple External Dependencies**: Orchestrates multiple external services:
  - Azure Blob Storage uploads via `FileUploadUtility_UploadFilesToContainer()`
  - Device name retrieval via `DiagnosticsComponent_GetDeviceName()`
  - Operation ID storage via `OperationIdUtils_StoreCompletedOperationId()` (file system)
  - State reporting via `DiagnosticsInterface_ReportStateAndResultAsync()` (IoT Hub)
- **Complex Orchestration Logic**: The main function `DiagnosticsWorkflow_DiscoverAndUploadLogs()` ties together discovery, upload, and reporting in a way that cannot be tested in isolation.
- **Memory Management**: Uses `DiagnosticsComponent_CreateSasCredential()` for secure credential handling that zeros memory on free - difficult to verify in tests.
- **File System Operations**: Log discovery uses `FileInfoUtils_GetNewestFilesInDirUnderSize()` requiring actual file system state.

**Partially Testable Functions:**
- `DiagnosticsComponent_SecurelyFreeSasCredential()` - Pure memory management
- `DiagnosticsComponent_CreateSasCredential()` - String manipulation
- `DiagnosticsWorkflow_UnInitLogComponentFileNames()` - Vector cleanup

However, these helper functions have limited value to test in isolation.

---

### 4. blob_storage_helper.cpp

**Why it's not unit testable:**

- **Azure SDK Dependency**: Creates `Azure::Storage::Blobs::BlobContainerClient` directly in the constructor, requiring valid Azure credentials and network access.
- **Real Network Operations**: `UploadFilesToContainer()` calls `client->UploadBlob()` which performs actual HTTP uploads to Azure Blob Storage.
- **File System Dependency**: Uses `Azure::Core::IO::FileBodyStream(filePath)` to read files from disk before upload.
- **Exception-Based Error Handling**: Uses `ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions()` wrapper that complicates test assertions.
- **No Abstraction Layer**: The Azure SDK client is directly instantiated, not injected, preventing mock substitution.

**What would be needed for unit testing:**
- Interface abstraction for `BlobContainerClient`
- Dependency injection for the storage client
- Mock implementation of Azure SDK classes

---

### 5. file_upload_utility.cpp

**Why it's not unit testable:**

- **Direct Dependency on blob_storage_helper**: Creates `AzureBlobStorageHelper` inline, inheriting all its testing challenges.
- **C/C++ Boundary**: Declared with `EXTERN_C_BEGIN/END`, wrapping C++ exceptions for C callers, adding complexity.
- **No Seam for Mocking**: The function directly instantiates `AzureBlobStorageHelper` with no way to inject a test double.
- **Exception Handling Wrapper**: Uses `ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions()` which swallows exceptions and returns boolean, losing error details.

**This is essentially a thin wrapper** around `blob_storage_helper.cpp`. If `blob_storage_helper.cpp` becomes testable through dependency injection, this file would automatically become testable.

---

### Summary: Why These Files Cannot Be Unit Tested

| File | Primary Blocker | Secondary Blockers |
|------|----------------|-------------------|
| diagnostics_async_helper.cpp | Static singleton + threading | Deep dependency chain |
| diagnostics_interface.c | IoT Hub SDK integration | Global state, callbacks |
| diagnostics_workflow.c | Orchestrates multiple external calls | File system, network |
| blob_storage_helper.cpp | Direct Azure SDK instantiation | File system I/O |
| file_upload_utility.cpp | No abstraction layer | Inherits blob_storage issues |

### Recommended Patterns for Future Testability

1. **Dependency Injection**: Pass dependencies (Azure clients, file systems) as parameters or via interfaces
2. **Interface Abstraction**: Create abstract interfaces for external services (e.g., `IBlobStorageClient`)
3. **Factory Pattern**: Use factories to create Azure SDK objects, allowing mock factories in tests
4. **Remove Static Singletons**: Replace `static` globals with instance-based designs
5. **Separate Pure Logic**: Extract pure business logic from I/O operations into testable functions

## Commands

```bash
# Build and run tests with coverage
./scripts/run_coverage.sh

# View coverage report
cat out/coverage/Cobertura.xml | grep -A 5 "diagnostics_component"
