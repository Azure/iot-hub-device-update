/**
 * @file startup_msg_helper_ut.cpp
 * @brief Unit Tests for startup_msg_helper module
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>
#include <cstring>
#include <string>

extern "C"
{
#include "startup_msg_helper.h"
#include "device_properties.h"
#include <aduc/config_utils.h>
#include <aduc/types/update_content.h>
#include <parson.h>

    // Mock control variables
    static bool mock_addManufacturerAndModel_result = true;
    static bool mock_addAdditionalProperties_result = true;
    static bool mock_clearInterfaceId_result = true;
    static bool mock_addContractModelId_result = true;
    static bool mock_addVersions_result = true;

    // Mock DeviceProperties functions
    bool DeviceProperties_AddManufacturerAndModel(JSON_Object* devicePropsObj, const ADUC_AgentInfo* agent)
    {
        if (!mock_addManufacturerAndModel_result)
        {
            return false;
        }
        json_object_set_string(devicePropsObj, "manufacturer", "testManufacturer");
        json_object_set_string(devicePropsObj, "model", "testModel");
        return true;
    }

    bool DeviceProperties_AddAdditionalProperties(JSON_Object* devicePropsObj, const ADUC_AgentInfo* agent)
    {
        return mock_addAdditionalProperties_result;
    }

    bool DeviceProperties_ClearInterfaceId(JSON_Object* devicePropsObj)
    {
        return mock_clearInterfaceId_result;
    }

    bool DeviceProperties_AddContractModelId(JSON_Object* devicePropsObj)
    {
        return mock_addContractModelId_result;
    }

    bool DeviceProperties_AddVersions(JSON_Object* devicePropsObj)
    {
        return mock_addVersions_result;
    }

    // Mock ADUC_ConfigInfo functions
    static ADUC_ConfigInfo mock_configInfo;
    static bool mock_configInfo_available = true;

    const ADUC_ConfigInfo* ADUC_ConfigInfo_GetInstance()
    {
        if (!mock_configInfo_available)
        {
            return NULL;
        }
        return &mock_configInfo;
    }

    int ADUC_ConfigInfo_ReleaseInstance(const ADUC_ConfigInfo* configInfo)
    {
        (void)configInfo;
        return 0;
    }

// Undefine logging macros so we can provide mock function implementations
#undef log_debug
#undef log_info
#undef log_warn
#undef log_error
#undef Log_Debug
#undef Log_Info
#undef Log_Warn
#undef Log_Error

    // Mock logging functions
    void Log_Error(const char* fmt, ...)
    {
        (void)fmt;
    }
    void Log_Warn(const char* fmt, ...)
    {
        (void)fmt;
    }
    void Log_Info(const char* fmt, ...)
    {
        (void)fmt;
    }
    void Log_Debug(const char* fmt, ...)
    {
        (void)fmt;
    }
}

using Catch::Matchers::Equals;

class StartupMsgTestFixture
{
public:
    StartupMsgTestFixture()
    {
        rootValue = json_value_init_object();
        rootObj = json_value_get_object(rootValue);
        memset(&agentInfo, 0, sizeof(agentInfo));
        memset(&mock_configInfo, 0, sizeof(mock_configInfo));

        // Reset all mocks to success
        mock_addManufacturerAndModel_result = true;
        mock_addAdditionalProperties_result = true;
        mock_clearInterfaceId_result = true;
        mock_addContractModelId_result = true;
        mock_addVersions_result = true;
        mock_configInfo_available = true;
    }

    ~StartupMsgTestFixture()
    {
        json_value_free(rootValue);
    }

protected:
    JSON_Value* rootValue;
    JSON_Object* rootObj;
    ADUC_AgentInfo agentInfo;
};

TEST_CASE_METHOD(StartupMsgTestFixture, "StartupMsg_AddDeviceProperties", "[startup_msg_helper]")
{
    SECTION("Returns false when startupObj is NULL")
    {
        bool result = StartupMsg_AddDeviceProperties(NULL, &agentInfo);
        REQUIRE(result == false);
    }

    SECTION("Succeeds with valid inputs")
    {
        bool result = StartupMsg_AddDeviceProperties(rootObj, &agentInfo);
        REQUIRE(result == true);

        // Verify deviceProperties was added
        JSON_Object* deviceProps =
            json_object_get_object(rootObj, ADUCITF_FIELDNAME_DEVICEPROPERTIES);
        REQUIRE(deviceProps != nullptr);

        // Verify manufacturer and model were set by mock
        const char* manufacturer = json_object_get_string(deviceProps, "manufacturer");
        CHECK_THAT(manufacturer, Equals("testManufacturer"));

        const char* model = json_object_get_string(deviceProps, "model");
        CHECK_THAT(model, Equals("testModel"));
    }

    SECTION("Succeeds with NULL agent")
    {
        bool result = StartupMsg_AddDeviceProperties(rootObj, NULL);
        REQUIRE(result == true);
    }

    SECTION("Fails when AddManufacturerAndModel fails")
    {
        mock_addManufacturerAndModel_result = false;
        bool result = StartupMsg_AddDeviceProperties(rootObj, &agentInfo);
        REQUIRE(result == false);
    }

    SECTION("Fails when AddAdditionalProperties fails")
    {
        mock_addAdditionalProperties_result = false;
        bool result = StartupMsg_AddDeviceProperties(rootObj, &agentInfo);
        REQUIRE(result == false);
    }

    SECTION("Fails when ClearInterfaceId fails")
    {
        mock_clearInterfaceId_result = false;
        bool result = StartupMsg_AddDeviceProperties(rootObj, &agentInfo);
        REQUIRE(result == false);
    }

    SECTION("Fails when AddContractModelId fails")
    {
        mock_addContractModelId_result = false;
        bool result = StartupMsg_AddDeviceProperties(rootObj, &agentInfo);
        REQUIRE(result == false);
    }

    SECTION("Can be called multiple times on same object")
    {
        bool result1 = StartupMsg_AddDeviceProperties(rootObj, &agentInfo);
        REQUIRE(result1 == true);

        // Second call should overwrite
        bool result2 = StartupMsg_AddDeviceProperties(rootObj, &agentInfo);
        REQUIRE(result2 == true);
    }
}

TEST_CASE_METHOD(StartupMsgTestFixture, "StartupMsg_AddCompatPropertyNames", "[startup_msg_helper]")
{
    SECTION("Returns false when startupObj is NULL")
    {
        bool result = StartupMsg_AddCompatPropertyNames(NULL);
        REQUIRE(result == false);
    }

    SECTION("Returns false when ConfigInfo is not available")
    {
        mock_configInfo_available = false;
        bool result = StartupMsg_AddCompatPropertyNames(rootObj);
        REQUIRE(result == false);
    }

    SECTION("Uses default compat property names when config value is NULL")
    {
        mock_configInfo.compatPropertyNames = NULL;
        bool result = StartupMsg_AddCompatPropertyNames(rootObj);
        REQUIRE(result == true);

        const char* compatNames =
            json_object_get_string(rootObj, ADUCITF_FIELDNAME_COMPAT_PROPERTY_NAMES);
        REQUIRE(compatNames != nullptr);
        CHECK_THAT(compatNames, Equals("manufacturer,model"));
    }

    SECTION("Uses default compat property names when config value is empty")
    {
        mock_configInfo.compatPropertyNames = "";
        bool result = StartupMsg_AddCompatPropertyNames(rootObj);
        REQUIRE(result == true);

        const char* compatNames =
            json_object_get_string(rootObj, ADUCITF_FIELDNAME_COMPAT_PROPERTY_NAMES);
        REQUIRE(compatNames != nullptr);
        CHECK_THAT(compatNames, Equals("manufacturer,model"));
    }

    SECTION("Uses custom compat property names from config")
    {
        mock_configInfo.compatPropertyNames = "manufacturer,model,customProp";
        bool result = StartupMsg_AddCompatPropertyNames(rootObj);
        REQUIRE(result == true);

        const char* compatNames =
            json_object_get_string(rootObj, ADUCITF_FIELDNAME_COMPAT_PROPERTY_NAMES);
        REQUIRE(compatNames != nullptr);
        CHECK_THAT(compatNames, Equals("manufacturer,model,customProp"));
    }

    SECTION("Can be called multiple times")
    {
        mock_configInfo.compatPropertyNames = "firstValue";
        bool result1 = StartupMsg_AddCompatPropertyNames(rootObj);
        REQUIRE(result1 == true);

        mock_configInfo.compatPropertyNames = "secondValue";
        bool result2 = StartupMsg_AddCompatPropertyNames(rootObj);
        REQUIRE(result2 == true);

        const char* compatNames =
            json_object_get_string(rootObj, ADUCITF_FIELDNAME_COMPAT_PROPERTY_NAMES);
        CHECK_THAT(compatNames, Equals("secondValue"));
    }
}
