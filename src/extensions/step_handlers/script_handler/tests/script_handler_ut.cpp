/**
 * @file script_handler_ut.cpp
 * @brief Unit Tests for Script Update Handler.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/parser_utils.h"
#include "aduc/script_handler.hpp"
#include "aduc/system_utils.h"
#include "aduc/workflow_utils.h"

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include <sstream>
#include <string>

/* Example of an Action PnP Data.
{
    "action": 0,
    "updateManifest": "{\"manifestVersion\":\"2.0\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00000\":{\"fileName\":\"contoso-motor-1.0-updatemanifest.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",
    "updateManifestSignature": "...",
    "fileUrls": {
        "00000": "file:///home/nox/tests/testfiles/contoso-motor-1.0-updatemanifest.json",
        "00001": "file:///home/nox/tests/testfiles/contoso-motor-1.0-installscript.sh",
        "00002": "file:///home/nox/tests/testfiles/component.json",
        "00003": "file:///home/nox/tests/testfiles/contoso-motor-1.0-instruction.json"
    }
*/

// clang-format off

const char* action_bundle =
R"( { )"
R"(     "action": 0, )"
R"(     "updateManifest": "{\"manifestVersion\":\"2.0\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00000\":{\"fileName\":\"contoso-motor-1.0-updatemanifest.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
R"(     "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pY2toV1FrVkdTMUl4ZG5Ob1p5dEJhRWxuTDFORVVVOHplRFJyYWpORFZWUTNaa2R1U21oQmJYVkVhSFpJWm1velowaDZhVEJVTWtsQmNVTXhlREpDUTFka1QyODFkamgwZFcxeFVtb3ZibGx3WnprM2FtcFFRMHQxWTJSUE5tMHpOMlJqVDIxaE5EWm9OMDh3YTBod2Qwd3pibFZJUjBWeVNqVkVRUzloY0ZsdWQwVmxjMlY0VkdwVU9GTndMeXRpVkhGWFJXMTZaMFF6TjNCbVpFdGhjV3AwU0V4SFZtbFpkMVpJVUhwMFFtRmlkM2RxYUVGMmVubFNXUzk1T1U5bWJYcEVabGh0Y2xreGNtOHZLekpvUlhGRmVXdDFhbmRSUlZscmFHcEtZU3RDTkRjMkt6QnRkVWQ1VjBrMVpVbDJMMjlzZERKU1pWaDRUV0k1VFd4c1dFNTViMUF6WVU1TFNVcHBZbHBOY3pkMVMyTnBkMnQ1YVZWSllWbGpUV3B6T1drdlVrVjVLMnhOT1haSlduRnlabkJEVlZoMU0zUnVNVXRuWXpKUmN5OVVaRGgwVGxSRFIxWTJkM1JXWVhGcFNYQlVaRlEwVW5KRFpFMXZUelZUVG1WbVprUjVZekpzUXpkMU9EVXJiMjFVYTJOcVVHcHRObVpoY0dSSmVVWXljV1Z0ZGxOQ1JHWkNOMk5oYWpWRVNVa3lOVmQzTlVWS1kyRjJabmxRTlRSdGNVNVJVVE5IWTAxUllqSmtaMmhwWTJ4d2FsbHZLelF6V21kWlEyUkhkR0ZhWkRKRlpreGFkMGd6VVdjeWNrUnNabXN2YVdFd0x6RjVjV2xyTDFoYU1XNXpXbFJwTUVKak5VTndUMDFGY1daT1NrWlJhek5DVjI5Qk1EVnlRMW9pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJd01EY3dNaTVTTGxNaWZRLmlTVGdBRUJYc2Q3QUFOa1FNa2FHLUZBVjZRT0dVRXV4dUhnMllmU3VXaHRZWHFicE0takk1UlZMS2VzU0xDZWhLLWxSQzl4Ni1fTGV5eE5oMURPRmMtRmE2b0NFR3dVajh6aU9GX0FUNnM2RU9tY2txUHJ4dXZDV3R5WWtrRFJGNzRkdGFLMWpOQTdTZFhyWnp2V0NzTXFPVU1OejBnQ29WUjBDczEyNTRrRk1SbVJQVmZFY2pnVDdqNGxDcHlEdVdncjlTZW5TZXFnS0xZeGphYUcwc1JoOWNkaTJkS3J3Z2FOYXFBYkhtQ3JyaHhTUENUQnpXTUV4WnJMWXp1ZEVvZnlZSGlWVlJoU0pwajBPUTE4ZWN1NERQWFYxVGN0MXkzazdMTGlvN244aXpLdXEybTNUeEY5dlBkcWI5TlA2U2M5LW15YXB0cGJGcEhlRmtVTC1GNXl0bF9VQkZLcHdOOUNMNHdwNnlaLWpkWE5hZ3JtVV9xTDFDeVh3MW9tTkNnVG1KRjNHZDNseXFLSEhEZXJEcy1NUnBtS2p3U3dwWkNRSkdEUmNSb3ZXeUwxMnZqdzNMQkpNaG1VeHNFZEJhWlA1d0dkc2ZEOGxkS1lGVkZFY1owb3JNTnJVa1NNQWw2cEl4dGVmRVhpeTVscW1pUHpxX0xKMWVSSXJxWTBfIn0.eyJzaGEyNTYiOiI3alo1YWpFN2Z5SWpzcTlBbWlKNmlaQlNxYUw1bkUxNXZkL0puVWgwNFhZPSJ9.EK5zcNiEgO2rHh_ichQWlDIvkIsPXrPMQK-0D5WK8ZnOR5oJdwhwhdpgBaB-tE-6QxQB1PKurbC2BtiGL8HI1DgQtL8Fq_2ASRfzgNtrtpp6rBiLRynJuWCy7drgM6g8WoSh8Utdxsx5lnGgAVAU67ijK0ITd0E70R7vWJRmY8YxxDh-Sh8BNz68pvU-YJQwKtVy64lD5zA0--BL432F-uZWTc6n-BduQdSB4J7Eu6zGlT75s8Ehd-SIylsstu4wdypU0tcwIH-MaSKcH5mgEmokaHncJrb4zKnZwxYQUeDMoFjF39P9hDmheHywY1gwYziXjUcnMn8_T00oMeycQ7PDCTJHIYB3PGbtM9KiA3RQH-08ofqiCVgOLeqbUHTP03Z0Cx3e02LzTgP8_Lerr4okAUPksT2IGvvsiMtj04asdrLSlv-AvFud-9U0a2mJEWcosI04Q5NAbqhZ5ZBzCkkowLGofS04SnfS-VssBfmbH5ue5SWb-AxBv1inZWUj", )"
R"(     "workflow": {   )"
R"(         "id": "action_bundle", )"
R"(         "action": 1 )"
R"(     }, )"
R"(     "fileUrls": {   )"
R"(         "00000": "file:///tmp/tests/testfiles/contoso-motor-1.0-updatemanifest.json",  )"
R"(         "00001": "file:///tmp/tests/testfiles/contoso-motor-1.0-installscript.sh",     )"
R"(         "gw001": "file:///tmp/tests/testfiles/behind-gateway-info.json" )"
R"(     } )"
R"( } )";

const char* action_leaf0 =
R"( { )"
R"(     "updateManifest": "{\"manifestVersion\":\"2.0\",\"updateId\":{\"provider\":\"contoso\",\"name\":\"motorUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00001\":{\"fileName\":\"contoso-motor-1.0-installscript.sh\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
R"(     "fileUrls": {   )"
R"(     } )"
R"( } )";

const char* action_leaf0_0 =
R"( { )"
R"(     "updateManifest": "{\"manifestVersion\":\"2.0\",\"updateId\":{\"provider\":\"fabrikam\",\"name\":\"peripheral-001-update\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"gw001\":{\"fileName\":\"behind-gateway-info.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
R"(     "fileUrls": {   )"
R"(     } )"
R"( } )";


const char* leaf0_instruction_installItems[] = {

    R"( { )"
    R"(     "updateType": "microsoft/script:1", )"
    R"(     "files": [ )"
    R"(         { )"
    R"(             "fileName": "contoso-motor-1.0-installscript.sh", )"
    R"(             "arguments": "--pre-install --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path", )"
    R"(             "fileType": "script" )"
    R"(         } )"
    R"(     ] )"
    R"( } )",

    R"( { )"
    R"(     "updateType": "microsoft/script:1", )"
    R"(     "files": [ )"
    R"(         { )"
    R"(             "fileName": "contoso-motor-1.0-installscript.sh", )"
    R"(             "arguments": "--post-install --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path --component-manufacturer --component-manufacturer-value --component-model --component-model-value --component-version --component-version-val", )"
    R"(             "fileType": "script" )"
    R"(         } )"
    R"(     ] )"
    R"( } )",

};

const char* selectedComponent_1 =
R"( { )"
R"(     "components" : [ )"
R"(     { )"
R"(         "id" : "contoso-mortor-serial-00001", )"
R"(         "name" : "left-motor", )"
R"(         "group" : "Motors", )"
R"(         "manufacturer" : "Contoso", )"
R"(         "model" : "Virtual-Motor", )"
R"(         "properties" : { )"
R"(             "path" : "/tmp/virtual-adu-device/motors/contoso-mortor-serial-00001" )"
R"(         } )"
R"(     } )"
R"(     ] )"
R"( } )"
;

// clang-format on

TEST_CASE("Install helper test")
{
    // Init bundle workflow.
    ADUC_WorkflowHandle bundle = nullptr;
    ADUC_Result result = workflow_init(action_bundle, false, &bundle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto filecount = workflow_get_update_files_count(bundle);
    REQUIRE(filecount == 1);

    // Set workfolder.
    std::string bundleWorkfolder = ADUC_SystemUtils_GetTemporaryPathName();
    bundleWorkfolder += "/adu_script_handle_tests/sandbox1/bundle";
    workflow_set_workfolder(bundle, bundleWorkfolder.c_str());

    // Init leaf workflow
    ADUC_WorkflowHandle leaf0 = nullptr;
    result = workflow_init(action_leaf0, false, &leaf0);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    // Set workfolder.
    std::string componentSandbox = ADUC_SystemUtils_GetTemporaryPathName();
    componentSandbox += "/adu_script_handle_tests/sandbox1/bundle/leaf-0";
    workflow_set_workfolder(leaf0, componentSandbox.c_str());

    workflow_insert_child(bundle, 0, leaf0);

    // Set selected components.
    bool selectCompOk = workflow_set_selected_components(leaf0, selectedComponent_1);
    CHECK(selectCompOk);

    //
    // Create component instance workflow.
    //
    ADUC_WorkflowHandle leaf0_instance_0 = nullptr;
    result = workflow_create_from_instruction(leaf0, leaf0_instruction_installItems[0], &leaf0_instance_0);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);
    CHECK(leaf0_instance_0 != nullptr);

    // Insert file to a tree to inherite fileUrls list.
    workflow_insert_child(leaf0, 0, leaf0_instance_0);

    // Set workfolder (this should be the same as parent's)
    char* parentWorkfolder = workflow_get_workfolder(workflow_get_parent(leaf0_instance_0));
    CHECK(parentWorkfolder != nullptr);
    CHECK_THAT(parentWorkfolder, Equals(componentSandbox));

    workflow_set_workfolder(leaf0_instance_0, parentWorkfolder);

    workflow_free_string(parentWorkfolder);
    parentWorkfolder = nullptr;

    int fileCount = workflow_get_update_files_count(leaf0_instance_0);
    CHECK(fileCount == 1);

    // Set selected components.
    selectCompOk = workflow_set_selected_components(leaf0_instance_0, selectedComponent_1);
    CHECK(selectCompOk);

    // Test script handler PrepareScriptArguments function.
    std::string scriptFilePath;
    std::vector<std::string> args;
    std::string resultFilePath = componentSandbox + "/adu-result.json";
    result = ScriptHandlerImpl::PrepareScriptArguments(
        leaf0_instance_0, resultFilePath, componentSandbox, scriptFilePath, args);

    std::string expectedPath = componentSandbox + "/contoso-motor-1.0-installscript.sh";
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);
    CHECK_THAT(scriptFilePath.c_str(), Equals(expectedPath));

    // Expecting 14 arguments. 8 specified in instruction, plus 6 defaults arguments added automatically:
    // --work-folder <value> --result-file <value> --installed-criteria <value>
    CHECK(args.size() == 14);

    /*
        arguments:
            "--pre-install --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path"

        components:

            const char* selectedComponent_1 =
            R"( { )"
            R"(     "components" : [ )"
            R"(     { )"
            R"(         "id" : "contoso-mortor-serial-00001", )"
            R"(         "name" : "left-motor", )"
            R"(         "group" : "Motors", )"
            R"(         "manufacturer" : "Contoso", )"
            R"(         "model" : "Virtual-Motor", )"
            R"(         "properties" : { )"
            R"(             "path" : "/tmp/virtual-adu-device/motors/contoso-mortor-serial-00001" )"
            R"(         } )"
            R"(     } )"
            R"(     ] )"
            R"( } )"
    */

    CHECK_THAT(args[0].c_str(), Equals("--pre-install"));
    CHECK_THAT(args[1].c_str(), Equals("--component-name"));
    CHECK_THAT(args[2].c_str(), Equals("left-motor"));
    CHECK_THAT(args[3].c_str(), Equals("--component-group"));
    CHECK_THAT(args[4].c_str(), Equals("Motors"));
    CHECK_THAT(args[5].c_str(), Equals("--component-prop"));
    CHECK_THAT(args[6].c_str(), Equals("path"));
    CHECK_THAT(args[7].c_str(), Equals("/tmp/virtual-adu-device/motors/contoso-mortor-serial-00001"));

    workflow_free(bundle);
}
