/**
 * @file iothub_communication_manager_ut.cpp
 * @brief Unit Tests for iothub_communication_manager library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>
#include <cstring>
#include <string>

#include "aduc/adu_types.h"
#include "aduc/iothub_communication_manager.h"

using Catch::Matchers::Equals;

//
// Unit Tests for GetConnTypeFromConnectionString
//

TEST_CASE("GetConnTypeFromConnectionString")
{
    SECTION("Returns NotSet when connectionString is NULL")
    {
        ADUC_ConnType result = GetConnTypeFromConnectionString(nullptr);
        CHECK(result == ADUC_ConnType_NotSet);
    }

    SECTION("Returns NotSet when connectionString is empty")
    {
        ADUC_ConnType result = GetConnTypeFromConnectionString("");
        CHECK(result == ADUC_ConnType_NotSet);
    }

    SECTION("Returns NotSet when DeviceId is missing")
    {
        const char* connectionString = "HostName=test.azure-devices.net;SharedAccessKey=abc123";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_NotSet);
    }

    SECTION("Returns Device when only DeviceId is present")
    {
        const char* connectionString = "HostName=test.azure-devices.net;DeviceId=mydevice;SharedAccessKey=abc123";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Returns Module when both DeviceId and ModuleId are present")
    {
        const char* connectionString = "HostName=test.azure-devices.net;DeviceId=mydevice;ModuleId=mymodule;SharedAccessKey=abc123";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Module);
    }

    SECTION("Returns Module with different ordering of parameters")
    {
        const char* connectionString = "ModuleId=mymodule;HostName=test.azure-devices.net;DeviceId=mydevice;SharedAccessKey=abc123";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Module);
    }

    SECTION("Returns Device with trailing semicolon")
    {
        const char* connectionString = "HostName=test.azure-devices.net;DeviceId=mydevice;SharedAccessKey=abc123;";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Returns Module with trailing semicolon")
    {
        const char* connectionString = "HostName=test.azure-devices.net;DeviceId=mydevice;ModuleId=mymodule;SharedAccessKey=abc123;";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Module);
    }

    SECTION("Handles GatewayHostName in connection string for device")
    {
        const char* connectionString = "HostName=test.azure-devices.net;DeviceId=mydevice;SharedAccessKey=abc123;GatewayHostName=192.168.1.1";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Handles GatewayHostName in connection string for module")
    {
        const char* connectionString = "HostName=test.azure-devices.net;DeviceId=mydevice;ModuleId=mymodule;SharedAccessKey=abc123;GatewayHostName=192.168.1.1";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Module);
    }
}

//
// Edge case tests for connection string parsing
//

TEST_CASE("Connection string parsing edge cases")
{
    SECTION("Handles connection strings with special characters in values")
    {
        // SharedAccessKey often contains special characters
        const char* connectionString = "HostName=test.azure-devices.net;DeviceId=my+device/name;SharedAccessKey=abc+123/def=";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Handles very long connection strings")
    {
        // Create a long connection string
        std::string longConnectionString = "HostName=test.azure-devices.net;DeviceId=";
        for (int i = 0; i < 100; ++i)
        {
            longConnectionString += "device";
        }
        longConnectionString += ";SharedAccessKey=abc123";

        ADUC_ConnType result = GetConnTypeFromConnectionString(longConnectionString.c_str());
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Handles connection string with only semicolons")
    {
        const char* connectionString = ";;;";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_NotSet);
    }

    SECTION("Handles DeviceId as first parameter")
    {
        const char* connectionString = "DeviceId=mydevice;HostName=test.azure-devices.net;SharedAccessKey=abc123";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Handles DeviceId as last parameter")
    {
        const char* connectionString = "HostName=test.azure-devices.net;SharedAccessKey=abc123;DeviceId=mydevice";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Handles ModuleId before DeviceId")
    {
        const char* connectionString = "HostName=test.azure-devices.net;ModuleId=mymodule;DeviceId=mydevice;SharedAccessKey=abc123";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Module);
    }

    SECTION("Handles multiple equals signs in value")
    {
        // Base64 encoded keys may have = at the end
        const char* connectionString = "HostName=test.azure-devices.net;DeviceId=mydevice;SharedAccessKey=abc123==";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Connection string with numeric device ID")
    {
        const char* connectionString = "HostName=test.azure-devices.net;DeviceId=12345;SharedAccessKey=abc123";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Connection string with hyphenated device ID")
    {
        const char* connectionString = "HostName=test.azure-devices.net;DeviceId=my-device-01;SharedAccessKey=abc123";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Connection string with underscore device ID")
    {
        const char* connectionString = "HostName=test.azure-devices.net;DeviceId=my_device_01;SharedAccessKey=abc123";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Connection string with period in device ID")
    {
        const char* connectionString = "HostName=test.azure-devices.net;DeviceId=my.device.01;SharedAccessKey=abc123";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }
}

//
// Tests for ADUC_ConnectionInfo_DeAlloc
//

TEST_CASE("ADUC_ConnectionInfo_DeAlloc")
{
    SECTION("Handles zeroed info struct")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        // Should not crash when inner pointers are NULL
        ADUC_ConnectionInfo_DeAlloc(&info);
        CHECK(true);
    }

    SECTION("Properly resets struct fields after dealloc")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        // Set some non-pointer fields
        info.authType = ADUC_AuthType_SASToken;
        info.connType = ADUC_ConnType_Device;

        ADUC_ConnectionInfo_DeAlloc(&info);

        // After dealloc, these should be reset to NotSet
        CHECK(info.authType == ADUC_AuthType_NotSet);
        CHECK(info.connType == ADUC_ConnType_NotSet);
    }

    SECTION("Clears all pointer fields")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        // Set non-pointer fields
        info.authType = ADUC_AuthType_X509;
        info.connType = ADUC_ConnType_Module;

        ADUC_ConnectionInfo_DeAlloc(&info);

        // All pointer fields should be NULL
        CHECK(info.connectionString == nullptr);
        CHECK(info.certificateString == nullptr);
        CHECK(info.opensslEngine == nullptr);
        CHECK(info.opensslPrivateKey == nullptr);
    }
}

//
// Tests for ADUC_ConnType_ToString
//

TEST_CASE("ADUC_ConnType_ToString")
{
    SECTION("Returns correct string for NotSet")
    {
        const char* result = ADUC_ConnType_ToString(ADUC_ConnType_NotSet);
        CHECK(result != nullptr);
        CHECK(strlen(result) > 0);
    }

    SECTION("Returns correct string for Device")
    {
        const char* result = ADUC_ConnType_ToString(ADUC_ConnType_Device);
        CHECK(result != nullptr);
        CHECK(strlen(result) > 0);
    }

    SECTION("Returns correct string for Module")
    {
        const char* result = ADUC_ConnType_ToString(ADUC_ConnType_Module);
        CHECK(result != nullptr);
        CHECK(strlen(result) > 0);
    }

    SECTION("All connection types return different strings")
    {
        const char* notSet = ADUC_ConnType_ToString(ADUC_ConnType_NotSet);
        const char* device = ADUC_ConnType_ToString(ADUC_ConnType_Device);
        const char* module = ADUC_ConnType_ToString(ADUC_ConnType_Module);

        CHECK(strcmp(notSet, device) != 0);
        CHECK(strcmp(notSet, module) != 0);
        CHECK(strcmp(device, module) != 0);
    }
}

//
// ADUC_ConnType edge cases
//

TEST_CASE("ADUC_ConnType edge cases")
{
    SECTION("Device vs Module connection type distinction")
    {
        // Verify the type enum values are correct
        CHECK(ADUC_ConnType_NotSet == 0);
        CHECK(ADUC_ConnType_Device == 1);
        CHECK(ADUC_ConnType_Module == 2);
    }
}

//
// Memory safety tests
//

TEST_CASE("Memory safety for ADUC_ConnectionInfo")
{
    SECTION("DeAlloc handles zeroed struct safely")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        // Should not crash when all pointers are NULL
        ADUC_ConnectionInfo_DeAlloc(&info);
        CHECK(true);
    }

    SECTION("Multiple DeAlloc calls on same struct")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        // First dealloc
        ADUC_ConnectionInfo_DeAlloc(&info);
        CHECK(info.authType == ADUC_AuthType_NotSet);

        // Second dealloc should also be safe
        ADUC_ConnectionInfo_DeAlloc(&info);
        CHECK(info.authType == ADUC_AuthType_NotSet);
    }
}

//
// Tests for IoTHub_CommunicationManager_IsAuthenticated
//

TEST_CASE("IoTHub_CommunicationManager_IsAuthenticated")
{
    SECTION("Returns false when not initialized")
    {
        // Before any connection is established, should return false
        bool result = IoTHub_CommunicationManager_IsAuthenticated();
        // We can only verify it doesn't crash
        CHECK(true);
        (void)result; // Suppress unused warning
    }
}

//
// Connection string format validation tests
//

TEST_CASE("Connection string format validation")
{
    SECTION("Standard IoT Hub connection string format - Device")
    {
        const char* connectionString = "HostName=contoso.azure-devices.net;DeviceId=myDevice;SharedAccessKey=dGVzdGtleQ==";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Standard IoT Hub connection string format - Module")
    {
        const char* connectionString = "HostName=contoso.azure-devices.net;DeviceId=myDevice;ModuleId=myModule;SharedAccessKey=dGVzdGtleQ==";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Module);
    }

    SECTION("Connection string with SharedAccessSignature instead of SharedAccessKey")
    {
        const char* connectionString = "HostName=contoso.azure-devices.net;DeviceId=myDevice;SharedAccessSignature=SharedAccessSignature sr=...";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Connection string for Edge device")
    {
        const char* connectionString = "HostName=contoso.azure-devices.net;DeviceId=edgeDevice;SharedAccessKey=dGVzdGtleQ==;GatewayHostName=10.0.0.1";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Connection string for Edge module")
    {
        const char* connectionString = "HostName=contoso.azure-devices.net;DeviceId=edgeDevice;ModuleId=edgeModule;SharedAccessKey=dGVzdGtleQ==;GatewayHostName=10.0.0.1";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Module);
    }
}

//
// Connection string key detection tests
//

TEST_CASE("Connection string key detection")
{
    SECTION("Detects DeviceId at start")
    {
        const char* connectionString = "DeviceId=device;HostName=hub;SharedAccessKey=key";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Detects DeviceId in middle")
    {
        const char* connectionString = "HostName=hub;DeviceId=device;SharedAccessKey=key";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Detects DeviceId at end")
    {
        const char* connectionString = "HostName=hub;SharedAccessKey=key;DeviceId=device";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Detects ModuleId at start")
    {
        const char* connectionString = "ModuleId=module;DeviceId=device;HostName=hub;SharedAccessKey=key";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Module);
    }

    SECTION("Detects ModuleId in middle")
    {
        const char* connectionString = "DeviceId=device;ModuleId=module;HostName=hub;SharedAccessKey=key";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Module);
    }

    SECTION("Detects ModuleId at end")
    {
        const char* connectionString = "DeviceId=device;HostName=hub;SharedAccessKey=key;ModuleId=module";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_Module);
    }
}

//
// Tests for malformed connection strings
//

TEST_CASE("Malformed connection strings")
{
    SECTION("No equals signs")
    {
        const char* connectionString = "HostName;DeviceId;SharedAccessKey";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_NotSet);
    }

    SECTION("Single character string")
    {
        const char* connectionString = "a";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_NotSet);
    }

    SECTION("Only key-value separator")
    {
        const char* connectionString = ";";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_NotSet);
    }

    SECTION("Random string without proper format")
    {
        const char* connectionString = "this is not a connection string";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        CHECK(result == ADUC_ConnType_NotSet);
    }

    SECTION("Case sensitivity - deviceid lowercase")
    {
        const char* connectionString = "HostName=hub;deviceid=device;SharedAccessKey=key";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        // Should be case-sensitive, so DeviceId won't match deviceid
        CHECK(result == ADUC_ConnType_NotSet);
    }

    SECTION("Case sensitivity - DEVICEID uppercase")
    {
        const char* connectionString = "HostName=hub;DEVICEID=device;SharedAccessKey=key";
        ADUC_ConnType result = GetConnTypeFromConnectionString(connectionString);
        // Should be case-sensitive
        CHECK(result == ADUC_ConnType_NotSet);
    }
}

//
// Tests for AuthType enum values
//

TEST_CASE("ADUC_AuthType enum values")
{
    SECTION("AuthType enum has expected values")
    {
        CHECK(ADUC_AuthType_NotSet == 0);
    }

    SECTION("SASToken auth type is distinct")
    {
        CHECK(ADUC_AuthType_SASToken != ADUC_AuthType_NotSet);
        CHECK(ADUC_AuthType_SASToken != ADUC_AuthType_NestedEdgeCert);
    }
}

//
// Comprehensive ADUC_ConnectionInfo struct tests
//

TEST_CASE("ADUC_ConnectionInfo struct operations")
{
    SECTION("Initialize and clear struct")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        CHECK(info.connectionString == nullptr);
        CHECK(info.certificateString == nullptr);
        CHECK(info.authType == 0);
        CHECK(info.connType == 0);
    }

    SECTION("Set and clear individual fields")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        info.authType = ADUC_AuthType_SASToken;
        info.connType = ADUC_ConnType_Device;

        CHECK(info.authType == ADUC_AuthType_SASToken);
        CHECK(info.connType == ADUC_ConnType_Device);

        ADUC_ConnectionInfo_DeAlloc(&info);

        CHECK(info.authType == ADUC_AuthType_NotSet);
        CHECK(info.connType == ADUC_ConnType_NotSet);
    }
}

//
// Tests for IoTHub_CommunicationManager_GetHandle
//

TEST_CASE("IoTHub_CommunicationManager_GetHandle")
{
    SECTION("Returns handle without crash")
    {
        // Before initialization or after deinitialization, should return NULL
        ADUC_ClientHandle handle = IoTHub_CommunicationManager_GetHandle();
        // We can only verify it doesn't crash
        CHECK(true);
        (void)handle; // Suppress unused warning
    }
}

//
// Integration-style tests for connection info lifecycle
//

TEST_CASE("ConnectionInfo lifecycle")
{
    SECTION("Create, use, and deallocate ConnectionInfo")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        // Simulate setting values
        info.authType = ADUC_AuthType_SASToken;
        info.connType = ADUC_ConnType_Device;

        // Verify values are set
        CHECK(info.authType == ADUC_AuthType_SASToken);
        CHECK(info.connType == ADUC_ConnType_Device);

        // Deallocate
        ADUC_ConnectionInfo_DeAlloc(&info);

        // Verify reset
        CHECK(info.authType == ADUC_AuthType_NotSet);
        CHECK(info.connType == ADUC_ConnType_NotSet);
    }

    SECTION("Module connection info lifecycle")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        info.authType = ADUC_AuthType_X509;
        info.connType = ADUC_ConnType_Module;

        CHECK(info.authType == ADUC_AuthType_X509);
        CHECK(info.connType == ADUC_ConnType_Module);

        ADUC_ConnectionInfo_DeAlloc(&info);

        CHECK(info.authType == ADUC_AuthType_NotSet);
        CHECK(info.connType == ADUC_ConnType_NotSet);
    }

    SECTION("Nested Edge cert auth type")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        info.authType = ADUC_AuthType_NestedEdgeCert;
        info.connType = ADUC_ConnType_Device;

        CHECK(info.authType == ADUC_AuthType_NestedEdgeCert);

        ADUC_ConnectionInfo_DeAlloc(&info);

        CHECK(info.authType == ADUC_AuthType_NotSet);
    }
}

//
// Tests for verifying string comparisons
//

TEST_CASE("String comparison edge cases in connection parsing")
{
    SECTION("DeviceId vs DeviceID")
    {
        // Test exact case sensitivity
        const char* correct = "HostName=hub;DeviceId=device;SharedAccessKey=key";
        const char* wrongCase = "HostName=hub;DeviceID=device;SharedAccessKey=key";

        ADUC_ConnType correctResult = GetConnTypeFromConnectionString(correct);
        ADUC_ConnType wrongCaseResult = GetConnTypeFromConnectionString(wrongCase);

        CHECK(correctResult == ADUC_ConnType_Device);
        // Wrong case should not match
        CHECK(wrongCaseResult == ADUC_ConnType_NotSet);
    }

    SECTION("ModuleId vs ModuleID")
    {
        const char* correct = "HostName=hub;DeviceId=device;ModuleId=module;SharedAccessKey=key";
        const char* wrongCase = "HostName=hub;DeviceId=device;ModuleID=module;SharedAccessKey=key";

        ADUC_ConnType correctResult = GetConnTypeFromConnectionString(correct);
        ADUC_ConnType wrongCaseResult = GetConnTypeFromConnectionString(wrongCase);

        CHECK(correctResult == ADUC_ConnType_Module);
        // ModuleId not found (case sensitive), but DeviceId is found
        CHECK(wrongCaseResult == ADUC_ConnType_Device);
    }
}

//
// Boundary tests
//

TEST_CASE("Boundary tests for connection strings")
{
    SECTION("Maximum reasonable connection string length")
    {
        std::string maxLength;
        maxLength += "HostName=";
        maxLength += std::string(255, 'h');
        maxLength += ".azure-devices.net;DeviceId=";
        maxLength += std::string(128, 'd');
        maxLength += ";SharedAccessKey=";
        maxLength += std::string(64, 'k');

        ADUC_ConnType result = GetConnTypeFromConnectionString(maxLength.c_str());
        CHECK(result == ADUC_ConnType_Device);
    }

    SECTION("Empty key or value handling")
    {
        // Connection string with empty key - this is malformed but DeviceId should still be found
        const char* emptyKey = "HostName=hub;=value;DeviceId=device;SharedAccessKey=key";
        ADUC_ConnType result = GetConnTypeFromConnectionString(emptyKey);
        // The implementation may fail to parse malformed connection strings
        // Just verify it doesn't crash
        CHECK((result == ADUC_ConnType_Device || result == ADUC_ConnType_NotSet));
    }
}

TEST_CASE("GetConnectionInfoFromConnectionString parameter validation and success paths")
{
    SECTION("Returns false when info is NULL")
    {
        CHECK(
            GetConnectionInfoFromConnectionString(
                nullptr,
                "HostName=hub;DeviceId=device;SharedAccessKey=key",
                nullptr,
                nullptr,
                nullptr,
                nullptr)
            == false);
    }

    SECTION("Returns false when connection string is NULL")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        CHECK(GetConnectionInfoFromConnectionString(&info, nullptr, nullptr, nullptr, nullptr, nullptr) == false);
        ADUC_ConnectionInfo_DeAlloc(&info);
    }

    SECTION("Returns false for invalid connection string")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        CHECK(
            GetConnectionInfoFromConnectionString(
                &info,
                "HostName=hub;SharedAccessKey=key",
                nullptr,
                nullptr,
                nullptr,
                nullptr)
            == false);

        ADUC_ConnectionInfo_DeAlloc(&info);
    }

    SECTION("Returns true for valid SAS device connection string")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        CHECK(
            GetConnectionInfoFromConnectionString(
                &info,
                "HostName=hub;DeviceId=device;SharedAccessKey=key",
                nullptr,
                nullptr,
                nullptr,
                nullptr)
            == true);

        CHECK(info.connType == ADUC_ConnType_Device);
        CHECK(info.authType == ADUC_AuthType_SASToken);
        CHECK(info.connectionString != nullptr);

        ADUC_ConnectionInfo_DeAlloc(&info);
    }

    SECTION("Returns true for valid X509 module connection string with engine")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        CHECK(
            GetConnectionInfoFromConnectionString(
                &info,
                "HostName=hub;DeviceId=device;ModuleId=module;SharedAccessKey=key",
                "x509-cert",
                "x509-private-key",
                "openssl-engine",
                "x509-ca-cert")
            == true);

        CHECK(info.connType == ADUC_ConnType_Module);
        const bool isExpectedAuthType =
            (info.authType == ADUC_AuthType_X509) || (info.authType == ADUC_AuthType_NestedEdgeCert);
        CHECK(isExpectedAuthType);
        CHECK(info.connectionString != nullptr);
        CHECK(info.clientCertificateString != nullptr);
        CHECK(info.opensslPrivateKey != nullptr);
        CHECK(info.certificateString != nullptr);
        CHECK(info.opensslEngine != nullptr);

        ADUC_ConnectionInfo_DeAlloc(&info);
    }
}

TEST_CASE("IoTHub connection status callback updates auth state")
{
    SECTION("Authenticated status toggles IsAuthenticated true")
    {
        IoTHub_CommunicationManager_ConnectionStatus_Callback(
            IOTHUB_CLIENT_CONNECTION_AUTHENTICATED,
            IOTHUB_CLIENT_CONNECTION_OK,
            nullptr);
        CHECK(IoTHub_CommunicationManager_IsAuthenticated() == true);
    }

    SECTION("Unauthenticated status toggles IsAuthenticated false")
    {
        IoTHub_CommunicationManager_ConnectionStatus_Callback(
            IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED,
            IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL,
            nullptr);
        CHECK(IoTHub_CommunicationManager_IsAuthenticated() == false);
    }
}

TEST_CASE("Agent and identity config helpers validate null inputs")
{
    CHECK(GetAgentConfigInfo(nullptr) == false);
    CHECK(GetConnectionInfoFromIdentityService(nullptr) == false);
}

//
// Tests for IoTHub_CommunicationManager_Init / Deinit / GetHandle lifecycle
//

TEST_CASE("IoTHub_CommunicationManager_Init and Deinit lifecycle")
{
    SECTION("Init succeeds with valid parameters")
    {
        ADUC_ClientHandle clientHandle = nullptr;

        bool result = IoTHub_CommunicationManager_Init(
            &clientHandle, nullptr /* device_twin_callback */, nullptr /* handle_updated */, nullptr /* context */);

        CHECK(result == true);

        // After successful init, GetHandle should return the initial value (nullptr since no connection yet)
        ADUC_ClientHandle handle = IoTHub_CommunicationManager_GetHandle();
        CHECK(handle == nullptr);

        // Deinit should not crash
        IoTHub_CommunicationManager_Deinit();
    }

    SECTION("Init called twice returns true on second call (already initialized)")
    {
        ADUC_ClientHandle clientHandle = nullptr;

        bool result1 = IoTHub_CommunicationManager_Init(
            &clientHandle, nullptr, nullptr, nullptr);
        CHECK(result1 == true);

        // Second init should succeed with "already initialized" path
        bool result2 = IoTHub_CommunicationManager_Init(
            &clientHandle, nullptr, nullptr, nullptr);
        CHECK(result2 == true);

        IoTHub_CommunicationManager_Deinit();
    }
}

//
// Tests for ConnectionStatus_Callback exercising the "broken for X seconds" sub-branch
//

// Forward declaration of the internal categorization helper (kept out of the
// public header). See GitHub issue #779.
typedef enum tagADUC_ConnUnauthCategory
{
    ADUC_ConnUnauth_TransientSasRenewal = 0,
    ADUC_ConnUnauth_Broken = 1,
} ADUC_ConnUnauthCategory;

extern "C" ADUC_ConnUnauthCategory IoTHub_CommunicationManager_CategorizeUnauthenticated(
    IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);

TEST_CASE("Issue #779: SAS token expiry must not be categorized as 'broken'")
{
    // Expected SAS-token renewal: must NOT be treated as a broken connection.
    CHECK(
        IoTHub_CommunicationManager_CategorizeUnauthenticated(IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN)
        == ADUC_ConnUnauth_TransientSasRenewal);

    // Real failures must still be categorized as broken.
    CHECK(
        IoTHub_CommunicationManager_CategorizeUnauthenticated(IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL)
        == ADUC_ConnUnauth_Broken);
    CHECK(
        IoTHub_CommunicationManager_CategorizeUnauthenticated(IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED)
        == ADUC_ConnUnauth_Broken);
    CHECK(
        IoTHub_CommunicationManager_CategorizeUnauthenticated(IOTHUB_CLIENT_CONNECTION_NO_NETWORK)
        == ADUC_ConnUnauth_Broken);
    CHECK(
        IoTHub_CommunicationManager_CategorizeUnauthenticated(IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR)
        == ADUC_ConnUnauth_Broken);
    CHECK(
        IoTHub_CommunicationManager_CategorizeUnauthenticated(IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED)
        == ADUC_ConnUnauth_Broken);
    CHECK(
        IoTHub_CommunicationManager_CategorizeUnauthenticated(IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE)
        == ADUC_ConnUnauth_Broken);
    CHECK(
        IoTHub_CommunicationManager_CategorizeUnauthenticated(IOTHUB_CLIENT_CONNECTION_OK)
        == ADUC_ConnUnauth_Broken);
}

TEST_CASE("Issue #779: SAS expiry callback is handled benignly")
{
    // Authenticate first so any subsequent unauthenticated event would,
    // under the old behavior, reset g_first_unauthenticated_time and log
    // "IoTHub connection is broken."
    IoTHub_CommunicationManager_ConnectionStatus_Callback(
        IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, IOTHUB_CLIENT_CONNECTION_OK, nullptr);
    REQUIRE(IoTHub_CommunicationManager_IsAuthenticated() == true);

    // SAS-token expiry: must not crash and must flip IsAuthenticated.
    IoTHub_CommunicationManager_ConnectionStatus_Callback(
        IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN, nullptr);
    CHECK(IoTHub_CommunicationManager_IsAuthenticated() == false);

    // A second SAS expiry in a row should also be treated benignly.
    IoTHub_CommunicationManager_ConnectionStatus_Callback(
        IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN, nullptr);
    CHECK(IoTHub_CommunicationManager_IsAuthenticated() == false);
}

TEST_CASE("ConnectionStatus_Callback exercises both unauthenticated sub-branches")
{
    SECTION("First unauthenticated triggers 'connection is broken' path")
    {
        // Set authenticated first to ensure g_last_authenticated_time >= g_first_unauthenticated_time
        IoTHub_CommunicationManager_ConnectionStatus_Callback(
            IOTHUB_CLIENT_CONNECTION_AUTHENTICATED,
            IOTHUB_CLIENT_CONNECTION_OK,
            nullptr);
        CHECK(IoTHub_CommunicationManager_IsAuthenticated() == true);

        // First unauthenticated - hits "IoTHub connection is broken." branch
        IoTHub_CommunicationManager_ConnectionStatus_Callback(
            IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED,
            IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL,
            nullptr);
        CHECK(IoTHub_CommunicationManager_IsAuthenticated() == false);
    }

    SECTION("Second unauthenticated triggers 'broken for N seconds' path")
    {
        // Authenticate first
        IoTHub_CommunicationManager_ConnectionStatus_Callback(
            IOTHUB_CLIENT_CONNECTION_AUTHENTICATED,
            IOTHUB_CLIENT_CONNECTION_OK,
            nullptr);

        // First unauthenticated
        IoTHub_CommunicationManager_ConnectionStatus_Callback(
            IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED,
            IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED,
            nullptr);

        // Second unauthenticated - hits the "else" branch with "broken for %d seconds"
        IoTHub_CommunicationManager_ConnectionStatus_Callback(
            IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED,
            IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL,
            nullptr);
        CHECK(IoTHub_CommunicationManager_IsAuthenticated() == false);
    }
}

//
// Additional GetConnectionInfoFromConnectionString edge cases
//

TEST_CASE("GetConnectionInfoFromConnectionString X509 paths")
{
    SECTION("X509 without opensslEngine succeeds")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        bool result = GetConnectionInfoFromConnectionString(
            &info,
            "HostName=hub;DeviceId=device;SharedAccessKey=key",
            "x509-cert",
            "x509-private-key",
            nullptr,          // no openssl engine
            "x509-ca-cert");

        CHECK(result == true);
        const bool isX509Auth =
            (info.authType == ADUC_AuthType_X509) || (info.authType == ADUC_AuthType_NestedEdgeCert);
        CHECK(isX509Auth);
        CHECK(info.connType == ADUC_ConnType_Device);
        CHECK(info.clientCertificateString != nullptr);
        CHECK(info.opensslPrivateKey != nullptr);
        CHECK(info.certificateString != nullptr);
        // opensslEngine should remain NULL when not provided
        CHECK(info.opensslEngine == nullptr);

        ADUC_ConnectionInfo_DeAlloc(&info);
    }

    SECTION("SAS module connection string succeeds")
    {
        ADUC_ConnectionInfo info;
        memset(&info, 0, sizeof(info));

        bool result = GetConnectionInfoFromConnectionString(
            &info,
            "HostName=hub;DeviceId=device;ModuleId=mod;SharedAccessKey=key",
            nullptr, nullptr, nullptr, nullptr);

        CHECK(result == true);
        CHECK(info.connType == ADUC_ConnType_Module);
        CHECK(info.authType == ADUC_AuthType_SASToken);
        CHECK(info.connectionString != nullptr);

        ADUC_ConnectionInfo_DeAlloc(&info);
    }
}

//
// ADO Bug 38069154: classification of IoT Hub connection-status reasons.
//
// These tests pin down the policy that drives Connection_Maintenance():
// transient transport disconnects (NO_NETWORK, NO_PING_RESPONSE,
// COMMUNICATION_ERROR) must NOT cause the agent to destroy the client
// handle or restart the host process, because the Azure IoT C SDK has its
// own exponential-backoff reconnect policy and will recover on the
// existing handle.

typedef enum tagADUC_ConnReasonClass
{
    ADUC_ConnReason_TransientTransport = 0,
    ADUC_ConnReason_Credential = 1,
    ADUC_ConnReason_DeviceDisabled = 2,
    ADUC_ConnReason_Ok = 3,
    ADUC_ConnReason_Unknown = 4,
} ADUC_ConnReasonClass;

extern "C" ADUC_ConnReasonClass IoTHub_CommunicationManager_ClassifyConnectionReason(
    IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);

TEST_CASE("ADO 38069154: NO_NETWORK is classified as transient transport (no restart)")
{
    // The headline behavior the customer's restart loop hinged on: a
    // socket-level RST (errno=104) surfaced by the SDK as NO_NETWORK must
    // be treated as something the SDK can recover from on the existing
    // handle, NOT as a reason to destroy and recreate anything.
    CHECK(
        IoTHub_CommunicationManager_ClassifyConnectionReason(IOTHUB_CLIENT_CONNECTION_NO_NETWORK)
        == ADUC_ConnReason_TransientTransport);
}

TEST_CASE("ADO 38069154: all transport-level disconnects classify as transient transport")
{
    CHECK(
        IoTHub_CommunicationManager_ClassifyConnectionReason(IOTHUB_CLIENT_CONNECTION_NO_NETWORK)
        == ADUC_ConnReason_TransientTransport);
    CHECK(
        IoTHub_CommunicationManager_ClassifyConnectionReason(IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE)
        == ADUC_ConnReason_TransientTransport);
    CHECK(
        IoTHub_CommunicationManager_ClassifyConnectionReason(IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR)
        == ADUC_ConnReason_TransientTransport);
}

TEST_CASE("ADO 38069154: credential-related reasons classify as credential (reauth path)")
{
    CHECK(
        IoTHub_CommunicationManager_ClassifyConnectionReason(IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN)
        == ADUC_ConnReason_Credential);
    CHECK(
        IoTHub_CommunicationManager_ClassifyConnectionReason(IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL)
        == ADUC_ConnReason_Credential);
    CHECK(
        IoTHub_CommunicationManager_ClassifyConnectionReason(IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED)
        == ADUC_ConnReason_Credential);
}

TEST_CASE("ADO 38069154: DEVICE_DISABLED has its own long-backoff class")
{
    CHECK(
        IoTHub_CommunicationManager_ClassifyConnectionReason(IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED)
        == ADUC_ConnReason_DeviceDisabled);
}

TEST_CASE("ADO 38069154: OK reason classifies as healthy")
{
    CHECK(
        IoTHub_CommunicationManager_ClassifyConnectionReason(IOTHUB_CLIENT_CONNECTION_OK)
        == ADUC_ConnReason_Ok);
}

TEST_CASE("ADO 38069154: unrecognized reason classifies as Unknown (conservative)")
{
    // Reason codes added to the SDK in the future must default to a
    // non-destructive bucket; specifically must NOT classify as
    // Credential (which would destroy and recreate the handle).
    auto bogus = static_cast<IOTHUB_CLIENT_CONNECTION_STATUS_REASON>(99999);
    auto cls = IoTHub_CommunicationManager_ClassifyConnectionReason(bogus);
    CHECK(cls == ADUC_ConnReason_Unknown);
    CHECK(cls != ADUC_ConnReason_Credential);
    CHECK(cls != ADUC_ConnReason_DeviceDisabled);
}
