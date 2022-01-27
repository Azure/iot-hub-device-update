/**
 * @file connection_string_utils_ut.cpp
 * @brief Unit Tests for connection_string_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch.hpp>

#include "aduc/connection_string_utils.h"
#include <aduc/c_utils.h> // ARRAY_SIZE

TEST_CASE("ConnectionStringUtils_DoesKeyExist")
{
    CHECK_FALSE(ConnectionStringUtils_DoesKeyExist("a=1;c=2", "b"));
    CHECK(ConnectionStringUtils_DoesKeyExist("a=1;c=2", "a"));
}

TEST_CASE("ConnectionStringUtils_GetValue_badoutarg")
{
    CHECK_FALSE(ConnectionStringUtils_GetValue("a=1;c=2", "b", nullptr /*value*/));
}

TEST_CASE("ConnectionStringUtils_GetValue_key_missing")
{
    char someChar = 'c';
    char* value = &someChar;
    CHECK_FALSE(ConnectionStringUtils_GetValue("a=1;c=2", "b", &value));
    CHECK(value == nullptr);
}

TEST_CASE("ConnectionStringUtils_GetValue_key_exists")
{
    char* ptr;
    CHECK(ConnectionStringUtils_GetValue("a=1;c=2", "c", &ptr));
    std::unique_ptr<char, decltype(&free)> managed(ptr, free);
    ptr = nullptr;

    CHECK(managed != nullptr);
    CHECK(std::string("2") == std::string(managed.get()));
}

TEST_CASE("ConnectionStringUtils_IsNestedEdge nullptr connection string returns false")
{
    // Not included in [generators] test because it will throw basic_string::_M_construct null not valid
    CHECK_FALSE(ConnectionStringUtils_IsNestedEdge(nullptr));
}

TEST_CASE("ConnectionStringUtils_IsNestedEdge returns false", "[generators]")
{
    // NOLINTNEXTLINE(google-build-using-namespace)
    auto falseTestCase = GENERATE(
        as<std::string>{},
        // empty case
        "",
        // valid but no nested edge parent gateway
        "HostName=some-iot-device.io;DeviceId=somedeviceid;SharedAccessKey=11111111111111111111111111111111111",
        // GatewayHostName is case-sensitive
        "HostName=some-iot-device.io;DeviceId=somedeviceid;SharedAccessKey=11111111111111111111111111111111111;GatewayHostname=255.255.255.254",
        // edge cases
        "Foo=GatewayHostName=;Bar=42",
        "Foo=GatewayHostName=",
        "Bar=42;Foo=GatewayHostName=",
        "FooGatewayHostName=1.2.3.4");

    INFO(falseTestCase);
    CHECK_FALSE(ConnectionStringUtils_IsNestedEdge(falseTestCase.c_str()));
}

TEST_CASE("ConnectionStringUtils_IsNestedEdge returns true", "[generators]")
{
    // NOLINTNEXTLINE(google-build-using-namespace)
    auto trueTestCase = GENERATE(
        as<std::string>{},
        // typical ordering of pairs
        "HostName=some-iot-device.io;DeviceId=somedeviceid;SharedAccessKey=11111111111111111111111111111111111;GatewayHostName=255.255.255.254",
        // GatewayHostName first
        "GatewayHostName=255.255.255.254;HostName=some-iot-device.io;DeviceId=somedeviceid;SharedAccessKey=11111111111111111111111111111111111",
        // GatewayHostName in the middle
        "HostName=some-iot-device.io;GatewayHostName=255.255.255.254;DeviceId=somedeviceid;SharedAccessKey=11111111111111111111111111111111111");

    INFO(trueTestCase);
    CHECK(ConnectionStringUtils_IsNestedEdge(trueTestCase.c_str()));
}
TEST_CASE("ConnectionStringUtils_GetModuleIdFromConnectionString Success Case")
{
    char* ptr;
    std::string connectionString{
        "HostName=some-iot-device.io;DeviceId=somedeviceid;ModuleId=somemoduleid;SharedAccessKey=asdfasdfasdf;"
    };
    CHECK(ConnectionStringUtils_GetModuleIdFromConnectionString(connectionString.c_str(), &ptr));
    std::unique_ptr<char, decltype(&free)> managed(ptr, free);
    ptr = nullptr;

    CHECK(managed != nullptr);
    CHECK(std::string("somemoduleid") == std::string(managed.get()));
}

TEST_CASE("ConnectionStringUtils_GetModuleIdFromConnectionString Failure Case")
{
    char* ptr;
    std::string connectionString{ "HostName=some-iot-device.io;DeviceId=somedeviceid;SharedAccessKey=asdfasdfasdf;" };
    CHECK_FALSE(ConnectionStringUtils_GetModuleIdFromConnectionString(connectionString.c_str(), &ptr));
    std::unique_ptr<char, decltype(&free)> managed(ptr, free);
    ptr = nullptr;

    CHECK(managed == nullptr);
}

TEST_CASE("ConnectionStringUtils_GetDeviceIdFromConnectionString Success Case")
{
    char* ptr;
    std::string connectionString{
        "HostName=some-iot-device.io;DeviceId=somedeviceid;ModuleId=somemoduleid;SharedAccessKey=asdfasdfasdf;"
    };
    CHECK(ConnectionStringUtils_GetDeviceIdFromConnectionString(connectionString.c_str(), &ptr));
    std::unique_ptr<char, decltype(&free)> managed(ptr, free);
    ptr = nullptr;

    CHECK(managed != nullptr);
    CHECK(std::string("somedeviceid") == std::string(managed.get()));
}

TEST_CASE("ConnectionStringUtils_GetDeviceIdFromConnectionString Failure Case")
{
    char* ptr;
    std::string connectionString{ "HostName=some-iot-device.io;SharedAccessKey=asdfasdfasdf;" };
    CHECK_FALSE(ConnectionStringUtils_GetDeviceIdFromConnectionString(connectionString.c_str(), &ptr));
    std::unique_ptr<char, decltype(&free)> managed(ptr, free);
    ptr = nullptr;

    CHECK(managed == nullptr);
}
