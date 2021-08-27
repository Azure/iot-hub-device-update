/**
 * @file agent_workflow_ut.cpp
 * @brief Unit tests for the agent workflow.
 *
 * @copyright Copyright (c) Microsoft Corp.
 */
#include <catch2/catch.hpp>

#include <sstream>
#include <string>

#include "aduc/agent_workflow_utils.h"

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
