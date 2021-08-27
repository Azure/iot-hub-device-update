/**
 * @file adu_core_export_helpers_ut.cpp
 * @brief Unit tests for adu_core_export_helpers.h
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#include <atomic>
#include <catch2/catch.hpp>
#include <chrono>
#include <condition_variable>
#include <iothub_device_client.h>
#include <iterator>
#include <map>
#include <sstream>
#include <thread>

#include "aduc/adu_core_export_helpers.h"
#include "aduc/agent_workflow_utils.h"
#include "aduc/c_utils.h"
#include "aduc/workflow_utils.h"

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

std::string CreateSWUpdateDownloadUpdateActionJson(const std::map<std::string, std::string>& files)
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

TEST_CASE("ADUC_MethodCall_Register and Unregister: Valid")
{
    ADUC_UpdateActionCallbacks updateActionCallbacks{};
    const ADUC_Result result{ ADUC_MethodCall_Register(&updateActionCallbacks, 0 /*argc*/, nullptr /*argv*/) };
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    REQUIRE(result.ExtendedResultCode == 0);

    ADUC_MethodCall_Unregister(&updateActionCallbacks);
}

TEST_CASE("ADUC_MethodCall_Register and Unregister: Invalid")
{
    SECTION("Invalid args passed to register returns failure")
    {
        ADUC_UpdateActionCallbacks updateActionCallbacks{};
        const char* argv[1] = {};
        const int argc = std::distance(std::begin(argv), std::end(argv));
        const ADUC_Result result{ ADUC_MethodCall_Register(&updateActionCallbacks, argc, argv) };
        INFO_ADUC_Result(result);
        CHECK(IsAducResultCodeFailure(result.ResultCode));
        CHECK(result.ExtendedResultCode == ADUC_ERC_NOTRECOVERABLE);
    }
}

// clang-format off
const char* workflow_test_download = 
R"( {                    )"
R"(     "workflow": {    )"
R"(         "action": 0, )"
R"(         "id": "action_bundle" )"
R"(      },  )"
R"(     "updateManifest": "{\"manifestVersion\":\"2.0\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00000\":{\"fileName\":\"contoso-motor-1.0-updatemanifest.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
R"(     "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pY2toV1FrVkdTMUl4ZG5Ob1p5dEJhRWxuTDFORVVVOHplRFJyYWpORFZWUTNaa2R1U21oQmJYVkVhSFpJWm1velowaDZhVEJVTWtsQmNVTXhlREpDUTFka1QyODFkamgwZFcxeFVtb3ZibGx3WnprM2FtcFFRMHQxWTJSUE5tMHpOMlJqVDIxaE5EWm9OMDh3YTBod2Qwd3pibFZJUjBWeVNqVkVRUzloY0ZsdWQwVmxjMlY0VkdwVU9GTndMeXRpVkhGWFJXMTZaMFF6TjNCbVpFdGhjV3AwU0V4SFZtbFpkMVpJVUhwMFFtRmlkM2RxYUVGMmVubFNXUzk1T1U5bWJYcEVabGh0Y2xreGNtOHZLekpvUlhGRmVXdDFhbmRSUlZscmFHcEtZU3RDTkRjMkt6QnRkVWQ1VjBrMVpVbDJMMjlzZERKU1pWaDRUV0k1VFd4c1dFNTViMUF6WVU1TFNVcHBZbHBOY3pkMVMyTnBkMnQ1YVZWSllWbGpUV3B6T1drdlVrVjVLMnhOT1haSlduRnlabkJEVlZoMU0zUnVNVXRuWXpKUmN5OVVaRGgwVGxSRFIxWTJkM1JXWVhGcFNYQlVaRlEwVW5KRFpFMXZUelZUVG1WbVprUjVZekpzUXpkMU9EVXJiMjFVYTJOcVVHcHRObVpoY0dSSmVVWXljV1Z0ZGxOQ1JHWkNOMk5oYWpWRVNVa3lOVmQzTlVWS1kyRjJabmxRTlRSdGNVNVJVVE5IWTAxUllqSmtaMmhwWTJ4d2FsbHZLelF6V21kWlEyUkhkR0ZhWkRKRlpreGFkMGd6VVdjeWNrUnNabXN2YVdFd0x6RjVjV2xyTDFoYU1XNXpXbFJwTUVKak5VTndUMDFGY1daT1NrWlJhek5DVjI5Qk1EVnlRMW9pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJd01EY3dNaTVTTGxNaWZRLmlTVGdBRUJYc2Q3QUFOa1FNa2FHLUZBVjZRT0dVRXV4dUhnMllmU3VXaHRZWHFicE0takk1UlZMS2VzU0xDZWhLLWxSQzl4Ni1fTGV5eE5oMURPRmMtRmE2b0NFR3dVajh6aU9GX0FUNnM2RU9tY2txUHJ4dXZDV3R5WWtrRFJGNzRkdGFLMWpOQTdTZFhyWnp2V0NzTXFPVU1OejBnQ29WUjBDczEyNTRrRk1SbVJQVmZFY2pnVDdqNGxDcHlEdVdncjlTZW5TZXFnS0xZeGphYUcwc1JoOWNkaTJkS3J3Z2FOYXFBYkhtQ3JyaHhTUENUQnpXTUV4WnJMWXp1ZEVvZnlZSGlWVlJoU0pwajBPUTE4ZWN1NERQWFYxVGN0MXkzazdMTGlvN244aXpLdXEybTNUeEY5dlBkcWI5TlA2U2M5LW15YXB0cGJGcEhlRmtVTC1GNXl0bF9VQkZLcHdOOUNMNHdwNnlaLWpkWE5hZ3JtVV9xTDFDeVh3MW9tTkNnVG1KRjNHZDNseXFLSEhEZXJEcy1NUnBtS2p3U3dwWkNRSkdEUmNSb3ZXeUwxMnZqdzNMQkpNaG1VeHNFZEJhWlA1d0dkc2ZEOGxkS1lGVkZFY1owb3JNTnJVa1NNQWw2cEl4dGVmRVhpeTVscW1pUHpxX0xKMWVSSXJxWTBfIn0.eyJzaGEyNTYiOiI3alo1YWpFN2Z5SWpzcTlBbWlKNmlaQlNxYUw1bkUxNXZkL0puVWgwNFhZPSJ9.EK5zcNiEgO2rHh_ichQWlDIvkIsPXrPMQK-0D5WK8ZnOR5oJdwhwhdpgBaB-tE-6QxQB1PKurbC2BtiGL8HI1DgQtL8Fq_2ASRfzgNtrtpp6rBiLRynJuWCy7drgM6g8WoSh8Utdxsx5lnGgAVAU67ijK0ITd0E70R7vWJRmY8YxxDh-Sh8BNz68pvU-YJQwKtVy64lD5zA0--BL432F-uZWTc6n-BduQdSB4J7Eu6zGlT75s8Ehd-SIylsstu4wdypU0tcwIH-MaSKcH5mgEmokaHncJrb4zKnZwxYQUeDMoFjF39P9hDmheHywY1gwYziXjUcnMn8_T00oMeycQ7PDCTJHIYB3PGbtM9KiA3RQH-08ofqiCVgOLeqbUHTP03Z0Cx3e02LzTgP8_Lerr4okAUPksT2IGvvsiMtj04asdrLSlv-AvFud-9U0a2mJEWcosI04Q5NAbqhZ5ZBzCkkowLGofS04SnfS-VssBfmbH5ue5SWb-AxBv1inZWUj", )"
R"(     "fileUrls": {   )"
R"(         "00000": "file:///tmp/tests/testfiles/contoso-motor-1.0-updatemanifest.json",  )"
R"(         "00001": "file:///tmp/tests/testfiles/contoso-motor-1.0-fileinstaller",     )"
R"(         "gw001": "file:///tmp/tests/testfiles/behind-gateway-info.json" )"
R"(     } )"
R"( } )";

const char* workflow_test_install = 
R"( {                    )"
R"(     "workflow": {    )"
R"(         "action": 1, )"
R"(         "id": "action_bundle" )"
R"(      },  )"
R"(     "updateManifest": "{\"manifestVersion\":\"2.0\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00000\":{\"fileName\":\"contoso-motor-1.0-updatemanifest.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
R"(     "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pY2toV1FrVkdTMUl4ZG5Ob1p5dEJhRWxuTDFORVVVOHplRFJyYWpORFZWUTNaa2R1U21oQmJYVkVhSFpJWm1velowaDZhVEJVTWtsQmNVTXhlREpDUTFka1QyODFkamgwZFcxeFVtb3ZibGx3WnprM2FtcFFRMHQxWTJSUE5tMHpOMlJqVDIxaE5EWm9OMDh3YTBod2Qwd3pibFZJUjBWeVNqVkVRUzloY0ZsdWQwVmxjMlY0VkdwVU9GTndMeXRpVkhGWFJXMTZaMFF6TjNCbVpFdGhjV3AwU0V4SFZtbFpkMVpJVUhwMFFtRmlkM2RxYUVGMmVubFNXUzk1T1U5bWJYcEVabGh0Y2xreGNtOHZLekpvUlhGRmVXdDFhbmRSUlZscmFHcEtZU3RDTkRjMkt6QnRkVWQ1VjBrMVpVbDJMMjlzZERKU1pWaDRUV0k1VFd4c1dFNTViMUF6WVU1TFNVcHBZbHBOY3pkMVMyTnBkMnQ1YVZWSllWbGpUV3B6T1drdlVrVjVLMnhOT1haSlduRnlabkJEVlZoMU0zUnVNVXRuWXpKUmN5OVVaRGgwVGxSRFIxWTJkM1JXWVhGcFNYQlVaRlEwVW5KRFpFMXZUelZUVG1WbVprUjVZekpzUXpkMU9EVXJiMjFVYTJOcVVHcHRObVpoY0dSSmVVWXljV1Z0ZGxOQ1JHWkNOMk5oYWpWRVNVa3lOVmQzTlVWS1kyRjJabmxRTlRSdGNVNVJVVE5IWTAxUllqSmtaMmhwWTJ4d2FsbHZLelF6V21kWlEyUkhkR0ZhWkRKRlpreGFkMGd6VVdjeWNrUnNabXN2YVdFd0x6RjVjV2xyTDFoYU1XNXpXbFJwTUVKak5VTndUMDFGY1daT1NrWlJhek5DVjI5Qk1EVnlRMW9pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJd01EY3dNaTVTTGxNaWZRLmlTVGdBRUJYc2Q3QUFOa1FNa2FHLUZBVjZRT0dVRXV4dUhnMllmU3VXaHRZWHFicE0takk1UlZMS2VzU0xDZWhLLWxSQzl4Ni1fTGV5eE5oMURPRmMtRmE2b0NFR3dVajh6aU9GX0FUNnM2RU9tY2txUHJ4dXZDV3R5WWtrRFJGNzRkdGFLMWpOQTdTZFhyWnp2V0NzTXFPVU1OejBnQ29WUjBDczEyNTRrRk1SbVJQVmZFY2pnVDdqNGxDcHlEdVdncjlTZW5TZXFnS0xZeGphYUcwc1JoOWNkaTJkS3J3Z2FOYXFBYkhtQ3JyaHhTUENUQnpXTUV4WnJMWXp1ZEVvZnlZSGlWVlJoU0pwajBPUTE4ZWN1NERQWFYxVGN0MXkzazdMTGlvN244aXpLdXEybTNUeEY5dlBkcWI5TlA2U2M5LW15YXB0cGJGcEhlRmtVTC1GNXl0bF9VQkZLcHdOOUNMNHdwNnlaLWpkWE5hZ3JtVV9xTDFDeVh3MW9tTkNnVG1KRjNHZDNseXFLSEhEZXJEcy1NUnBtS2p3U3dwWkNRSkdEUmNSb3ZXeUwxMnZqdzNMQkpNaG1VeHNFZEJhWlA1d0dkc2ZEOGxkS1lGVkZFY1owb3JNTnJVa1NNQWw2cEl4dGVmRVhpeTVscW1pUHpxX0xKMWVSSXJxWTBfIn0.eyJzaGEyNTYiOiI3alo1YWpFN2Z5SWpzcTlBbWlKNmlaQlNxYUw1bkUxNXZkL0puVWgwNFhZPSJ9.EK5zcNiEgO2rHh_ichQWlDIvkIsPXrPMQK-0D5WK8ZnOR5oJdwhwhdpgBaB-tE-6QxQB1PKurbC2BtiGL8HI1DgQtL8Fq_2ASRfzgNtrtpp6rBiLRynJuWCy7drgM6g8WoSh8Utdxsx5lnGgAVAU67ijK0ITd0E70R7vWJRmY8YxxDh-Sh8BNz68pvU-YJQwKtVy64lD5zA0--BL432F-uZWTc6n-BduQdSB4J7Eu6zGlT75s8Ehd-SIylsstu4wdypU0tcwIH-MaSKcH5mgEmokaHncJrb4zKnZwxYQUeDMoFjF39P9hDmheHywY1gwYziXjUcnMn8_T00oMeycQ7PDCTJHIYB3PGbtM9KiA3RQH-08ofqiCVgOLeqbUHTP03Z0Cx3e02LzTgP8_Lerr4okAUPksT2IGvvsiMtj04asdrLSlv-AvFud-9U0a2mJEWcosI04Q5NAbqhZ5ZBzCkkowLGofS04SnfS-VssBfmbH5ue5SWb-AxBv1inZWUj" )"
R"( } )";

const char* workflow_test_apply = 
R"( {                    )"
R"(     "workflow": {    )"
R"(         "action": 2, )"
R"(         "id": "action_bundle" )"
R"(      },  )"
R"(     "updateManifest": "{\"manifestVersion\":\"2.0\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00000\":{\"fileName\":\"contoso-motor-1.0-updatemanifest.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
R"(     "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pY2toV1FrVkdTMUl4ZG5Ob1p5dEJhRWxuTDFORVVVOHplRFJyYWpORFZWUTNaa2R1U21oQmJYVkVhSFpJWm1velowaDZhVEJVTWtsQmNVTXhlREpDUTFka1QyODFkamgwZFcxeFVtb3ZibGx3WnprM2FtcFFRMHQxWTJSUE5tMHpOMlJqVDIxaE5EWm9OMDh3YTBod2Qwd3pibFZJUjBWeVNqVkVRUzloY0ZsdWQwVmxjMlY0VkdwVU9GTndMeXRpVkhGWFJXMTZaMFF6TjNCbVpFdGhjV3AwU0V4SFZtbFpkMVpJVUhwMFFtRmlkM2RxYUVGMmVubFNXUzk1T1U5bWJYcEVabGh0Y2xreGNtOHZLekpvUlhGRmVXdDFhbmRSUlZscmFHcEtZU3RDTkRjMkt6QnRkVWQ1VjBrMVpVbDJMMjlzZERKU1pWaDRUV0k1VFd4c1dFNTViMUF6WVU1TFNVcHBZbHBOY3pkMVMyTnBkMnQ1YVZWSllWbGpUV3B6T1drdlVrVjVLMnhOT1haSlduRnlabkJEVlZoMU0zUnVNVXRuWXpKUmN5OVVaRGgwVGxSRFIxWTJkM1JXWVhGcFNYQlVaRlEwVW5KRFpFMXZUelZUVG1WbVprUjVZekpzUXpkMU9EVXJiMjFVYTJOcVVHcHRObVpoY0dSSmVVWXljV1Z0ZGxOQ1JHWkNOMk5oYWpWRVNVa3lOVmQzTlVWS1kyRjJabmxRTlRSdGNVNVJVVE5IWTAxUllqSmtaMmhwWTJ4d2FsbHZLelF6V21kWlEyUkhkR0ZhWkRKRlpreGFkMGd6VVdjeWNrUnNabXN2YVdFd0x6RjVjV2xyTDFoYU1XNXpXbFJwTUVKak5VTndUMDFGY1daT1NrWlJhek5DVjI5Qk1EVnlRMW9pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJd01EY3dNaTVTTGxNaWZRLmlTVGdBRUJYc2Q3QUFOa1FNa2FHLUZBVjZRT0dVRXV4dUhnMllmU3VXaHRZWHFicE0takk1UlZMS2VzU0xDZWhLLWxSQzl4Ni1fTGV5eE5oMURPRmMtRmE2b0NFR3dVajh6aU9GX0FUNnM2RU9tY2txUHJ4dXZDV3R5WWtrRFJGNzRkdGFLMWpOQTdTZFhyWnp2V0NzTXFPVU1OejBnQ29WUjBDczEyNTRrRk1SbVJQVmZFY2pnVDdqNGxDcHlEdVdncjlTZW5TZXFnS0xZeGphYUcwc1JoOWNkaTJkS3J3Z2FOYXFBYkhtQ3JyaHhTUENUQnpXTUV4WnJMWXp1ZEVvZnlZSGlWVlJoU0pwajBPUTE4ZWN1NERQWFYxVGN0MXkzazdMTGlvN244aXpLdXEybTNUeEY5dlBkcWI5TlA2U2M5LW15YXB0cGJGcEhlRmtVTC1GNXl0bF9VQkZLcHdOOUNMNHdwNnlaLWpkWE5hZ3JtVV9xTDFDeVh3MW9tTkNnVG1KRjNHZDNseXFLSEhEZXJEcy1NUnBtS2p3U3dwWkNRSkdEUmNSb3ZXeUwxMnZqdzNMQkpNaG1VeHNFZEJhWlA1d0dkc2ZEOGxkS1lGVkZFY1owb3JNTnJVa1NNQWw2cEl4dGVmRVhpeTVscW1pUHpxX0xKMWVSSXJxWTBfIn0.eyJzaGEyNTYiOiI3alo1YWpFN2Z5SWpzcTlBbWlKNmlaQlNxYUw1bkUxNXZkL0puVWgwNFhZPSJ9.EK5zcNiEgO2rHh_ichQWlDIvkIsPXrPMQK-0D5WK8ZnOR5oJdwhwhdpgBaB-tE-6QxQB1PKurbC2BtiGL8HI1DgQtL8Fq_2ASRfzgNtrtpp6rBiLRynJuWCy7drgM6g8WoSh8Utdxsx5lnGgAVAU67ijK0ITd0E70R7vWJRmY8YxxDh-Sh8BNz68pvU-YJQwKtVy64lD5zA0--BL432F-uZWTc6n-BduQdSB4J7Eu6zGlT75s8Ehd-SIylsstu4wdypU0tcwIH-MaSKcH5mgEmokaHncJrb4zKnZwxYQUeDMoFjF39P9hDmheHywY1gwYziXjUcnMn8_T00oMeycQ7PDCTJHIYB3PGbtM9KiA3RQH-08ofqiCVgOLeqbUHTP03Z0Cx3e02LzTgP8_Lerr4okAUPksT2IGvvsiMtj04asdrLSlv-AvFud-9U0a2mJEWcosI04Q5NAbqhZ5ZBzCkkowLGofS04SnfS-VssBfmbH5ue5SWb-AxBv1inZWUj" )"
R"( } )";

TEST_CASE_METHOD(TestCaseFixture, "MethodCall workflow: Valid")
{
    std::mutex workCompletionCallbackMTX;
    std::unique_lock<std::mutex> lock(workCompletionCallbackMTX);

    //
    // Register
    //

    ADUC_WorkflowData workflowData{};

    ADUC_Result result =
        ADUC_MethodCall_Register(&(workflowData.UpdateActionCallbacks), 0 /*argc*/, nullptr /*argv*/);

    workflowData.DownloadProgressCallback = DownloadProgressCallback;

    workflow_set_last_reported_state(ADUCITF_State_Idle);

    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    REQUIRE(result.ExtendedResultCode == 0);

    //
    // Download
    //

    ADUC_WorkflowHandle handle = nullptr;
    result = workflow_init(workflow_test_download, true, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    workflowData.WorkflowHandle = handle;

    // ADUC_MethodCall_Data must be on the heap, as the download callback uses it asynchronously.
    ADUC_MethodCall_Data methodCallData{};
    // NOLINTNEXTLNE(cppcoreguidelines-pro-type-union-access)
    methodCallData.WorkflowData = &workflowData;
    methodCallData.WorkCompletionData.WorkCompletionCallback = WorkCompletionCallback;

    workflowData.CurrentAction = ADUCITF_UpdateAction_Download;

    result = ADUC_MethodCall_Download(&methodCallData);

    CHECK(result.ResultCode == ADUC_Result_Download_InProgress);
    CHECK(result.ExtendedResultCode == 0);

    CHECK(workflow_get_last_reported_state() == ADUCITF_State_DownloadStarted);
    CHECK(workflowData.IsRegistered == false);
    CHECK(workflow_get_operation_in_progress(workflowData.WorkflowHandle) == false);
    CHECK(workflow_get_operation_cancel_requested(workflowData.WorkflowHandle) == false);

    // Wait for async operation completion
    workCompletionCallbackCV.wait(lock);

    ADUC_MethodCall_Download_Complete(&methodCallData, result);

    //
    // Install
    //
    workflow_uninit(handle);
    handle = nullptr;
    result = workflow_init(workflow_test_install, true, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    workflowData.WorkflowHandle = handle;

    workflow_set_last_reported_state(ADUCITF_State_DownloadSucceeded);
    workflowData.CurrentAction = ADUCITF_UpdateAction_Install;

    result = ADUC_MethodCall_Install(&methodCallData);

    CHECK(result.ResultCode == ADUC_Result_Install_InProgress);
    CHECK(result.ExtendedResultCode == 0);

    CHECK(workflow_get_last_reported_state() == ADUCITF_State_InstallStarted);
    CHECK(workflowData.IsRegistered == false);
    CHECK(workflow_get_operation_in_progress(workflowData.WorkflowHandle) == false);
    CHECK(workflow_get_operation_cancel_requested(workflowData.WorkflowHandle) == false);

    // Wait for async operation completion
    workCompletionCallbackCV.wait(lock);

    ADUC_MethodCall_Install_Complete(&methodCallData, result);

    //
    // Apply
    //
    workflow_uninit(handle);
    handle = nullptr;
    result = workflow_init(workflow_test_apply, true, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    workflowData.WorkflowHandle = handle;

    workflow_set_last_reported_state(ADUCITF_State_InstallSucceeded);

    result = ADUC_MethodCall_Apply(&methodCallData);

    CHECK(result.ResultCode == ADUC_Result_Apply_InProgress);
    CHECK(result.ExtendedResultCode == 0);

    CHECK(workflow_get_last_reported_state() == ADUCITF_State_ApplyStarted);
    CHECK(workflowData.IsRegistered == false);
    CHECK(workflow_get_operation_in_progress(workflowData.WorkflowHandle) == false);
    CHECK(workflow_get_operation_cancel_requested(workflowData.WorkflowHandle) == false);

    // Wait for async operation completion
    workCompletionCallbackCV.wait(lock);

    ADUC_MethodCall_Apply_Complete(&methodCallData, result);

    //
    // Unregister
    //

    workflow_uninit(handle);
    handle = nullptr;
    workflowData.WorkflowHandle = nullptr;

    ADUC_MethodCall_Unregister(&workflowData.UpdateActionCallbacks);
}
