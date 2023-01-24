/**
 * @file config_utils_ut.cpp
 * @brief Unit Tests for config_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "umock_c/umock_c.h"

#include <catch2/catch.hpp>

#include <aduc/c_utils.h>
#include <aduc/calloc_wrapper.hpp>
#include <aduc/string_utils.hpp>
#include <azure_c_shared_utility/crt_abstractions.h>
#include <parson.h>
#include <string.h>

#define ENABLE_MOCKS
#include "aduc/config_utils.h"
#undef ENABLE_MOCKS

using ADUC::StringUtils::cstr_wrapper;
using Catch::Matchers::Equals;

// clang-format off
static const char* validConfigContentStr =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("compatPropertyNames": "manufacturer,model",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box")"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* validConfigContentNoCompatPropertyNames =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box")"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* validConfigContentMqttIotHubProtocol =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("iotHubProtocol": "mqtt",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box")"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* validConfigContentMqttWebSocketsIotHubProtocol =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("iotHubProtocol": "mqtt/ws",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box")"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* validConfigContentMissingIotHubProtocol =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box")"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* invalidConfigContentNoDeviceInfoStr =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box")"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static const char* invalidConfigContentNoDevicePropertiesStr =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("compatPropertyNames": "manufacturer,model",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(})"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(})"
            R"(})"
        R"(])"
    R"(})";

static const char* invalidConfigContentStrEmpty = R"({})";

static const char* invalidConfigContentStr =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("agents": [])"
    R"(})";

static const char* validConfigContentAdditionalPropertyNames =
    R"({)"
        R"("schemaVersion": "1.0",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("agents": [)"
            R"({ )"
            R"("name": "host-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "AIS",)"
                R"("connectionData": "iotHubDeviceUpdate")"
            R"(},)"
            R"("manufacturer": "Contoso",)"
            R"("model": "Smart-Box",)"
            R"("additionalDeviceProperties": {)"
                R"("location": "US",)"
                R"("language": "English")"
            R"(})"
            R"(},)"
            R"({)"
            R"("name": "leaf-update",)"
            R"("runas": "adu",)"
            R"("connectionSource": {)"
                R"("connectionType": "string",)"
                R"("connectionData": "HOSTNAME=...")"
            R"(},)"
            R"("manufacturer": "Fabrikam",)"
            R"("model": "Camera")"
            R"(})"
        R"(])"
    R"(})";

static char* g_configContentString = nullptr;

static JSON_Value* MockParse_JSON_File(const char* configFilePath)
{
    UNREFERENCED_PARAMETER(configFilePath);
    return json_parse_string(g_configContentString);
}

class GlobalMockHookTestCaseFixture
{
public:
    GlobalMockHookTestCaseFixture()
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast): clang-tidy can't handle the way that UMock expands macro's so we're suppressing
        REGISTER_GLOBAL_MOCK_HOOK(Parse_JSON_File, MockParse_JSON_File);

    }

    ~GlobalMockHookTestCaseFixture() = default;

    GlobalMockHookTestCaseFixture(const GlobalMockHookTestCaseFixture&) = delete;
    GlobalMockHookTestCaseFixture& operator=(const GlobalMockHookTestCaseFixture&) = delete;
    GlobalMockHookTestCaseFixture(GlobalMockHookTestCaseFixture&&) = delete;
    GlobalMockHookTestCaseFixture& operator=(GlobalMockHookTestCaseFixture&&) = delete;
};

TEST_CASE_METHOD(GlobalMockHookTestCaseFixture, "ADUC_ConfigInfo_Init Functional Tests")
{
    SECTION("Valid config content, Success Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentStr) == 0);
        cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu/du-config.json"));
        CHECK_THAT(json_array_get_string(config.aduShellTrustedUsers, 0), Equals("adu"));
        CHECK_THAT(json_array_get_string(config.aduShellTrustedUsers, 1), Equals("do"));
        CHECK_THAT(config.schemaVersion, Equals("1.1"));
        CHECK_THAT(config.manufacturer, Equals("device_info_manufacturer"));
        CHECK_THAT(config.model, Equals("device_info_model"));
        CHECK_THAT(config.compatPropertyNames, Equals("manufacturer,model"));
        CHECK(config.agentCount == 2);
        const ADUC_AgentInfo* first_agent_info = ADUC_ConfigInfo_GetAgent(&config, 0);
        CHECK_THAT(first_agent_info->name, Equals("host-update"));
        CHECK_THAT(first_agent_info->runas, Equals("adu"));
        CHECK_THAT(first_agent_info->manufacturer, Equals("Contoso"));
        CHECK_THAT(first_agent_info->model, Equals("Smart-Box"));
        CHECK_THAT(first_agent_info->connectionType, Equals("AIS"));
        CHECK_THAT(first_agent_info->connectionData, Equals("iotHubDeviceUpdate"));
        const ADUC_AgentInfo* second_agent_info = ADUC_ConfigInfo_GetAgent(&config, 1);
        CHECK_THAT(second_agent_info->name, Equals("leaf-update"));
        CHECK_THAT(second_agent_info->runas, Equals("adu"));
        CHECK_THAT(second_agent_info->manufacturer, Equals("Fabrikam"));
        CHECK_THAT(second_agent_info->model, Equals("Camera"));
        CHECK_THAT(second_agent_info->connectionType, Equals("string"));
        CHECK_THAT(second_agent_info->connectionData, Equals("HOSTNAME=..."));
        CHECK(first_agent_info->additionalDeviceProperties == nullptr);

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content without compatPropertyNames, Success Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentNoCompatPropertyNames) == 0);
        cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu/du-config.json"));
        CHECK(config.compatPropertyNames == nullptr);

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Config content with customized additional device properties, Successful Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentAdditionalPropertyNames) == 0);

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu/du-config.json"));
        const ADUC_AgentInfo* first_agent_info = ADUC_ConfigInfo_GetAgent(&config, 0);
        CHECK(first_agent_info->additionalDeviceProperties != nullptr);
        ADUC_ConfigInfo_UnInit(&config);

        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc, hicpp-no-malloc): g_configContentString is a basic C-string so it must be freed by a call to free()
        free(g_configContentString);
    }

    SECTION("Valid config content without device info, Failure Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, invalidConfigContentNoDeviceInfoStr) == 0);
        cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(!ADUC_ConfigInfo_Init(&config, "/etc/adu/du-config.json"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content without device properties, Failure Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, invalidConfigContentNoDevicePropertiesStr) == 0);
        cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(!ADUC_ConfigInfo_Init(&config, "/etc/adu/du-config.json"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Empty config content, Failure Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, invalidConfigContentStrEmpty) == 0);
        cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(!ADUC_ConfigInfo_Init(&config, "/etc/adu/du-config.json"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Invalid config content, Failure Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, invalidConfigContentStr) == 0);
        cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(!ADUC_ConfigInfo_Init(&config, "/etc/adu/du-config.json"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content, mqtt iotHubProtocol")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentMqttIotHubProtocol) == 0);
        cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu/du-config.json"));
        CHECK_THAT(config.iotHubProtocol, Equals("mqtt"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content, mqtt/ws iotHubProtocol")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentMqttWebSocketsIotHubProtocol) == 0);
        cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu/du-config.json"));
        CHECK_THAT(config.iotHubProtocol, Equals("mqtt/ws"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content, missing iotHubProtocol defaults to mqtt.")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentMissingIotHubProtocol) == 0);
        cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu/du-config.json"));
        CHECK(config.iotHubProtocol == nullptr);

        ADUC_ConfigInfo_UnInit(&config);
    }
}
