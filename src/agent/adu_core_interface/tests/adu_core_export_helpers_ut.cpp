/**
 * @file adu_core_export_helpers_ut.cpp
 * @brief Unit tests for adu_core_export_helpers.h
 *
 * @copyright Copyright (c), Microsoft Corp.
 */
#include <atomic>
#include <catch2/catch.hpp>
using Catch::Matchers::Equals;
#include <chrono>
#include <condition_variable>
#include <iothub_device_client.h>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <thread>

#include "aduc/agent_workflow.h"
#include "aduc/adu_core_export_helpers.h"
#include "aduc/c_utils.h"
#include "aduc/client_handle.h"
#include "aduc/content_handler.hpp"
#include "aduc/result.h"
#include "aduc/workflow_utils.h"
#include "workflow_internal.h"

// clang-format off
const char* workflow_test_process_deployment =
R"( {                    )"
R"(     "workflow": {    )"
R"(         "action": 3, )"
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

const char* workflow_test_process_deployment_REPLACEMENT =
R"( {                    )"
R"(     "workflow": {    )"
R"(         "action": 3, )"
R"(         "id": "REPLACEMENT_bundle_update" )"
R"(      },  )"
R"(     "updateManifest": "{\"manifestVersion\":\"2.0\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00000\":{\"fileName\":\"contoso-motor-1.0-updatemanifest.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
R"(     "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pY2toV1FrVkdTMUl4ZG5Ob1p5dEJhRWxuTDFORVVVOHplRFJyYWpORFZWUTNaa2R1U21oQmJYVkVhSFpJWm1velowaDZhVEJVTWtsQmNVTXhlREpDUTFka1QyODFkamgwZFcxeFVtb3ZibGx3WnprM2FtcFFRMHQxWTJSUE5tMHpOMlJqVDIxaE5EWm9OMDh3YTBod2Qwd3pibFZJUjBWeVNqVkVRUzloY0ZsdWQwVmxjMlY0VkdwVU9GTndMeXRpVkhGWFJXMTZaMFF6TjNCbVpFdGhjV3AwU0V4SFZtbFpkMVpJVUhwMFFtRmlkM2RxYUVGMmVubFNXUzk1T1U5bWJYcEVabGh0Y2xreGNtOHZLekpvUlhGRmVXdDFhbmRSUlZscmFHcEtZU3RDTkRjMkt6QnRkVWQ1VjBrMVpVbDJMMjlzZERKU1pWaDRUV0k1VFd4c1dFNTViMUF6WVU1TFNVcHBZbHBOY3pkMVMyTnBkMnQ1YVZWSllWbGpUV3B6T1drdlVrVjVLMnhOT1haSlduRnlabkJEVlZoMU0zUnVNVXRuWXpKUmN5OVVaRGgwVGxSRFIxWTJkM1JXWVhGcFNYQlVaRlEwVW5KRFpFMXZUelZUVG1WbVprUjVZekpzUXpkMU9EVXJiMjFVYTJOcVVHcHRObVpoY0dSSmVVWXljV1Z0ZGxOQ1JHWkNOMk5oYWpWRVNVa3lOVmQzTlVWS1kyRjJabmxRTlRSdGNVNVJVVE5IWTAxUllqSmtaMmhwWTJ4d2FsbHZLelF6V21kWlEyUkhkR0ZhWkRKRlpreGFkMGd6VVdjeWNrUnNabXN2YVdFd0x6RjVjV2xyTDFoYU1XNXpXbFJwTUVKak5VTndUMDFGY1daT1NrWlJhek5DVjI5Qk1EVnlRMW9pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJd01EY3dNaTVTTGxNaWZRLmlTVGdBRUJYc2Q3QUFOa1FNa2FHLUZBVjZRT0dVRXV4dUhnMllmU3VXaHRZWHFicE0takk1UlZMS2VzU0xDZWhLLWxSQzl4Ni1fTGV5eE5oMURPRmMtRmE2b0NFR3dVajh6aU9GX0FUNnM2RU9tY2txUHJ4dXZDV3R5WWtrRFJGNzRkdGFLMWpOQTdTZFhyWnp2V0NzTXFPVU1OejBnQ29WUjBDczEyNTRrRk1SbVJQVmZFY2pnVDdqNGxDcHlEdVdncjlTZW5TZXFnS0xZeGphYUcwc1JoOWNkaTJkS3J3Z2FOYXFBYkhtQ3JyaHhTUENUQnpXTUV4WnJMWXp1ZEVvZnlZSGlWVlJoU0pwajBPUTE4ZWN1NERQWFYxVGN0MXkzazdMTGlvN244aXpLdXEybTNUeEY5dlBkcWI5TlA2U2M5LW15YXB0cGJGcEhlRmtVTC1GNXl0bF9VQkZLcHdOOUNMNHdwNnlaLWpkWE5hZ3JtVV9xTDFDeVh3MW9tTkNnVG1KRjNHZDNseXFLSEhEZXJEcy1NUnBtS2p3U3dwWkNRSkdEUmNSb3ZXeUwxMnZqdzNMQkpNaG1VeHNFZEJhWlA1d0dkc2ZEOGxkS1lGVkZFY1owb3JNTnJVa1NNQWw2cEl4dGVmRVhpeTVscW1pUHpxX0xKMWVSSXJxWTBfIn0.eyJzaGEyNTYiOiI3alo1YWpFN2Z5SWpzcTlBbWlKNmlaQlNxYUw1bkUxNXZkL0puVWgwNFhZPSJ9.EK5zcNiEgO2rHh_ichQWlDIvkIsPXrPMQK-0D5WK8ZnOR5oJdwhwhdpgBaB-tE-6QxQB1PKurbC2BtiGL8HI1DgQtL8Fq_2ASRfzgNtrtpp6rBiLRynJuWCy7drgM6g8WoSh8Utdxsx5lnGgAVAU67ijK0ITd0E70R7vWJRmY8YxxDh-Sh8BNz68pvU-YJQwKtVy64lD5zA0--BL432F-uZWTc6n-BduQdSB4J7Eu6zGlT75s8Ehd-SIylsstu4wdypU0tcwIH-MaSKcH5mgEmokaHncJrb4zKnZwxYQUeDMoFjF39P9hDmheHywY1gwYziXjUcnMn8_T00oMeycQ7PDCTJHIYB3PGbtM9KiA3RQH-08ofqiCVgOLeqbUHTP03Z0Cx3e02LzTgP8_Lerr4okAUPksT2IGvvsiMtj04asdrLSlv-AvFud-9U0a2mJEWcosI04Q5NAbqhZ5ZBzCkkowLGofS04SnfS-VssBfmbH5ue5SWb-AxBv1inZWUj", )"
R"(     "fileUrls": {   )"
R"(         "00000": "file:///tmp/tests/testfiles/contoso-motor-1.0-updatemanifest.json",  )"
R"(         "00001": "file:///tmp/tests/testfiles/contoso-motor-1.0-fileinstaller",     )"
R"(         "gw001": "file:///tmp/tests/testfiles/behind-gateway-info.json" )"
R"(     } )"
R"( } )";

const char* expectedUpdateManifestJson =
    R"( {                                                                            )"
    R"(      "manifestVersion": "2.0",                                               )"
    R"(      "updateId": {                                                           )"
    R"(          "provider": "Contoso",                                              )"
    R"(          "name": "VacuumBundleUpdate",                                       )"
    R"(          "version": "1.0"                                                    )"
    R"(      },                                                                      )"
    R"(      "updateType": "microsoft\/bundle:1",                                    )"
    R"(      "installedCriteria": "1.0",                                             )"
    R"(      "files": {                                                              )"
    R"(          "00000": {                                                          )"
    R"(              "fileName": "contoso-motor-1.0-updatemanifest.json",            )"
    R"(              "sizeInBytes": 1396,                                            )"
    R"(              "hashes": {                                                     )"
    R"(                  "sha256": "E2o94XQss\/K8niR1pW6OdaIS\/y3tInwhEKMn\/6Rw1Gw=" )"
    R"(              }                                                               )"
    R"(          }                                                                   )"
    R"(      },                                                                      )"
    R"(      "createdDateTime": "2021-06-07T07:25:59.0781905Z"                       )"
    R"( }                                                                            )";


EXTERN_C_BEGIN
// fwd-decl to completion signal handler fn.
void ADUC_MethodCall_Idle(ADUC_WorkflowData* workflowData);
void ADUC_Workflow_HandlePropertyUpdate(ADUC_WorkflowData* currentWorkflowData, const unsigned char* propertyUpdateValue);
void ADUC_Workflow_WorkCompletionCallback(const void* workCompletionToken, ADUC_Result result, bool isAsync);
EXTERN_C_END

extern ADUC_ClientHandle g_iotHubClientHandleForADUComponent;

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
std::condition_variable replacementCV;
std::mutex replacementMutex;

static bool s_downloadFirstWorkflow_completed = false;
static bool s_replacementWorkflowIsDone = false;
static bool s_idleDone = false;
static bool s_workflowComplete = false;
static std::string s_expectedWorkflowIdWhenIdle;

#define ADUC_ClientHandle_Invalid (-1)

static unsigned int s_mockWorkCompletionCallbackCallCount = 0;

static IdleCallbackFunc s_platform_idle_callback = nullptr;

void Mock_Idle_Callback(ADUC_Token token, const char* workflowId)
{
    CHECK(token != nullptr);
    CHECK(workflowId != nullptr);
    CHECK(strcmp(workflowId, "action_bundle") == 0);

    // call the original update callback
    REQUIRE(s_platform_idle_callback != nullptr);
    (*s_platform_idle_callback)(token, workflowId);

    // notify now so that test can cleanup
    workCompletionCallbackCV.notify_all();
}

void Mock_Idle_Callback_for_REPLACEMENT(ADUC_Token token, const char* workflowId)
{
    CHECK(token != nullptr);
    CHECK(workflowId != nullptr);

    CHECK(strcmp(workflowId, "REPLACEMENT_bundle_update") == 0);

    // call the original update callback
    REQUIRE(s_platform_idle_callback != nullptr);
    (*s_platform_idle_callback)(token, workflowId);

    // notify now so that test can cleanup
    {
        std::unique_lock<std::mutex> lock(replacementMutex);

        s_idleDone = true; // update condition

        // need to manually unlock before notifying to prevent waking wait
        // thread to only block again because lock was already taken
        lock.unlock();
        replacementCV.notify_one();
    }
}

static void MockIdleCallback()
{
    s_workflowComplete = true;
}

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

    static void Mock_WorkCompletionCallback(const void* workCompletionToken, ADUC_Result result, bool isAsync)
    {
        CHECK(workCompletionToken != nullptr);
        CHECK(IsAducResultCodeSuccess(result.ResultCode));
        CHECK(result.ExtendedResultCode == 0);

        auto methodCallData = (ADUC_MethodCall_Data*) workCompletionToken;
        auto workflowData = methodCallData->WorkflowData;

        switch (s_mockWorkCompletionCallbackCallCount)
        {
            case 0:
                //
                // Process Deployment
                //
                REQUIRE_FALSE(isAsync);
                REQUIRE(workflow_get_last_reported_state() == ADUCITF_State_Idle);
                REQUIRE(workflowData->CurrentAction == ADUCITF_UpdateAction_ProcessDeployment);
                REQUIRE(workflow_get_current_workflowstep(workflowData->WorkflowHandle) == ADUCITF_WorkflowStep_ProcessDeployment);
                REQUIRE(workflow_get_operation_in_progress(workflowData->WorkflowHandle) == true);
                REQUIRE(workflow_get_operation_cancel_requested(workflowData->WorkflowHandle) == false);
                break;

            case 1:
                //
                // Download
                //
                REQUIRE(isAsync);
                REQUIRE(workflow_get_last_reported_state() == ADUCITF_State_DownloadStarted);
                REQUIRE(workflowData->CurrentAction == ADUCITF_UpdateAction_ProcessDeployment);
                REQUIRE(workflow_get_current_workflowstep(workflowData->WorkflowHandle) == ADUCITF_WorkflowStep_Download);

                REQUIRE(workflowData->IsRegistered == false);
                REQUIRE(workflow_get_operation_in_progress(workflowData->WorkflowHandle) == true);
                REQUIRE(workflow_get_operation_cancel_requested(workflowData->WorkflowHandle) == false);
                break;

            case 2:
                //
                // Install
                //
                REQUIRE(isAsync);
                REQUIRE(workflow_get_last_reported_state() == ADUCITF_State_InstallStarted);
                REQUIRE(workflowData->CurrentAction == ADUCITF_UpdateAction_ProcessDeployment);
                REQUIRE(workflow_get_current_workflowstep(workflowData->WorkflowHandle) == ADUCITF_WorkflowStep_Install);

                REQUIRE(workflowData->IsRegistered == false);
                REQUIRE(workflow_get_operation_in_progress(workflowData->WorkflowHandle) == true);
                REQUIRE(workflow_get_operation_cancel_requested(workflowData->WorkflowHandle) == false);
                break;

            case 3:
                //
                // Apply
                //
                REQUIRE(isAsync);
                REQUIRE(workflow_get_last_reported_state() == ADUCITF_State_ApplyStarted);
                REQUIRE(workflowData->CurrentAction == ADUCITF_UpdateAction_ProcessDeployment);
                REQUIRE(workflow_get_current_workflowstep(workflowData->WorkflowHandle) == ADUCITF_WorkflowStep_Apply);

                REQUIRE(workflowData->IsRegistered == false);
                REQUIRE(workflow_get_operation_in_progress(workflowData->WorkflowHandle) == true);
                REQUIRE(workflow_get_operation_cancel_requested(workflowData->WorkflowHandle) == false);

                s_platform_idle_callback = workflowData->UpdateActionCallbacks.IdleCallback;
                workflowData->UpdateActionCallbacks.IdleCallback = Mock_Idle_Callback;

                workCompletionCallbackCV.notify_all();
                break;

            default:
                workCompletionCallbackCV.notify_all();
        }

        ++s_mockWorkCompletionCallbackCallCount;

        // Call the normal work completion callback to continue the workflow processing
        ADUC_Workflow_WorkCompletionCallback(workCompletionToken, result, isAsync);
    }

    static void Mock_WorkCompletionCallback_for_REPLACEMENT(const void* workCompletionToken, ADUC_Result result, bool isAsync)
    {
        CHECK(workCompletionToken != nullptr);

        auto methodCallData = (ADUC_MethodCall_Data*) workCompletionToken;
        auto workflowData = methodCallData->WorkflowData;

        switch (s_mockWorkCompletionCallbackCallCount)
        {
            case 0:
                //
                // Process Deployment { 1st workflow }
                //
                REQUIRE_FALSE(isAsync);
                {
                    ADUC_Workflow* wf = ((ADUC_Workflow*)(workflowData->WorkflowHandle));
                    char* propertiesJson = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->PropertiesObject));
                    CHECK_THAT(propertiesJson, Equals(
                        "{\n"
                        "    \"_workFolder\": \"\\/var\\/lib\\/adu\\/downloads\\/action_bundle\"\n"
                        "}"
                    ));
                }
                REQUIRE(workflow_get_last_reported_state() == ADUCITF_State_Idle);
                REQUIRE(workflowData->CurrentAction == ADUCITF_UpdateAction_ProcessDeployment);
                REQUIRE(workflow_get_current_workflowstep(workflowData->WorkflowHandle) == ADUCITF_WorkflowStep_ProcessDeployment);
                REQUIRE(workflow_get_operation_in_progress(workflowData->WorkflowHandle) == true);
                REQUIRE(workflow_get_operation_cancel_requested(workflowData->WorkflowHandle) == false);
                CHECK(workflow_get_cancellation_type(workflowData->WorkflowHandle) == ADUC_WorkflowCancellationType_None);
                break;

            case 1:
                //
                // Download { 1st workflow }
                //
                // This is completing due to Cancellation by Replacement workflow, so Cancellation should've been requested.
                //
                REQUIRE(isAsync);
                {
                    ADUC_Workflow* wf = ((ADUC_Workflow*)(workflowData->WorkflowHandle));
                    char* propertiesJson = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->PropertiesObject));
                    CHECK_THAT(propertiesJson, Equals(
                        "{\n"
                        "    \"_workFolder\": \"\\/var\\/lib\\/adu\\/downloads\\/action_bundle\"\n"
                        "}"
                    ));
                }
                REQUIRE(workflow_get_last_reported_state() == ADUCITF_State_DownloadStarted);
                REQUIRE(workflowData->CurrentAction == ADUCITF_UpdateAction_ProcessDeployment);
                REQUIRE(workflow_get_current_workflowstep(workflowData->WorkflowHandle) == ADUCITF_WorkflowStep_Download);

                REQUIRE(workflowData->IsRegistered == false);
                REQUIRE(workflow_get_operation_in_progress(workflowData->WorkflowHandle) == true);
                CHECK(workflow_get_operation_cancel_requested(workflowData->WorkflowHandle) == true); // Cancellation
                CHECK(workflow_get_cancellation_type(workflowData->WorkflowHandle) == ADUC_WorkflowCancellationType_Replacement); // Should be Replacement Cancellation Type
                break;

            // The remainder of the "script" is the same as the non-replacement completion callback above, which is successful processing
            // of each WorkflowStep phase.
            case 2:
                //
                // Process Deployment
                //
                REQUIRE_FALSE(isAsync);
                {
                    ADUC_Workflow* wf = ((ADUC_Workflow*)(workflowData->WorkflowHandle));
                    char* propertiesJson = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->PropertiesObject));
                    CHECK_THAT(propertiesJson, Equals("{}"));

                    char* updateActionObject = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->UpdateActionObject));
                    CHECK_THAT(updateActionObject, Equals(json_serialize_to_string_pretty(json_parse_string(workflow_test_process_deployment_REPLACEMENT))));

                    char* updateManifestObject = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->UpdateManifestObject));
                    CHECK_THAT(updateManifestObject, Equals(json_serialize_to_string_pretty(json_parse_string(expectedUpdateManifestJson))));
                }

                REQUIRE(workflow_get_last_reported_state() == ADUCITF_State_Idle);
                REQUIRE(workflowData->CurrentAction == ADUCITF_UpdateAction_ProcessDeployment);
                REQUIRE(workflow_get_current_workflowstep(workflowData->WorkflowHandle) == ADUCITF_WorkflowStep_ProcessDeployment);
                REQUIRE(workflow_get_operation_in_progress(workflowData->WorkflowHandle) == true);
                REQUIRE(workflow_get_operation_cancel_requested(workflowData->WorkflowHandle) == false);
                CHECK(workflow_get_cancellation_type(workflowData->WorkflowHandle) == ADUC_WorkflowCancellationType_None); // Should be reset
                break;

            case 3:
                //
                // Download
                //
                REQUIRE(isAsync);
                {
                    ADUC_Workflow* wf = ((ADUC_Workflow*)(workflowData->WorkflowHandle));
                    char* propertiesJson = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->PropertiesObject));
                    REQUIRE(propertiesJson != nullptr);
                    CHECK_THAT(propertiesJson, Equals("{}"));

                    char* updateActionObject = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->UpdateActionObject));
                    REQUIRE(updateActionObject != nullptr);
                    CHECK_THAT(updateActionObject, Equals(json_serialize_to_string_pretty(json_parse_string(workflow_test_process_deployment_REPLACEMENT))));

                    char* updateManifestObject = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->UpdateManifestObject));
                    REQUIRE(updateManifestObject != nullptr);
                    CHECK_THAT(updateManifestObject, Equals(json_serialize_to_string_pretty(json_parse_string(expectedUpdateManifestJson))));
                }

                REQUIRE(workflow_get_last_reported_state() == ADUCITF_State_DownloadStarted);
                REQUIRE(workflowData->CurrentAction == ADUCITF_UpdateAction_ProcessDeployment);
                REQUIRE(workflow_get_current_workflowstep(workflowData->WorkflowHandle) == ADUCITF_WorkflowStep_Download);

                REQUIRE(workflowData->IsRegistered == false);
                REQUIRE(workflow_get_operation_in_progress(workflowData->WorkflowHandle) == true);
                REQUIRE(workflow_get_operation_cancel_requested(workflowData->WorkflowHandle) == false);
                break;

            case 4:
                //
                // Install
                //
                REQUIRE(isAsync);
                {
                    ADUC_Workflow* wf = ((ADUC_Workflow*)(workflowData->WorkflowHandle));
                    char* propertiesJson = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->PropertiesObject));
                    REQUIRE(propertiesJson != nullptr);
                    CHECK_THAT(propertiesJson, Equals("{}"));

                    char* updateActionObject = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->UpdateActionObject));
                    REQUIRE(updateActionObject != nullptr);
                    CHECK_THAT(updateActionObject, Equals(json_serialize_to_string_pretty(json_parse_string(workflow_test_process_deployment_REPLACEMENT))));

                    char* updateManifestObject = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->UpdateManifestObject));
                    REQUIRE(updateManifestObject != nullptr);
                    CHECK_THAT(updateManifestObject, Equals(json_serialize_to_string_pretty(json_parse_string(expectedUpdateManifestJson))));
                }

                REQUIRE(workflow_get_last_reported_state() == ADUCITF_State_InstallStarted);
                REQUIRE(workflowData->CurrentAction == ADUCITF_UpdateAction_ProcessDeployment);
                REQUIRE(workflow_get_current_workflowstep(workflowData->WorkflowHandle) == ADUCITF_WorkflowStep_Install);

                REQUIRE(workflowData->IsRegistered == false);
                REQUIRE(workflow_get_operation_in_progress(workflowData->WorkflowHandle) == true);
                REQUIRE(workflow_get_operation_cancel_requested(workflowData->WorkflowHandle) == false);
                break;

            case 5:
                //
                // Apply
                //
                REQUIRE(isAsync);
                {
                    ADUC_Workflow* wf = ((ADUC_Workflow*)(workflowData->WorkflowHandle));
                    char* propertiesJson = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->PropertiesObject));
                    CHECK_THAT(propertiesJson, Equals("{}"));

                    char* updateActionObject = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->UpdateActionObject));
                    CHECK_THAT(updateActionObject, Equals(json_serialize_to_string_pretty(json_parse_string(workflow_test_process_deployment_REPLACEMENT))));

                    char* updateManifestObject = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->UpdateManifestObject));
                    CHECK_THAT(updateManifestObject, Equals(json_serialize_to_string_pretty(json_parse_string(expectedUpdateManifestJson))));
                }

                REQUIRE(workflow_get_last_reported_state() == ADUCITF_State_ApplyStarted);
                REQUIRE(workflowData->CurrentAction == ADUCITF_UpdateAction_ProcessDeployment);
                REQUIRE(workflow_get_current_workflowstep(workflowData->WorkflowHandle) == ADUCITF_WorkflowStep_Apply);

                REQUIRE(workflowData->IsRegistered == false);
                REQUIRE(workflow_get_operation_in_progress(workflowData->WorkflowHandle) == true);
                REQUIRE(workflow_get_operation_cancel_requested(workflowData->WorkflowHandle) == false);

                s_platform_idle_callback = workflowData->UpdateActionCallbacks.IdleCallback;
                workflowData->UpdateActionCallbacks.IdleCallback = Mock_Idle_Callback_for_REPLACEMENT;

                {
                    std::unique_lock<std::mutex> lock(replacementMutex);

                    s_replacementWorkflowIsDone = true; // update condition

                    // need to manually unlock before notifying to prevent waking wait
                    // thread to only block again because lock was already taken
                    lock.unlock();
                    replacementCV.notify_one();
                }
                break;

            default:
                break;

        }

        ++s_mockWorkCompletionCallbackCallCount;

        // Call the normal work completion callback to continue the workflow processing
        ADUC_Workflow_WorkCompletionCallback(workCompletionToken, result, isAsync);
    }
}

ADUC_ResultCode s_download_result_code = ADUC_Result_Download_Success;
ADUC_ResultCode s_install_result_code = ADUC_Result_Install_Success;
ADUC_ResultCode s_apply_result_code = ADUC_Result_Apply_Success;
ADUC_ResultCode s_cancel_result_code = ADUC_Result_Cancel_Success;
ADUC_ResultCode s_isInstalled_result_code = ADUC_Result_IsInstalled_NotInstalled;

void Reset_Mocks_State()
{
    s_mockWorkCompletionCallbackCallCount = 0;
    s_downloadFirstWorkflow_completed = false;
    s_replacementWorkflowIsDone = false;
    s_idleDone = false;
    s_workflowComplete = false;
    s_expectedWorkflowIdWhenIdle = "";

    s_download_result_code = ADUC_Result_Download_Success;
    s_install_result_code = ADUC_Result_Install_Success;
    s_apply_result_code = ADUC_Result_Apply_Success;
    s_cancel_result_code = ADUC_Result_Cancel_Success;
    s_isInstalled_result_code = ADUC_Result_IsInstalled_NotInstalled;
}

// Mock the content handler so that these tests do not require the simulator platform or simulator content handler.
// A pointer to mock content handler instance will be set in the workflowData in the below tests.
class MockContentHandler : public ContentHandler
{
public:
    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    MockContentHandler(const MockContentHandler&) = delete;
    MockContentHandler& operator=(const MockContentHandler&) = delete;
    MockContentHandler(MockContentHandler&&) = delete;
    MockContentHandler& operator=(MockContentHandler&&) = delete;

    MockContentHandler() = default;
    ~MockContentHandler() override = default;

    ADUC_Result Download(const ADUC_WorkflowData* workflowData) override {
        ADUC_Result result = { s_download_result_code };
        return result;
    }

    ADUC_Result Install(const ADUC_WorkflowData* workflowData) override {
        ADUC_Result result = { s_install_result_code };
        return result;
    }

    ADUC_Result Apply(const ADUC_WorkflowData* workflowData) override {
        ADUC_Result result = { s_apply_result_code };
        return result;
    }

    ADUC_Result Cancel(const ADUC_WorkflowData* workflowData) override {
        ADUC_Result result = { s_cancel_result_code };
        return result;
    }

    ADUC_Result IsInstalled(const ADUC_WorkflowData* workflowData) override {
        ADUC_Result result = { s_isInstalled_result_code };
        return result;
    }
};

// Mock Content Handler that takes a long time in the download phase and waits there
// until it gets a Cancel call.
// It increments a counter so that when the 2nd ProcessDeployment comes in, it will
// not pause in continue for that one and continue processing the entire workflow.
class MockContentHandlerForReplacement : public ContentHandler
{
    unsigned int counter = 0;
    bool received_cancel = false;

public:
    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    MockContentHandlerForReplacement(const MockContentHandlerForReplacement&) = delete;
    MockContentHandlerForReplacement& operator=(const MockContentHandlerForReplacement&) = delete;
    MockContentHandlerForReplacement(MockContentHandlerForReplacement&&) = delete;
    MockContentHandlerForReplacement& operator=(MockContentHandlerForReplacement&&) = delete;

    MockContentHandlerForReplacement() = default;
    ~MockContentHandlerForReplacement() override = default;

    ADUC_Result Download(const ADUC_WorkflowData* workflowData) override {
        ADUC_Result result = { s_download_result_code };

        if (counter++ == 0)
        {
            // This is the first workflow and we want to simulate a long-running download
            // so that we can ensure that the replacement workflow that comes in will
            // have an in-progress operation and workflow to replace.

            // poll with 500 millisecond interval until canceled
            struct timeval half_sec
            {
               .tv_sec = 0,
               .tv_usec = 500,
            };

            bool polling = false;

            while (!received_cancel)
            {
                select(0, NULL, NULL, NULL, &half_sec);

                if (!polling)
                {
                    // Let the main thread test case move on once receive Cancel due to replacement
                    if (!polling)
                    {
                        std::unique_lock<std::mutex> lock(replacementMutex);

                        s_downloadFirstWorkflow_completed = true; // update condition
                        polling = true;

                        // need to manually unlock before notifying to prevent waking wait
                        // thread to only block again because lock was already taken
                        lock.unlock();
                        replacementCV.notify_one();
                    }
                }
            }
        }
        // else it is processing the replacement workflow eventually notify_all when apply is done
        // in Mock_WorkCompletionCallback_for_REPLACEMENT (3 more worker threads will created in
        // succession via autoTransitionWorkflow)

        return result;
    }

    ADUC_Result Install(const ADUC_WorkflowData* workflowData) override {
        ADUC_Result result = { s_install_result_code };
        return result;
    }

    ADUC_Result Apply(const ADUC_WorkflowData* workflowData) override {
        ADUC_Result result = { s_apply_result_code };
        return result;
    }

    ADUC_Result Cancel(const ADUC_WorkflowData* workflowData) override {
        ADUC_Result result = { s_cancel_result_code };

        // signal to exit poll loop in download
        received_cancel = true;

        return result;
    }

    ADUC_Result IsInstalled(const ADUC_WorkflowData* workflowData) override {
        ADUC_Result result = { s_isInstalled_result_code };
        return result;
    }

};

static ADUC_Result Mock_SandboxCreateCallback(ADUC_Token token, const char* workflowId, char* workFolder)
{
    ADUC_Result result = { ADUC_Result_Success };
    return result;
}

static void Mock_SandboxDestroyCallback(ADUC_Token token, const char* workflowId, const char* workFolder)
{
}

static void Mock_IdleCallback(ADUC_Token token, const char* workflowId)
{
    UNREFERENCED_PARAMETER(token);
    CHECK(s_expectedWorkflowIdWhenIdle == workflowId);
    s_workflowComplete = true;
}

static void wait_for_workflow_complete()
{
    const unsigned MaxIterations = 100;
    const unsigned SleepIntervalInMs = 10;

    unsigned count = 0;
    while (count++ < MaxIterations && !s_workflowComplete)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(SleepIntervalInMs));
    }
    CHECK(s_workflowComplete);
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

// Linux platform register doesn't use argc/argv so this test fails
// TEST_CASE("ADUC_MethodCall_Register and Unregister: Invalid")
// {
//     SECTION("Invalid args passed to register returns failure")
//     {
//         ADUC_UpdateActionCallbacks updateActionCallbacks{};
//         const char* argv[1] = {};
//         const int argc = std::distance(std::begin(argv), std::end(argv));
//         const ADUC_Result result{ ADUC_MethodCall_Register(&updateActionCallbacks, argc, argv) };
//         INFO_ADUC_Result(result);
//         CHECK(IsAducResultCodeFailure(result.ResultCode));
//         CHECK(result.ExtendedResultCode == ADUC_ERC_NOTRECOVERABLE);
//     }
// }

// This test exercises the happy path for the entire agent-orchestrated workflow
// via HandlePropertyUpdate, but with mocked
// ADUC_Workflow_WorkCompletionCallback and mocked ContentHandler layer.
// It does not depend on simulator platform / simulator content handler and
// exercises the platform layer as well, including the async worker threads for
// the individual operations (i.e. download, install, apply).
// It indirectly is exercising all the ADUC_MethodCall_<operation> functions in
// adu_core_exports_helpers.
TEST_CASE_METHOD(TestCaseFixture, "Process Workflow E2E Functional")
{
    Reset_Mocks_State();
    s_expectedWorkflowIdWhenIdle = "action_bundle";

    std::mutex workCompletionCallbackMTX;
    std::unique_lock<std::mutex> lock(workCompletionCallbackMTX);

    MockContentHandler mockContentHandler;

    ADUC_WorkflowData workflowData{};

    // set test overrides
    ADUC_TestOverride_Hooks hooks{};
    hooks.WorkCompletionCallbackFunc_TestOverride = Mock_WorkCompletionCallback;
    hooks.ContentHandler_TestOverride = &mockContentHandler;
    workflowData.TestOverrides = &hooks;

    ADUC_Result result =
        ADUC_MethodCall_Register(&(workflowData.UpdateActionCallbacks), 0 /*argc*/, nullptr /*argv*/);

    workflowData.UpdateActionCallbacks.SandboxCreateCallback = Mock_SandboxCreateCallback;
    workflowData.UpdateActionCallbacks.SandboxDestroyCallback = Mock_SandboxDestroyCallback;
    workflowData.UpdateActionCallbacks.IdleCallback = Mock_IdleCallback;

    workflowData.DownloadProgressCallback = DownloadProgressCallback;
    workflow_set_last_reported_state(ADUCITF_State_Idle);

    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    REQUIRE(result.ExtendedResultCode == 0);

    // simulating non-startup processing of twin
    workflowData.WorkflowHandle = nullptr;
    workflowData.StartupIdleCallSent = true;
    ADUC_Workflow_HandlePropertyUpdate(&workflowData, (const unsigned char*)workflow_test_process_deployment);

    // Wait for entire workflow to complete
    workCompletionCallbackCV.wait(lock);

    // Wait again for it to go to Idle so we don't trash the workflowData by going out of scope
    workCompletionCallbackCV.wait(lock);

    wait_for_workflow_complete();

    ADUC_MethodCall_Unregister(&workflowData.UpdateActionCallbacks);
}

// This test exercises the deployment Replacement logic when a deployment with a different workflow id comes
// in while a deployment is ongoing and the deferred processing of the next workflow during the
// workCompletionCallback of the first operation due to canceling.
TEST_CASE_METHOD(TestCaseFixture, "Process workflow - Replacement")
{
    Reset_Mocks_State();

    s_expectedWorkflowIdWhenIdle = "REPLACEMENT_bundle_update";

    MockContentHandlerForReplacement mockContentHandler;

    ADUC_WorkflowData workflowData{};

    // set test overrides
    ADUC_TestOverride_Hooks hooks{};
    hooks.WorkCompletionCallbackFunc_TestOverride = Mock_WorkCompletionCallback_for_REPLACEMENT;
    hooks.ContentHandler_TestOverride = &mockContentHandler;
    workflowData.TestOverrides = &hooks;

    ADUC_Result result =
        ADUC_MethodCall_Register(&(workflowData.UpdateActionCallbacks), 0 /*argc*/, nullptr /*argv*/);

    workflowData.UpdateActionCallbacks.SandboxCreateCallback = Mock_SandboxCreateCallback;
    workflowData.UpdateActionCallbacks.SandboxDestroyCallback = Mock_SandboxDestroyCallback;

    workflowData.DownloadProgressCallback = DownloadProgressCallback;

    workflow_set_last_reported_state(ADUCITF_State_Idle);

    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    REQUIRE(result.ExtendedResultCode == 0);

    // download result be failure for cancel first workflow
    s_download_result_code = ADUC_Result_Failure_Cancelled;

    // simulating non-startup processing of twin
    workflowData.WorkflowHandle = nullptr;
    workflowData.StartupIdleCallSent = true;
    ADUC_Workflow_HandlePropertyUpdate(&workflowData, (const unsigned char*)workflow_test_process_deployment); // workflow id => "bundle_update"

    // The mock content handler will loop with sleep during each poll iteration. It will exit poll loop when
    // it receives Cancel in the content handler due to cancel request from 2nd ProcessDeployment update action
    // coming in.

    // notify will come once mock content handler Download method starts polling for cancellation request.
    {
        std::unique_lock<std::mutex> lock(replacementMutex);
        replacementCV.wait(lock, [] { return s_downloadFirstWorkflow_completed; });
    }

    // Now we kick off the Replacement deployment, which should cause the first worker thread to exit
    // poll loop in MockContent handler due to cancellation, and then that worker thread will do the auto-transition with
    // the DeferredReplacementWorkflow that was saved in the current WorkflowData Handle.
    ADUC_Workflow_HandlePropertyUpdate(&workflowData, (const unsigned char*)workflow_test_process_deployment_REPLACEMENT); // workflow id => "REPLACEMENT_bundle_update"

    // make download result be a success for the next workflow download
    s_download_result_code = ADUC_Result_Download_Success;

    // Also hook into IdleCallback to know workflow is done and WorkflowHandle has been freed
    workflowData.UpdateActionCallbacks.IdleCallback = Mock_IdleCallback;

    {
        std::unique_lock<std::mutex> lock(replacementMutex);
        replacementCV.wait(lock, [] { return s_replacementWorkflowIsDone; });
    }

    // Wait again for it to go to Idle so we don't trash the workflowData by going out of scope
    {
        std::unique_lock<std::mutex> lock(replacementMutex);
        replacementCV.wait(lock, [] { return s_idleDone; });
    }

    wait_for_workflow_complete();

    ADUC_MethodCall_Unregister(&workflowData.UpdateActionCallbacks);
}