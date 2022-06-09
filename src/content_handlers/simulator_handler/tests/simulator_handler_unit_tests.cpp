/**
 * @file simulator_handler_unit_tests.cpp
 * @brief Unit Tests for Simulator Update Handler.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/simulator_handler.hpp"
#include "aduc/workflow_utils.h"

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include <fstream>
#include <memory>
#include <string.h>

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class SimulatorHandlerDataFile
{
    char* _dataFilePath = nullptr;

public:
    explicit SimulatorHandlerDataFile(const char* data)
    {
        _dataFilePath = GetSimulatorDataFilePath();
        try
        {
            std::ofstream file{ _dataFilePath, std::ios::trunc | std::ios::binary };
            file.write(data, strlen(data));
            REQUIRE(!file.bad());
        }
        catch (...)
        {
            FAIL("Cannot create simulator data file.");
        }
    }

    ~SimulatorHandlerDataFile()
    {
        remove(_dataFilePath);
        free(_dataFilePath); // NOLINT(cppcoreguidelines-owning-memory)
    }
};


// clang-format off

const char* action_process_deployment =
    R"( {                       )"
    R"(     "workflow": {       )"
    R"(            "action": 3, )"
    R"(            "id": "dcb112da-bfc9-47b7-b7ed-617feba1e6c4" )"
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

const char* downloadSucceeded500 =
    R"({)"
    R"(  "download" : {)"
    R"(    "contoso.contoso-virtual-motors.1.1.updatemanifest.json" : {)"
    R"(      "resultCode" : 500,)"
    R"(      "extendedResultCode" : 0,)"
    R"(      "resultDetails" : "")"
    R"(    })"
    R"(  })"
    R"(})";

const char* downloadFailed12345 =
    R"({)"
    R"(  "download" : {)"
    R"(    "contoso.contoso-virtual-motors.1.1.updatemanifest.json" : {)"
    R"(      "resultCode" : 0,)"
    R"(      "extendedResultCode" : 12345,)"
    R"(      "resultDetails" : "Mock failure result - 12345")"
    R"(    })"
    R"(  })"
    R"(})";

const char* catchAlldownloadResultFailed22222 =
    R"({)"
    R"(  "download" : {)"
    R"(    "*" : {)"
    R"(      "resultCode" : 0,)"
    R"(      "extendedResultCode" : 22222,)"
    R"(      "resultDetails" : "Mock failure result - 22222")"
    R"(    })"
    R"(  })"
    R"(})";

const char* noResultsSpecified =
    R"({)"
    R"(  "bad_key" : {)"
    R"(    "*" : {)"
    R"(      "resultCode" : 0,)"
    R"(      "extendedResultCode" : 12345,)"
    R"(      "resultDetails" : "Mock failure result - 12345")"
    R"(    })"
    R"(  })"
    R"(})";

const char* installSucceeded603 =
    R"({)"
    R"(  "install" : {)"
    R"(    "resultCode" : 603,)"
    R"(    "extendedResultCode" : 0,)"
    R"(    "resultDetails" : "Mock installation skipped.")"
    R"(  })"
    R"(})";

const char* installFailed33333 =
    R"({)"
    R"(  "install" : {)"
    R"(    "resultCode" : 0,)"
    R"(    "extendedResultCode" : 33333,)"
    R"(    "resultDetails" : "Mock failure result - 33333")"
    R"(  })"
    R"(})";

const char* applySucceed710 =
    R"({)"
    R"(  "apply" : {)"
    R"(    "resultCode" : 710,)"
    R"(    "extendedResultCode" : 0,)"
    R"(    "resultDetails" : "Mock apply succeeded - 710")"
    R"(  })"
    R"(})";

const char* applyFailed44444 =
    R"({)"
    R"(  "apply" : {)"
    R"(    "resultCode" : 0,)"
    R"(    "extendedResultCode" : 44444,)"
    R"(    "resultDetails" : "Mock failure result - 44444")"
    R"(  })"
    R"(})";

const char* cancelFailed55555 =
    R"({)"
    R"(  "cancel" : {)"
    R"(    "resultCode" : 0,)"
    R"(    "extendedResultCode" : 55555,)"
    R"(    "resultDetails" : "Mock failure result - 55555")"
    R"(  })"
    R"(})";

// Note: update manifest version 4 doesn't contain installedCriteria at the root level.
// Specify catch-all mock result below, insead of specific 'installedCriteria' string.
const char* isInstalled_installed =
    R"({)"
    R"(  "isInstalled" : {)"
    R"(    "*" : {)"
    R"(      "resultCode" : 900,)"
    R"(      "extendedResultCode" : 0,)"
    R"(      "resultDetails" : "")"
    R"(    })"
    R"(  })"
    R"(})";

const char* isInstalled_notInstalled =
    R"({)"
    R"(  "isInstalled" : {)"
    R"(    "*" : {)"
    R"(      "resultCode" : 901,)"
    R"(      "extendedResultCode" : 0,)"
    R"(      "resultDetails" : "Mock result - 901")"
    R"(    })"
    R"(  })"
    R"(})";

// clang-format on

TEST_CASE("Download Succeeded 500")
{
    SimulatorHandlerDataFile simData(downloadSucceeded500);

    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_process_deployment, false, &handle);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_WorkflowData testWorkflow{};
    testWorkflow.WorkflowHandle = handle;

    std::unique_ptr<ContentHandler> simHandler{ CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG) };

    result = simHandler->Download(&testWorkflow);
    testWorkflow.WorkflowHandle = nullptr;

    CHECK(result.ResultCode == 500);
    CHECK(result.ExtendedResultCode == 0);

    workflow_free(handle);
}

TEST_CASE("Download Failed 12345")
{
    SimulatorHandlerDataFile simData(downloadFailed12345);

    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_process_deployment, false, &handle);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_WorkflowData testWorkflow{};
    testWorkflow.WorkflowHandle = handle;

    std::unique_ptr<ContentHandler> simHandler{ CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG) };

    result = simHandler->Download(&testWorkflow);
    testWorkflow.WorkflowHandle = nullptr;

    CHECK(result.ResultCode == 0);
    CHECK(result.ExtendedResultCode == 12345);
    CHECK_THAT(workflow_peek_result_details(handle), Equals("Mock failure result - 12345"));

    workflow_free(handle);
}

TEST_CASE("Download Catch-All 22222")
{
    SimulatorHandlerDataFile simData(catchAlldownloadResultFailed22222);

    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_process_deployment, false, &handle);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_WorkflowData testWorkflow{};
    testWorkflow.WorkflowHandle = handle;

    std::unique_ptr<ContentHandler> simHandler{ CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG) };

    result = simHandler->Download(&testWorkflow);
    testWorkflow.WorkflowHandle = nullptr;

    CHECK(result.ResultCode == 0);
    CHECK(result.ExtendedResultCode == 22222);
    CHECK_THAT(workflow_peek_result_details(handle), Equals("Mock failure result - 22222"));

    workflow_free(handle);
}

TEST_CASE("Download - No Results - Succeeded 500")
{
    SimulatorHandlerDataFile simData(noResultsSpecified);

    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_process_deployment, false, &handle);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_WorkflowData testWorkflow{};
    testWorkflow.WorkflowHandle = handle;

    std::unique_ptr<ContentHandler> simHandler{ CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG) };

    result = simHandler->Download(&testWorkflow);
    testWorkflow.WorkflowHandle = nullptr;

    CHECK(result.ResultCode == 500);
    CHECK(result.ExtendedResultCode == 0);

    workflow_free(handle);
}

TEST_CASE("Install Skipped 603")
{
    SimulatorHandlerDataFile simData(installSucceeded603);

    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_process_deployment, false, &handle);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_WorkflowData testWorkflow{};
    testWorkflow.WorkflowHandle = handle;

    std::unique_ptr<ContentHandler> simHandler{ CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG) };

    result = simHandler->Install(&testWorkflow);
    testWorkflow.WorkflowHandle = nullptr;

    CHECK(result.ResultCode == 603);
    CHECK(result.ExtendedResultCode == 0);

    workflow_free(handle);
}

TEST_CASE("Install Failed 333333")
{
    SimulatorHandlerDataFile simData(installFailed33333);

    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_process_deployment, false, &handle);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_WorkflowData testWorkflow{};
    testWorkflow.WorkflowHandle = handle;

    std::unique_ptr<ContentHandler> simHandler{ CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG) };

    result = simHandler->Install(&testWorkflow);
    testWorkflow.WorkflowHandle = nullptr;

    CHECK(result.ResultCode == 0);
    CHECK(result.ExtendedResultCode == 33333);
    CHECK_THAT(workflow_peek_result_details(handle), Equals("Mock failure result - 33333"));

    workflow_free(handle);
}

TEST_CASE("Install - No Results - Succeeded 600")
{
    SimulatorHandlerDataFile simData(noResultsSpecified);

    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_process_deployment, false, &handle);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_WorkflowData testWorkflow{};
    testWorkflow.WorkflowHandle = handle;

    std::unique_ptr<ContentHandler> simHandler{ CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG) };

    result = simHandler->Install(&testWorkflow);
    testWorkflow.WorkflowHandle = nullptr;

    CHECK(result.ResultCode == 600);
    CHECK(result.ExtendedResultCode == 0);

    workflow_free(handle);
}

TEST_CASE("Apply failed 44444")
{
    SimulatorHandlerDataFile simData(applyFailed44444);

    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_process_deployment, false, &handle);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_WorkflowData testWorkflow{};
    testWorkflow.WorkflowHandle = handle;

    std::unique_ptr<ContentHandler> simHandler{ CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG) };

    result = simHandler->Apply(&testWorkflow);
    testWorkflow.WorkflowHandle = nullptr;

    CHECK(result.ResultCode == 0);
    CHECK(result.ExtendedResultCode == 44444);
    CHECK_THAT(workflow_peek_result_details(handle), Equals("Mock failure result - 44444"));

    workflow_free(handle);
}

TEST_CASE("Apply - No Results - Succeeded 700")
{
    SimulatorHandlerDataFile simData(noResultsSpecified);

    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_process_deployment, false, &handle);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_WorkflowData testWorkflow{};
    testWorkflow.WorkflowHandle = handle;

    std::unique_ptr<ContentHandler> simHandler{ CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG) };

    result = simHandler->Apply(&testWorkflow);
    testWorkflow.WorkflowHandle = nullptr;

    CHECK(result.ResultCode == 700);
    CHECK(result.ExtendedResultCode == 0);

    workflow_free(handle);
}

TEST_CASE("Cancel failed 55555")
{
    SimulatorHandlerDataFile simData(cancelFailed55555);

    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_process_deployment, false, &handle);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_WorkflowData testWorkflow{};
    testWorkflow.WorkflowHandle = handle;

    std::unique_ptr<ContentHandler> simHandler{ CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG) };

    result = simHandler->Cancel(&testWorkflow);
    testWorkflow.WorkflowHandle = nullptr;

    CHECK(result.ResultCode == 0);
    CHECK(result.ExtendedResultCode == 55555);
    CHECK_THAT(workflow_peek_result_details(handle), Equals("Mock failure result - 55555"));

    workflow_free(handle);
}

TEST_CASE("Cancel - No Results - Succeeded 800")
{
    SimulatorHandlerDataFile simData(noResultsSpecified);

    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_process_deployment, false, &handle);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_WorkflowData testWorkflow{};
    testWorkflow.WorkflowHandle = handle;

    std::unique_ptr<ContentHandler> simHandler{ CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG) };

    result = simHandler->Cancel(&testWorkflow);
    testWorkflow.WorkflowHandle = nullptr;

    CHECK(result.ResultCode == 800);
    CHECK(result.ExtendedResultCode == 0);

    workflow_free(handle);
}

TEST_CASE("IsInstalled - installed")
{
    SimulatorHandlerDataFile simData(isInstalled_installed);

    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_process_deployment, false, &handle);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_WorkflowData testWorkflow{};
    testWorkflow.WorkflowHandle = handle;

    std::unique_ptr<ContentHandler> simHandler{ CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG) };

    result = simHandler->IsInstalled(&testWorkflow);
    testWorkflow.WorkflowHandle = nullptr;

    CHECK(result.ResultCode == 900);
    CHECK(result.ExtendedResultCode == 0);

    workflow_free(handle);
}

TEST_CASE("IsInstalled - notInstalled")
{
    SimulatorHandlerDataFile simData(isInstalled_notInstalled);

    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_process_deployment, false, &handle);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_WorkflowData testWorkflow{};
    testWorkflow.WorkflowHandle = handle;

    std::unique_ptr<ContentHandler> simHandler{ CreateUpdateContentHandlerExtension(ADUC_LOG_DEBUG) };

    result = simHandler->IsInstalled(&testWorkflow);
    testWorkflow.WorkflowHandle = nullptr;

    CHECK(result.ResultCode == 901);
    CHECK(result.ExtendedResultCode == 0);
    CHECK_THAT(workflow_peek_result_details(handle), Equals("Mock result - 901"));

    workflow_free(handle);
}
