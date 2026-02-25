/**
 * @file client_handle_helper_ut.cpp
 * @brief Unit Tests for client_handle_helper library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>
#include <cstring>
#include <string>

#include "aduc/client_handle_helper.h"
#include "aduc/adu_types.h"

using Catch::Matchers::Equals;

//
// Unit Tests for ADUC_ConnType enum
//

TEST_CASE("ADUC_ConnType enum values")
{
    SECTION("Enum values are correctly defined")
    {
        CHECK(ADUC_ConnType_NotSet == 0);
        CHECK(ADUC_ConnType_Device == 1);
        CHECK(ADUC_ConnType_Module == 2);
    }

    SECTION("Device and Module are distinct")
    {
        CHECK(ADUC_ConnType_Device != ADUC_ConnType_Module);
        CHECK(ADUC_ConnType_Device != ADUC_ConnType_NotSet);
        CHECK(ADUC_ConnType_Module != ADUC_ConnType_NotSet);
    }
}

//
// ADUC_ClientHandle type tests
//

TEST_CASE("ADUC_ClientHandle type behavior")
{
    SECTION("NULL handle is valid")
    {
        ADUC_ClientHandle handle = nullptr;
        CHECK(handle == nullptr);
    }

    SECTION("Handle can be assigned and compared")
    {
        ADUC_ClientHandle handle1 = nullptr;
        ADUC_ClientHandle handle2 = nullptr;

        CHECK(handle1 == handle2);
    }
}

//
// Unit Tests for ClientHandle_DoWork with NULL
//

TEST_CASE("ClientHandle_DoWork parameter validation")
{
    SECTION("Does not crash when handle is NULL")
    {
        // Should silently return without crashing
        ClientHandle_DoWork(nullptr);
        CHECK(true); // If we get here, no crash occurred
    }
}

//
// Unit Tests for ClientHandle_CreateFromConnectionString parameter validation
//

TEST_CASE("ClientHandle_CreateFromConnectionString parameter validation")
{
    SECTION("Returns false when iotHubClientHandle is NULL")
    {
        const char* connectionString = "HostName=test.azure.com;DeviceId=device1;SharedAccessKey=key";

        bool result = ClientHandle_CreateFromConnectionString(
            nullptr, // NULL output handle
            ADUC_ConnType_Device,
            connectionString,
            nullptr // protocol
        );

        CHECK(result == false);
    }

    SECTION("Returns false when connectionString is NULL")
    {
        ADUC_ClientHandle handle = nullptr;

        bool result = ClientHandle_CreateFromConnectionString(
            &handle,
            ADUC_ConnType_Device,
            nullptr, // NULL connection string
            nullptr
        );

        CHECK(result == false);
        CHECK(handle == nullptr);
    }

    SECTION("Returns false when both parameters are NULL")
    {
        bool result = ClientHandle_CreateFromConnectionString(
            nullptr,
            ADUC_ConnType_Device,
            nullptr,
            nullptr
        );

        CHECK(result == false);
    }
}
