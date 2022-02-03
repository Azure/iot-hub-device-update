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
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 3, )"
    R"(         "id": "mock_workflow_id" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"2.0\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/foo:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00000\":{\"fileName\":\"contoso-motor-1.0-updatemanifest.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
    R"(     "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pY2toV1FrVkdTMUl4ZG5Ob1p5dEJhRWxuTDFORVVVOHplRFJyYWpORFZWUTNaa2R1U21oQmJYVkVhSFpJWm1velowaDZhVEJVTWtsQmNVTXhlREpDUTFka1QyODFkamgwZFcxeFVtb3ZibGx3WnprM2FtcFFRMHQxWTJSUE5tMHpOMlJqVDIxaE5EWm9OMDh3YTBod2Qwd3pibFZJUjBWeVNqVkVRUzloY0ZsdWQwVmxjMlY0VkdwVU9GTndMeXRpVkhGWFJXMTZaMFF6TjNCbVpFdGhjV3AwU0V4SFZtbFpkMVpJVUhwMFFtRmlkM2RxYUVGMmVubFNXUzk1T1U5bWJYcEVabGh0Y2xreGNtOHZLekpvUlhGRmVXdDFhbmRSUlZscmFHcEtZU3RDTkRjMkt6QnRkVWQ1VjBrMVpVbDJMMjlzZERKU1pWaDRUV0k1VFd4c1dFNTViMUF6WVU1TFNVcHBZbHBOY3pkMVMyTnBkMnQ1YVZWSllWbGpUV3B6T1drdlVrVjVLMnhOT1haSlduRnlabkJEVlZoMU0zUnVNVXRuWXpKUmN5OVVaRGgwVGxSRFIxWTJkM1JXWVhGcFNYQlVaRlEwVW5KRFpFMXZUelZUVG1WbVprUjVZekpzUXpkMU9EVXJiMjFVYTJOcVVHcHRObVpoY0dSSmVVWXljV1Z0ZGxOQ1JHWkNOMk5oYWpWRVNVa3lOVmQzTlVWS1kyRjJabmxRTlRSdGNVNVJVVE5IWTAxUllqSmtaMmhwWTJ4d2FsbHZLelF6V21kWlEyUkhkR0ZhWkRKRlpreGFkMGd6VVdjeWNrUnNabXN2YVdFd0x6RjVjV2xyTDFoYU1XNXpXbFJwTUVKak5VTndUMDFGY1daT1NrWlJhek5DVjI5Qk1EVnlRMW9pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJd01EY3dNaTVTTGxNaWZRLmlTVGdBRUJYc2Q3QUFOa1FNa2FHLUZBVjZRT0dVRXV4dUhnMllmU3VXaHRZWHFicE0takk1UlZMS2VzU0xDZWhLLWxSQzl4Ni1fTGV5eE5oMURPRmMtRmE2b0NFR3dVajh6aU9GX0FUNnM2RU9tY2txUHJ4dXZDV3R5WWtrRFJGNzRkdGFLMWpOQTdTZFhyWnp2V0NzTXFPVU1OejBnQ29WUjBDczEyNTRrRk1SbVJQVmZFY2pnVDdqNGxDcHlEdVdncjlTZW5TZXFnS0xZeGphYUcwc1JoOWNkaTJkS3J3Z2FOYXFBYkhtQ3JyaHhTUENUQnpXTUV4WnJMWXp1ZEVvZnlZSGlWVlJoU0pwajBPUTE4ZWN1NERQWFYxVGN0MXkzazdMTGlvN244aXpLdXEybTNUeEY5dlBkcWI5TlA2U2M5LW15YXB0cGJGcEhlRmtVTC1GNXl0bF9VQkZLcHdOOUNMNHdwNnlaLWpkWE5hZ3JtVV9xTDFDeVh3MW9tTkNnVG1KRjNHZDNseXFLSEhEZXJEcy1NUnBtS2p3U3dwWkNRSkdEUmNSb3ZXeUwxMnZqdzNMQkpNaG1VeHNFZEJhWlA1d0dkc2ZEOGxkS1lGVkZFY1owb3JNTnJVa1NNQWw2cEl4dGVmRVhpeTVscW1pUHpxX0xKMWVSSXJxWTBfIn0.eyJzaGEyNTYiOiI3alo1YWpFN2Z5SWpzcTlBbWlKNmlaQlNxYUw1bkUxNXZkL0puVWgwNFhZPSJ9.EK5zcNiEgO2rHh_ichQWlDIvkIsPXrPMQK-0D5WK8ZnOR5oJdwhwhdpgBaB-tE-6QxQB1PKurbC2BtiGL8HI1DgQtL8Fq_2ASRfzgNtrtpp6rBiLRynJuWCy7drgM6g8WoSh8Utdxsx5lnGgAVAU67ijK0ITd0E70R7vWJRmY8YxxDh-Sh8BNz68pvU-YJQwKtVy64lD5zA0--BL432F-uZWTc6n-BduQdSB4J7Eu6zGlT75s8Ehd-SIylsstu4wdypU0tcwIH-MaSKcH5mgEmokaHncJrb4zKnZwxYQUeDMoFjF39P9hDmheHywY1gwYziXjUcnMn8_T00oMeycQ7PDCTJHIYB3PGbtM9KiA3RQH-08ofqiCVgOLeqbUHTP03Z0Cx3e02LzTgP8_Lerr4okAUPksT2IGvvsiMtj04asdrLSlv-AvFud-9U0a2mJEWcosI04Q5NAbqhZ5ZBzCkkowLGofS04SnfS-VssBfmbH5ue5SWb-AxBv1inZWUj", )"
    R"(     "fileUrls": {   )"
    R"(         "00000": "file:///tmp/tests/testfiles/contoso-motor-1.0-updatemanifest.json",  )"
    R"(         "00001": "file:///tmp/tests/testfiles/contoso-motor-1.0-fileinstaller",     )"
    R"(         "gw001": "file:///tmp/tests/testfiles/behind-gateway-info.json" )"
    R"(     } )"
    R"( } )";

const char* downloadSucceeded500 =
    R"({)"
    R"(  "download" : {)"
    R"(    "contoso-motor-1.0-updatemanifest.json" : {)"
    R"(      "resultCode" : 500,)"
    R"(      "extendedResultCode" : 0,)"
    R"(      "resultDetails" : "")"
    R"(    })"
    R"(  })"
    R"(})";

const char* downloadFailed12345 =
    R"({)"
    R"(  "download" : {)"
    R"(    "contoso-motor-1.0-updatemanifest.json" : {)"
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

const char* isInstalled_installed =
    R"({)"
    R"(  "isInstalled" : {)"
    R"(    "1.0" : {)"
    R"(      "resultCode" : 900,)"
    R"(      "extendedResultCode" : 0,)"
    R"(      "resultDetails" : "")"
    R"(    })"
    R"(  })"
    R"(})";

const char* isInstalled_notInstalled =
    R"({)"
    R"(  "isInstalled" : {)"
    R"(    "1.0" : {)"
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
