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
#include <string>

// Include the module under test
extern "C"
{
#include "device_properties.h"
#include <aduc/c_utils.h>
#include <aduc/config_utils.h>
}

using Catch::Matchers::Equals;

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

/**
 * @brief Test DeviceProperties_ClearInterfaceId function
 */
TEST_CASE_METHOD(DevicePropertiesTestFixture, "DeviceProperties_ClearInterfaceId", "[device_properties]")
{
    SECTION("Should set interfaceId to null in JSON object")
    {
        // Act
        bool result = DeviceProperties_ClearInterfaceId(device_props_obj);

        // Assert
        REQUIRE(result == true);

        // Verify interfaceId is set to null
        JSON_Value* interfaceIdValue = json_object_get_value(device_props_obj, "interfaceId");
        REQUIRE(interfaceIdValue != nullptr);
        CHECK(json_value_get_type(interfaceIdValue) == JSONNull);
    }

    SECTION("Should overwrite existing interfaceId with null")
    {
        // Arrange - set an initial interfaceId
        json_object_set_string(device_props_obj, "interfaceId", "old-interface-id");

        // Act
        bool result = DeviceProperties_ClearInterfaceId(device_props_obj);

        // Assert
        REQUIRE(result == true);

        JSON_Value* interfaceIdValue = json_object_get_value(device_props_obj, "interfaceId");
        REQUIRE(interfaceIdValue != nullptr);
        CHECK(json_value_get_type(interfaceIdValue) == JSONNull);
    }
}

/**
 * @brief Test DeviceProperties_AddContractModelId function
 */
TEST_CASE_METHOD(DevicePropertiesTestFixture, "DeviceProperties_AddContractModelId", "[device_properties]")
{
    SECTION("Should add contractModelId to JSON object")
    {
        // Act
        bool result = DeviceProperties_AddContractModelId(device_props_obj);

        // Assert
        REQUIRE(result == true);

        const char* contractModelId = json_object_get_string(device_props_obj, "contractModelId");
        REQUIRE(contractModelId != nullptr);
        CHECK_THAT(contractModelId, Equals("dtmi:azure:iot:deviceUpdateContractModel;3"));
    }

    SECTION("Should overwrite existing contractModelId")
    {
        // Arrange - set an initial contractModelId
        json_object_set_string(device_props_obj, "contractModelId", "old-contract-model-id");

        // Act
        bool result = DeviceProperties_AddContractModelId(device_props_obj);

        // Assert
        REQUIRE(result == true);

        const char* contractModelId = json_object_get_string(device_props_obj, "contractModelId");
        REQUIRE(contractModelId != nullptr);
        CHECK_THAT(contractModelId, Equals("dtmi:azure:iot:deviceUpdateContractModel;3"));
    }
}

/**
 * @brief Test DeviceProperties_AddVersions function
 */
TEST_CASE_METHOD(DevicePropertiesTestFixture, "DeviceProperties_AddVersions", "[device_properties]")
{
    SECTION("Should add version to JSON object")
    {
        // Act
        bool result = DeviceProperties_AddVersions(device_props_obj);

        // Assert
        REQUIRE(result == true);

        const char* aducVersion = json_object_get_string(device_props_obj, "aduVer");
        REQUIRE(aducVersion != nullptr);
        // Version string format is "DU;agent/X.Y.Z" - verify it's not empty
        CHECK(strlen(aducVersion) > 0);
    }
}

/**
 * @brief Test DeviceProperties_AddAdditionalProperties function
 */
TEST_CASE_METHOD(DevicePropertiesTestFixture, "DeviceProperties_AddAdditionalProperties", "[device_properties]")
{
    SECTION("Should succeed with null agent")
    {
        // Act
        bool result = DeviceProperties_AddAdditionalProperties(device_props_obj, nullptr);

        // Assert
        REQUIRE(result == true);
    }

    SECTION("Should succeed with agent having null additionalDeviceProperties")
    {
        // Arrange
        ADUC_AgentInfo agent = {};
        agent.additionalDeviceProperties = nullptr;

        // Act
        bool result = DeviceProperties_AddAdditionalProperties(device_props_obj, &agent);

        // Assert
        REQUIRE(result == true);
    }

    SECTION("Should add additional properties from agent")
    {
        // Arrange - create additional properties JSON
        JSON_Value* additionalPropsValue = json_value_init_object();
        JSON_Object* additionalPropsObj = json_value_get_object(additionalPropsValue);
        json_object_set_string(additionalPropsObj, "customProp1", "value1");
        json_object_set_string(additionalPropsObj, "customProp2", "value2");

        ADUC_AgentInfo agent = {};
        agent.additionalDeviceProperties = additionalPropsObj;

        // Act
        bool result = DeviceProperties_AddAdditionalProperties(device_props_obj, &agent);

        // Assert
        REQUIRE(result == true);

        const char* prop1 = json_object_get_string(device_props_obj, "customProp1");
        const char* prop2 = json_object_get_string(device_props_obj, "customProp2");
        REQUIRE(prop1 != nullptr);
        REQUIRE(prop2 != nullptr);
        CHECK_THAT(prop1, Equals("value1"));
        CHECK_THAT(prop2, Equals("value2"));

        // Clean up
        json_value_free(additionalPropsValue);
    }

    SECTION("Should handle empty additional properties")
    {
        // Arrange - create empty additional properties JSON
        JSON_Value* additionalPropsValue = json_value_init_object();
        JSON_Object* additionalPropsObj = json_value_get_object(additionalPropsValue);

        ADUC_AgentInfo agent = {};
        agent.additionalDeviceProperties = additionalPropsObj;

        // Act
        bool result = DeviceProperties_AddAdditionalProperties(device_props_obj, &agent);

        // Assert
        REQUIRE(result == true);

        // Clean up
        json_value_free(additionalPropsValue);
    }
}

/**
 * @brief Test combined DeviceProperties operations
 */
TEST_CASE_METHOD(DevicePropertiesTestFixture, "DeviceProperties - Combined operations", "[device_properties]")
{
    SECTION("Should handle all device properties additions successfully")
    {
        // Arrange
        ADUC_AgentInfo agent = {};
        agent.manufacturer = (char*)"TestManufacturer";
        agent.model = (char*)"TestModel";
        agent.additionalDeviceProperties = nullptr;

        // Act
        bool result1 = DeviceProperties_AddManufacturerAndModel(device_props_obj, &agent);
        bool result2 = DeviceProperties_ClearInterfaceId(device_props_obj);
        bool result3 = DeviceProperties_AddContractModelId(device_props_obj);
        bool result4 = DeviceProperties_AddVersions(device_props_obj);
        bool result5 = DeviceProperties_AddAdditionalProperties(device_props_obj, &agent);

        // Assert
        REQUIRE(result1 == true);
        REQUIRE(result2 == true);
        REQUIRE(result3 == true);
        REQUIRE(result4 == true);
        REQUIRE(result5 == true);

        // Verify all properties are present
        CHECK(json_object_get_string(device_props_obj, "manufacturer") != nullptr);
        CHECK(json_object_get_string(device_props_obj, "model") != nullptr);
        CHECK(json_value_get_type(json_object_get_value(device_props_obj, "interfaceId")) == JSONNull);
        CHECK(json_object_get_string(device_props_obj, "contractModelId") != nullptr);
        CHECK(json_object_get_string(device_props_obj, "aduVer") != nullptr);
    }
}

/**
 * @brief Test DeviceProperties_AddVersions with version string validation
 */
TEST_CASE_METHOD(DevicePropertiesTestFixture, "DeviceProperties_AddVersions - Format validation", "[device_properties]")
{
    SECTION("Version string should follow expected format")
    {
        // Act
        bool result = DeviceProperties_AddVersions(device_props_obj);

        // Assert
        REQUIRE(result == true);

        const char* aducVersion = json_object_get_string(device_props_obj, "aduVer");
        REQUIRE(aducVersion != nullptr);

        // Version format should be "BUILDER;agent/X.Y.Z"
        std::string versionStr(aducVersion);
        CHECK(versionStr.find(";agent/") != std::string::npos);
        CHECK(versionStr.length() > 0);
    }

    SECTION("Multiple calls should overwrite previous version")
    {
        // Act
        bool result1 = DeviceProperties_AddVersions(device_props_obj);
        bool result2 = DeviceProperties_AddVersions(device_props_obj);

        // Assert
        REQUIRE(result1 == true);
        REQUIRE(result2 == true);

        // Should only have one version key
        size_t count = 0;
        size_t numKeys = json_object_get_count(device_props_obj);
        for (size_t i = 0; i < numKeys; i++)
        {
            const char* key = json_object_get_name(device_props_obj, i);
            if (key && strcmp(key, "aduVer") == 0)
            {
                count++;
            }
        }
        CHECK(count == 1);
    }
}

/**
 * @brief Test DeviceProperties_AddContractModelId with contract model validation
 */
TEST_CASE_METHOD(DevicePropertiesTestFixture, "DeviceProperties_AddContractModelId - Format validation", "[device_properties]")
{
    SECTION("Contract model ID should follow DTMI format")
    {
        // Act
        bool result = DeviceProperties_AddContractModelId(device_props_obj);

        // Assert
        REQUIRE(result == true);

        const char* contractModelId = json_object_get_string(device_props_obj, "contractModelId");
        REQUIRE(contractModelId != nullptr);

        // DTMI format: dtmi:<domain>:<model>;<version>
        std::string modelIdStr(contractModelId);
        CHECK(modelIdStr.find("dtmi:") == 0);
        CHECK(modelIdStr.find(";") != std::string::npos);
    }

    SECTION("Multiple calls should not duplicate the property")
    {
        // Act
        bool result1 = DeviceProperties_AddContractModelId(device_props_obj);
        bool result2 = DeviceProperties_AddContractModelId(device_props_obj);

        // Assert
        REQUIRE(result1 == true);
        REQUIRE(result2 == true);

        // Verify value is still correct
        const char* contractModelId = json_object_get_string(device_props_obj, "contractModelId");
        REQUIRE(contractModelId != nullptr);
        CHECK_THAT(contractModelId, Equals("dtmi:azure:iot:deviceUpdateContractModel;3"));
    }
}

/**
 * @brief Test DeviceProperties_ClearInterfaceId error handling
 */
TEST_CASE("DeviceProperties_ClearInterfaceId - Error handling", "[device_properties]")
{
    SECTION("Should handle null JSON object gracefully")
    {
        // Act - passing null device_props_obj
        bool result = DeviceProperties_ClearInterfaceId(nullptr);

        // Assert - should fail gracefully
        REQUIRE(result == false);
    }
}

/**
 * @brief Test DeviceProperties_AddContractModelId error handling
 */
TEST_CASE("DeviceProperties_AddContractModelId - Error handling", "[device_properties]")
{
    SECTION("Should handle null JSON object gracefully")
    {
        // Act - passing null device_props_obj
        bool result = DeviceProperties_AddContractModelId(nullptr);

        // Assert - should fail gracefully
        REQUIRE(result == false);
    }
}

/**
 * @brief Test DeviceProperties_AddVersions error handling
 */
TEST_CASE("DeviceProperties_AddVersions - Error handling", "[device_properties]")
{
    SECTION("Should handle null JSON object gracefully")
    {
        // Act - passing null device_props_obj
        bool result = DeviceProperties_AddVersions(nullptr);

        // Assert - should fail gracefully
        REQUIRE(result == false);
    }
}

/**
 * @brief Test DeviceProperties_AddAdditionalProperties error handling
 */
TEST_CASE("DeviceProperties_AddAdditionalProperties - Error handling", "[device_properties]")
{
    SECTION("Should handle null JSON object gracefully")
    {
        // Arrange
        ADUC_AgentInfo agent = {};
        JSON_Value* additionalPropsValue = json_value_init_object();
        JSON_Object* additionalPropsObj = json_value_get_object(additionalPropsValue);
        json_object_set_string(additionalPropsObj, "prop1", "value1");
        agent.additionalDeviceProperties = additionalPropsObj;

        // Act - passing null device_props_obj
        bool result = DeviceProperties_AddAdditionalProperties(nullptr, &agent);

        // Assert - should fail gracefully
        REQUIRE(result == false);

        // Cleanup
        json_value_free(additionalPropsValue);
    }
}

/**
 * @brief Test DeviceProperties_AddAdditionalProperties with various property types
 */
TEST_CASE_METHOD(DevicePropertiesTestFixture, "DeviceProperties_AddAdditionalProperties - Multiple properties", "[device_properties]")
{
    SECTION("Should add multiple string properties")
    {
        // Arrange
        JSON_Value* additionalPropsValue = json_value_init_object();
        JSON_Object* additionalPropsObj = json_value_get_object(additionalPropsValue);
        json_object_set_string(additionalPropsObj, "location", "Building A");
        json_object_set_string(additionalPropsObj, "floor", "3");
        json_object_set_string(additionalPropsObj, "room", "301");

        ADUC_AgentInfo agent = {};
        agent.additionalDeviceProperties = additionalPropsObj;

        // Act
        bool result = DeviceProperties_AddAdditionalProperties(device_props_obj, &agent);

        // Assert
        REQUIRE(result == true);

        CHECK_THAT(json_object_get_string(device_props_obj, "location"), Equals("Building A"));
        CHECK_THAT(json_object_get_string(device_props_obj, "floor"), Equals("3"));
        CHECK_THAT(json_object_get_string(device_props_obj, "room"), Equals("301"));

        // Cleanup
        json_value_free(additionalPropsValue);
    }

    SECTION("Should handle property with empty string value")
    {
        // Arrange
        JSON_Value* additionalPropsValue = json_value_init_object();
        JSON_Object* additionalPropsObj = json_value_get_object(additionalPropsValue);
        json_object_set_string(additionalPropsObj, "emptyProp", "");

        ADUC_AgentInfo agent = {};
        agent.additionalDeviceProperties = additionalPropsObj;

        // Act
        bool result = DeviceProperties_AddAdditionalProperties(device_props_obj, &agent);

        // Assert
        REQUIRE(result == true);

        const char* val = json_object_get_string(device_props_obj, "emptyProp");
        REQUIRE(val != nullptr);
        CHECK_THAT(val, Equals(""));

        // Cleanup
        json_value_free(additionalPropsValue);
    }

    SECTION("Should handle property with special characters")
    {
        // Arrange
        JSON_Value* additionalPropsValue = json_value_init_object();
        JSON_Object* additionalPropsObj = json_value_get_object(additionalPropsValue);
        json_object_set_string(additionalPropsObj, "specialProp", "value with \"quotes\" and 'apostrophes'");

        ADUC_AgentInfo agent = {};
        agent.additionalDeviceProperties = additionalPropsObj;

        // Act
        bool result = DeviceProperties_AddAdditionalProperties(device_props_obj, &agent);

        // Assert
        REQUIRE(result == true);

        const char* val = json_object_get_string(device_props_obj, "specialProp");
        REQUIRE(val != nullptr);
        CHECK(strlen(val) > 0);

        // Cleanup
        json_value_free(additionalPropsValue);
    }
}

/**
 * @brief Test DeviceProperties_AddManufacturerAndModel with edge cases
 */
TEST_CASE_METHOD(DevicePropertiesTestFixture, "DeviceProperties_AddManufacturerAndModel - Edge cases", "[device_properties]")
{
    SECTION("Should handle empty manufacturer string")
    {
        // Arrange
        ADUC_AgentInfo agent = {};
        agent.manufacturer = (char*)"";
        agent.model = (char*)"TestModel";

        // Act - empty string is still a valid string
        bool result = DeviceProperties_AddManufacturerAndModel(device_props_obj, &agent);

        // Assert
        REQUIRE(result == true);

        const char* manufacturer = json_object_get_string(device_props_obj, "manufacturer");
        REQUIRE(manufacturer != nullptr);
        CHECK_THAT(manufacturer, Equals(""));
    }

    SECTION("Should handle empty model string")
    {
        // Arrange
        ADUC_AgentInfo agent = {};
        agent.manufacturer = (char*)"TestManufacturer";
        agent.model = (char*)"";

        // Act
        bool result = DeviceProperties_AddManufacturerAndModel(device_props_obj, &agent);

        // Assert
        REQUIRE(result == true);

        const char* model = json_object_get_string(device_props_obj, "model");
        REQUIRE(model != nullptr);
        CHECK_THAT(model, Equals(""));
    }

    SECTION("Should handle long manufacturer and model strings")
    {
        // Arrange
        std::string longManufacturer(256, 'M'); // 256 character manufacturer
        std::string longModel(256, 'D'); // 256 character model

        ADUC_AgentInfo agent = {};
        agent.manufacturer = const_cast<char*>(longManufacturer.c_str());
        agent.model = const_cast<char*>(longModel.c_str());

        // Act
        bool result = DeviceProperties_AddManufacturerAndModel(device_props_obj, &agent);

        // Assert
        REQUIRE(result == true);

        const char* manufacturer = json_object_get_string(device_props_obj, "manufacturer");
        const char* model = json_object_get_string(device_props_obj, "model");
        REQUIRE(manufacturer != nullptr);
        REQUIRE(model != nullptr);
        CHECK(strlen(manufacturer) == 256);
        CHECK(strlen(model) == 256);
    }

    SECTION("Should handle manufacturer and model with special characters")
    {
        // Arrange
        ADUC_AgentInfo agent = {};
        agent.manufacturer = (char*)"Test-Manufacturer_123";
        agent.model = (char*)"Model.v2.0-beta";

        // Act
        bool result = DeviceProperties_AddManufacturerAndModel(device_props_obj, &agent);

        // Assert
        REQUIRE(result == true);

        CHECK_THAT(json_object_get_string(device_props_obj, "manufacturer"), Equals("Test-Manufacturer_123"));
        CHECK_THAT(json_object_get_string(device_props_obj, "model"), Equals("Model.v2.0-beta"));
    }
}

/**
 * @brief Test that all DeviceProperties functions can be called in sequence and produce valid JSON
 */
TEST_CASE_METHOD(DevicePropertiesTestFixture, "DeviceProperties - Full workflow integration", "[device_properties]")
{
    SECTION("Complete workflow produces valid serializable JSON")
    {
        // Arrange
        JSON_Value* additionalPropsValue = json_value_init_object();
        JSON_Object* additionalPropsObj = json_value_get_object(additionalPropsValue);
        json_object_set_string(additionalPropsObj, "customId", "device-001");

        ADUC_AgentInfo agent = {};
        agent.manufacturer = (char*)"Contoso";
        agent.model = (char*)"SmartDevice";
        agent.additionalDeviceProperties = additionalPropsObj;

        // Act - call all device property functions
        REQUIRE(DeviceProperties_AddManufacturerAndModel(device_props_obj, &agent));
        REQUIRE(DeviceProperties_ClearInterfaceId(device_props_obj));
        REQUIRE(DeviceProperties_AddContractModelId(device_props_obj));
        REQUIRE(DeviceProperties_AddVersions(device_props_obj));
        REQUIRE(DeviceProperties_AddAdditionalProperties(device_props_obj, &agent));

        // Assert - all properties should be present
        CHECK_THAT(json_object_get_string(device_props_obj, "manufacturer"), Equals("Contoso"));
        CHECK_THAT(json_object_get_string(device_props_obj, "model"), Equals("SmartDevice"));
        CHECK(json_value_get_type(json_object_get_value(device_props_obj, "interfaceId")) == JSONNull);
        CHECK(json_object_get_string(device_props_obj, "contractModelId") != nullptr);
        CHECK(json_object_get_string(device_props_obj, "aduVer") != nullptr);
        CHECK_THAT(json_object_get_string(device_props_obj, "customId"), Equals("device-001"));

        // Verify the entire JSON can be serialized
        char* serialized = json_serialize_to_string(root_value);
        REQUIRE(serialized != nullptr);
        CHECK(strlen(serialized) > 0);

        // Verify it can be parsed back
        JSON_Value* parsed = json_parse_string(serialized);
        REQUIRE(parsed != nullptr);

        json_free_serialized_string(serialized);
        json_value_free(parsed);
        json_value_free(additionalPropsValue);
    }
}
