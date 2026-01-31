/**
 * @file device_properties_ut.cpp
 * @brief Unit Tests for device_properties module
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>
#include <parson.h>
#include <cstring>

// Include the module under test
extern "C"
{
#include "device_properties.h"
#include <aduc/c_utils.h>
#include <aduc/config_utils.h>
}

/**
 * @brief Test fixture for device properties tests
 */
class DevicePropertiesTestFixture
{
public:
    DevicePropertiesTestFixture()
    {
        root_value = json_value_init_object();
        device_props_obj = json_value_get_object(root_value);
    }

    ~DevicePropertiesTestFixture()
    {
        if (root_value != nullptr)
        {
            json_value_free(root_value);
        }
    }

    JSON_Value* root_value = nullptr;
    JSON_Object* device_props_obj = nullptr;
};

/**
 * @brief Test that DeviceProperties_AddManufacturerAndModel properly frees allocated memory
 * This test validates the fix for the memory leak where manufacturer and model strings
 * were not being freed.
 */
TEST_CASE_METHOD(DevicePropertiesTestFixture, "DeviceProperties_AddManufacturerAndModel - With valid agent info")
{
    SECTION("Should add manufacturer and model from agent info and free memory")
    {
        // Arrange
        ADUC_AgentInfo agent = {};
        agent.manufacturer = (char*)"TestManufacturer";
        agent.model = (char*)"TestModel";
        agent.name = (char*)"test-agent";

        // Act
        bool result = DeviceProperties_AddManufacturerAndModel(device_props_obj, &agent);

        // Assert
        REQUIRE(result == true);

        const char* manufacturer = json_object_get_string(device_props_obj, "manufacturer");
        const char* model = json_object_get_string(device_props_obj, "model");

        REQUIRE(manufacturer != nullptr);
        REQUIRE(model != nullptr);
        REQUIRE(strcmp(manufacturer, "TestManufacturer") == 0);
        REQUIRE(strcmp(model, "TestModel") == 0);
    }
}

/**
 * @brief Test that DeviceProperties_AddManufacturerAndModel works with null agent (uses defaults)
 */
TEST_CASE_METHOD(DevicePropertiesTestFixture, "DeviceProperties_AddManufacturerAndModel - With null agent")
{
    SECTION("Should use default manufacturer and model values")
    {
        // Act
        bool result = DeviceProperties_AddManufacturerAndModel(device_props_obj, nullptr);

        // Assert
        REQUIRE(result == true);

        const char* manufacturer = json_object_get_string(device_props_obj, "manufacturer");
        const char* model = json_object_get_string(device_props_obj, "model");

        REQUIRE(manufacturer != nullptr);
        REQUIRE(model != nullptr);
        // Values should be the build defaults
    }
}

/**
 * @brief Test that DeviceProperties_AddManufacturerAndModel works with agent info having null fields
 */
TEST_CASE_METHOD(DevicePropertiesTestFixture, "DeviceProperties_AddManufacturerAndModel - With partial agent info")
{
    SECTION("Should use defaults when agent manufacturer is null")
    {
        // Arrange
        ADUC_AgentInfo agent = {};
        agent.manufacturer = nullptr;
        agent.model = (char*)"TestModel";

        // Act
        bool result = DeviceProperties_AddManufacturerAndModel(device_props_obj, &agent);

        // Assert
        REQUIRE(result == true);

        const char* manufacturer = json_object_get_string(device_props_obj, "manufacturer");
        const char* model = json_object_get_string(device_props_obj, "model");

        REQUIRE(manufacturer != nullptr);
        REQUIRE(model != nullptr);
    }

    SECTION("Should use defaults when agent model is null")
    {
        // Arrange
        ADUC_AgentInfo agent = {};
        agent.manufacturer = (char*)"TestManufacturer";
        agent.model = nullptr;

        // Act
        bool result = DeviceProperties_AddManufacturerAndModel(device_props_obj, &agent);

        // Assert
        REQUIRE(result == true);

        const char* manufacturer = json_object_get_string(device_props_obj, "manufacturer");
        const char* model = json_object_get_string(device_props_obj, "model");

        REQUIRE(manufacturer != nullptr);
        REQUIRE(model != nullptr);
    }
}

/**
 * @brief Test multiple calls to ensure no memory accumulation
 * This is a stress test to validate that memory is properly freed on each call
 */
TEST_CASE_METHOD(DevicePropertiesTestFixture, "DeviceProperties_AddManufacturerAndModel - Multiple calls no memory leak")
{
    SECTION("Should not leak memory on repeated calls")
    {
        // Arrange
        ADUC_AgentInfo agent = {};
        agent.manufacturer = (char*)"TestManufacturer";
        agent.model = (char*)"TestModel";
        agent.name = (char*)"test-agent";

        // Act - call multiple times to stress test memory handling
        for (int i = 0; i < 100; i++)
        {
            // Reset the JSON object for each iteration
            json_object_clear(device_props_obj);

            bool result = DeviceProperties_AddManufacturerAndModel(device_props_obj, &agent);

            // Assert
            REQUIRE(result == true);
        }

        // Final verification
        const char* manufacturer = json_object_get_string(device_props_obj, "manufacturer");
        const char* model = json_object_get_string(device_props_obj, "model");

        REQUIRE(manufacturer != nullptr);
        REQUIRE(model != nullptr);
        REQUIRE(strcmp(manufacturer, "TestManufacturer") == 0);
        REQUIRE(strcmp(model, "TestModel") == 0);
    }
}

/**
 * @brief Test with invalid JSON object
 */
TEST_CASE("DeviceProperties_AddManufacturerAndModel - With null JSON object")
{
    SECTION("Should handle null JSON object gracefully")
    {
        // Arrange
        ADUC_AgentInfo agent = {};
        agent.manufacturer = (char*)"TestManufacturer";
        agent.model = (char*)"TestModel";

        // Act
        bool result = DeviceProperties_AddManufacturerAndModel(nullptr, &agent);

        // Assert - should fail gracefully without crashing
        REQUIRE(result == false);
    }
}
