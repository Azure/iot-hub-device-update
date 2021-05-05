/**
 * @file adu_core_interface_ut.cpp
 * @brief Unit tests for adu_core_interface.h
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#include <catch2/catch.hpp>
using Catch::Matchers::Equals;
using Catch::Matchers::Matches;

#include <sstream>

#include "aduc/adu_core_export_helpers.h"
#include "aduc/adu_core_exports.h"
#include "aduc/adu_core_interface.h"
#include "aduc/agent_workflow.h"

#define ENABLE_MOCKS
#include "aduc/client_handle_helper.h"
#undef ENABLE_MOCKS

#include "umock_c/umock_c.h"

//
// Test Helpers
//

static void mockIdleCallback(ADUC_Token /*token*/, const char* /*workflowId*/)
{
}

static std::string escaped(const std::string& input)
{
    std::string output;
    output.reserve(input.size());
    for (const char c : input)
    {
        // more cases can be added here if we can to escape certain characters
        switch (c)
        {
        case '{':
            output += "\\{";
            break;
        case '}':
            output += "\\}";
            break;
        case '+':
            output += "\\+";
            break;
        case '\\':
            output += "\\\\";
            break;
        default:
            output += c;
            break;
        }
    }
    return output;
}

static ADUC_Result mockIsInstalledCallback(
    ADUC_Token /*token*/, const char* /*workflowId*/, const char* /*installedCriteria*/, const char* /*updateType*/)
{
    return ADUC_Result{ ADUC_IsInstalledResult_Installed };
}

class ADUC_UT_ReportPropertyAsyncValues
{
public:
    void
    set(ADUC_ClientHandle deviceHandleIn,
        const unsigned char* reportedStateIn,
        size_t reportedStateLenIn,
        IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallbackIn,
        void* userContextCallbackIn)
    {
        deviceHandle = deviceHandleIn;

        memcpy(reportedState, reportedStateIn, reportedStateLenIn);

        reportedStateLen = reportedStateLenIn;
        reportedStateCallback = reportedStateCallbackIn;
        userContextCallback = userContextCallbackIn;
    }

    ADUC_ClientHandle deviceHandle{ nullptr };
    unsigned char reportedState[1024]{}; /* The API treats this as a blob. */
    size_t reportedStateLen{ 0 };
    IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback{ nullptr };
    const void* userContextCallback{ nullptr };
} g_SendReportedStateValues;

static IOTHUB_CLIENT_RESULT mockClientHandle_SendReportedState(
    ADUC_ClientHandle deviceHandle,
    const unsigned char* reportedState,
    size_t reportedStateLen,
    IOTHUB_CLIENT_REPORTED_STATE_CALLBACK reportedStateCallback,
    void* userContextCallback)
{
    g_SendReportedStateValues.set(
        deviceHandle, reportedState, reportedStateLen, reportedStateCallback, userContextCallback);

    return IOTHUB_CLIENT_OK;
}

class TestCaseFixture
{
public:
    TestCaseFixture()
    {
        m_previousDeviceHandle = g_iotHubClientHandleForADUComponent;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        g_iotHubClientHandleForADUComponent = reinterpret_cast<ADUC_ClientHandle>(42);

        REGISTER_UMOCK_ALIAS_TYPE(ADUC_ClientHandle, void*);
        REGISTER_UMOCK_ALIAS_TYPE(IOTHUB_CLIENT_REPORTED_STATE_CALLBACK, void*);

        REGISTER_GLOBAL_MOCK_HOOK(ClientHandle_SendReportedState, mockClientHandle_SendReportedState);
    }

    ~TestCaseFixture()
    {
        CHECK(g_iotHubClientHandleForADUComponent != nullptr);
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
// Test Cases
//

TEST_CASE_METHOD(TestCaseFixture, "AzureDeviceUpdateCoreInterface_Create")
{
    void* context = nullptr;

    CHECK(AzureDeviceUpdateCoreInterface_Create(&context, 0 /*argc*/, nullptr /*argv*/));
    CHECK(context != nullptr);

    AzureDeviceUpdateCoreInterface_Destroy(&context);
}

TEST_CASE_METHOD(TestCaseFixture, "AzureDeviceUpdateCoreInterface_Connected")
{
    ADUC_WorkflowData workflowData{};
    // Typically Register would initialize the IdleCallback.
    workflowData.RegisterData.IdleCallback = mockIdleCallback;
    // Typically Register would initialize the IsInstalledCallback.
    workflowData.RegisterData.IsInstalledCallback = mockIsInstalledCallback;

    AzureDeviceUpdateCoreInterface_Connected(&workflowData);

    CHECK(workflowData.LastReportedState == ADUCITF_State_Idle);
    CHECK(workflowData.StartupIdleCallSent);
    CHECK(!workflowData.OperationInProgress);
    CHECK(!workflowData.OperationCancelled);
}

TEST_CASE_METHOD(TestCaseFixture, "AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync")
{
    SECTION("Download Success")
    {
        ADUC_Result result{ ADUC_DownloadResult_Success, 0 };
        const ADUCITF_State updateState = ADUCITF_State_DownloadSucceeded;
        AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync(updateState, &result);

        CHECK(g_SendReportedStateValues.deviceHandle != nullptr);
        std::stringstream strm;

#ifndef ENABLE_ADU_TELEMETRY_REPORTING
        // clang-format off
        strm << R"({)"
                << R"("azureDeviceUpdateAgent":{)"
                    << R"("__t":"c",)"
                    << R"("client":{)"
                        << R"("state":)" << static_cast<unsigned int>(updateState) << R"(,)"
                        << R"("resultCode":200,)"
                        << R"("extendedResultCode":0)"
                    << R"(})"
                << R"(})"
             << R"(})";
        // clang-format on
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        CHECK_THAT(reinterpret_cast<const char*>(g_SendReportedStateValues.reportedState), Equals(strm.str()));
        CHECK(g_SendReportedStateValues.reportedStateLen == strm.str().length());
#else
        // clang-format off
        strm << R"({)"
                << R"("azureDeviceUpdateAgent":{)"
                    << R"("__t":"c",)"
                    << R"("client":{)"
                        << R"("state":)" << static_cast<unsigned int>(updateState) << R"(,)"
                        << R"("resultCode":200,)"
                        << R"("extendedResultCode":0)"
                    << R"(})"
                << R"(})"
             << R"(})";
        // clang-format on
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        CHECK_THAT(
            reinterpret_cast<const char*>(g_SendReportedStateValues.reportedState), Matches(escaped(strm.str())));
#endif
        CHECK(g_SendReportedStateValues.reportedStateCallback != nullptr);
        CHECK(g_SendReportedStateValues.userContextCallback == nullptr);
    }

    SECTION("Failed")
    {
        ADUC_Result result{ ADUC_DownloadResult_Failure, ADUC_ERC_NOTPERMITTED };

        const ADUCITF_State updateState = ADUCITF_State_Failed;
        AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync(updateState, &result);

        CHECK(g_SendReportedStateValues.deviceHandle != nullptr);
        std::stringstream strm;

#ifndef ENABLE_ADU_TELEMETRY_REPORTING
        // clang-format off
        strm << R"({)"
                << R"("azureDeviceUpdateAgent":{)"
                    << R"("__t":"c",)"
                    << R"("client":{)"
                        << R"("state":)" << static_cast<unsigned int>(updateState) << R"(,)"
                        << R"("resultCode":500,)"
                        << R"("extendedResultCode":)"<< ADUC_ERC_NOTPERMITTED
                    << R"(})"
                << R"(})"
             << R"(})";
        // clang-format on
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        CHECK_THAT(reinterpret_cast<const char*>(g_SendReportedStateValues.reportedState), Equals(strm.str()));
        CHECK(g_SendReportedStateValues.reportedStateLen == strm.str().length());
#else
        // clang-format off
        strm << R"({)"
                << R"("azureDeviceUpdateAgent":{)"
                    << R"("__t":"c",)"
                    << R"("client":{)"
                        << R"("state":)" << static_cast<unsigned int>(updateState) << R"(,)"
                        << R"("resultCode":500,)"
                        << R"("extendedResultCode":)"<< ADUC_ERC_NOTPERMITTED
                    << R"(})"
                << R"(})"
             << R"(})";
        // clang-format on
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        CHECK_THAT(
            reinterpret_cast<const char*>(g_SendReportedStateValues.reportedState), Matches(escaped(strm.str())));
#endif
        CHECK(g_SendReportedStateValues.reportedStateCallback != nullptr);
        CHECK(g_SendReportedStateValues.userContextCallback == nullptr);
    }
}

TEST_CASE_METHOD(TestCaseFixture, "AzureDeviceUpdateCoreInterface_ReportContentIdAndIdleAsync")
{
    const std::string provider{ "Microsoft" };
    const std::string name{ "adu" };
    const std::string version{ "1.2.3.4" };

    ADUC_UpdateId* updateId = ADUC_UpdateId_AllocAndInit(provider.c_str(), name.c_str(), version.c_str());
    CHECK(updateId != nullptr);

    AzureDeviceUpdateCoreInterface_ReportUpdateIdAndIdleAsync(updateId);

    CHECK(g_SendReportedStateValues.deviceHandle != nullptr);
    std::stringstream installedUpdateIdStr;

    // clang-format off
    installedUpdateIdStr
                    << R"({)"
                        << R"(\"provider\":\")" << updateId->Provider << R"(\",)"
                        << R"(\"name\":\")"<< updateId->Name << R"(\",)"
                        << R"(\"version\":\")" << updateId->Version << R"(\")"
                    << R"(})";
    // clang-format on

    std::stringstream strm;
    // clang-format off
    strm << R"({)" 
            << R"("azureDeviceUpdateAgent":{)"
                << R"("__t":"c",)"
                << R"("client":{)"
                    << R"("installedUpdateId":")" << installedUpdateIdStr.str() << R"(",)"
                    << R"("state":)" << static_cast<unsigned int>(ADUCITF_State_Idle) << R"(,)"
                    << R"("resultCode":200,"extendedResultCode":0)"
                << R"(})"
            << R"(})"
         << R"(})";
    // clang-format on
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    CHECK_THAT(reinterpret_cast<const char*>(g_SendReportedStateValues.reportedState), Equals(strm.str()));
    CHECK(g_SendReportedStateValues.reportedStateLen == strm.str().length());
    CHECK(g_SendReportedStateValues.reportedStateCallback != nullptr);
    CHECK(g_SendReportedStateValues.userContextCallback == nullptr);

    ADUC_UpdateId_Free(updateId);
}
