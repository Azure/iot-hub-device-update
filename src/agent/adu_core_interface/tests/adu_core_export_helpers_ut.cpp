/**
 * @file adu_core_export_helpers_ut.cpp
 * @brief Unit tests for adu_core_export_helpers.h
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#include <iterator>
#include <map>
#include <sstream>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <thread>

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include "aduc/adu_core_export_helpers.h"
#include "agent_workflow_utils.h"

#include <aduc/c_utils.h>
#include <iothub_device_client.h>

//
// Test Helpers
//

static uint8_t FacilityFromExtendedResultCode(ADUC_Result_t extendedResultCode)
{
    return (static_cast<uint32_t>(extendedResultCode) >> 0x1C) & 0xF;
}

static uint32_t CodeFromExtendedResultCode(ADUC_Result_t extendedResultCode)
{
    return extendedResultCode & 0xFFFFFFF;
}

// Needs to be a define as INFO is method scope specific.
// Need to cast to uint16_t as catch2 doesn't have conversion for uint8_t.
#define INFO_ADUC_Result(result)                                                                               \
    INFO(                                                                                                      \
        "Code: " << (result).ResultCode << "; Extended: { 0x" << std::hex                                      \
                 << static_cast<uint16_t>(FacilityFromExtendedResultCode((result).ExtendedResultCode)) << ", " \
                 << CodeFromExtendedResultCode((result).ExtendedResultCode) << " }")

std::condition_variable workCompletionCallbackCV;

#define ADUC_ClientHandle_Invalid (-1)

extern "C"
{
    static void DownloadProgressCallback(
        const char* /*workflowId*/,
        const char* /*fileId*/,
        ADUC_DownloadProgressState /*state*/,
        uint64_t /*bytesTransferred*/,
        uint64_t /*bytesTotal*/)
    {
    }

    static void WorkCompletionCallback(const void* /*workCompletionToken*/, ADUC_Result result)
    {
        CHECK(IsAducResultCodeSuccess(result.ResultCode));
        CHECK(result.ExtendedResultCode == 0);

        workCompletionCallbackCV.notify_all();
    }
}

std::string CreateDownloadUpdateActionJson(const std::map<std::string, std::string>& files)
{
    std::stringstream main_json_strm;
    std::stringstream url_json_strm;

    main_json_strm << R"({)"
                   << R"("action":0,)"
                   << R"("updateManifest":"{)"
                   << R"(\"updateId\":{)"
                   << R"(\"provider\": \"Azure\",)"
                   << R"(\"name\": \"IOT-Firmware\",)"
                   << R"(\"version\": \"1.2.0.0\")"
                   << R"(},)"
                   << R"(\"updateType\": \"microsoft/swupdate:1\",)"
                   << R"(\"installedCriteria\": \"1.0.2903.1\",)"
                   << R"(\"files\":{)";

    url_json_strm << R"("fileUrls": {)";

    // clang-format on

    int i = 0;
    for (auto it = files.begin(); it != files.end(); ++it)
    {
        main_json_strm << R"(\")" << i << R"(\":{)";
        main_json_strm << R"(\"fileName\":\"file)" << i << R"(.txt\",)";
        main_json_strm << R"(\"hashes\": {\"sha256\": \")" << it->first << R"(\"})";
        main_json_strm << R"(})";

        url_json_strm << R"(")" << i << R"(":")" << it->second << R"(")";

        if (it != (--files.end()))
        {
            main_json_strm << ",";
            url_json_strm << ",";
        }

        ++i;
    }

    url_json_strm << R"(})";
    main_json_strm << R"(}}",)" << url_json_strm.str() << R"(})";

    return main_json_strm.str();
}

class TestCaseFixture
{
public:
    TestCaseFixture()
    {
        m_previousDeviceHandle = g_iotHubClientHandleForADUComponent;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        g_iotHubClientHandleForADUComponent = reinterpret_cast<ADUC_ClientHandle>(ADUC_ClientHandle_Invalid);
    }

    ~TestCaseFixture()
    {
        g_iotHubClientHandleForADUComponent = m_previousDeviceHandle;
    }

    TestCaseFixture(const TestCaseFixture&) = delete;
    TestCaseFixture& operator=(const TestCaseFixture&) = delete;
    TestCaseFixture(TestCaseFixture&&) = delete;
    TestCaseFixture& operator=(TestCaseFixture&&) = delete;

private:
    ADUC_ClientHandle m_previousDeviceHandle;
};

//
// Test cases
//

TEST_CASE("ADUC_PrepareInfo_Init")
{
    ADUC_WorkflowData workflowData{};

    const std::string workflowId{ "unit_test" };
    constexpr size_t workflowIdSize = ARRAY_SIZE(workflowData.WorkflowId);
    strncpy(workflowData.WorkflowId, workflowId.c_str(), workflowIdSize);
    workflowData.WorkflowId[workflowIdSize - 1] = '\0';

    workflowData.LastReportedState = ADUCITF_State_Idle;
    workflowData.DownloadProgressCallback = DownloadProgressCallback;

    ADUC_Result result;

    result = ADUC_MethodCall_Register(&workflowData.RegisterData, 0 /*argc*/, nullptr /*argv*/);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    REQUIRE(result.ExtendedResultCode == 0);

    // clang-format off
    const std::map<std::string, std::string> files{
        { "E8sbKCW3L/ALBTYE3umfzXMLel8Pk3TUCixceIAtnDo=", "https://example.com/tf01228997.accdt" },
        { "Ilo1MnAkIDyIWjzSN9BaqHvzKetFhItdKQILFCOIuus=", "https://example.com/tf01225343.accdt" }
    };
    // clang-format on

    std::string updateActionJson{ CreateDownloadUpdateActionJson(files) };

    workflowData.UpdateActionJson = ADUC_Json_GetRoot(updateActionJson.c_str());
    REQUIRE(workflowData.UpdateActionJson != nullptr);
    workflowData.ContentData = ADUC_ContentData_AllocAndInit(workflowData.UpdateActionJson);

    ADUC_PrepareInfo info{};
    CHECK(ADUC_PrepareInfo_Init(&info, &workflowData));
    CHECK(info.fileCount == files.size());
    for (unsigned int i = 0; i < info.fileCount; ++i)
    {
        std::stringstream strm;
        strm << "aduc-tempfile-" << i;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        CHECK_THAT(info.files[i].DownloadUri, Equals(files.find(info.files[i].Hash[0].value)->second));
    }
    CHECK_THAT(info.updateType, Equals("microsoft/swupdate:1"));

    ADUC_PrepareInfo_UnInit(&info);

    REQUIRE(info.updateType == NULL);
    REQUIRE(info.updateTypeName == NULL);
    REQUIRE(info.updateTypeVersion == 0);
    REQUIRE(info.files == NULL);
    REQUIRE(info.fileCount == 0);

    ADUC_MethodCall_Unregister(&workflowData.RegisterData);
}

TEST_CASE("ADUC_DownloadInfo_Init")
{
    ADUC_DownloadInfo info{};
    // clang-format off
    const std::map<std::string, std::string> files{
        { "E8sbKCW3L/ALBTYE3umfzXMLel8Pk3TUCixceIAtnDo=", "https://example.com/tf01228997.accdt" },
        { "Ilo1MnAkIDyIWjzSN9BaqHvzKetFhItdKQILFCOIuus=", "https://example.com/tf01225343.accdt" }
    };
    // clang-format on

    const std::string updateAction{ CreateDownloadUpdateActionJson(files) };

    INFO(updateAction);

    const JSON_Value* updateActionJson{ ADUC_Json_GetRoot(updateAction.c_str()) };
    REQUIRE(updateActionJson != nullptr);

    const std::string workFolder{ "/tmp" };

    CHECK(ADUC_DownloadInfo_Init(&info, updateActionJson, workFolder.c_str(), DownloadProgressCallback));

    CHECK_THAT(info.WorkFolder, Equals(workFolder));
    CHECK(info.FileCount == files.size());
    for (unsigned int i = 0; i < info.FileCount; ++i)
    {
        std::stringstream strm;
        strm << "aduc-tempfile-" << i;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        CHECK_THAT(info.Files[i].DownloadUri, Equals(files.find(info.Files[i].Hash[0].value)->second));
    }
    CHECK(info.NotifyDownloadProgress == DownloadProgressCallback);

    ADUC_DownloadInfo_UnInit(&info);
}

TEST_CASE("ADUC_InstallInfo_Init")
{
    ADUC_InstallInfo info{};
    const std::string workFolder{ "/tmp" };

    CHECK(ADUC_InstallInfo_Init(&info, workFolder.c_str()));

    CHECK_THAT(info.WorkFolder, Equals(workFolder));

    ADUC_InstallInfo_UnInit(&info);
}

TEST_CASE("ADUC_ApplyInfo_Init")
{
    ADUC_ApplyInfo info{};
    const std::string workFolder{ "/tmp" };

    CHECK(ADUC_ApplyInfo_Init(&info, workFolder.c_str()));

    CHECK_THAT(info.WorkFolder, Equals(workFolder));

    ADUC_ApplyInfo_UnInit(&info);
}

TEST_CASE("ADUC_MethodCall_Register and Unregister: Valid")
{
    ADUC_RegisterData registerData{};
    const ADUC_Result result{ ADUC_MethodCall_Register(&registerData, 0 /*argc*/, nullptr /*argv*/) };
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    REQUIRE(result.ExtendedResultCode == 0);

    ADUC_MethodCall_Unregister(&registerData);
}

TEST_CASE("ADUC_MethodCall_Register and Unregister: Invalid")
{
    SECTION("Invalid args passed to register returns failure")
    {
        ADUC_RegisterData registerData{};
        const char* argv[1] = {};
        const int argc = std::distance(std::begin(argv), std::end(argv));
        const ADUC_Result result{ ADUC_MethodCall_Register(&registerData, argc, argv) };
        INFO_ADUC_Result(result);
        CHECK(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode == ADUC_ERC_NOTRECOVERABLE);
    }
}

TEST_CASE_METHOD(TestCaseFixture, "MethodCall workflow: Valid")
{
    std::mutex workCompletionCallbackMTX;
    std::unique_lock<std::mutex> lock(workCompletionCallbackMTX);

    //
    // Register
    //

    ADUC_WorkflowData workflowData{};

    workflowData.LastReportedState = ADUCITF_State_Idle;
    workflowData.DownloadProgressCallback = DownloadProgressCallback;

    ADUC_Result result;

    result = ADUC_MethodCall_Register(&workflowData.RegisterData, 0 /*argc*/, nullptr /*argv*/);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    REQUIRE(result.ExtendedResultCode == 0);

    //
    // Download
    //

    // clang-format off
    const std::map<std::string, std::string> files{
        { "E8sbKCW3L/ALBTYE3umfzXMLel8Pk3TUCixceIAtnDo=", "https://example.com/tf01228997.accdt" },
        { "Ilo1MnAkIDyIWjzSN9BaqHvzKetFhItdKQILFCOIuus=", "https://example.com/tf01225343.accdt" }
    };
    // clang-format on

    std::string downloadJson{ CreateDownloadUpdateActionJson(files) };

    workflowData.UpdateActionJson = ADUC_Json_GetRoot(downloadJson.c_str());
    REQUIRE(workflowData.UpdateActionJson != nullptr);
    workflowData.ContentData = ADUC_ContentData_AllocAndInit(workflowData.UpdateActionJson);

    // ADUC_MethodCall_Data must be on the heap, as the download callback uses it asynchronously.
    ADUC_MethodCall_Data methodCallData{};
    // NOLINTNEXTLNE(cppcoreguidelines-pro-type-union-access)
    methodCallData.WorkflowData = &workflowData;
    methodCallData.WorkCompletionData.WorkCompletionCallback = WorkCompletionCallback;

    ADUC_DownloadInfo downloadInfo{};
    ADUC_DownloadInfo_Init(&downloadInfo, workflowData.UpdateActionJson, "/tmp", DownloadProgressCallback);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    methodCallData.MethodSpecificData.DownloadInfo = &downloadInfo;

    workflowData.CurrentAction = ADUCITF_UpdateAction_Download;

    const std::string workflowId{ "unit_test" };
    constexpr size_t workflowIdSize = ARRAY_SIZE(workflowData.WorkflowId);
    strncpy(workflowData.WorkflowId, workflowId.c_str(), workflowIdSize);
    workflowData.WorkflowId[workflowIdSize - 1] = '\0';

    result = ADUC_MethodCall_Download(&methodCallData);

    CHECK(result.ResultCode == ADUC_DownloadResult_InProgress);
    CHECK(result.ExtendedResultCode == 0);

    CHECK(workflowData.LastReportedState == ADUCITF_State_DownloadStarted);
    CHECK(workflowData.IsRegistered == false);
    CHECK(workflowData.OperationInProgress == false);
    CHECK(workflowData.OperationCancelled == false);

    // Wait for async operation completion
    workCompletionCallbackCV.wait(lock);

    ADUC_MethodCall_Download_Complete(&methodCallData, result);

    //
    // Install
    //

    workflowData.LastReportedState = ADUCITF_State_DownloadSucceeded;
    workflowData.CurrentAction = ADUCITF_UpdateAction_Install;

    result = ADUC_MethodCall_Install(&methodCallData);

    CHECK(result.ResultCode == ADUC_InstallResult_InProgress);
    CHECK(result.ExtendedResultCode == 0);

    CHECK(workflowData.LastReportedState == ADUCITF_State_InstallStarted);
    CHECK(workflowData.IsRegistered == false);
    CHECK(workflowData.OperationInProgress == false);
    CHECK(workflowData.OperationCancelled == false);

    // Wait for async operation completion
    workCompletionCallbackCV.wait(lock);

    ADUC_MethodCall_Install_Complete(&methodCallData, result);

    //
    // Apply
    //

    workflowData.LastReportedState = ADUCITF_State_InstallSucceeded;
    workflowData.CurrentAction = ADUCITF_UpdateAction_Apply;

    result = ADUC_MethodCall_Apply(&methodCallData);

    CHECK(result.ResultCode == ADUC_InstallResult_InProgress);
    CHECK(result.ExtendedResultCode == 0);

    CHECK(workflowData.LastReportedState == ADUCITF_State_ApplyStarted);
    CHECK(workflowData.IsRegistered == false);
    CHECK(workflowData.OperationInProgress == false);
    CHECK(workflowData.OperationCancelled == false);

    // Wait for async operation completion
    workCompletionCallbackCV.wait(lock);

    ADUC_MethodCall_Apply_Complete(&methodCallData, result);

    //
    // Unregister
    //

    ADUC_MethodCall_Unregister(&workflowData.RegisterData);
}
