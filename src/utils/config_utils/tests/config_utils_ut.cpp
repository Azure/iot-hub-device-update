/**
 * @file config_utils_ut.cpp
 * @brief Unit Tests for config_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "umock_c/umock_c.h"

#include <catch2/catch_all.hpp>

#include <aduc/c_utils.h>
#include <aduc/calloc_wrapper.hpp>
#include <azure_c_shared_utility/crt_abstractions.h>
#include <parson.h>
#include <string.h>

#define ENABLE_MOCKS
#include "aduc/config_utils.h"
#undef ENABLE_MOCKS

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
        R"(],)"
        R"("idlePauseMilliseconds": 30000)"
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

static const char* validConfigContentDownloadTimeout =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("downloadTimeoutInMinutes": 1440,)"
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

static const char* validConfigWithOverrideFolder =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("downloadTimeoutInMinutes": 1440,)"
        R"("aduShellFolder": "/usr/mybin",)"
        R"("dataFolder": "/var/lib/adu/mydata",)"
        R"("extensionsFolder": "/var/lib/adu/myextensions",)"
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

static const char* invalidConfigContentDownloadTimeout =
    R"({)"
        R"("schemaVersion": "1.1",)"
        R"("aduShellTrustedUsers": ["adu","do"],)"
        R"("manufacturer": "device_info_manufacturer",)"
        R"("model": "device_info_model",)"
        R"("downloadTimeoutInMinutes": -1,)"
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
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
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
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        CHECK(config.compatPropertyNames == nullptr);

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Config content with customized additional device properties, Successful Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentAdditionalPropertyNames) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        const ADUC_AgentInfo* first_agent_info = ADUC_ConfigInfo_GetAgent(&config, 0);
        CHECK(first_agent_info->additionalDeviceProperties != nullptr);
        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content without device info, Failure Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, invalidConfigContentNoDeviceInfoStr) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK_FALSE(ADUC_ConfigInfo_Init(&config, "/etc/adu"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content without device properties, Failure Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, invalidConfigContentNoDevicePropertiesStr) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK_FALSE(ADUC_ConfigInfo_Init(&config, "/etc/adu"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Empty config content, Failure Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, invalidConfigContentStrEmpty) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK_FALSE(ADUC_ConfigInfo_Init(&config, "/etc/adu"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Invalid config content, Failure Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, invalidConfigContentStr) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK_FALSE(ADUC_ConfigInfo_Init(&config, "/etc/adu"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content, downloadTimeoutInMinutes")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentDownloadTimeout) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        CHECK(config.downloadTimeoutInMinutes == 1440);

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Invalid config content, downloadTimeoutInMinutes")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, invalidConfigContentDownloadTimeout) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        CHECK(config.downloadTimeoutInMinutes == 0);
        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content, mqtt iotHubProtocol")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentMqttIotHubProtocol) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        CHECK_THAT(config.iotHubProtocol, Equals("mqtt"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content, mqtt/ws iotHubProtocol")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentMqttWebSocketsIotHubProtocol) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        CHECK_THAT(config.iotHubProtocol, Equals("mqtt/ws"));

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Valid config content, missing iotHubProtocol defaults to mqtt.")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentMissingIotHubProtocol) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        CHECK(config.iotHubProtocol == nullptr);

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("Refcount Test")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentDownloadTimeout) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };
        const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
        CHECK(config != NULL);
        CHECK(config->refCount == 1);

        const ADUC_ConfigInfo* config2 = ADUC_ConfigInfo_GetInstance();
        CHECK(config2 != NULL);
        CHECK(config2->refCount == 2);

        ADUC_ConfigInfo_ReleaseInstance(config2);
        CHECK(config2->refCount == 1);
        CHECK(config->refCount == 1);

        ADUC_ConfigInfo_ReleaseInstance(config);
        CHECK(config->refCount == 0);
    }

    SECTION("User folders from build configs")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentDownloadTimeout) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };
        const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
        CHECK(config != NULL);
        CHECK_THAT(config->aduShellFolder, Equals("/usr/bin"));

#if defined(WIN32)
        CHECK_THAT(config->aduShellFilePath, Equals("/usr/bin/adu-shell.exe"));
#else
        CHECK_THAT(config->aduShellFilePath, Equals("/usr/bin/adu-shell"));
#endif
        CHECK_THAT(config->dataFolder, Equals("/var/lib/adu"));
        CHECK_THAT(config->extensionsFolder, Equals("/var/lib/adu/extensions"));
        CHECK_THAT(config->extensionsComponentEnumeratorFolder, Equals("/var/lib/adu/extensions/component_enumerator"));
        CHECK_THAT(config->extensionsContentDownloaderFolder, Equals("/var/lib/adu/extensions/content_downloader"));
        CHECK_THAT(config->extensionsStepHandlerFolder, Equals("/var/lib/adu/extensions/update_content_handlers"));
        CHECK_THAT(config->extensionsDownloadHandlerFolder, Equals("/var/lib/adu/extensions/download_handlers"));
        CHECK_THAT(config->downloadsFolder, Equals("/var/lib/adu/downloads"));
        ADUC_ConfigInfo_ReleaseInstance(config);
        CHECK(config->refCount == 0);
    }

    SECTION("User folders from du-config file")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigWithOverrideFolder) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };
        const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
        CHECK_THAT(config->aduShellFolder, Equals("/usr/mybin"));

#if defined(WIN32)
        CHECK_THAT(config->aduShellFilePath, Equals("/usr/mybin/adu-shell.exe"));
#else
        CHECK_THAT(config->aduShellFilePath, Equals("/usr/mybin/adu-shell"));
#endif

        CHECK_THAT(config->dataFolder, Equals("/var/lib/adu/mydata"));
        CHECK_THAT(config->extensionsFolder, Equals("/var/lib/adu/myextensions"));
        CHECK_THAT(config->extensionsComponentEnumeratorFolder, Equals("/var/lib/adu/myextensions/component_enumerator"));
        CHECK_THAT(config->extensionsContentDownloaderFolder, Equals("/var/lib/adu/myextensions/content_downloader"));
        CHECK_THAT(config->extensionsStepHandlerFolder, Equals("/var/lib/adu/myextensions/update_content_handlers"));
        CHECK_THAT(config->extensionsDownloadHandlerFolder, Equals("/var/lib/adu/myextensions/download_handlers"));
        CHECK_THAT(config->downloadsFolder, Equals("/var/lib/adu/mydata/downloads"));
        ADUC_ConfigInfo_ReleaseInstance(config);
        CHECK(config->refCount == 0);
    }

    SECTION("apiRequestFifoPath is optional")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentStr) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        CHECK(config.apiRequestFifoPath == nullptr);

    SECTION("GetAduShellTrustedUsers returns and frees trusted-user vector")
    {
        REQUIRE(ADUC_ConfigInfo_Init(&config, "/etc/adu"));

        VECTOR_HANDLE users = ADUC_ConfigInfo_GetAduShellTrustedUsers(&config);
        REQUIRE(users != nullptr);
        REQUIRE(VECTOR_size(users) == 2);

        STRING_HANDLE* firstUser = static_cast<STRING_HANDLE*>(VECTOR_element(users, 0));
        STRING_HANDLE* secondUser = static_cast<STRING_HANDLE*>(VECTOR_element(users, 1));
        REQUIRE(firstUser != nullptr);
        REQUIRE(secondUser != nullptr);
        CHECK_THAT(STRING_c_str(*firstUser), Equals("adu"));
        CHECK_THAT(STRING_c_str(*secondUser), Equals("do"));

        ADUC_ConfigInfo_FreeAduShellTrustedUsers(users);
        CHECK(VECTOR_size(users) == 0);
        VECTOR_destroy(users);

        ADUC_ConfigInfo_UnInit(&config);
    }

    SECTION("idlePuaseMilliseconds is optional and defaults to 0 (no pause)")
    {
        REQUIRE(mallocAndStrcpy_s(&g_configContentString, validConfigContentStr) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};

        CHECK(ADUC_ConfigInfo_Init(&config, "/etc/adu"));
        CHECK(config.idlePauseMilliseconds == 0);

        ADUC_ConfigInfo_UnInit(&config);

    SECTION("X509 connection reads cert, key and ca files")
    {
        const char* certPath = "/tmp/adu_config_utils_ut_cert.pem";
        const char* keyPath = "/tmp/adu_config_utils_ut_key.pem";
        const char* caPath = "/tmp/adu_config_utils_ut_ca.pem";

        {
            FILE* f = fopen(certPath, "wb");
            REQUIRE(f != nullptr);
            REQUIRE(fwrite("CERT", 1, 4, f) == 4);
            fclose(f);
        }
        {
            FILE* f = fopen(keyPath, "wb");
            REQUIRE(f != nullptr);
            REQUIRE(fwrite("KEY", 1, 3, f) == 3);
            fclose(f);
        }
        {
            FILE* f = fopen(caPath, "wb");
            REQUIRE(f != nullptr);
            REQUIRE(fwrite("CA", 1, 2, f) == 2);
            fclose(f);
        }

        std::string x509Config = std::string("{") +
            "\"schemaVersion\":\"1.1\"," +
            "\"aduShellTrustedUsers\":[\"adu\",\"do\"]," +
            "\"manufacturer\":\"device_info_manufacturer\"," +
            "\"model\":\"device_info_model\"," +
            "\"agents\":[{" +
                "\"name\":\"host-update\"," +
                "\"runas\":\"adu\"," +
                "\"connectionSource\":{" +
                    "\"connectionType\":\"AIS\"," +
                    "\"connectionData\":\"iotHubDeviceUpdate\"," +
                    "\"connectionX509CertFilePath\":\"" + certPath + "\"," +
                    "\"connectionX509PrivateKeyFilePath\":\"" + keyPath + "\"," +
                    "\"connectionX509CaCertFilePath\":\"" + caPath + "\"" +
                "}," +
                "\"manufacturer\":\"Contoso\"," +
                "\"model\":\"Smart-Box\"" +
            "}]}";

        REQUIRE(mallocAndStrcpy_s(&g_configContentString, x509Config.c_str()) == 0);
        ADUC::StringUtils::cstr_wrapper configStr{ g_configContentString };

        ADUC_ConfigInfo config = {};
        REQUIRE(ADUC_ConfigInfo_Init(&config, "/etc/adu"));

        const ADUC_AgentInfo* agent = ADUC_ConfigInfo_GetAgent(&config, 0);
        REQUIRE(agent != nullptr);
        CHECK(agent->x509Cert != nullptr);
        CHECK(agent->x509PrivateKey != nullptr);
        CHECK(agent->x509CaCert != nullptr);

        ADUC_ConfigInfo_UnInit(&config);

        remove(certPath);
        remove(keyPath);
        remove(caPath);
    }
}
