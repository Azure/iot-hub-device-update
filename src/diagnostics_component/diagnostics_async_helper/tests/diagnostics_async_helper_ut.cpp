/**
 * @file diagnostics_async_helper_ut.cpp
 * @brief Unit Tests for the Diagnostics Async Helper module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "diagnostics_async_helper.h"
#include <diagnostics_config_utils.h>

#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include <catch2/catch_all.hpp>
#include <chrono>
#include <cstring>
#include <thread>

// ============================================================================
// --wrap mock infrastructure
// ============================================================================
extern "C"
{
    // Mock control flags
    static bool g_mock_operation_is_complete_return = false;
    static int g_mock_operation_is_complete_call_count = 0;

    static int g_mock_discover_upload_call_count = 0;
    static const char* g_mock_discover_upload_last_jsonString = nullptr;

    // --wrap implementations

    bool __wrap_OperationIdUtils_OperationIsComplete(const char* jsonString)
    {
        (void)jsonString;
        g_mock_operation_is_complete_call_count++;
        return g_mock_operation_is_complete_return;
    }

    void __wrap_DiagnosticsWorkflow_DiscoverAndUploadLogs(
        const DiagnosticsWorkflowData* workflowData, const char* jsonString)
    {
        (void)workflowData;
        g_mock_discover_upload_call_count++;
        g_mock_discover_upload_last_jsonString = jsonString;
    }
}

// ============================================================================
// Test helper class - resets all mock state
// ============================================================================
class DiagnosticsAsyncHelperTestHelper
{
public:
    DiagnosticsAsyncHelperTestHelper()
    {
        g_mock_operation_is_complete_return = false;
        g_mock_operation_is_complete_call_count = 0;
        g_mock_discover_upload_call_count = 0;
        g_mock_discover_upload_last_jsonString = nullptr;
    }

    ~DiagnosticsAsyncHelperTestHelper() = default;
};

// Helper: wait for the async worker to complete (mock returns instantly, so a short sleep suffices)
static void WaitForAsyncCompletion()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ============================================================================
// DiagnosticsWorkflow_DiscoverAndUploadLogsAsync tests
// ============================================================================
TEST_CASE("DiagnosticsWorkflow_DiscoverAndUploadLogsAsync")
{
    DiagnosticsAsyncHelperTestHelper helper;

    SECTION("Normal call triggers workflow in a thread")
    {
        g_mock_operation_is_complete_return = false;

        DiagnosticsWorkflowData data = {};
        data.maxBytesToUploadPerLogPath = 1024;
        data.components = VECTOR_create(sizeof(void*));

        const char* json = "{\"operationId\": \"op-async-1\", \"storageSasUrl\": \"https://sas.url\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogsAsync(&data, json);

        WaitForAsyncCompletion();

        CHECK(g_mock_operation_is_complete_call_count == 1);
        CHECK(g_mock_discover_upload_call_count == 1);

        VECTOR_destroy(data.components);
    }

    SECTION("Already-completed operation skips workflow")
    {
        g_mock_operation_is_complete_return = true;

        DiagnosticsWorkflowData data = {};
        data.maxBytesToUploadPerLogPath = 1024;
        data.components = VECTOR_create(sizeof(void*));

        const char* json = "{\"operationId\": \"op-already-done\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogsAsync(&data, json);

        WaitForAsyncCompletion();

        CHECK(g_mock_operation_is_complete_call_count == 1);
        CHECK(g_mock_discover_upload_call_count == 0);

        VECTOR_destroy(data.components);
    }

    SECTION("Sequential calls - second call joins first thread")
    {
        g_mock_operation_is_complete_return = false;

        DiagnosticsWorkflowData data = {};
        data.maxBytesToUploadPerLogPath = 1024;
        data.components = VECTOR_create(sizeof(void*));

        // First call starts a thread
        const char* json1 = "{\"operationId\": \"op-first\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogsAsync(&data, json1);

        WaitForAsyncCompletion();

        // Second call should join the first thread, then start a new one
        const char* json2 = "{\"operationId\": \"op-second\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogsAsync(&data, json2);

        WaitForAsyncCompletion();

        CHECK(g_mock_operation_is_complete_call_count == 2);
        CHECK(g_mock_discover_upload_call_count == 2);

        VECTOR_destroy(data.components);
    }

    SECTION("Second call with completed operation joins and returns early")
    {
        DiagnosticsWorkflowData data = {};
        data.maxBytesToUploadPerLogPath = 1024;
        data.components = VECTOR_create(sizeof(void*));

        // First call - operation not complete, runs workflow
        g_mock_operation_is_complete_return = false;
        const char* json1 = "{\"operationId\": \"op-run\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogsAsync(&data, json1);

        WaitForAsyncCompletion();

        CHECK(g_mock_discover_upload_call_count == 1);

        // Second call - operation already complete, skips workflow
        g_mock_operation_is_complete_return = true;
        const char* json2 = "{\"operationId\": \"op-run\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogsAsync(&data, json2);

        WaitForAsyncCompletion();

        CHECK(g_mock_operation_is_complete_call_count == 2);
        CHECK(g_mock_discover_upload_call_count == 1); // Not incremented for second call

        VECTOR_destroy(data.components);
    }

    SECTION("NULL workflowData is passed through to workflow")
    {
        g_mock_operation_is_complete_return = false;

        const char* json = "{\"operationId\": \"op-null-data\"}";
        DiagnosticsWorkflow_DiscoverAndUploadLogsAsync(nullptr, json);

        WaitForAsyncCompletion();

        CHECK(g_mock_operation_is_complete_call_count == 1);
        CHECK(g_mock_discover_upload_call_count == 1);
    }
}
