/**
 * @file diagnostics_workflow_ut.cpp
 * @brief Unit Tests for the Diagnostics Workflow module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "diagnostics_workflow.h"
#include "diagnostics_result.h"
#include <diagnostics_config_utils.h>

#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include <catch2/catch_all.hpp>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <string>

// Field name constants (from diagnostics_interface.h, redefined here to avoid
// pulling in heavy IoT Hub SDK header dependencies)
#define DIAGNOSTICSITF_FIELDNAME_SASURL "storageSasUrl"
#define DIAGNOSTICSITF_FIELDNAME_OPERATIONID "operationId"

// ============================================================================
// Extern "C" declarations for non-header (but non-static) workflow functions
// ============================================================================
extern "C"
{
    void DiagnosticsComponent_SecurelyFreeSasCredential(STRING_HANDLE* sasCredential, char** memory);

    STRING_HANDLE DiagnosticsComponent_CreateSasCredential(const char* sasCredential, char** memory);

    Diagnostics_Result DiagnosticsWorkflow_GetFilesForComponent(
        VECTOR_HANDLE* fileNames, const DiagnosticsLogComponent* logComponent, long long maxUploadSize);

    Diagnostics_Result DiagnosticsWorkflow_UploadFilesForComponent(
        VECTOR_HANDLE fileNames,
        const DiagnosticsLogComponent* logComponent,
        const char* deviceName,
        const char* operationId,
        const char* storageSasUrl);

    void DiagnosticsWorkflow_UnInitLogComponentFileNames(VECTOR_HANDLE logComponentFileNames);
}

// ============================================================================
// --wrap mock infrastructure
// ============================================================================
extern "C"
{
    // Real function declarations
    int __real_mallocAndStrcpy_s(char** destination, const char* source);
    STRING_HANDLE __real_STRING_new_with_memory(char* memory);
    STRING_HANDLE __real_STRING_new(void);
    int __real_STRING_sprintf(STRING_HANDLE handle, const char* format, ...);

    // Mock control flags
    static bool g_fail_mallocAndStrcpy_s = false;
    static bool g_fail_STRING_new_with_memory = false;
    static bool g_fail_STRING_new = false;
    static bool g_fail_STRING_sprintf = false;

    // Mock for FileUploadUtility_UploadFilesToContainer
    static bool g_mock_upload_return = true;
    static int g_mock_upload_call_count = 0;

    // Mock for FileInfoUtils_GetNewestFilesInDirUnderSize
    static bool g_mock_file_discovery_return = true;
    static int g_mock_file_discovery_call_count = 0;
    static VECTOR_HANDLE g_mock_discovered_files = nullptr;

    // Mock for DiagnosticsInterface_ReportStateAndResultAsync
    static int g_mock_report_call_count = 0;
    static Diagnostics_Result g_mock_last_reported_result = Diagnostics_Result_Failure;
    static std::string g_mock_last_reported_operationId;

    // Mock for OperationIdUtils_StoreCompletedOperationId
    static bool g_mock_store_opid_return = true;
    static int g_mock_store_opid_call_count = 0;

    // Mock for DiagnosticsComponent_GetDeviceName
    static bool g_mock_get_devicename_return = true;
    static int g_mock_get_devicename_call_count = 0;

    // --wrap implementations

    int __wrap_mallocAndStrcpy_s(char** destination, const char* source)
    {
        if (g_fail_mallocAndStrcpy_s)
        {
            return 1;
        }
        return __real_mallocAndStrcpy_s(destination, source);
    }

    STRING_HANDLE __wrap_STRING_new_with_memory(char* memory)
    {
        if (g_fail_STRING_new_with_memory)
        {
            return nullptr;
        }
        return __real_STRING_new_with_memory(memory);
    }

    STRING_HANDLE __wrap_STRING_new(void)
    {
        if (g_fail_STRING_new)
        {
            return nullptr;
        }
        return __real_STRING_new();
    }

    int __wrap_STRING_sprintf(STRING_HANDLE handle, const char* format, ...)
    {
        if (g_fail_STRING_sprintf)
        {
            return 1; // non-zero means failure
        }
        va_list args;
        va_start(args, format);
        // For simplicity, use the real STRING_sprintf via a manual approach
        // since va_list forwarding to __real_ is complex. We construct the string manually.
        char buf[512];
        vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        return STRING_concat(handle, buf);
    }

    // Wrapped external dependency functions

    bool __wrap_FileUploadUtility_UploadFilesToContainer(
        const void* blobInfo, VECTOR_HANDLE fileNames, const char* directoryPath)
    {
        (void)blobInfo;
        (void)fileNames;
        (void)directoryPath;
        g_mock_upload_call_count++;
        return g_mock_upload_return;
    }

    bool __wrap_FileInfoUtils_GetNewestFilesInDirUnderSize(
        VECTOR_HANDLE* fileNames, const char* directoryPath, long long maxSize)
    {
        (void)directoryPath;
        (void)maxSize;
        g_mock_file_discovery_call_count++;
        if (!g_mock_file_discovery_return)
        {
            return false;
        }
        // Create a vector with one mock filename
        if (g_mock_discovered_files != nullptr)
        {
            *fileNames = g_mock_discovered_files;
        }
        else
        {
            *fileNames = VECTOR_create(sizeof(STRING_HANDLE));
            STRING_HANDLE name = STRING_construct("test.log");
            VECTOR_push_back(*fileNames, &name, 1);
        }
        return true;
    }

    void __wrap_DiagnosticsInterface_ReportStateAndResultAsync(
        Diagnostics_Result result, const char* operationId)
    {
        g_mock_report_call_count++;
        g_mock_last_reported_result = result;
        g_mock_last_reported_operationId = (operationId != nullptr) ? operationId : "";
    }

    bool __wrap_OperationIdUtils_StoreCompletedOperationId(const char* operationId)
    {
        (void)operationId;
        g_mock_store_opid_call_count++;
        return g_mock_store_opid_return;
    }

    bool __wrap_DiagnosticsComponent_GetDeviceName(char** deviceName)
    {
        g_mock_get_devicename_call_count++;
        if (!g_mock_get_devicename_return)
        {
            return false;
        }
        *deviceName = strdup("test-device");
        return true;
    }
}

// ============================================================================
// Test helper class - resets all mock state
// ============================================================================
class DiagnosticsWorkflowTestHelper
{
public:
    DiagnosticsWorkflowTestHelper()
    {
        g_fail_mallocAndStrcpy_s = false;
        g_fail_STRING_new_with_memory = false;
        g_fail_STRING_new = false;
        g_fail_STRING_sprintf = false;
        g_mock_upload_return = true;
        g_mock_upload_call_count = 0;
        g_mock_file_discovery_return = true;
        g_mock_file_discovery_call_count = 0;
        g_mock_discovered_files = nullptr;
        g_mock_report_call_count = 0;
        g_mock_last_reported_result = Diagnostics_Result_Failure;
        g_mock_last_reported_operationId.clear();
        g_mock_store_opid_return = true;
        g_mock_store_opid_call_count = 0;
        g_mock_get_devicename_return = true;
        g_mock_get_devicename_call_count = 0;
    }

    ~DiagnosticsWorkflowTestHelper() = default;
};

// Helper to create a DiagnosticsWorkflowData with one component
static DiagnosticsWorkflowData CreateTestWorkflowData(
    const char* componentName, const char* logPath, long long maxBytes)
{
    DiagnosticsWorkflowData data = {};
    data.maxBytesToUploadPerLogPath = maxBytes;
    data.components = VECTOR_create(sizeof(DiagnosticsLogComponent));

    DiagnosticsLogComponent comp = {};
    comp.componentName = STRING_construct(componentName);
    comp.logPath = STRING_construct(logPath);
    VECTOR_push_back(data.components, &comp, 1);

    return data;
}

static void CleanupTestWorkflowData(DiagnosticsWorkflowData* data)
{
    if (data == nullptr || data->components == nullptr)
    {
        return;
    }
    size_t count = VECTOR_size(data->components);
    for (size_t i = 0; i < count; ++i)
    {
        DiagnosticsLogComponent* comp =
            static_cast<DiagnosticsLogComponent*>(VECTOR_element(data->components, i));
        STRING_delete(comp->componentName);
        STRING_delete(comp->logPath);
    }
    VECTOR_destroy(data->components);
    data->components = nullptr;
}

// ============================================================================
// SecurelyFreeSasCredential tests
// ============================================================================
TEST_CASE("DiagnosticsComponent_SecurelyFreeSasCredential")
{
    DiagnosticsWorkflowTestHelper helper;

    SECTION("NULL memory pointer does nothing")
    {
        DiagnosticsComponent_SecurelyFreeSasCredential(nullptr, nullptr);
        // Should not crash
    }

    SECTION("NULL memory value does nothing")
    {
        STRING_HANDLE handle = STRING_construct("test");
        char* memory = nullptr;
        DiagnosticsComponent_SecurelyFreeSasCredential(&handle, &memory);
        // handle should remain unchanged since memory is NULL
        STRING_delete(handle);
    }

    SECTION("Valid credential is securely freed")
    {
        char* memory = nullptr;
        STRING_HANDLE handle = DiagnosticsComponent_CreateSasCredential("https://test.blob.core.windows.net", &memory);
        REQUIRE(handle != nullptr);
        REQUIRE(memory != nullptr);

        DiagnosticsComponent_SecurelyFreeSasCredential(&handle, &memory);
        CHECK(handle == nullptr);
        CHECK(memory == nullptr);
    }
}

// ============================================================================
// CreateSasCredential tests
// ============================================================================
TEST_CASE("DiagnosticsComponent_CreateSasCredential")
{
    DiagnosticsWorkflowTestHelper helper;

    SECTION("NULL memory parameter returns NULL")
    {
        STRING_HANDLE result = DiagnosticsComponent_CreateSasCredential("test", nullptr);
        CHECK(result == nullptr);
    }

    SECTION("NULL sasCredential parameter returns NULL")
    {
        char* memory = nullptr;
        STRING_HANDLE result = DiagnosticsComponent_CreateSasCredential(nullptr, &memory);
        CHECK(result == nullptr);
        CHECK(memory == nullptr);
    }

    SECTION("Valid inputs return handle")
    {
        char* memory = nullptr;
        STRING_HANDLE result = DiagnosticsComponent_CreateSasCredential("https://sas.url/token", &memory);
        REQUIRE(result != nullptr);
        CHECK(strcmp(STRING_c_str(result), "https://sas.url/token") == 0);
        CHECK(memory != nullptr);
        DiagnosticsComponent_SecurelyFreeSasCredential(&result, &memory);
    }

    SECTION("mallocAndStrcpy_s failure returns NULL")
    {
        g_fail_mallocAndStrcpy_s = true;
        char* memory = nullptr;
        STRING_HANDLE result = DiagnosticsComponent_CreateSasCredential("test", &memory);
        CHECK(result == nullptr);
    }

    SECTION("STRING_new_with_memory failure returns NULL and cleans up")
    {
        g_fail_STRING_new_with_memory = true;
        char* memory = nullptr;
        STRING_HANDLE result = DiagnosticsComponent_CreateSasCredential("test", &memory);
        CHECK(result == nullptr);
    }
}

// ============================================================================
// GetFilesForComponent tests
// ============================================================================
TEST_CASE("DiagnosticsWorkflow_GetFilesForComponent")
{
    DiagnosticsWorkflowTestHelper helper;

    SECTION("NULL logComponent returns Failure")
    {
        VECTOR_HANDLE fileNames = nullptr;
        Diagnostics_Result result = DiagnosticsWorkflow_GetFilesForComponent(&fileNames, nullptr, 1024);
        CHECK(result == Diagnostics_Result_Failure);
    }

    SECTION("Zero maxUploadSize returns Failure")
    {
        DiagnosticsLogComponent comp = {};
        comp.componentName = STRING_construct("test");
        comp.logPath = STRING_construct("/tmp/logs");

        VECTOR_HANDLE fileNames = nullptr;
        Diagnostics_Result result = DiagnosticsWorkflow_GetFilesForComponent(&fileNames, &comp, 0);
        CHECK(result == Diagnostics_Result_Failure);

        STRING_delete(comp.componentName);
        STRING_delete(comp.logPath);
    }

    SECTION("NULL fileNames returns Failure")
    {
        DiagnosticsLogComponent comp = {};
        comp.componentName = STRING_construct("test");
        comp.logPath = STRING_construct("/tmp/logs");

        Diagnostics_Result result = DiagnosticsWorkflow_GetFilesForComponent(nullptr, &comp, 1024);
        CHECK(result == Diagnostics_Result_Failure);

        STRING_delete(comp.componentName);
        STRING_delete(comp.logPath);
    }

    SECTION("File discovery failure returns NoLogsFound")
    {
        g_mock_file_discovery_return = false;

        DiagnosticsLogComponent comp = {};
        comp.componentName = STRING_construct("adu-agent");
        comp.logPath = STRING_construct("/tmp/logs");

        VECTOR_HANDLE fileNames = nullptr;
        Diagnostics_Result result = DiagnosticsWorkflow_GetFilesForComponent(&fileNames, &comp, 1024);
        CHECK(result == Diagnostics_Result_NoLogsFound);
        CHECK(g_mock_file_discovery_call_count == 1);

        STRING_delete(comp.componentName);
        STRING_delete(comp.logPath);
    }

    SECTION("Successful discovery returns Success")
    {
        g_mock_file_discovery_return = true;

        DiagnosticsLogComponent comp = {};
        comp.componentName = STRING_construct("adu-agent");
        comp.logPath = STRING_construct("/tmp/logs");

        VECTOR_HANDLE fileNames = nullptr;
        Diagnostics_Result result = DiagnosticsWorkflow_GetFilesForComponent(&fileNames, &comp, 1024);
        CHECK(result == Diagnostics_Result_Success);
        CHECK(g_mock_file_discovery_call_count == 1);
        REQUIRE(fileNames != nullptr);
        CHECK(VECTOR_size(fileNames) == 1);

        // Cleanup discovered file names
        for (size_t i = 0; i < VECTOR_size(fileNames); ++i)
        {
            STRING_HANDLE* name = static_cast<STRING_HANDLE*>(VECTOR_element(fileNames, i));
            STRING_delete(*name);
        }
        VECTOR_destroy(fileNames);

        STRING_delete(comp.componentName);
        STRING_delete(comp.logPath);
    }
}

// ============================================================================
// UploadFilesForComponent tests
// ============================================================================
TEST_CASE("DiagnosticsWorkflow_UploadFilesForComponent")
{
    DiagnosticsWorkflowTestHelper helper;

    SECTION("NULL fileNames returns Failure")
    {
        DiagnosticsLogComponent comp = {};
        comp.componentName = STRING_construct("test");
        comp.logPath = STRING_construct("/tmp");
        Diagnostics_Result result =
            DiagnosticsWorkflow_UploadFilesForComponent(nullptr, &comp, "dev", "op1", "sas://url");
        CHECK(result == Diagnostics_Result_Failure);
        STRING_delete(comp.componentName);
        STRING_delete(comp.logPath);
    }

    SECTION("NULL logComponent returns Failure")
    {
        VECTOR_HANDLE fileNames = VECTOR_create(sizeof(STRING_HANDLE));
        Diagnostics_Result result =
            DiagnosticsWorkflow_UploadFilesForComponent(fileNames, nullptr, "dev", "op1", "sas://url");
        CHECK(result == Diagnostics_Result_Failure);
        VECTOR_destroy(fileNames);
    }

    SECTION("NULL deviceName returns Failure")
    {
        VECTOR_HANDLE fileNames = VECTOR_create(sizeof(STRING_HANDLE));
        DiagnosticsLogComponent comp = {};
        comp.componentName = STRING_construct("test");
        comp.logPath = STRING_construct("/tmp");
        Diagnostics_Result result =
            DiagnosticsWorkflow_UploadFilesForComponent(fileNames, &comp, nullptr, "op1", "sas://url");
        CHECK(result == Diagnostics_Result_Failure);
        VECTOR_destroy(fileNames);
        STRING_delete(comp.componentName);
        STRING_delete(comp.logPath);
    }

    SECTION("NULL operationId returns Failure")
    {
        VECTOR_HANDLE fileNames = VECTOR_create(sizeof(STRING_HANDLE));
        DiagnosticsLogComponent comp = {};
        comp.componentName = STRING_construct("test");
        comp.logPath = STRING_construct("/tmp");
        Diagnostics_Result result =
            DiagnosticsWorkflow_UploadFilesForComponent(fileNames, &comp, "dev", nullptr, "sas://url");
        CHECK(result == Diagnostics_Result_Failure);
        VECTOR_destroy(fileNames);
        STRING_delete(comp.componentName);
        STRING_delete(comp.logPath);
    }

    SECTION("NULL storageSasUrl returns Failure")
    {
        VECTOR_HANDLE fileNames = VECTOR_create(sizeof(STRING_HANDLE));
        DiagnosticsLogComponent comp = {};
        comp.componentName = STRING_construct("test");
        comp.logPath = STRING_construct("/tmp");
        Diagnostics_Result result =
            DiagnosticsWorkflow_UploadFilesForComponent(fileNames, &comp, "dev", "op1", nullptr);
        CHECK(result == Diagnostics_Result_Failure);
        VECTOR_destroy(fileNames);
        STRING_delete(comp.componentName);
        STRING_delete(comp.logPath);
    }

    SECTION("Uninitialized logComponent componentName returns Failure")
    {
        VECTOR_HANDLE fileNames = VECTOR_create(sizeof(STRING_HANDLE));
        DiagnosticsLogComponent comp = {};
        comp.componentName = nullptr;
        comp.logPath = STRING_construct("/tmp");
        Diagnostics_Result result =
            DiagnosticsWorkflow_UploadFilesForComponent(fileNames, &comp, "dev", "op1", "sas://url");
        CHECK(result == Diagnostics_Result_Failure);
        VECTOR_destroy(fileNames);
        STRING_delete(comp.logPath);
    }

    SECTION("Uninitialized logComponent logPath returns Failure")
    {
        VECTOR_HANDLE fileNames = VECTOR_create(sizeof(STRING_HANDLE));
        DiagnosticsLogComponent comp = {};
        comp.componentName = STRING_construct("test");
        comp.logPath = nullptr;
        Diagnostics_Result result =
            DiagnosticsWorkflow_UploadFilesForComponent(fileNames, &comp, "dev", "op1", "sas://url");
        CHECK(result == Diagnostics_Result_Failure);
        VECTOR_destroy(fileNames);
        STRING_delete(comp.componentName);
    }

    SECTION("Successful upload returns Success")
    {
        g_mock_upload_return = true;

        VECTOR_HANDLE fileNames = VECTOR_create(sizeof(STRING_HANDLE));
        STRING_HANDLE fileName = STRING_construct("test.log");
        VECTOR_push_back(fileNames, &fileName, 1);

        DiagnosticsLogComponent comp = {};
        comp.componentName = STRING_construct("adu-agent");
        comp.logPath = STRING_construct("/var/log/adu");

        Diagnostics_Result result =
            DiagnosticsWorkflow_UploadFilesForComponent(fileNames, &comp, "device1", "op-123", "https://sas.url/token");
        CHECK(result == Diagnostics_Result_Success);
        CHECK(g_mock_upload_call_count == 1);

        STRING_delete(fileName);
        VECTOR_destroy(fileNames);
        STRING_delete(comp.componentName);
        STRING_delete(comp.logPath);
    }

    SECTION("Upload failure returns UploadFailed")
    {
        g_mock_upload_return = false;

        VECTOR_HANDLE fileNames = VECTOR_create(sizeof(STRING_HANDLE));
        STRING_HANDLE fileName = STRING_construct("test.log");
        VECTOR_push_back(fileNames, &fileName, 1);

        DiagnosticsLogComponent comp = {};
        comp.componentName = STRING_construct("adu-agent");
        comp.logPath = STRING_construct("/var/log/adu");

        Diagnostics_Result result =
            DiagnosticsWorkflow_UploadFilesForComponent(fileNames, &comp, "device1", "op-123", "https://sas.url/token");
        CHECK(result == Diagnostics_Result_UploadFailed);

        STRING_delete(fileName);
        VECTOR_destroy(fileNames);
        STRING_delete(comp.componentName);
        STRING_delete(comp.logPath);
    }

    SECTION("SAS credential creation failure returns Failure")
    {
        g_fail_mallocAndStrcpy_s = true;

        VECTOR_HANDLE fileNames = VECTOR_create(sizeof(STRING_HANDLE));
        DiagnosticsLogComponent comp = {};
        comp.componentName = STRING_construct("adu-agent");
        comp.logPath = STRING_construct("/var/log/adu");

        Diagnostics_Result result =
            DiagnosticsWorkflow_UploadFilesForComponent(fileNames, &comp, "device1", "op-123", "https://sas.url/token");
        CHECK(result == Diagnostics_Result_Failure);

        VECTOR_destroy(fileNames);
        STRING_delete(comp.componentName);
        STRING_delete(comp.logPath);
    }

    SECTION("STRING_new failure for virtualDirectoryPath returns Failure")
    {
        g_fail_STRING_new = true;

        VECTOR_HANDLE fileNames = VECTOR_create(sizeof(STRING_HANDLE));
        DiagnosticsLogComponent comp = {};
        comp.componentName = STRING_construct("adu-agent");
        comp.logPath = STRING_construct("/var/log/adu");

        Diagnostics_Result result =
            DiagnosticsWorkflow_UploadFilesForComponent(fileNames, &comp, "device1", "op-123", "https://sas.url/token");
        CHECK(result == Diagnostics_Result_Failure);

        VECTOR_destroy(fileNames);
        STRING_delete(comp.componentName);
        STRING_delete(comp.logPath);
    }

    SECTION("STRING_sprintf failure returns Failure")
    {
        g_fail_STRING_sprintf = true;

        VECTOR_HANDLE fileNames = VECTOR_create(sizeof(STRING_HANDLE));
        DiagnosticsLogComponent comp = {};
        comp.componentName = STRING_construct("adu-agent");
        comp.logPath = STRING_construct("/var/log/adu");

        Diagnostics_Result result =
            DiagnosticsWorkflow_UploadFilesForComponent(fileNames, &comp, "device1", "op-123", "https://sas.url/token");
        CHECK(result == Diagnostics_Result_Failure);

        VECTOR_destroy(fileNames);
        STRING_delete(comp.componentName);
        STRING_delete(comp.logPath);
    }
}

// ============================================================================
// UnInitLogComponentFileNames tests
// ============================================================================
TEST_CASE("DiagnosticsWorkflow_UnInitLogComponentFileNames")
{
    DiagnosticsWorkflowTestHelper helper;

    SECTION("NULL input does nothing")
    {
        DiagnosticsWorkflow_UnInitLogComponentFileNames(nullptr);
        // Should not crash
    }

    SECTION("Empty vector does nothing")
    {
        VECTOR_HANDLE outer = VECTOR_create(sizeof(VECTOR_HANDLE));
        DiagnosticsWorkflow_UnInitLogComponentFileNames(outer);
        VECTOR_destroy(outer);
    }

    SECTION("Properly cleans up nested vectors")
    {
        VECTOR_HANDLE outer = VECTOR_create(sizeof(VECTOR_HANDLE));

        VECTOR_HANDLE inner = VECTOR_create(sizeof(STRING_HANDLE));
        STRING_HANDLE name1 = STRING_construct("file1.log");
        STRING_HANDLE name2 = STRING_construct("file2.log");
        VECTOR_push_back(inner, &name1, 1);
        VECTOR_push_back(inner, &name2, 1);
        VECTOR_push_back(outer, &inner, 1);

        VECTOR_HANDLE inner2 = VECTOR_create(sizeof(STRING_HANDLE));
        STRING_HANDLE name3 = STRING_construct("file3.log");
        VECTOR_push_back(inner2, &name3, 1);
        VECTOR_push_back(outer, &inner2, 1);

        DiagnosticsWorkflow_UnInitLogComponentFileNames(outer);
        VECTOR_destroy(outer);
    }
}

// ============================================================================
// DiscoverAndUploadLogs tests
// ============================================================================
TEST_CASE("DiagnosticsWorkflow_DiscoverAndUploadLogs")
{
    DiagnosticsWorkflowTestHelper helper;

    SECTION("NULL jsonString reports failure")
    {
        DiagnosticsWorkflowData data = CreateTestWorkflowData("adu", "/tmp/logs", 1024);
        DiagnosticsWorkflow_DiscoverAndUploadLogs(&data, nullptr);
        CHECK(g_mock_report_call_count == 1);
        CHECK(g_mock_last_reported_result == Diagnostics_Result_Failure);
        CleanupTestWorkflowData(&data);
    }

    SECTION("NULL workflowData reports NoDiagnosticsComponents")
    {
        const char* json = "{\"operationId\": \"op1\", \"storageSasUrl\": \"https://sas.url\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogs(nullptr, json);
        CHECK(g_mock_report_call_count == 1);
        CHECK(g_mock_last_reported_result == Diagnostics_Result_NoDiagnosticsComponents);
    }

    SECTION("Empty components vector reports NoDiagnosticsComponents")
    {
        DiagnosticsWorkflowData data = {};
        data.maxBytesToUploadPerLogPath = 1024;
        data.components = VECTOR_create(sizeof(DiagnosticsLogComponent));

        const char* json = "{\"operationId\": \"op1\", \"storageSasUrl\": \"https://sas.url\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogs(&data, json);
        CHECK(g_mock_report_call_count == 1);
        CHECK(g_mock_last_reported_result == Diagnostics_Result_NoDiagnosticsComponents);

        VECTOR_destroy(data.components);
    }

    SECTION("Invalid JSON reports failure")
    {
        DiagnosticsWorkflowData data = CreateTestWorkflowData("adu", "/tmp/logs", 1024);
        DiagnosticsWorkflow_DiscoverAndUploadLogs(&data, "not valid json {{{");
        CHECK(g_mock_report_call_count == 1);
        CHECK(g_mock_last_reported_result == Diagnostics_Result_Failure);
        CleanupTestWorkflowData(&data);
    }

    SECTION("Missing operationId reports NoOperationId")
    {
        DiagnosticsWorkflowData data = CreateTestWorkflowData("adu", "/tmp/logs", 1024);
        const char* json = "{\"storageSasUrl\": \"https://sas.url\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogs(&data, json);
        CHECK(g_mock_report_call_count == 1);
        CHECK(g_mock_last_reported_result == Diagnostics_Result_NoOperationId);
        CleanupTestWorkflowData(&data);
    }

    SECTION("Missing storageSasUrl reports NoSasCredential")
    {
        DiagnosticsWorkflowData data = CreateTestWorkflowData("adu", "/tmp/logs", 1024);
        const char* json = "{\"operationId\": \"op-123\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogs(&data, json);
        CHECK(g_mock_report_call_count == 1);
        CHECK(g_mock_last_reported_result == Diagnostics_Result_NoSasCredential);
        CHECK(g_mock_last_reported_operationId == "op-123");
        CleanupTestWorkflowData(&data);
    }

    SECTION("Zero maxBytesToUploadPerLogPath reports Failure")
    {
        DiagnosticsWorkflowData data = CreateTestWorkflowData("adu", "/tmp/logs", 0);
        const char* json = "{\"operationId\": \"op-123\", \"storageSasUrl\": \"https://sas.url\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogs(&data, json);
        CHECK(g_mock_report_call_count == 1);
        CHECK(g_mock_last_reported_result == Diagnostics_Result_Failure);
        CleanupTestWorkflowData(&data);
    }

    SECTION("GetDeviceName failure reports Failure")
    {
        g_mock_get_devicename_return = false;
        DiagnosticsWorkflowData data = CreateTestWorkflowData("adu", "/tmp/logs", 1024);
        const char* json = "{\"operationId\": \"op-123\", \"storageSasUrl\": \"https://sas.url\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogs(&data, json);
        CHECK(g_mock_report_call_count == 1);
        CHECK(g_mock_last_reported_result == Diagnostics_Result_Failure);
        CHECK(g_mock_get_devicename_call_count == 1);
        CleanupTestWorkflowData(&data);
    }

    SECTION("File discovery failure reports NoLogsFound")
    {
        g_mock_file_discovery_return = false;
        DiagnosticsWorkflowData data = CreateTestWorkflowData("adu", "/tmp/logs", 1024);
        const char* json = "{\"operationId\": \"op-123\", \"storageSasUrl\": \"https://sas.url\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogs(&data, json);
        CHECK(g_mock_report_call_count == 1);
        CHECK(g_mock_last_reported_result == Diagnostics_Result_NoLogsFound);
        CleanupTestWorkflowData(&data);
    }

    SECTION("Upload failure reports UploadFailed")
    {
        g_mock_upload_return = false;
        DiagnosticsWorkflowData data = CreateTestWorkflowData("adu", "/tmp/logs", 1024);
        const char* json = "{\"operationId\": \"op-123\", \"storageSasUrl\": \"https://sas.url\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogs(&data, json);
        CHECK(g_mock_report_call_count == 1);
        CHECK(g_mock_last_reported_result == Diagnostics_Result_UploadFailed);
        CleanupTestWorkflowData(&data);
    }

    SECTION("Full successful workflow reports Success and stores operation ID")
    {
        DiagnosticsWorkflowData data = CreateTestWorkflowData("adu", "/tmp/logs", 1024);
        const char* json = "{\"operationId\": \"op-success\", \"storageSasUrl\": \"https://sas.url/token\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogs(&data, json);

        CHECK(g_mock_report_call_count == 1);
        CHECK(g_mock_last_reported_result == Diagnostics_Result_Success);
        CHECK(g_mock_last_reported_operationId == "op-success");
        CHECK(g_mock_store_opid_call_count == 1);
        CHECK(g_mock_get_devicename_call_count == 1);
        CHECK(g_mock_file_discovery_call_count == 1);
        CHECK(g_mock_upload_call_count == 1);
        CleanupTestWorkflowData(&data);
    }

    SECTION("StoreCompletedOperationId failure still completes (just warns)")
    {
        g_mock_store_opid_return = false;
        DiagnosticsWorkflowData data = CreateTestWorkflowData("adu", "/tmp/logs", 1024);
        const char* json = "{\"operationId\": \"op-store-fail\", \"storageSasUrl\": \"https://sas.url/token\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogs(&data, json);

        CHECK(g_mock_report_call_count == 1);
        CHECK(g_mock_last_reported_result == Diagnostics_Result_Success);
        CHECK(g_mock_store_opid_call_count == 1);
        CleanupTestWorkflowData(&data);
    }

    SECTION("Multiple components all discovered and uploaded")
    {
        DiagnosticsWorkflowData data = {};
        data.maxBytesToUploadPerLogPath = 2048;
        data.components = VECTOR_create(sizeof(DiagnosticsLogComponent));

        DiagnosticsLogComponent comp1 = {};
        comp1.componentName = STRING_construct("adu-agent");
        comp1.logPath = STRING_construct("/var/log/adu");
        VECTOR_push_back(data.components, &comp1, 1);

        DiagnosticsLogComponent comp2 = {};
        comp2.componentName = STRING_construct("do-agent");
        comp2.logPath = STRING_construct("/var/log/deliveryoptimization-agent");
        VECTOR_push_back(data.components, &comp2, 1);

        const char* json = "{\"operationId\": \"op-multi\", \"storageSasUrl\": \"https://sas.url/token\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogs(&data, json);

        CHECK(g_mock_report_call_count == 1);
        CHECK(g_mock_last_reported_result == Diagnostics_Result_Success);
        CHECK(g_mock_file_discovery_call_count == 2);
        CHECK(g_mock_upload_call_count == 2);

        CleanupTestWorkflowData(&data);
    }
}
