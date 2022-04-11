/**
 * @file adu_core_export_helpers_ut.cpp
 * @brief Unit tests for adu_core_export_helpers.h
 *
 * @copyright Copyright (c) Microsoft Corp.
 * Licensed under the MIT License.
 */
#include <atomic>
#include <catch2/catch.hpp>
using Catch::Matchers::Equals;
#include <chrono>
#include <condition_variable>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <thread>

#include "aduc/adu_core_export_helpers.h"
#include "aduc/adu_core_interface.h"
#include "aduc/agent_workflow.h"
#include "aduc/c_utils.h"
#include "aduc/client_handle.h"
#include "aduc/content_handler.hpp"
#include "aduc/result.h"
#include "aduc/types/workflow.h"
#include "aduc/workflow_data_utils.h"
#include "aduc/workflow_internal.h"
#include "aduc/workflow_utils.h"

// clang-format off
static const char* workflow_test_process_deployment =
    R"( {                       )"
    R"(     "workflow": {       )"
    R"(            "action": 3, )"
    R"(            "id": "action_bundle" )"
    R"(        },   )"
    R"(        "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"20.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/apt:1\",\"files\":[\"f483750ebb885d32c\"],\"handlerProperties\":{\"installedCriteria\":\"apt-update-tree-1.0\"}},{\"type\":\"reference\",\"detachedManifestFileId\":\"f222b9ffefaaac577\"}]},\"files\":{\"f483750ebb885d32c\":{\"fileName\":\"apt-manifest-tree-1.0.json\",\"sizeInBytes\":136,\"hashes\":{\"sha256\":\"Uk1vsEL/nT4btMngo0YSJjheOL2aqm6/EAFhzPb0rXs=\"}},\"f222b9ffefaaac577\":{\"fileName\":\"contoso.contoso-virtual-motors.1.1.updatemanifest.json\",\"sizeInBytes\":1031,\"hashes\":{\"sha256\":\"9Rnjw7ThZhGacOGn3uvvVq0ccQTHc/UFSL9khR2oKsc=\"}}},\"createdDateTime\":\"2022-01-27T13:45:05.8993329Z\"}",  )"
    R"(        "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJqSW12eGpsc2pqZ29JeUJuYThuZTk2d0RYYlVsU3N6eGFoM0NibkF6STFJPSJ9.PzpvU13h6VhN8VHXUTYKAlpDW5t3JaQ-gs895_Q10XshKPYpeZUtViXGHGC-aQSQAYPhhYV-lLia9niXzZz4Qs4ehwFLHJfkmKR8eRwWvoOgJtAY0IIUA_8SeShmoOc9cdpC35N3OeaM4hV9shxvvrphDib5sLpkrv3LQrt3DHvK_L2n0HsybC-pwS7MzaSUIYoU-fXwZo6x3z7IbSaSNwS0P-50qeV99Mc0AUSIvB26GjmjZ2gEH5R3YD9kp0DOrYvE5tIymVHPTqkmunv2OrjKu2UOhNj8Om3RoVzxIkVM89cVGb1u1yB2kxEmXogXPz64cKqQWm22tV-jalS4dAc_1p9A9sKzZ632HxnlavOBjTKDGFgM95gg8M5npXBP3QIvkwW3yervCukViRUKIm-ljpDmnBJsZTMx0uzTaAk5XgoCUCADuLLol8EXB-0V4m2w-6tV6kAzRiwkqw1PRrGqplf-gmfU7TuFlQ142-EZLU5rK_dAiQRXx-f7LxNH",  )"
    R"(        "fileUrls": {    )"
    R"(            "f483750ebb885d32c": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/e5cc19d5e9174c93ada35cc315f1fb1d/apt-manifest-tree-1.0.json",      )"
    R"(            "f222b9ffefaaac577": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/31c38c3340a84e38ae8d30ce340f4a49/contoso.contoso-virtual-motors.1.1.updatemanifest.json",  )"
    R"(            "f2c5d1f3b0295db0f": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/9ff068f7c2bf43eb9561da14a7cbcecd/motor-firmware-1.1.json",         )"
    R"(            "f13b5435aab7c18da": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/c02058a476a242d7bc0e3c576c180051/contoso-motor-installscript.sh"   )"
    R"(        }    )"
    R"( } )";

static const char* workflow_test_process_deployment_REPLACEMENT =
    R"( {                       )"
    R"(     "workflow": {       )"
    R"(            "action": 3, )"
    R"(            "id": "REPLACEMENT_bundle_update" )"
    R"(        },   )"
    R"(        "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"20.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/apt:1\",\"files\":[\"f483750ebb885d32c\"],\"handlerProperties\":{\"installedCriteria\":\"apt-update-tree-1.0\"}},{\"type\":\"reference\",\"detachedManifestFileId\":\"f222b9ffefaaac577\"}]},\"files\":{\"f483750ebb885d32c\":{\"fileName\":\"apt-manifest-tree-1.0.json\",\"sizeInBytes\":136,\"hashes\":{\"sha256\":\"Uk1vsEL/nT4btMngo0YSJjheOL2aqm6/EAFhzPb0rXs=\"}},\"f222b9ffefaaac577\":{\"fileName\":\"contoso.contoso-virtual-motors.1.1.updatemanifest.json\",\"sizeInBytes\":1031,\"hashes\":{\"sha256\":\"9Rnjw7ThZhGacOGn3uvvVq0ccQTHc/UFSL9khR2oKsc=\"}}},\"createdDateTime\":\"2022-01-27T13:45:05.8993329Z\"}",  )"
    R"(        "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJqSW12eGpsc2pqZ29JeUJuYThuZTk2d0RYYlVsU3N6eGFoM0NibkF6STFJPSJ9.PzpvU13h6VhN8VHXUTYKAlpDW5t3JaQ-gs895_Q10XshKPYpeZUtViXGHGC-aQSQAYPhhYV-lLia9niXzZz4Qs4ehwFLHJfkmKR8eRwWvoOgJtAY0IIUA_8SeShmoOc9cdpC35N3OeaM4hV9shxvvrphDib5sLpkrv3LQrt3DHvK_L2n0HsybC-pwS7MzaSUIYoU-fXwZo6x3z7IbSaSNwS0P-50qeV99Mc0AUSIvB26GjmjZ2gEH5R3YD9kp0DOrYvE5tIymVHPTqkmunv2OrjKu2UOhNj8Om3RoVzxIkVM89cVGb1u1yB2kxEmXogXPz64cKqQWm22tV-jalS4dAc_1p9A9sKzZ632HxnlavOBjTKDGFgM95gg8M5npXBP3QIvkwW3yervCukViRUKIm-ljpDmnBJsZTMx0uzTaAk5XgoCUCADuLLol8EXB-0V4m2w-6tV6kAzRiwkqw1PRrGqplf-gmfU7TuFlQ142-EZLU5rK_dAiQRXx-f7LxNH",  )"
    R"(        "fileUrls": {    )"
    R"(            "f483750ebb885d32c": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/e5cc19d5e9174c93ada35cc315f1fb1d/apt-manifest-tree-1.0.json",      )"
    R"(            "f222b9ffefaaac577": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/31c38c3340a84e38ae8d30ce340f4a49/contoso.contoso-virtual-motors.1.1.updatemanifest.json",  )"
    R"(            "f2c5d1f3b0295db0f": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/9ff068f7c2bf43eb9561da14a7cbcecd/motor-firmware-1.1.json",         )"
    R"(            "f13b5435aab7c18da": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/c02058a476a242d7bc0e3c576c180051/contoso-motor-installscript.sh"   )"
    R"(        }    )"
    R"( } )";

static const char* expectedUpdateManifestJson =
    R"( {                                                                               )"
    R"(      "manifestVersion": "4",                                                    )"
    R"(      "updateId": {                                                              )"
    R"(          "provider": "Contoso",                                                 )"
    R"(          "name": "Virtual-Vacuum",                                              )"
    R"(          "version": "20.0"                                                      )"
    R"(      },                                                                         )"
    R"(      "compatibility": [                                                         )"
    R"(          {                                                                      )"
    R"(              "deviceManufacturer": "contoso",                                   )"
    R"(              "deviceModel": "virtual-vacuum-v1"                                 )"
    R"(          }                                                                      )"
    R"(      ],                                                                         )"
    R"(      "instructions": {                                                          )"
    R"(      "steps": [                                                                 )"
    R"(         {                                                                       )"
    R"(             "handler": "microsoft\/apt:1",                                      )"
    R"(             "files": [                                                          )"
    R"(                 "f483750ebb885d32c"                                             )"
    R"(             ],                                                                  )"
    R"(             "handlerProperties": {                                              )"
    R"(                 "installedCriteria": "apt-update-tree-1.0"                      )"
    R"(             }                                                                   )"
    R"(         },                                                                      )"
    R"(         {                                                                       )"
    R"(             "type": "reference",                                                )"
    R"(             "detachedManifestFileId": "f222b9ffefaaac577"                       )"
    R"(         }                                                                       )"
    R"(      ]                                                                          )"
    R"( },                                                                              )"
    R"( "files": {                                                                      )"
    R"(     "f483750ebb885d32c": {                                                      )"
    R"(         "fileName": "apt-manifest-tree-1.0.json",                               )"
    R"(         "sizeInBytes": 136,                                                     )"
    R"(         "hashes": {                                                             )"
    R"(             "sha256": "Uk1vsEL\/nT4btMngo0YSJjheOL2aqm6\/EAFhzPb0rXs="          )"
    R"(         }                                                                       )"
    R"(     },                                                                          )"
    R"(     "f222b9ffefaaac577": {                                                      )"
    R"(         "fileName": "contoso.contoso-virtual-motors.1.1.updatemanifest.json",   )"
    R"(         "sizeInBytes": 1031,                                                    )"
    R"(         "hashes": {                                                             )"
    R"(             "sha256": "9Rnjw7ThZhGacOGn3uvvVq0ccQTHc\/UFSL9khR2oKsc="           )"
    R"(         }                                                                       )"
    R"(     }                                                                           )"
    R"( },                                                                              )"
    R"( "createdDateTime": "2022-01-27T13:45:05.8993329Z"                               )"
    R"( }                                                                               )";

EXTERN_C_BEGIN
// fwd-decl to completion signal handler fn.
void ADUC_Workflow_WorkCompletionCallback(const void* workCompletionToken, ADUC_Result result, bool isAsync);
EXTERN_C_END

// Note: g_iotHubClientHandleForADUComponent declared in adu_core_intefaace.h

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

static std::condition_variable workCompletionCallbackCV;
static std::condition_variable replacementCV;
static std::mutex replacementMutex;

static bool s_downloadFirstWorkflow_completed = false;
static bool s_replacementWorkflowIsDone = false;
static bool s_idleDone = false;
static bool s_workflowComplete = false;
static std::string s_expectedWorkflowIdWhenIdle;

#define ADUC_ClientHandle_Invalid (-1)

static unsigned int s_mockWorkCompletionCallbackCallCount = 0;

static IdleCallbackFunc s_platform_idle_callback = nullptr;

static void Mock_Idle_Callback_for_REPLACEMENT(ADUC_Token token, const char* workflowId)
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

extern "C"
{
    static void Mock_DownloadProgressCallback(
        const char* /*workflowId*/,
        const char* /*fileId*/,
        ADUC_DownloadProgressState /*state*/,
        uint64_t /*bytesTransferred*/,
        uint64_t /*bytesTotal*/)
    {
    }

    static void Mock_WorkCompletionCallback_for_REPLACEMENT(const void* workCompletionToken, ADUC_Result result, bool isAsync)
    {
        CHECK(workCompletionToken != nullptr);

        auto methodCallData = (ADUC_MethodCall_Data*)workCompletionToken; // NOLINT
        auto workflowData = methodCallData->WorkflowData;

        switch (s_mockWorkCompletionCallbackCallCount)
        {
            case 0:
                //
                // Process Deployment { 1st workflow }
                //
                REQUIRE_FALSE(isAsync);
                {
                    auto wf = (ADUC_Workflow*)workflowData->WorkflowHandle; // NOLINT
                    char* propertiesJson = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->PropertiesObject));
                    CHECK_THAT(propertiesJson, Equals(
                        "{\n"
                        "    \"_workFolder\": \"\\/var\\/lib\\/adu\\/downloads\\/action_bundle\"\n"
                        "}"
                    ));
                }
                REQUIRE(ADUC_WorkflowData_GetLastReportedState(workflowData) == ADUCITF_State_Idle);
                REQUIRE(ADUC_WorkflowData_GetCurrentAction(workflowData) == ADUCITF_UpdateAction_ProcessDeployment);
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
                    auto wf = static_cast<ADUC_Workflow*>(workflowData->WorkflowHandle);
                    char* propertiesJson = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->PropertiesObject));
                    CHECK_THAT(propertiesJson, Equals(
                        "{\n"
                        "    \"_workFolder\": \"\\/var\\/lib\\/adu\\/downloads\\/action_bundle\"\n"
                        "}"
                    ));
                }
                REQUIRE(ADUC_WorkflowData_GetLastReportedState(workflowData) == ADUCITF_State_DownloadStarted);
                REQUIRE(ADUC_WorkflowData_GetCurrentAction(workflowData) == ADUCITF_UpdateAction_ProcessDeployment);
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
                    auto wf = static_cast<ADUC_Workflow*>(workflowData->WorkflowHandle);
                    char* propertiesJson = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->PropertiesObject));
                    CHECK_THAT(propertiesJson, Equals("{}"));

                    char* updateActionObject = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->UpdateActionObject));
                    CHECK_THAT(updateActionObject, Equals(json_serialize_to_string_pretty(json_parse_string(workflow_test_process_deployment_REPLACEMENT))));

                    char* updateManifestObject = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->UpdateManifestObject));
                    CHECK_THAT(updateManifestObject, Equals(json_serialize_to_string_pretty(json_parse_string(expectedUpdateManifestJson))));
                }

                REQUIRE(ADUC_WorkflowData_GetLastReportedState(workflowData) == ADUCITF_State_Idle);
                REQUIRE(ADUC_WorkflowData_GetCurrentAction(workflowData) == ADUCITF_UpdateAction_ProcessDeployment);
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
                    auto wf = static_cast<ADUC_Workflow*>(workflowData->WorkflowHandle);
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

                REQUIRE(ADUC_WorkflowData_GetLastReportedState(workflowData) == ADUCITF_State_DownloadStarted);
                REQUIRE(ADUC_WorkflowData_GetCurrentAction(workflowData) == ADUCITF_UpdateAction_ProcessDeployment);
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
                    auto wf = static_cast<ADUC_Workflow*>(workflowData->WorkflowHandle);
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

                REQUIRE(ADUC_WorkflowData_GetLastReportedState(workflowData) == ADUCITF_State_InstallStarted);
                REQUIRE(ADUC_WorkflowData_GetCurrentAction(workflowData) == ADUCITF_UpdateAction_ProcessDeployment);
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
                    auto wf = static_cast<ADUC_Workflow*>(workflowData->WorkflowHandle);
                    char* propertiesJson = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->PropertiesObject));
                    CHECK_THAT(propertiesJson, Equals("{}"));

                    char* updateActionObject = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->UpdateActionObject));
                    CHECK_THAT(updateActionObject, Equals(json_serialize_to_string_pretty(json_parse_string(workflow_test_process_deployment_REPLACEMENT))));

                    char* updateManifestObject = json_serialize_to_string_pretty(json_object_get_wrapping_value(wf->UpdateManifestObject));
                    CHECK_THAT(updateManifestObject, Equals(json_serialize_to_string_pretty(json_parse_string(expectedUpdateManifestJson))));
                }

                REQUIRE(ADUC_WorkflowData_GetLastReportedState(workflowData) == ADUCITF_State_ApplyStarted);
                REQUIRE(ADUC_WorkflowData_GetCurrentAction(workflowData) == ADUCITF_UpdateAction_ProcessDeployment);
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

static ADUC_ResultCode s_download_result_code = ADUC_Result_Download_Success;
static ADUC_ResultCode s_install_result_code = ADUC_Result_Install_Success;
static ADUC_ResultCode s_apply_result_code = ADUC_Result_Apply_Success;
static ADUC_ResultCode s_cancel_result_code = ADUC_Result_Cancel_Success;
static ADUC_ResultCode s_isInstalled_result_code = ADUC_Result_IsInstalled_NotInstalled;

static void Reset_Mocks_State()
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
        UNREFERENCED_PARAMETER(workflowData);

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
                select(0, nullptr, nullptr, nullptr, &half_sec);

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
        // else it is processing the replacement workflow eventually notify_all when apply is done
        // in Mock_WorkCompletionCallback_for_REPLACEMENT (3 more worker threads will created in
        // succession via ADUC_Workflow_AutoTransitionWorkflow)

        return result;
    }

    ADUC_Result Install(const ADUC_WorkflowData* workflowData) override {
        UNREFERENCED_PARAMETER(workflowData);

        ADUC_Result result = { s_install_result_code };
        return result;
    }

    ADUC_Result Apply(const ADUC_WorkflowData* workflowData) override {
        UNREFERENCED_PARAMETER(workflowData);

        ADUC_Result result = { s_apply_result_code };
        return result;
    }

    ADUC_Result Cancel(const ADUC_WorkflowData* workflowData) override {
        UNREFERENCED_PARAMETER(workflowData);

        ADUC_Result result = { s_cancel_result_code };

        // signal to exit poll loop in download
        received_cancel = true;

        return result;
    }

    ADUC_Result IsInstalled(const ADUC_WorkflowData* workflowData) override {
        UNREFERENCED_PARAMETER(workflowData);

        ADUC_Result result = { s_isInstalled_result_code };
        return result;
    }

};

// NOLINTNEXTLINE(readability-non-const-parameter)
static ADUC_Result Mock_SandboxCreateCallback(ADUC_Token token, const char* workflowId, char* workFolder)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);

    ADUC_Result result = { ADUC_Result_Success };
    return result;
}

static void Mock_SandboxDestroyCallback(ADUC_Token token, const char* workflowId, const char* workFolder)
{
    UNREFERENCED_PARAMETER(token);
    UNREFERENCED_PARAMETER(workflowId);
    UNREFERENCED_PARAMETER(workFolder);
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

static IOTHUB_CLIENT_RESULT_TYPE mockClientHandle_SendReportedState(
    ADUC_CLIENT_HANDLE_TYPE deviceHandle,
    const unsigned char* reportedState,
    size_t reportedStateLen,
    IOTHUB_CLIENT_REPORTED_STATE_CALLBACK_TYPE reportedStateCallback,
    void* userContextCallback)
{
    UNREFERENCED_PARAMETER(deviceHandle);
    UNREFERENCED_PARAMETER(reportedState);
    UNREFERENCED_PARAMETER(reportedStateLen);
    UNREFERENCED_PARAMETER(reportedStateCallback);
    UNREFERENCED_PARAMETER(userContextCallback);

    return ADUC_IOTHUB_CLIENT_OK;
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
    hooks.ClientHandle_SendReportedStateFunc_TestOverride = (void*)mockClientHandle_SendReportedState; // NOLINT
    workflowData.TestOverrides = &hooks;

    ADUC_Result result =
        ADUC_MethodCall_Register(&(workflowData.UpdateActionCallbacks), 0 /*argc*/, nullptr /*argv*/);

    workflowData.UpdateActionCallbacks.SandboxCreateCallback = Mock_SandboxCreateCallback;
    workflowData.UpdateActionCallbacks.SandboxDestroyCallback = Mock_SandboxDestroyCallback;

    workflowData.DownloadProgressCallback = Mock_DownloadProgressCallback;
    workflowData.ReportStateAndResultAsyncCallback = AzureDeviceUpdateCoreInterface_ReportStateAndResultAsync;

    workflowData.LastReportedState = ADUCITF_State_Idle;

    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    REQUIRE(result.ExtendedResultCode == 0);

    // download result be failure for cancel first workflow
    s_download_result_code = ADUC_Result_Failure_Cancelled;

    // simulating non-startup processing of twin
    workflowData.WorkflowHandle = nullptr;
    workflowData.StartupIdleCallSent = true;

    // NOLINTNEXTLINE
    ADUC_Workflow_HandlePropertyUpdate(&workflowData, (const unsigned char*)workflow_test_process_deployment, false /* forceDeferral */); // workflow id => "bundle_update"

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

    // NOLINTNEXTLINE
    ADUC_Workflow_HandlePropertyUpdate(&workflowData, (const unsigned char*)workflow_test_process_deployment_REPLACEMENT, false /* forceDeferral */); // workflow id => "REPLACEMENT_bundle_update"

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
