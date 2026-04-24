/**
 * @file pnp_protocol_ut.cpp
 * @brief Unit Tests for pnp_protocol module
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>
#include <cstring>
#include <string>
#include <vector>

extern "C"
{
#include "pnp_protocol.h"
#include "parson.h"
}

using Catch::Matchers::Equals;

/**
 * @brief Test PnP_CreateReportedProperty function
 */
TEST_CASE("PnP_CreateReportedProperty", "[pnp_helper]")
{
    SECTION("Create property without component name")
    {
        STRING_HANDLE result = PnP_CreateReportedProperty(nullptr, "testProperty", "\"testValue\"");
        REQUIRE(result != nullptr);

        const char* jsonStr = STRING_c_str(result);
        REQUIRE(jsonStr != nullptr);

        // Parse and verify JSON structure
        JSON_Value* jsonValue = json_parse_string(jsonStr);
        REQUIRE(jsonValue != nullptr);

        JSON_Object* jsonObject = json_value_get_object(jsonValue);
        REQUIRE(jsonObject != nullptr);

        const char* value = json_object_get_string(jsonObject, "testProperty");
        REQUIRE(value != nullptr);
        CHECK_THAT(value, Equals("testValue"));

        json_value_free(jsonValue);
        STRING_delete(result);
    }

    SECTION("Create property with component name")
    {
        STRING_HANDLE result = PnP_CreateReportedProperty("testComponent", "testProperty", "\"testValue\"");
        REQUIRE(result != nullptr);

        const char* jsonStr = STRING_c_str(result);
        REQUIRE(jsonStr != nullptr);

        // Parse and verify JSON structure
        JSON_Value* jsonValue = json_parse_string(jsonStr);
        REQUIRE(jsonValue != nullptr);

        JSON_Object* rootObject = json_value_get_object(jsonValue);
        REQUIRE(rootObject != nullptr);

        JSON_Object* componentObject = json_object_get_object(rootObject, "testComponent");
        REQUIRE(componentObject != nullptr);

        // Component marker should be present
        const char* componentMarker = json_object_get_string(componentObject, "__t");
        REQUIRE(componentMarker != nullptr);
        CHECK_THAT(componentMarker, Equals("c"));

        // Property value should be present
        const char* value = json_object_get_string(componentObject, "testProperty");
        REQUIRE(value != nullptr);
        CHECK_THAT(value, Equals("testValue"));

        json_value_free(jsonValue);
        STRING_delete(result);
    }

    SECTION("Create property with numeric value")
    {
        STRING_HANDLE result = PnP_CreateReportedProperty(nullptr, "temperature", "25.5");
        REQUIRE(result != nullptr);

        const char* jsonStr = STRING_c_str(result);
        REQUIRE(jsonStr != nullptr);

        JSON_Value* jsonValue = json_parse_string(jsonStr);
        REQUIRE(jsonValue != nullptr);

        JSON_Object* jsonObject = json_value_get_object(jsonValue);
        REQUIRE(jsonObject != nullptr);

        double value = json_object_get_number(jsonObject, "temperature");
        CHECK(value == 25.5);

        json_value_free(jsonValue);
        STRING_delete(result);
    }

    SECTION("Create property with boolean value")
    {
        STRING_HANDLE result = PnP_CreateReportedProperty(nullptr, "enabled", "true");
        REQUIRE(result != nullptr);

        const char* jsonStr = STRING_c_str(result);
        REQUIRE(jsonStr != nullptr);

        JSON_Value* jsonValue = json_parse_string(jsonStr);
        REQUIRE(jsonValue != nullptr);

        JSON_Object* jsonObject = json_value_get_object(jsonValue);
        REQUIRE(jsonObject != nullptr);

        int value = json_object_get_boolean(jsonObject, "enabled");
        CHECK(value == 1);

        json_value_free(jsonValue);
        STRING_delete(result);
    }
}

/**
 * @brief Test PnP_CreateReportedPropertyWithStatus function
 */
TEST_CASE("PnP_CreateReportedPropertyWithStatus", "[pnp_helper]")
{
    SECTION("Create property with status without component name")
    {
        STRING_HANDLE result = PnP_CreateReportedPropertyWithStatus(
            nullptr, "targetTemperature", "25", PNP_STATUS_SUCCESS, "Success", 1);
        REQUIRE(result != nullptr);

        const char* jsonStr = STRING_c_str(result);
        REQUIRE(jsonStr != nullptr);

        JSON_Value* jsonValue = json_parse_string(jsonStr);
        REQUIRE(jsonValue != nullptr);

        JSON_Object* rootObject = json_value_get_object(jsonValue);
        REQUIRE(rootObject != nullptr);

        JSON_Object* propObject = json_object_get_object(rootObject, "targetTemperature");
        REQUIRE(propObject != nullptr);

        // Check value
        double value = json_object_get_number(propObject, "value");
        CHECK(value == 25);

        // Check ack code (ac)
        int ac = (int)json_object_get_number(propObject, "ac");
        CHECK(ac == PNP_STATUS_SUCCESS);

        // Check ack description (ad)
        const char* ad = json_object_get_string(propObject, "ad");
        REQUIRE(ad != nullptr);
        CHECK_THAT(ad, Equals("Success"));

        // Check ack version (av)
        int av = (int)json_object_get_number(propObject, "av");
        CHECK(av == 1);

        json_value_free(jsonValue);
        STRING_delete(result);
    }

    SECTION("Create property with status with component name")
    {
        STRING_HANDLE result = PnP_CreateReportedPropertyWithStatus(
            "thermostat1", "targetTemperature", "30", PNP_STATUS_SUCCESS, "OK", 2);
        REQUIRE(result != nullptr);

        const char* jsonStr = STRING_c_str(result);
        REQUIRE(jsonStr != nullptr);

        JSON_Value* jsonValue = json_parse_string(jsonStr);
        REQUIRE(jsonValue != nullptr);

        JSON_Object* rootObject = json_value_get_object(jsonValue);
        REQUIRE(rootObject != nullptr);

        JSON_Object* componentObject = json_object_get_object(rootObject, "thermostat1");
        REQUIRE(componentObject != nullptr);

        // Check component marker
        const char* marker = json_object_get_string(componentObject, "__t");
        REQUIRE(marker != nullptr);
        CHECK_THAT(marker, Equals("c"));

        JSON_Object* propObject = json_object_get_object(componentObject, "targetTemperature");
        REQUIRE(propObject != nullptr);

        // Check value
        double value = json_object_get_number(propObject, "value");
        CHECK(value == 30);

        // Check ack version
        int av = (int)json_object_get_number(propObject, "av");
        CHECK(av == 2);

        json_value_free(jsonValue);
        STRING_delete(result);
    }

    SECTION("Create property with error status")
    {
        STRING_HANDLE result = PnP_CreateReportedPropertyWithStatus(
            nullptr, "brightness", "101", PNP_STATUS_BAD_FORMAT, "Value out of range", 5);
        REQUIRE(result != nullptr);

        const char* jsonStr = STRING_c_str(result);
        REQUIRE(jsonStr != nullptr);

        JSON_Value* jsonValue = json_parse_string(jsonStr);
        REQUIRE(jsonValue != nullptr);

        JSON_Object* rootObject = json_value_get_object(jsonValue);
        JSON_Object* propObject = json_object_get_object(rootObject, "brightness");
        REQUIRE(propObject != nullptr);

        int ac = (int)json_object_get_number(propObject, "ac");
        CHECK(ac == PNP_STATUS_BAD_FORMAT);

        const char* ad = json_object_get_string(propObject, "ad");
        REQUIRE(ad != nullptr);
        CHECK_THAT(ad, Equals("Value out of range"));

        json_value_free(jsonValue);
        STRING_delete(result);
    }
}

/**
 * @brief Test PnP_ParseCommandName function
 */
TEST_CASE("PnP_ParseCommandName", "[pnp_helper]")
{
    SECTION("Parse command without component (root command)")
    {
        const unsigned char* componentName = nullptr;
        size_t componentNameSize = 0;
        const char* pnpCommandName = nullptr;

        PnP_ParseCommandName("reboot", &componentName, &componentNameSize, &pnpCommandName);

        CHECK(componentName == nullptr);
        CHECK(componentNameSize == 0);
        REQUIRE(pnpCommandName != nullptr);
        CHECK_THAT(pnpCommandName, Equals("reboot"));
    }

    SECTION("Parse command with component")
    {
        const unsigned char* componentName = nullptr;
        size_t componentNameSize = 0;
        const char* pnpCommandName = nullptr;

        PnP_ParseCommandName("thermostat1*getMaxMinReport", &componentName, &componentNameSize, &pnpCommandName);

        REQUIRE(componentName != nullptr);
        CHECK(componentNameSize == 11); // "thermostat1" length
        REQUIRE(pnpCommandName != nullptr);
        CHECK_THAT(pnpCommandName, Equals("getMaxMinReport"));

        // Verify component name content
        std::string componentStr(reinterpret_cast<const char*>(componentName), componentNameSize);
        CHECK_THAT(componentStr, Equals("thermostat1"));
    }

    SECTION("Parse command with multiple separators - takes first")
    {
        const unsigned char* componentName = nullptr;
        size_t componentNameSize = 0;
        const char* pnpCommandName = nullptr;

        PnP_ParseCommandName("comp1*cmd*extra", &componentName, &componentNameSize, &pnpCommandName);

        REQUIRE(componentName != nullptr);
        CHECK(componentNameSize == 5); // "comp1" length
        REQUIRE(pnpCommandName != nullptr);
        CHECK_THAT(pnpCommandName, Equals("cmd*extra"));
    }

    SECTION("Parse empty component name before separator")
    {
        const unsigned char* componentName = nullptr;
        size_t componentNameSize = 0;
        const char* pnpCommandName = nullptr;

        PnP_ParseCommandName("*command", &componentName, &componentNameSize, &pnpCommandName);

        REQUIRE(componentName != nullptr);
        CHECK(componentNameSize == 0);
        REQUIRE(pnpCommandName != nullptr);
        CHECK_THAT(pnpCommandName, Equals("command"));
    }
}

/**
 * @brief Test PnP_CopyPayloadToString function
 */
TEST_CASE("PnP_CopyPayloadToString", "[pnp_helper]")
{
    SECTION("Copy simple payload")
    {
        const char* payload = "Hello, World!";
        size_t size = strlen(payload);

        char* result = PnP_CopyPayloadToString(reinterpret_cast<const unsigned char*>(payload), size);
        REQUIRE(result != nullptr);

        CHECK_THAT(result, Equals("Hello, World!"));
        CHECK(strlen(result) == size);

        free(result);
    }

    SECTION("Copy JSON payload")
    {
        const char* payload = "{\"temperature\":25.5}";
        size_t size = strlen(payload);

        char* result = PnP_CopyPayloadToString(reinterpret_cast<const unsigned char*>(payload), size);
        REQUIRE(result != nullptr);

        // Verify it's valid JSON
        JSON_Value* jsonValue = json_parse_string(result);
        REQUIRE(jsonValue != nullptr);

        JSON_Object* jsonObject = json_value_get_object(jsonValue);
        double temp = json_object_get_number(jsonObject, "temperature");
        CHECK(temp == 25.5);

        json_value_free(jsonValue);
        free(result);
    }

    SECTION("Copy empty payload")
    {
        const char* payload = "";
        size_t size = 0;

        char* result = PnP_CopyPayloadToString(reinterpret_cast<const unsigned char*>(payload), size);
        REQUIRE(result != nullptr);

        CHECK(strlen(result) == 0);
        CHECK_THAT(result, Equals(""));

        free(result);
    }

    SECTION("Copy binary-like data (non-null terminated)")
    {
        const unsigned char payload[] = { 'A', 'B', 'C', 'D' }; // No null terminator
        size_t size = sizeof(payload);

        char* result = PnP_CopyPayloadToString(payload, size);
        REQUIRE(result != nullptr);

        CHECK(strlen(result) == 4);
        CHECK(result[0] == 'A');
        CHECK(result[1] == 'B');
        CHECK(result[2] == 'C');
        CHECK(result[3] == 'D');
        CHECK(result[4] == '\0'); // Should be null terminated

        free(result);
    }
}

TEST_CASE("PnP_CreateTelemetryMessageHandle", "[pnp_helper]")
{
    SECTION("Creates telemetry message without component")
    {
        IOTHUB_MESSAGE_HANDLE messageHandle =
            PnP_CreateTelemetryMessageHandle(nullptr, "{\"temp\":25}");

        REQUIRE(messageHandle != nullptr);
        IoTHubMessage_Destroy(messageHandle);
    }

    SECTION("Creates telemetry message with component property")
    {
        IOTHUB_MESSAGE_HANDLE messageHandle =
            PnP_CreateTelemetryMessageHandle("deviceUpdate", "{\"state\":\"ok\"}");

        REQUIRE(messageHandle != nullptr);

        const char* value = IoTHubMessage_GetProperty(messageHandle, "$.sub");
        REQUIRE(value != nullptr);
        CHECK_THAT(value, Equals("deviceUpdate"));

        IoTHubMessage_Destroy(messageHandle);
    }

    SECTION("Returns nullptr for invalid telemetry payload")
    {
        IOTHUB_MESSAGE_HANDLE messageHandle = PnP_CreateTelemetryMessageHandle("deviceUpdate", nullptr);
        CHECK(messageHandle == nullptr);
    }
}

/**
 * @brief Test PnP status codes
 */
TEST_CASE("PnP Status Codes", "[pnp_helper]")
{
    SECTION("Verify status code values match HTTP semantics")
    {
        CHECK(PNP_STATUS_SUCCESS == 200);
        CHECK(PNP_STATUS_BAD_FORMAT == 400);
        CHECK(PNP_STATUS_NOT_FOUND == 404);
        CHECK(PNP_STATUS_INTERNAL_ERROR == 500);
    }
}

/**
 * @brief Test PnP maximum component length
 */
TEST_CASE("PnP Constants", "[pnp_helper]")
{
    SECTION("Verify maximum component length")
    {
        CHECK(PNP_MAXIMUM_COMPONENT_LENGTH == 64);
    }
}

/**
 * @brief Test callback context for PnP_ProcessTwinData tests
 */
struct PropertyCallbackContext
{
    int callCount = 0;
    std::vector<std::string> componentNames;
    std::vector<std::string> propertyNames;
    std::vector<int> versions;
    std::vector<std::string> propertyValues;
};

/**
 * @brief Callback function for PnP_ProcessTwinData tests
 */
static void TestPropertyCallback(
    const char* componentName,
    const char* propertyName,
    JSON_Value* propertyValue,
    int version,
    void* userContextCallback)
{
    PropertyCallbackContext* ctx = static_cast<PropertyCallbackContext*>(userContextCallback);
    ctx->callCount++;
    ctx->componentNames.push_back(componentName ? componentName : "");
    ctx->propertyNames.push_back(propertyName ? propertyName : "");
    ctx->versions.push_back(version);

    // Serialize the property value to string for comparison
    char* serialized = json_serialize_to_string(propertyValue);
    if (serialized)
    {
        ctx->propertyValues.push_back(serialized);
        json_free_serialized_string(serialized);
    }
    else
    {
        ctx->propertyValues.push_back("");
    }
}

/**
 * @brief Test PnP_ProcessTwinData function with complete twin update
 */
TEST_CASE("PnP_ProcessTwinData - Complete Twin Update", "[pnp_helper]")
{
    SECTION("Process complete twin with root properties only")
    {
        const char* twinJson = R"({
            "desired": {
                "targetTemperature": 25,
                "fanSpeed": "high",
                "$version": 5
            },
            "reported": {
                "temperature": 22
            }
        })";

        PropertyCallbackContext ctx;
        const char* components[] = {};

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_COMPLETE,
            reinterpret_cast<const unsigned char*>(twinJson),
            strlen(twinJson),
            components,
            0,
            TestPropertyCallback,
            &ctx);

        REQUIRE(result == true);
        CHECK(ctx.callCount == 2);

        // Verify properties were received (order may vary)
        bool foundTargetTemp = false;
        bool foundFanSpeed = false;
        for (int i = 0; i < ctx.callCount; i++)
        {
            CHECK(ctx.componentNames[i].empty()); // Root component
            CHECK(ctx.versions[i] == 5);

            if (ctx.propertyNames[i] == "targetTemperature")
            {
                foundTargetTemp = true;
                CHECK(ctx.propertyValues[i] == "25");
            }
            else if (ctx.propertyNames[i] == "fanSpeed")
            {
                foundFanSpeed = true;
                CHECK(ctx.propertyValues[i] == "\"high\"");
            }
        }
        CHECK(foundTargetTemp);
        CHECK(foundFanSpeed);
    }

    SECTION("Process complete twin with component properties")
    {
        const char* twinJson = R"({
            "desired": {
                "thermostat1": {
                    "__t": "c",
                    "targetTemperature": 30
                },
                "thermostat2": {
                    "__t": "c",
                    "targetTemperature": 22
                },
                "$version": 10
            },
            "reported": {}
        })";

        PropertyCallbackContext ctx;
        const char* components[] = { "thermostat1", "thermostat2" };

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_COMPLETE,
            reinterpret_cast<const unsigned char*>(twinJson),
            strlen(twinJson),
            components,
            2,
            TestPropertyCallback,
            &ctx);

        REQUIRE(result == true);
        CHECK(ctx.callCount == 2);

        // Find thermostat1 and thermostat2 properties
        bool foundThermostat1 = false;
        bool foundThermostat2 = false;
        for (int i = 0; i < ctx.callCount; i++)
        {
            CHECK(ctx.versions[i] == 10);
            CHECK(ctx.propertyNames[i] == "targetTemperature");

            if (ctx.componentNames[i] == "thermostat1")
            {
                foundThermostat1 = true;
                CHECK(ctx.propertyValues[i] == "30");
            }
            else if (ctx.componentNames[i] == "thermostat2")
            {
                foundThermostat2 = true;
                CHECK(ctx.propertyValues[i] == "22");
            }
        }
        CHECK(foundThermostat1);
        CHECK(foundThermostat2);
    }

    SECTION("Process complete twin with mixed root and component properties")
    {
        const char* twinJson = R"({
            "desired": {
                "rootProp": "rootValue",
                "thermostat1": {
                    "__t": "c",
                    "targetTemperature": 25
                },
                "$version": 7
            },
            "reported": {}
        })";

        PropertyCallbackContext ctx;
        const char* components[] = { "thermostat1" };

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_COMPLETE,
            reinterpret_cast<const unsigned char*>(twinJson),
            strlen(twinJson),
            components,
            1,
            TestPropertyCallback,
            &ctx);

        REQUIRE(result == true);
        CHECK(ctx.callCount == 2);

        // Find root property and component property
        bool foundRootProp = false;
        bool foundComponentProp = false;
        for (int i = 0; i < ctx.callCount; i++)
        {
            CHECK(ctx.versions[i] == 7);

            if (ctx.propertyNames[i] == "rootProp")
            {
                foundRootProp = true;
                CHECK(ctx.componentNames[i].empty());
                CHECK(ctx.propertyValues[i] == "\"rootValue\"");
            }
            else if (ctx.propertyNames[i] == "targetTemperature")
            {
                foundComponentProp = true;
                CHECK(ctx.componentNames[i] == "thermostat1");
                CHECK(ctx.propertyValues[i] == "25");
            }
        }
        CHECK(foundRootProp);
        CHECK(foundComponentProp);
    }
}

/**
 * @brief Test PnP_ProcessTwinData function with partial/patch twin update
 */
TEST_CASE("PnP_ProcessTwinData - Patch Twin Update", "[pnp_helper]")
{
    SECTION("Process patch update with root properties")
    {
        // Patch updates don't have the "desired" wrapper
        const char* twinJson = R"({
            "targetTemperature": 28,
            "$version": 15
        })";

        PropertyCallbackContext ctx;
        const char* components[] = {};

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_PARTIAL,
            reinterpret_cast<const unsigned char*>(twinJson),
            strlen(twinJson),
            components,
            0,
            TestPropertyCallback,
            &ctx);

        REQUIRE(result == true);
        CHECK(ctx.callCount == 1);
        CHECK(ctx.componentNames[0].empty());
        CHECK(ctx.propertyNames[0] == "targetTemperature");
        CHECK(ctx.propertyValues[0] == "28");
        CHECK(ctx.versions[0] == 15);
    }

    SECTION("Process patch update with component properties")
    {
        const char* twinJson = R"({
            "thermostat1": {
                "__t": "c",
                "targetTemperature": 35,
                "maxTemp": 50
            },
            "$version": 20
        })";

        PropertyCallbackContext ctx;
        const char* components[] = { "thermostat1" };

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_PARTIAL,
            reinterpret_cast<const unsigned char*>(twinJson),
            strlen(twinJson),
            components,
            1,
            TestPropertyCallback,
            &ctx);

        REQUIRE(result == true);
        CHECK(ctx.callCount == 2); // Two properties in thermostat1

        for (int i = 0; i < ctx.callCount; i++)
        {
            CHECK(ctx.componentNames[i] == "thermostat1");
            CHECK(ctx.versions[i] == 20);
        }
    }
}

/**
 * @brief Test PnP_ProcessTwinData error handling
 */
TEST_CASE("PnP_ProcessTwinData - Error Cases", "[pnp_helper]")
{
    PropertyCallbackContext ctx;
    const char* components[] = {};

    SECTION("Invalid JSON should return false")
    {
        const char* invalidJson = "{ invalid json }";

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_PARTIAL,
            reinterpret_cast<const unsigned char*>(invalidJson),
            strlen(invalidJson),
            components,
            0,
            TestPropertyCallback,
            &ctx);

        CHECK(result == false);
        CHECK(ctx.callCount == 0);
    }

    SECTION("Missing $version in patch should return false")
    {
        const char* noVersionJson = R"({
            "targetTemperature": 25
        })";

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_PARTIAL,
            reinterpret_cast<const unsigned char*>(noVersionJson),
            strlen(noVersionJson),
            components,
            0,
            TestPropertyCallback,
            &ctx);

        CHECK(result == false);
        CHECK(ctx.callCount == 0);
    }

    SECTION("$version as non-number should return false")
    {
        const char* stringVersionJson = R"({
            "targetTemperature": 25,
            "$version": "five"
        })";

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_PARTIAL,
            reinterpret_cast<const unsigned char*>(stringVersionJson),
            strlen(stringVersionJson),
            components,
            0,
            TestPropertyCallback,
            &ctx);

        CHECK(result == false);
        CHECK(ctx.callCount == 0);
    }

    SECTION("Complete twin missing 'desired' object should return false")
    {
        const char* noDesiredJson = R"({
            "reported": {
                "temperature": 22
            }
        })";

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_COMPLETE,
            reinterpret_cast<const unsigned char*>(noDesiredJson),
            strlen(noDesiredJson),
            components,
            0,
            TestPropertyCallback,
            &ctx);

        CHECK(result == false);
        CHECK(ctx.callCount == 0);
    }

    SECTION("Empty payload should return false")
    {
        const char* emptyJson = "";

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_PARTIAL,
            reinterpret_cast<const unsigned char*>(emptyJson),
            0,
            components,
            0,
            TestPropertyCallback,
            &ctx);

        CHECK(result == false);
        CHECK(ctx.callCount == 0);
    }
}

/**
 * @brief Test PnP_ProcessTwinData with complex nested values
 */
TEST_CASE("PnP_ProcessTwinData - Complex Values", "[pnp_helper]")
{
    SECTION("Process property with object value")
    {
        const char* twinJson = R"({
            "config": {
                "setting1": "value1",
                "setting2": 42
            },
            "$version": 3
        })";

        PropertyCallbackContext ctx;
        const char* components[] = {};

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_PARTIAL,
            reinterpret_cast<const unsigned char*>(twinJson),
            strlen(twinJson),
            components,
            0,
            TestPropertyCallback,
            &ctx);

        REQUIRE(result == true);
        CHECK(ctx.callCount == 1);
        CHECK(ctx.propertyNames[0] == "config");
        // The value should be a serialized JSON object
        CHECK(ctx.propertyValues[0].find("setting1") != std::string::npos);
        CHECK(ctx.propertyValues[0].find("setting2") != std::string::npos);
    }

    SECTION("Process property with array value")
    {
        const char* twinJson = R"({
            "supportedModes": ["auto", "manual", "eco"],
            "$version": 4
        })";

        PropertyCallbackContext ctx;
        const char* components[] = {};

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_PARTIAL,
            reinterpret_cast<const unsigned char*>(twinJson),
            strlen(twinJson),
            components,
            0,
            TestPropertyCallback,
            &ctx);

        REQUIRE(result == true);
        CHECK(ctx.callCount == 1);
        CHECK(ctx.propertyNames[0] == "supportedModes");
        CHECK(ctx.propertyValues[0].find("auto") != std::string::npos);
    }

    SECTION("Process property with null value")
    {
        const char* twinJson = R"({
            "optionalProp": null,
            "$version": 5
        })";

        PropertyCallbackContext ctx;
        const char* components[] = {};

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_PARTIAL,
            reinterpret_cast<const unsigned char*>(twinJson),
            strlen(twinJson),
            components,
            0,
            TestPropertyCallback,
            &ctx);

        REQUIRE(result == true);
        CHECK(ctx.callCount == 1);
        CHECK(ctx.propertyNames[0] == "optionalProp");
        CHECK(ctx.propertyValues[0] == "null");
    }

    SECTION("Process property with boolean values")
    {
        const char* twinJson = R"({
            "enabled": true,
            "disabled": false,
            "$version": 6
        })";

        PropertyCallbackContext ctx;
        const char* components[] = {};

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_PARTIAL,
            reinterpret_cast<const unsigned char*>(twinJson),
            strlen(twinJson),
            components,
            0,
            TestPropertyCallback,
            &ctx);

        REQUIRE(result == true);
        CHECK(ctx.callCount == 2);
    }
}

/**
 * @brief Test PnP_ProcessTwinData with multiple components
 */
TEST_CASE("PnP_ProcessTwinData - Multiple Components", "[pnp_helper]")
{
    SECTION("Unknown component treated as root property")
    {
        const char* twinJson = R"({
            "unknownComponent": {
                "__t": "c",
                "prop": "value"
            },
            "$version": 8
        })";

        PropertyCallbackContext ctx;
        const char* components[] = { "thermostat1" }; // unknownComponent not in list

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_PARTIAL,
            reinterpret_cast<const unsigned char*>(twinJson),
            strlen(twinJson),
            components,
            1,
            TestPropertyCallback,
            &ctx);

        REQUIRE(result == true);
        CHECK(ctx.callCount == 1);
        // unknownComponent should be treated as root property since it's not in the model
        CHECK(ctx.componentNames[0].empty());
        CHECK(ctx.propertyNames[0] == "unknownComponent");
    }

    SECTION("Empty components list - all treated as root")
    {
        const char* twinJson = R"({
            "thermostat1": {
                "__t": "c",
                "prop": "value"
            },
            "$version": 9
        })";

        PropertyCallbackContext ctx;
        const char* components[] = {};

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_PARTIAL,
            reinterpret_cast<const unsigned char*>(twinJson),
            strlen(twinJson),
            components,
            0,
            TestPropertyCallback,
            &ctx);

        REQUIRE(result == true);
        CHECK(ctx.callCount == 1);
        CHECK(ctx.componentNames[0].empty());
        CHECK(ctx.propertyNames[0] == "thermostat1");
    }

    SECTION("Complex model with many components")
    {
        const char* twinJson = R"({
            "desired": {
                "comp1": { "__t": "c", "p1": 1 },
                "comp2": { "__t": "c", "p2": 2 },
                "comp3": { "__t": "c", "p3": 3 },
                "rootProp": "rootVal",
                "$version": 100
            }
        })";

        PropertyCallbackContext ctx;
        const char* components[] = { "comp1", "comp2", "comp3" };

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_COMPLETE,
            reinterpret_cast<const unsigned char*>(twinJson),
            strlen(twinJson),
            components,
            3,
            TestPropertyCallback,
            &ctx);

        REQUIRE(result == true);
        CHECK(ctx.callCount == 4); // 3 component props + 1 root prop
    }
}

/**
 * @brief Test PnP_ProcessTwinData with JSON payloads whose root is not an object.
 * Covers the GetDesiredJson error path where json_value_get_object returns NULL.
 */
TEST_CASE("PnP_ProcessTwinData - Non-Object Root JSON", "[pnp_helper]")
{
    SECTION("JSON array root returns false (complete update)")
    {
        const char* arrayJson = R"([1, 2, 3])";

        PropertyCallbackContext ctx;
        const char* components[] = { "comp1" };

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_COMPLETE,
            reinterpret_cast<const unsigned char*>(arrayJson),
            strlen(arrayJson),
            components,
            1,
            TestPropertyCallback,
            &ctx);

        CHECK(result == false);
        CHECK(ctx.callCount == 0);
    }

    SECTION("JSON array root returns false (partial update)")
    {
        const char* arrayJson = R"([{"key": "value"}])";

        PropertyCallbackContext ctx;
        const char* components[] = { "comp1" };

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_PARTIAL,
            reinterpret_cast<const unsigned char*>(arrayJson),
            strlen(arrayJson),
            components,
            1,
            TestPropertyCallback,
            &ctx);

        CHECK(result == false);
        CHECK(ctx.callCount == 0);
    }

    SECTION("JSON string root returns false")
    {
        const char* stringJson = R"("just a string")";

        PropertyCallbackContext ctx;
        const char* components[] = {};

        bool result = PnP_ProcessTwinData(
            DEVICE_TWIN_UPDATE_COMPLETE,
            reinterpret_cast<const unsigned char*>(stringJson),
            strlen(stringJson),
            components,
            0,
            TestPropertyCallback,
            &ctx);

        CHECK(result == false);
        CHECK(ctx.callCount == 0);
    }
}
