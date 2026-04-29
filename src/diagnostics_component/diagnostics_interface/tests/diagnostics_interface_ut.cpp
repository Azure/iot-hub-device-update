/**
 * @file diagnostics_interface_ut.cpp
 * @brief Unit Tests for the Diagnostics Interface module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "diagnostics_interface.h"
#include "diagnostics_result.h"
#include <diagnostics_config_utils.h>

#include <aduc/d2c_messaging.h>
#include <azure_c_shared_utility/strings.h>
#include <catch2/catch_all.hpp>
#include <cstdlib>
#include <cstring>
#include <parson.h>
#include <pnp_protocol.h>
#include <string>

// ============================================================================
// Extern "C" for non-header (but non-static) interface functions
// ============================================================================
extern "C"
{
    void DiagnosticsOrchestratorUpdateCallback(
        ADUC_ClientHandle clientHandle, JSON_Value* propertyValue, int propertyVersion, void* context);
}

// ============================================================================
// --wrap mock infrastructure (GCC/ld --wrap linker option)
// On MSVC, --wrap is not available; mock-dependent tests are skipped.
// ============================================================================
#if !defined(_MSC_VER)
extern "C"
{
    // Mock control flags
    static bool g_mock_initFromFile_return = true;
    static int g_mock_initFromFile_call_count = 0;

    static int g_mock_uninit_call_count = 0;

    static STRING_HANDLE g_mock_pnp_reported_return = nullptr;
    static int g_mock_pnp_reported_call_count = 0;

    static STRING_HANDLE g_mock_pnp_reported_status_return = nullptr;
    static int g_mock_pnp_reported_status_call_count = 0;

    static bool g_mock_d2c_send_return = true;
    static int g_mock_d2c_send_call_count = 0;
    static ADUC_D2C_MESSAGE_COMPLETED_CALLBACK g_mock_d2c_last_completed_callback = nullptr;

    static int g_mock_async_discover_call_count = 0;

    // --wrap implementations

    bool __wrap_DiagnosticsConfigUtils_InitFromFile(DiagnosticsWorkflowData* workflowData, const char* filePath)
    {
        (void)workflowData;
        (void)filePath;
        g_mock_initFromFile_call_count++;
        return g_mock_initFromFile_return;
    }

    void __wrap_DiagnosticsConfigUtils_UnInit(DiagnosticsWorkflowData* workflowData)
    {
        (void)workflowData;
        g_mock_uninit_call_count++;
    }

    STRING_HANDLE __wrap_PnP_CreateReportedProperty(
        const char* componentName, const char* propertyName, const char* propertyValue)
    {
        (void)componentName;
        (void)propertyName;
        (void)propertyValue;
        g_mock_pnp_reported_call_count++;
        return g_mock_pnp_reported_return;
    }

    STRING_HANDLE __wrap_PnP_CreateReportedPropertyWithStatus(
        const char* componentName,
        const char* propertyName,
        const char* propertyValue,
        int status,
        const char* description,
        int ackVersion)
    {
        (void)componentName;
        (void)propertyName;
        (void)propertyValue;
        (void)status;
        (void)description;
        (void)ackVersion;
        g_mock_pnp_reported_status_call_count++;
        return g_mock_pnp_reported_status_return;
    }

    bool __wrap_ADUC_D2C_Message_SendAsync(
        ADUC_D2C_Message_Type type,
        void* cloudServiceHandle,
        const char* message,
        ADUC_D2C_MESSAGE_HTTP_RESPONSE_CALLBACK responseCallback,
        ADUC_D2C_MESSAGE_COMPLETED_CALLBACK completedCallback,
        ADUC_D2C_MESSAGE_STATUS_CHANGED_CALLBACK statusChangedCallback,
        void* userData)
    {
        (void)type;
        (void)cloudServiceHandle;
        (void)message;
        (void)responseCallback;
        (void)statusChangedCallback;
        (void)userData;
        g_mock_d2c_send_call_count++;
        g_mock_d2c_last_completed_callback = completedCallback;
        return g_mock_d2c_send_return;
    }

    void __wrap_DiagnosticsWorkflow_DiscoverAndUploadLogsAsync(
        const DiagnosticsWorkflowData* workflowData, const char* jsonString)
    {
        (void)workflowData;
        (void)jsonString;
        g_mock_async_discover_call_count++;
    }
}

// ============================================================================
// Test helper class - resets all mock state
// ============================================================================
class DiagnosticsInterfaceTestHelper
{
public:
    DiagnosticsInterfaceTestHelper()
    {
        g_mock_initFromFile_return = true;
        g_mock_initFromFile_call_count = 0;
        g_mock_uninit_call_count = 0;
        g_mock_pnp_reported_return = nullptr;
        g_mock_pnp_reported_call_count = 0;
        g_mock_pnp_reported_status_return = nullptr;
        g_mock_pnp_reported_status_call_count = 0;
        g_mock_d2c_send_return = true;
        g_mock_d2c_send_call_count = 0;
        g_mock_d2c_last_completed_callback = nullptr;
        g_mock_async_discover_call_count = 0;
        g_iotHubClientHandleForDiagnosticsComponent = nullptr;
    }

    ~DiagnosticsInterfaceTestHelper() = default;
};

// ============================================================================
// DiagnosticsInterface_Create tests
// ============================================================================
TEST_CASE("DiagnosticsInterface_Create")
{
    DiagnosticsInterfaceTestHelper helper;

    SECTION("Successful creation")
    {
        g_mock_initFromFile_return = true;
        void* context = nullptr;
        bool result = DiagnosticsInterface_Create(&context, 0, nullptr);
        CHECK(result == true);
        CHECK(context != nullptr);
        CHECK(g_mock_initFromFile_call_count == 1);

        // Cleanup
        DiagnosticsInterface_Destroy(&context);
    }

    SECTION("InitFromFile failure returns false")
    {
        g_mock_initFromFile_return = false;
        void* context = nullptr;
        bool result = DiagnosticsInterface_Create(&context, 0, nullptr);
        CHECK(result == false);
        CHECK(context == nullptr);
        CHECK(g_mock_initFromFile_call_count == 1);
        CHECK(g_mock_uninit_call_count == 1); // Should call uninit on failure path
    }
}

// ============================================================================
// DiagnosticsInterface_Connected tests
// ============================================================================
TEST_CASE("DiagnosticsInterface_Connected")
{
    DiagnosticsInterfaceTestHelper helper;

    SECTION("Connected does not crash with NULL context")
    {
        DiagnosticsInterface_Connected(nullptr);
        // Should just log and not crash
    }

    SECTION("Connected does not crash with valid context")
    {
        int dummyContext = 42;
        DiagnosticsInterface_Connected(&dummyContext);
        // Should just log and not crash
    }
}

// ============================================================================
// DiagnosticsInterface_Destroy tests
// ============================================================================
TEST_CASE("DiagnosticsInterface_Destroy")
{
    DiagnosticsInterfaceTestHelper helper;

    SECTION("NULL componentContext logs error and returns")
    {
        DiagnosticsInterface_Destroy(nullptr);
        // Should not crash, should log error
        CHECK(g_mock_uninit_call_count == 0);
    }

    SECTION("Valid context calls UnInit and frees memory")
    {
        g_mock_initFromFile_return = true;
        void* context = nullptr;
        DiagnosticsInterface_Create(&context, 0, nullptr);
        REQUIRE(context != nullptr);

        DiagnosticsInterface_Destroy(&context);
        CHECK(context == nullptr);
        CHECK(g_mock_uninit_call_count == 1);
    }
}

// ============================================================================
// DiagnosticsInterface_ReportStateAndResultAsync tests
// ============================================================================
TEST_CASE("DiagnosticsInterface_ReportStateAndResultAsync")
{
    DiagnosticsInterfaceTestHelper helper;

    SECTION("Successful report sends PnP message")
    {
        STRING_HANDLE mockJson = STRING_construct("{\"reported\": true}");
        g_mock_pnp_reported_return = mockJson;
        g_mock_d2c_send_return = true;

        DiagnosticsInterface_ReportStateAndResultAsync(Diagnostics_Result_Success, "op-123");

        CHECK(g_mock_pnp_reported_call_count == 1);
        CHECK(g_mock_d2c_send_call_count == 1);

        // The completed callback should have been captured; invoke it for coverage
        if (g_mock_d2c_last_completed_callback != nullptr)
        {
            g_mock_d2c_last_completed_callback(nullptr, ADUC_D2C_Message_Status_Success);
        }
    }

    SECTION("PnP_CreateReportedProperty failure doesn't send D2C")
    {
        g_mock_pnp_reported_return = nullptr;

        DiagnosticsInterface_ReportStateAndResultAsync(Diagnostics_Result_Failure, "op-fail");

        CHECK(g_mock_pnp_reported_call_count == 1);
        CHECK(g_mock_d2c_send_call_count == 0);
    }

    SECTION("D2C send failure logs error")
    {
        STRING_HANDLE mockJson = STRING_construct("{\"reported\": true}");
        g_mock_pnp_reported_return = mockJson;
        g_mock_d2c_send_return = false;

        DiagnosticsInterface_ReportStateAndResultAsync(Diagnostics_Result_UploadFailed, "op-d2c-fail");

        CHECK(g_mock_pnp_reported_call_count == 1);
        CHECK(g_mock_d2c_send_call_count == 1);
    }

    SECTION("Reports various result types")
    {
        STRING_HANDLE mockJson = STRING_construct("{\"reported\": true}");
        g_mock_pnp_reported_return = mockJson;
        g_mock_d2c_send_return = true;

        DiagnosticsInterface_ReportStateAndResultAsync(Diagnostics_Result_NoLogsFound, "op-no-logs");
        CHECK(g_mock_pnp_reported_call_count == 1);
    }
}

// ============================================================================
// DiagnosticsOrchestratorUpdateCallback tests
// ============================================================================
TEST_CASE("DiagnosticsOrchestratorUpdateCallback")
{
    DiagnosticsInterfaceTestHelper helper;

    SECTION("Valid property value triggers async discover and sends ACK")
    {
        // Set up a fake client handle for ACK sending
        int fakeHandle = 1;
        g_iotHubClientHandleForDiagnosticsComponent = &fakeHandle;

        STRING_HANDLE mockAckJson = STRING_construct("{\"ack\": true}");
        g_mock_pnp_reported_status_return = mockAckJson;
        g_mock_d2c_send_return = true;

        JSON_Value* testValue = json_parse_string("{\"operationId\": \"op1\", \"storageSasUrl\": \"https://sas.url\"}");
        REQUIRE(testValue != nullptr);

        DiagnosticsOrchestratorUpdateCallback(&fakeHandle, testValue, 1, nullptr);

        CHECK(g_mock_async_discover_call_count == 1);
        CHECK(g_mock_pnp_reported_status_call_count == 1);
        CHECK(g_mock_d2c_send_call_count == 1);

        json_value_free(testValue);
    }

    SECTION("NULL client handle causes ACK to fail gracefully")
    {
        g_iotHubClientHandleForDiagnosticsComponent = nullptr;

        JSON_Value* testValue = json_parse_string("{\"test\": true}");
        REQUIRE(testValue != nullptr);

        DiagnosticsOrchestratorUpdateCallback(nullptr, testValue, 1, nullptr);

        CHECK(g_mock_async_discover_call_count == 1);
        // ACK will fail because g_iotHubClientHandleForDiagnosticsComponent is NULL
        // SendPnPMessageToIotHubWithStatus checks for this

        json_value_free(testValue);
    }

    SECTION("PnP_CreateReportedPropertyWithStatus failure handled")
    {
        int fakeHandle = 1;
        g_iotHubClientHandleForDiagnosticsComponent = &fakeHandle;
        g_mock_pnp_reported_status_return = nullptr;

        JSON_Value* testValue = json_parse_string("{\"operationId\": \"op2\"}");
        REQUIRE(testValue != nullptr);

        DiagnosticsOrchestratorUpdateCallback(&fakeHandle, testValue, 2, nullptr);

        CHECK(g_mock_async_discover_call_count == 1);
        CHECK(g_mock_pnp_reported_status_call_count == 1);
        CHECK(g_mock_d2c_send_call_count == 0); // Should not attempt send if PnP create failed

        json_value_free(testValue);
    }

    SECTION("D2C send failure in ACK handled gracefully")
    {
        int fakeHandle = 1;
        g_iotHubClientHandleForDiagnosticsComponent = &fakeHandle;

        STRING_HANDLE mockAckJson = STRING_construct("{\"ack\": true}");
        g_mock_pnp_reported_status_return = mockAckJson;
        g_mock_d2c_send_return = false;

        JSON_Value* testValue = json_parse_string("{\"test\": \"val\"}");
        REQUIRE(testValue != nullptr);

        DiagnosticsOrchestratorUpdateCallback(&fakeHandle, testValue, 3, nullptr);

        CHECK(g_mock_async_discover_call_count == 1);
        CHECK(g_mock_d2c_send_call_count == 1);

        json_value_free(testValue);
    }
}

// ============================================================================
// DiagnosticsInterface_PropertyUpdateCallback tests
// ============================================================================
TEST_CASE("DiagnosticsInterface_PropertyUpdateCallback")
{
    DiagnosticsInterfaceTestHelper helper;

    SECTION("Matching property name triggers orchestrator callback")
    {
        int fakeHandle = 1;
        g_iotHubClientHandleForDiagnosticsComponent = &fakeHandle;

        STRING_HANDLE mockAckJson = STRING_construct("{\"ack\": true}");
        g_mock_pnp_reported_status_return = mockAckJson;
        g_mock_d2c_send_return = true;

        JSON_Value* testValue = json_parse_string("{\"operationId\": \"op-prop\"}");
        REQUIRE(testValue != nullptr);

        DiagnosticsInterface_PropertyUpdateCallback(&fakeHandle, "service", testValue, 1, nullptr, nullptr);

        CHECK(g_mock_async_discover_call_count == 1);

        json_value_free(testValue);
    }

    SECTION("Non-matching property name is ignored")
    {
        JSON_Value* testValue = json_parse_string("{\"test\": true}");
        REQUIRE(testValue != nullptr);

        DiagnosticsInterface_PropertyUpdateCallback(nullptr, "unknown_property", testValue, 1, nullptr, nullptr);

        CHECK(g_mock_async_discover_call_count == 0);
        CHECK(g_mock_pnp_reported_status_call_count == 0);

        json_value_free(testValue);
    }

    SECTION("Empty property name is ignored")
    {
        JSON_Value* testValue = json_parse_string("{\"test\": true}");
        REQUIRE(testValue != nullptr);

        DiagnosticsInterface_PropertyUpdateCallback(nullptr, "", testValue, 1, nullptr, nullptr);

        CHECK(g_mock_async_discover_call_count == 0);

        json_value_free(testValue);
    }
}

#else // _MSC_VER
// On MSVC, --wrap linker mocking is not available. Provide placeholder tests.
TEST_CASE("DiagnosticsInterface_Create")
{
    SUCCEED("Test skipped: --wrap linker mocking not supported on MSVC");
}
TEST_CASE("DiagnosticsInterface_Destroy")
{
    SUCCEED("Test skipped: --wrap linker mocking not supported on MSVC");
}
TEST_CASE("DiagnosticsInterface_ReportStateAndResultAsync")
{
    SUCCEED("Test skipped: --wrap linker mocking not supported on MSVC");
}
TEST_CASE("DiagnosticsOrchestratorUpdateCallback")
{
    SUCCEED("Test skipped: --wrap linker mocking not supported on MSVC");
}
TEST_CASE("DiagnosticsInterface_PropertyUpdateCallback")
{
    SUCCEED("Test skipped: --wrap linker mocking not supported on MSVC");
}
#endif // !defined(_MSC_VER)
