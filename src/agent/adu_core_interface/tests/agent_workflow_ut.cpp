/**
 * @file agent_workflow_ut.cpp
 * @brief Unit tests for the agent workflow.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include <sstream>
#include <string>

#include "agent_workflow_utils.h"

#include <aduc/adu_core_export_helpers.h>
#include <aduc/adu_core_json.h>
#include <aduc/calloc_wrapper.hpp>

using JSON_Value_wrapper = ADUC::StringUtils::calloc_wrapper<JSON_Value>;

TEST_CASE("IsDuplicateRequest")
{
    SECTION("Download Action")
    {
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Download, ADUCITF_State_Idle));
        CHECK(IsDuplicateRequest(ADUCITF_UpdateAction_Download, ADUCITF_State_DownloadStarted));
        CHECK(IsDuplicateRequest(ADUCITF_UpdateAction_Download, ADUCITF_State_DownloadSucceeded));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Download, ADUCITF_State_InstallStarted));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Download, ADUCITF_State_InstallSucceeded));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Download, ADUCITF_State_ApplyStarted));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Download, ADUCITF_State_Failed));
    }

    SECTION("Install Action")
    {
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Install, ADUCITF_State_Idle));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Install, ADUCITF_State_DownloadStarted));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Install, ADUCITF_State_DownloadSucceeded));
        CHECK(IsDuplicateRequest(ADUCITF_UpdateAction_Install, ADUCITF_State_InstallStarted));
        CHECK(IsDuplicateRequest(ADUCITF_UpdateAction_Install, ADUCITF_State_InstallSucceeded));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Install, ADUCITF_State_ApplyStarted));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Install, ADUCITF_State_Failed));
    }

    SECTION("Apply Action")
    {
        CHECK(IsDuplicateRequest(ADUCITF_UpdateAction_Apply, ADUCITF_State_Idle));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Apply, ADUCITF_State_DownloadStarted));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Apply, ADUCITF_State_DownloadSucceeded));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Apply, ADUCITF_State_InstallStarted));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Apply, ADUCITF_State_InstallSucceeded));
        CHECK(IsDuplicateRequest(ADUCITF_UpdateAction_Apply, ADUCITF_State_ApplyStarted));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Apply, ADUCITF_State_Failed));
    }

    SECTION("Cancel Action")
    {
        CHECK(IsDuplicateRequest(ADUCITF_UpdateAction_Cancel, ADUCITF_State_Idle));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Cancel, ADUCITF_State_DownloadStarted));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Cancel, ADUCITF_State_DownloadSucceeded));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Cancel, ADUCITF_State_InstallStarted));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Cancel, ADUCITF_State_InstallSucceeded));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Cancel, ADUCITF_State_ApplyStarted));
        CHECK_FALSE(IsDuplicateRequest(ADUCITF_UpdateAction_Cancel, ADUCITF_State_Failed));
    }
}

std::string
CreateContentIdJSON(const char* installedCriteria, const ADUC_UpdateId* expectedUpdateId, const char* updateType);

TEST_CASE("ADUC_ContentData_AllocAndInit")
{
    SECTION("Null JSON")
    {
        ADUC_ContentData* data = ADUC_ContentData_AllocAndInit(nullptr);
        CHECK(data == nullptr);
    }

    SECTION("Empty JSON")
    {
        JSON_Value_wrapper json{ ADUC_Json_GetRoot("{}") };
        REQUIRE(json.get() != nullptr);

        ADUC_ContentData* data = ADUC_ContentData_AllocAndInit(json.get());
        CHECK(data == nullptr);
    }

    SECTION("Only Action")
    {
        std::string jsonString = CreateContentIdJSON(nullptr, nullptr, nullptr);

        JSON_Value_wrapper json{ ADUC_Json_GetRoot(jsonString.c_str()) };
        REQUIRE(json.get() != nullptr);

        ADUC_ContentData* data = ADUC_ContentData_AllocAndInit(json.get());
        CHECK(data == nullptr);
    }

    SECTION("Only installedCriteria")
    {
        std::string installedCriteria{ "1.0.0.0" };
        std::string jsonString = CreateContentIdJSON(installedCriteria.c_str(), nullptr, nullptr);

        JSON_Value_wrapper json{ ADUC_Json_GetRoot(jsonString.c_str()) };
        REQUIRE(json.get() != nullptr);

        ADUC_ContentData* data = ADUC_ContentData_AllocAndInit(json.get());
        CHECK(data == nullptr);
    }

    SECTION("Only UpdateType")
    {
        std::string updateType{ "microsoft/apt:1" };
        std::string jsonString = CreateContentIdJSON(nullptr, nullptr, updateType.c_str());

        JSON_Value_wrapper json{ ADUC_Json_GetRoot(jsonString.c_str()) };
        REQUIRE(json.get() != nullptr);

        ADUC_ContentData* data = ADUC_ContentData_AllocAndInit(json.get());
        CHECK(data == nullptr);
    }

    SECTION("Only ExpectedUpdateId")
    {
        std::string expectedContentId{ "namespace:name:1.0.0.0" };

        ADUC_UpdateId* expectedUpdateId = ADUC_UpdateId_AllocAndInit("example-provider", "example-name", "1.0.0.0");
        CHECK(expectedUpdateId != nullptr);

        std::string jsonString = CreateContentIdJSON(nullptr, expectedUpdateId, nullptr);
        JSON_Value_wrapper json{ ADUC_Json_GetRoot(jsonString.c_str()) };
        REQUIRE(json.get() != nullptr);

        ADUC_ContentData* data = ADUC_ContentData_AllocAndInit(json.get());
        CHECK(data == nullptr);

        ADUC_UpdateId_Free(expectedUpdateId);
    }

    SECTION("All required properties")
    {
        std::string installedCriteria{ "1.0.0.0" };
        std::string updateType{ "microsoft/apt:1" };
        ADUC_UpdateId* expectedUpdateId = ADUC_UpdateId_AllocAndInit("example-provider", "example-name", "1.0.0.0");
        CHECK(expectedUpdateId != nullptr);

        std::string jsonString = CreateContentIdJSON(installedCriteria.c_str(), expectedUpdateId, updateType.c_str());

        JSON_Value_wrapper json{ ADUC_Json_GetRoot(jsonString.c_str()) };
        REQUIRE(json.get() != nullptr);

        ADUC_ContentData* data = ADUC_ContentData_AllocAndInit(json.get());
        REQUIRE(data != nullptr);
        CHECK(data->InstalledCriteria != nullptr);
        CHECK_THAT(installedCriteria, Equals(data->InstalledCriteria));

        CHECK(data->UpdateType != nullptr);
        CHECK_THAT(updateType, Equals(data->UpdateType));

        CHECK(data->ExpectedUpdateId != nullptr);
        CHECK_THAT(expectedUpdateId->Provider, Equals(data->ExpectedUpdateId->Provider));
        CHECK_THAT(expectedUpdateId->Name, Equals(data->ExpectedUpdateId->Name));
        CHECK_THAT(expectedUpdateId->Version, Equals(data->ExpectedUpdateId->Version));

        ADUC_UpdateId_Free(expectedUpdateId);
        ADUC_ContentData_Free(data);
    }
}

ADUC_ContentData* CreateInitialContentData(
    const std::string& installedCriteria, const ADUC_UpdateId* expectedUpdateId, const std::string& updateType);

TEST_CASE("ADUC_ContentData_Update")
{
    SECTION("Null JSON")
    {
        std::string initialInstalledCriteria{ "1.0.0.0" };
        std::string initialUpdateType{ "microsoft/apt:1" };
        ADUC_UpdateId* initialExpectedUpdateId =
            ADUC_UpdateId_AllocAndInit("example-provider", "example-name", "1.0.0.0");
        CHECK(initialExpectedUpdateId != nullptr);

        ADUC_ContentData* data =
            CreateInitialContentData(initialInstalledCriteria, initialExpectedUpdateId, initialUpdateType);
        REQUIRE(data != nullptr);

        ADUC_ContentData_Update(data, nullptr, false);
        CHECK(data->InstalledCriteria != nullptr);
        CHECK_THAT(initialInstalledCriteria, Equals(data->InstalledCriteria));

        CHECK(data->UpdateType != nullptr);
        CHECK_THAT(initialUpdateType, Equals(data->UpdateType));

        CHECK(data->ExpectedUpdateId != nullptr);
        CHECK_THAT(initialExpectedUpdateId->Provider, Equals(data->ExpectedUpdateId->Provider));
        CHECK_THAT(initialExpectedUpdateId->Name, Equals(data->ExpectedUpdateId->Name));
        CHECK_THAT(initialExpectedUpdateId->Version, Equals(data->ExpectedUpdateId->Version));

        ADUC_UpdateId_Free(initialExpectedUpdateId);
        ADUC_ContentData_Free(data);
    }

    SECTION("Empty JSON")
    {
        std::string initialInstalledCriteria{ "1.0.0.0" };
        std::string initialUpdateType{ "microsoft/apt:1" };
        ADUC_UpdateId* initialExpectedUpdateId =
            ADUC_UpdateId_AllocAndInit("example-provider", "example-name", "1.0.0.0");
        CHECK(initialExpectedUpdateId != nullptr);

        ADUC_ContentData* data =
            CreateInitialContentData(initialInstalledCriteria, initialExpectedUpdateId, initialUpdateType);
        REQUIRE(data != nullptr);
        JSON_Value_wrapper json{ ADUC_Json_GetRoot("{}") };
        REQUIRE(json.get() != nullptr);

        ADUC_ContentData_Update(data, json.get(), false);
        CHECK(data->InstalledCriteria != nullptr);
        CHECK_THAT(initialInstalledCriteria, Equals(data->InstalledCriteria));

        CHECK(data->UpdateType != nullptr);
        CHECK_THAT(initialUpdateType, Equals(data->UpdateType));

        CHECK(data->ExpectedUpdateId != nullptr);
        CHECK_THAT(initialExpectedUpdateId->Provider, Equals(data->ExpectedUpdateId->Provider));
        CHECK_THAT(initialExpectedUpdateId->Name, Equals(data->ExpectedUpdateId->Name));
        CHECK_THAT(initialExpectedUpdateId->Version, Equals(data->ExpectedUpdateId->Version));

        ADUC_ContentData_Free(data);
        ADUC_UpdateId_Free(initialExpectedUpdateId);
    }

    SECTION("Only Action")
    {
        std::string initialInstalledCriteria{ "1.0.0.0" };
        std::string initialUpdateType{ "microsoft/apt:1" };
        ADUC_UpdateId* initialExpectedUpdateId =
            ADUC_UpdateId_AllocAndInit("example-provider", "example-name", "1.0.0.0");
        CHECK(initialExpectedUpdateId != nullptr);

        ADUC_ContentData* data =
            CreateInitialContentData(initialInstalledCriteria, initialExpectedUpdateId, initialUpdateType);
        REQUIRE(data != nullptr);

        std::string jsonString = CreateContentIdJSON(nullptr, nullptr, nullptr);

        JSON_Value_wrapper json{ ADUC_Json_GetRoot(jsonString.c_str()) };
        REQUIRE(json.get() != nullptr);

        ADUC_ContentData_Update(data, json.get(), false);
        CHECK(data->InstalledCriteria != nullptr);
        CHECK_THAT(initialInstalledCriteria, Equals(data->InstalledCriteria));

        CHECK(data->UpdateType != nullptr);
        CHECK_THAT(initialUpdateType, Equals(data->UpdateType));

        CHECK(data->ExpectedUpdateId != nullptr);
        CHECK_THAT(initialExpectedUpdateId->Provider, Equals(data->ExpectedUpdateId->Provider));
        CHECK_THAT(initialExpectedUpdateId->Name, Equals(data->ExpectedUpdateId->Name));
        CHECK_THAT(initialExpectedUpdateId->Version, Equals(data->ExpectedUpdateId->Version));

        ADUC_UpdateId_Free(initialExpectedUpdateId);
        ADUC_ContentData_Free(data);
    }

    SECTION("Only installedCriteria")
    {
        std::string initialInstalledCriteria{ "1.0.0.0" };
        std::string initialUpdateType{ "microsoft/apt:1" };
        ADUC_UpdateId* initialExpectedUpdateId =
            ADUC_UpdateId_AllocAndInit("example-provider", "example-name", "1.0.0.0");
        CHECK(initialExpectedUpdateId != nullptr);

        ADUC_ContentData* data =
            CreateInitialContentData(initialInstalledCriteria, initialExpectedUpdateId, initialUpdateType);

        std::string installedCriteria{ "2.0.0.0" };
        std::string jsonString = CreateContentIdJSON(installedCriteria.c_str(), nullptr, nullptr);

        JSON_Value_wrapper json{ ADUC_Json_GetRoot(jsonString.c_str()) };
        REQUIRE(json.get() != nullptr);

        ADUC_ContentData_Update(data, json.get(), false);
        CHECK(data->InstalledCriteria != nullptr);
        CHECK_THAT(installedCriteria, Equals(data->InstalledCriteria));

        CHECK(data->UpdateType != nullptr);
        CHECK_THAT(initialUpdateType, Equals(data->UpdateType));

        CHECK(data->ExpectedUpdateId != nullptr);
        CHECK_THAT(initialExpectedUpdateId->Provider, Equals(data->ExpectedUpdateId->Provider));
        CHECK_THAT(initialExpectedUpdateId->Name, Equals(data->ExpectedUpdateId->Name));
        CHECK_THAT(initialExpectedUpdateId->Version, Equals(data->ExpectedUpdateId->Version));

        ADUC_UpdateId_Free(initialExpectedUpdateId);
        ADUC_ContentData_Free(data);
    }

    SECTION("Only UpdateType")
    {
        std::string initialInstalledCriteria{ "1.0.0.0" };
        std::string initialUpdateType{ "microsoft/apt:1" };
        ADUC_UpdateId* initialExpectedUpdateId =
            ADUC_UpdateId_AllocAndInit("example-provider", "example-name", "1.0.0.0");
        CHECK(initialExpectedUpdateId != nullptr);

        ADUC_ContentData* data =
            CreateInitialContentData(initialInstalledCriteria, initialExpectedUpdateId, initialUpdateType);

        std::string updateType{ "microsoft/apt:2" };
        std::string jsonString =
            CreateContentIdJSON(initialInstalledCriteria.c_str(), initialExpectedUpdateId, updateType.c_str());

        JSON_Value_wrapper json{ ADUC_Json_GetRoot(jsonString.c_str()) };
        REQUIRE(json.get() != nullptr);

        ADUC_ContentData_Update(data, json.get(), false);
        CHECK(data->InstalledCriteria != nullptr);
        CHECK_THAT(initialInstalledCriteria, Equals(data->InstalledCriteria));

        CHECK(data->UpdateType != nullptr);
        CHECK_THAT(updateType, Equals(data->UpdateType));

        CHECK(data->ExpectedUpdateId != nullptr);
        CHECK_THAT(initialExpectedUpdateId->Provider, Equals(data->ExpectedUpdateId->Provider));
        CHECK_THAT(initialExpectedUpdateId->Name, Equals(data->ExpectedUpdateId->Name));
        CHECK_THAT(initialExpectedUpdateId->Version, Equals(data->ExpectedUpdateId->Version));

        ADUC_UpdateId_Free(initialExpectedUpdateId);
        ADUC_ContentData_Free(data);
    }

    SECTION("Only ExpectedContentId")
    {
        std::string initialInstalledCriteria{ "1.0.0.0" };
        std::string initialUpdateType{ "microsoft/apt:1" };
        ADUC_UpdateId* initialUpdateId = ADUC_UpdateId_AllocAndInit("example-provider", "example-name", "1.0.0.0");
        CHECK(initialUpdateId != nullptr);

        ADUC_ContentData* data =
            CreateInitialContentData(initialInstalledCriteria, initialUpdateId, initialUpdateType);

        ADUC_UpdateId* expectedUpdateId = ADUC_UpdateId_AllocAndInit("example-provider", "example-name", "2.0.0.0");
        CHECK(expectedUpdateId != nullptr);

        std::string jsonString = CreateContentIdJSON(nullptr, expectedUpdateId, nullptr);

        JSON_Value_wrapper json{ ADUC_Json_GetRoot(jsonString.c_str()) };
        REQUIRE(json.get() != nullptr);

        ADUC_ContentData_Update(data, json.get(), false);
        CHECK_THAT(initialInstalledCriteria, Equals(data->InstalledCriteria));
        CHECK_THAT(initialUpdateType, Equals(data->UpdateType));
        CHECK_THAT(expectedUpdateId->Provider, Equals(data->ExpectedUpdateId->Provider));
        CHECK_THAT(expectedUpdateId->Name, Equals(data->ExpectedUpdateId->Name));
        CHECK_THAT(expectedUpdateId->Version, Equals(data->ExpectedUpdateId->Version));

        ADUC_UpdateId_Free(initialUpdateId);
        ADUC_UpdateId_Free(expectedUpdateId);
        ADUC_ContentData_Free(data);
    }

    SECTION("All properties")
    {
        std::string initialInstalledCriteria{ "1.0.0.0" };
        std::string initialUpdateType{ "microsoft/apt:1" };
        ADUC_UpdateId* initialExpectedUpdateId =
            ADUC_UpdateId_AllocAndInit("example-provider", "example-name", "1.0.0.0");
        CHECK(initialExpectedUpdateId != nullptr);

        ADUC_ContentData* data =
            CreateInitialContentData(initialInstalledCriteria, initialExpectedUpdateId, initialUpdateType);

        std::string installedCriteria{ "2.0.0.0" };
        std::string updateType{ "microsoft/apt:2" };
        ADUC_UpdateId* expectedUpdateId = ADUC_UpdateId_AllocAndInit("example-provider", "example-name", "2.0.0.0");
        CHECK(expectedUpdateId != nullptr);

        std::string jsonString = CreateContentIdJSON(installedCriteria.c_str(), expectedUpdateId, updateType.c_str());

        JSON_Value_wrapper json{ ADUC_Json_GetRoot(jsonString.c_str()) };
        REQUIRE(json.get() != nullptr);

        ADUC_ContentData_Update(data, json.get(), false);
        CHECK_THAT(installedCriteria, Equals(data->InstalledCriteria));

        CHECK_THAT(updateType, Equals(data->UpdateType));

        CHECK_THAT(expectedUpdateId->Provider, Equals(data->ExpectedUpdateId->Provider));
        CHECK_THAT(expectedUpdateId->Name, Equals(data->ExpectedUpdateId->Name));
        CHECK_THAT(expectedUpdateId->Version, Equals(data->ExpectedUpdateId->Version));

        ADUC_UpdateId_Free(initialExpectedUpdateId);
        ADUC_UpdateId_Free(expectedUpdateId);
        ADUC_ContentData_Free(data);
    }
}
std::string
CreateContentIdJSON(const char* installedCriteria, const ADUC_UpdateId* expectedUpdateId, const char* updateType)
{
    std::stringstream jsonStm;
    jsonStm << "{";
    jsonStm << R"(")" << ADUCITF_FIELDNAME_ACTION << R"(":0,)";
    jsonStm << R"("updateManifest":"{)";
    if (expectedUpdateId != nullptr)
    {
        jsonStm << R"(\")" << ADUCITF_FIELDNAME_UPDATEID << R"(\":{)";
        jsonStm << R"(\")" << ADUCITF_FIELDNAME_PROVIDER << R"(\":\")" << expectedUpdateId->Provider << R"(\",)";
        jsonStm << R"(\")" << ADUCITF_FIELDNAME_NAME << R"(\":\")" << expectedUpdateId->Name << R"(\",)";
        jsonStm << R"(\")" << ADUCITF_FIELDNAME_VERSION << R"(\":\")" << expectedUpdateId->Version << R"(\")";
        jsonStm << R"(})";
    }
    if (updateType != nullptr && expectedUpdateId != nullptr)
    {
        jsonStm << R"(,)";
    }
    if (updateType != nullptr)
    {
        jsonStm << R"(\")" << ADUCITF_FIELDNAME_UPDATETYPE << R"(\":\")" << updateType << R"(\")";
    }
    if (installedCriteria != nullptr && updateType != nullptr)
    {
        jsonStm << R"(,)";
    }
    if (installedCriteria != nullptr)
    {
        jsonStm << R"(\")" << ADUCITF_FIELDNAME_INSTALLEDCRITERIA << R"(\":\")" << installedCriteria << R"(\")";
    }
    jsonStm << R"(}")";
    jsonStm << "}";
    return jsonStm.str();
}

ADUC_ContentData* CreateInitialContentData(
    const std::string& installedCriteria, const ADUC_UpdateId* expectedUpdateId, const std::string& updateType)
{
    std::string jsonString = CreateContentIdJSON(installedCriteria.c_str(), expectedUpdateId, updateType.c_str());
    JSON_Value_wrapper json{ ADUC_Json_GetRoot(jsonString.c_str()) };
    ADUC_ContentData* data = ADUC_ContentData_AllocAndInit(json.get());

    return data;
}
