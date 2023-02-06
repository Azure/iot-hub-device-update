/**
 * @file workflow_utils_ut.cpp
 * @brief Unit Tests for workflow_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/parser_utils.h"
#include "aduc/result.h"
#include "aduc/workflow_utils.h"

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include <sstream>
#include <string>

// clang-format off

const char* action_parent_update =
    R"( {                       )"
    R"(     "workflow": {       )"
    R"(            "action": 3, )"
    R"(            "id": "dcb112da-bfc9-47b7-b7ed-617feba1e6c4" )"
    R"(        },   )"
    R"(        "updateManifest": "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"20.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/apt:1\",\"files\":[\"f483750ebb885d32c\"],\"handlerProperties\":{\"installedCriteria\":\"apt-update-tree-1.0\"}},{\"type\":\"reference\",\"detachedManifestFileId\":\"f222b9ffefaaac577\"}]},\"files\":{\"f483750ebb885d32c\":{\"fileName\":\"apt-manifest-tree-1.0.json\",\"sizeInBytes\":136,\"hashes\":{\"sha256\":\"Uk1vsEL/nT4btMngo0YSJjheOL2aqm6/EAFhzPb0rXs=\"}},\"f222b9ffefaaac577\":{\"fileName\":\"contoso.contoso-virtual-motors.1.1.updatemanifest.json\",\"sizeInBytes\":1031,\"hashes\":{\"sha256\":\"9Rnjw7ThZhGacOGn3uvvVq0ccQTHc/UFSL9khR2oKsc=\"}}},\"createdDateTime\":\"2022-01-27T13:45:05.8993329Z\"}",  )"
    R"(        "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJqSW12eGpsc2pqZ29JeUJuYThuZTk2d0RYYlVsU3N6eGFoM0NibkF6STFJPSJ9.PzpvU13h6VhN8VHXUTYKAlpDW5t3JaQ-gs895_Q10XshKPYpeZUtViXGHGC-aQSQAYPhhYV-lLia9niXzZz4Qs4ehwFLHJfkmKR8eRwWvoOgJtAY0IIUA_8SeShmoOc9cdpC35N3OeaM4hV9shxvvrphDib5sLpkrv3LQrt3DHvK_L2n0HsybC-pwS7MzaSUIYoU-fXwZo6x3z7IbSaSNwS0P-50qeV99Mc0AUSIvB26GjmjZ2gEH5R3YD9kp0DOrYvE5tIymVHPTqkmunv2OrjKu2UOhNj8Om3RoVzxIkVM89cVGb1u1yB2kxEmXogXPz64cKqQWm22tV-jalS4dAc_1p9A9sKzZ632HxnlavOBjTKDGFgM95gg8M5npXBP3QIvkwW3yervCukViRUKIm-ljpDmnBJsZTMx0uzTaAk5XgoCUCADuLLol8EXB-0V4m2w-6tV6kAzRiwkqw1PRrGqplf-gmfU7TuFlQ142-EZLU5rK_dAiQRXx-f7LxNH",  )"
    R"(        "fileUrls": {    )"
    R"(            "f483750ebb885d32c": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/e5cc19d5e9174c93ada35cc315f1fb1d/apt-manifest-tree-1.0.json",      )"
    R"(            "f222b9ffefaaac577": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/31c38c3340a84e38ae8d30ce340f4a49/contoso.contoso-virtual-motors.1.1.updatemanifest.json",  )"
    R"(            "f2c5d1f3b0295db0f": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/9ff068f7c2bf43eb9561da14a7cbcecd/motor-firmware-1.1.json",         )"
    R"(            "f13b5435aab7c18da": "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/c02058a476a242d7bc0e3c576c180051/contoso-motor-installscript.sh"   )"
    R"(        }    )"
    R"( } )";

const char* action_child_update_0 =
    R"( { "updateManifest":"{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"contoso\",\"name\":\"contoso-virtual-motors\",\"version\":\"1.1\"},\"compatibility\":[{\"group\":\"motors\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/script:1\",\"files\":[\"f13b5435aab7c18da\",\"f2c5d1f3b0295db0f\"],\"handlerProperties\":{\"scriptFileName\":\"contoso-motor-installscript.sh\",\"arguments\":\"--firmware-file motor-firmware-1.1.json --component-name --component-name-val --component-group --component-group-val --component-prop path --component-prop-val path\",\"installedCriteria\":\"contoso-contoso-virtual-motors-1.1-step-1\"}}]},\"files\":{\"f13b5435aab7c18da\":{\"fileName\":\"contoso-motor-installscript.sh\",\"sizeInBytes\":27030,\"hashes\":{\"sha256\":\"DYb4/+P3mq2yjq6n987msufTo3GUb5tpMtk+f7IeHx0=\"}},\"f2c5d1f3b0295db0f\":{\"fileName\":\"motor-firmware-1.1.json\",\"sizeInBytes\":123,\"hashes\":{\"sha256\":\"b8CC9E/93hUuMT19VjGVLDWGShq4GzpMYBO8vzlej74=\"}}},\"createdDateTime\":\"2022-01-27T13:45:05.8836909Z\"}"} )";

const char* action_no_update_action_data =
    R"( {                       )"
    R"(     "workflow": {       )"
    R"(            "id": "aaaaaaaa-bfc9-47b7-b7ed-617feba1e6c4" )"
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

// clang-format on

TEST_CASE("Initialization test")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_parent_update, false /* validateManifest */, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto action = workflow_get_action(handle);
    REQUIRE(action == ADUCITF_UpdateAction_ProcessDeployment);

    // Get and set id.
    char* id = workflow_get_id(handle);
    CHECK_THAT(id, Equals("dcb112da-bfc9-47b7-b7ed-617feba1e6c4"));

    workflow_free_string(id);

    workflow_set_id(handle, "new_id_1");
    id = workflow_get_id(handle);

    CHECK(id != nullptr);

    CHECK_THAT(id, Equals("new_id_1"));
    workflow_free_string(id);
    id = nullptr;

    auto filecount = workflow_get_update_files_count(handle);
    REQUIRE(filecount == 2);

    ADUC_FileEntity file0;
    memset(&file0, 0, sizeof(file0));
    bool success = workflow_get_update_file(handle, 0, &file0);
    REQUIRE(success);
    CHECK_THAT(file0.FileId, Equals("f483750ebb885d32c"));
    CHECK(file0.HashCount == 1);
    CHECK_THAT(
        file0.DownloadUri,
        Equals(
            "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/e5cc19d5e9174c93ada35cc315f1fb1d/apt-manifest-tree-1.0.json"));

    ADUC_FileEntity_Uninit(&file0);

    char* updateId = workflow_get_expected_update_id_string(handle);
    CHECK_THAT(updateId, Equals("{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"20.0\"}"));
    workflow_free_string(updateId);

    workflow_uninit(handle);

    // After free, 'action' should no longer valid.
    action = workflow_get_action(handle);
    CHECK(action < 0);

    workflow_free(handle);
}

TEST_CASE("Undefined update action")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_no_update_action_data, false /* validateManifest */, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto action = workflow_get_action(handle);
    REQUIRE(action == ADUCITF_UpdateAction_Undefined);

    workflow_free(handle);
}

TEST_CASE("Get Compatibility")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_parent_update, false /* validateManifest */, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    const char* expectedValue = R"([{"deviceManufacturer":"contoso","deviceModel":"virtual-vacuum-v1"}])";

    char* compats = workflow_get_compatibility(handle);
    CHECK_THAT(compats, Equals(expectedValue));

    workflow_free(handle);
}

// \"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"20.0\"}
TEST_CASE("Update Id")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_parent_update, false /* validateManifest */, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_UpdateId* updateId = nullptr;
    result = workflow_get_expected_update_id(handle, &updateId);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    REQUIRE_THAT(updateId->Name, Equals("Virtual-Vacuum"));
    REQUIRE_THAT(updateId->Provider, Equals("Contoso"));
    REQUIRE_THAT(updateId->Version, Equals("20.0"));

    workflow_free_update_id(updateId);

    workflow_free(handle);
}

TEST_CASE("Child workflow uses fileUrls from parent")
{
    ADUC_WorkflowHandle bundle = nullptr;
    ADUC_Result result = workflow_init(action_parent_update, false /* validateManifest */, &bundle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto filecount = workflow_get_update_files_count(bundle);
    REQUIRE(filecount == 2);

    ADUC_WorkflowHandle leaf0 = nullptr;
    result = workflow_init(action_child_update_0, false /* validateManifest */, &leaf0);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    workflow_insert_child(bundle, 0, leaf0);

    // Check that leaf 0 file has the right download uri.
    ADUC_FileEntity file0;
    memset(&file0, 0, sizeof(file0));
    bool success = workflow_get_update_file(leaf0, 0, &file0);
    REQUIRE(success);
    CHECK_THAT(file0.FileId, Equals("f13b5435aab7c18da"));
    CHECK(file0.HashCount == 1);
    CHECK_THAT(
        file0.DownloadUri,
        Equals(
            "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/c02058a476a242d7bc0e3c576c180051/contoso-motor-installscript.sh"));

    ADUC_FileEntity_Uninit(&file0);

    workflow_free(bundle);
}

TEST_CASE("Get update file by name")
{
    ADUC_WorkflowHandle bundle = nullptr;
    ADUC_Result result = workflow_init(action_parent_update, false /* validateManifest */, &bundle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto filecount = workflow_get_update_files_count(bundle);
    REQUIRE(filecount == 2);

    // Check that leaf 0 file has the right download uri.
    ADUC_FileEntity file0;
    memset(&file0, 0, sizeof(file0));
    bool success =
        workflow_get_update_file_by_name(bundle, "contoso.contoso-virtual-motors.1.1.updatemanifest.json", &file0);
    CHECK(success);
    CHECK_THAT(file0.FileId, Equals("f222b9ffefaaac577"));
    CHECK(file0.HashCount == 1);
    CHECK_THAT(
        file0.DownloadUri,
        Equals(
            "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/31c38c3340a84e38ae8d30ce340f4a49/contoso.contoso-virtual-motors.1.1.updatemanifest.json"));

    ADUC_FileEntity_Uninit(&file0);
    workflow_free(bundle);
}

TEST_CASE("Get update file by name - mixed case")
{
    ADUC_WorkflowHandle bundle = nullptr;
    ADUC_Result result = workflow_init(action_parent_update, false /* validateManifest */, &bundle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto filecount = workflow_get_update_files_count(bundle);
    REQUIRE(filecount == 2);

    // Check that leaf 0 file has the right download uri.
    ADUC_FileEntity file0;
    memset(&file0, 0, sizeof(file0));
    bool success =
        workflow_get_update_file_by_name(bundle, "contoso.Contoso-virtual-motors.1.1.updatemanifest.json", &file0);
    CHECK(success);
    CHECK_THAT(file0.FileId, Equals("f222b9ffefaaac577"));
    CHECK(file0.HashCount == 1);
    CHECK_THAT(
        file0.DownloadUri,
        Equals(
            "http://duinstance2.b.nlu.dl.adu.microsoft.com/westus2/duinstance2/31c38c3340a84e38ae8d30ce340f4a49/contoso.contoso-virtual-motors.1.1.updatemanifest.json"));

    ADUC_FileEntity_Uninit(&file0);
    workflow_free(bundle);
}

TEST_CASE("Add and remove children")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_parent_update, false /* validateManifest */, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto filecount = workflow_get_update_files_count(handle);
    REQUIRE(filecount == 2);

    ADUC_WorkflowHandle childWorkflow[12];

    char name[40];
    for (int i = 0; i < ARRAY_SIZE(childWorkflow); i++)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        result = workflow_init(action_child_update_0, false /* validateManifest */, &childWorkflow[i]);
        CHECK(result.ResultCode != 0);
        CHECK(result.ExtendedResultCode == 0);

        sprintf(name, "leaf%d", i);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        bool nameOk = workflow_set_id(childWorkflow[i], name);
        CHECK(nameOk);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        bool insertOk = workflow_insert_child(handle, -1, childWorkflow[i]);
        CHECK(insertOk);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        auto p = workflow_get_parent(childWorkflow[i]);
        CHECK(p == handle);

        int childCount = workflow_get_children_count(handle);
        CHECK(childCount == (i + 1));
    }

    // Remove child #5
    auto c5 = workflow_remove_child(handle, 5);
    CHECK(c5 != nullptr);

    auto c5_id = workflow_get_id(c5);
    CHECK_THAT(c5_id, Equals("leaf5"));
    workflow_free_string(c5_id);
    c5_id = nullptr;

    // parent should be nullptr.
    auto p5 = workflow_get_parent(c5);
    CHECK(p5 == nullptr);

    // Child #5 should be 'leaf6' now.
    auto c6 = workflow_get_child(handle, 5);
    auto c6_id = workflow_get_id(c6);
    CHECK_THAT(c6_id, Equals("leaf6"));
    workflow_free_string(c6_id);
    c6_id = nullptr;

    // Child cound should be 10.
    CHECK(11 == workflow_get_children_count(handle));

    // Add Child #5 at 0.
    workflow_insert_child(handle, 0, c5);
    // Child cound should be 11.
    CHECK(12 == workflow_get_children_count(handle));
    // parent should be 'handle'.
    p5 = workflow_get_parent(c5);
    CHECK(p5 == handle);

    auto c0 = workflow_get_child(handle, 0);
    CHECK(c0 == c5);

    workflow_free(handle);
}

TEST_CASE("Set workflow result")
{
    ADUC_WorkflowHandle bundle = nullptr;
    ADUC_Result result = workflow_init(action_parent_update, false /* validateManifest */, &bundle);
    workflow_set_id(bundle, "testWorkflow_001");
    workflow_set_workfolder(bundle, "/tmp/workflow_ut/testWorkflow_001");

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto filecount = workflow_get_update_files_count(bundle);
    REQUIRE(filecount == 2);

    ADUC_WorkflowHandle leaf0 = nullptr;
    result = workflow_init(action_child_update_0, false /* validateManifest */, &leaf0);
    workflow_set_id(leaf0, "testLeaf_0");
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    workflow_insert_child(bundle, 0, leaf0);

    // Test set result
    workflow_set_state(bundle, ADUCITF_State_DownloadStarted);

    CHECK(ADUCITF_State_DownloadStarted == workflow_get_root_state(leaf0));

    workflow_free(bundle);
}

// clang-format off
const char* manifest_1_0 =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 0, )"
    R"(         "id": "action_bundle" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"1.0\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00000\":{\"fileName\":\"contoso-motor-1.0-updatemanifest.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
    R"(     "updateManifestSignature": "" )"
    R"( } )";
// clang-format on

TEST_CASE("Get update manifest version - 1.0")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(manifest_1_0, false /* validateManifest */, &handle);
    CHECK(result.ResultCode > 0);
    int versionNumber = workflow_get_update_manifest_version(handle);
    CHECK(versionNumber == 1);
    workflow_free(handle);
}

// clang-format off
const char* manifest_2_0 =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 0, )"
    R"(         "id": "action_bundle" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"2.0\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00000\":{\"fileName\":\"contoso-motor-1.0-updatemanifest.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
    R"(     "updateManifestSignature": "" )"
    R"( } )";
// clang-format on

TEST_CASE("Get update manifest version - 2.0")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(manifest_2_0, false /* validateManifest */, &handle);
    CHECK(result.ResultCode > 0);
    int versionNumber = workflow_get_update_manifest_version(handle);
    CHECK(versionNumber == 2);
    workflow_free(handle);
}

// clang-format off
const char* manifest_2 =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 0, )"
    R"(         "id": "action_bundle" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"2\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00000\":{\"fileName\":\"contoso-motor-1.0-updatemanifest.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
    R"(     "updateManifestSignature": "" )"
    R"( } )";
// clang-format on

TEST_CASE("Get update manifest version - 2")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(manifest_2, false /* validateManifest */, &handle);
    CHECK(result.ResultCode > 0);
    int versionNumber = workflow_get_update_manifest_version(handle);
    CHECK(versionNumber == 2);
    workflow_free(handle);
}

// clang-format off
const char* manifest_3 =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 0, )"
    R"(         "id": "action_bundle" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"3\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00000\":{\"fileName\":\"contoso-motor-1.0-updatemanifest.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
    R"(     "updateManifestSignature": "" )"
    R"( } )";
// clang-format on

TEST_CASE("Get update manifest version - 3")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(manifest_3, false /* validateManifest */, &handle);
    CHECK(result.ResultCode > 0);
    int versionNumber = workflow_get_update_manifest_version(handle);
    CHECK(versionNumber == 3);
    workflow_free(handle);
}

// clang-format off
const char* manifest_x =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 0, )"
    R"(         "id": "action_bundle" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"x\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00000\":{\"fileName\":\"contoso-motor-1.0-updatemanifest.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
    R"(     "updateManifestSignature": "" )"
    R"( } )";
// clang-format on

TEST_CASE("Get update manifest version - x")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(manifest_x, false /* validateManifest */, &handle);
    CHECK(result.ResultCode > 0);

    // Non-number version will return 0.
    int versionNumber = workflow_get_update_manifest_version(handle);
    CHECK(versionNumber == 0);
    workflow_free(handle);
}

// clang-format off
const char* manifest_empty =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 0, )"
    R"(         "id": "action_bundle" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00000\":{\"fileName\":\"contoso-motor-1.0-updatemanifest.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
    R"(     "updateManifestSignature": "" )"
    R"( } )";
// clang-format on

TEST_CASE("Get update manifest version - empty")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(manifest_empty, false /* validateManifest */, &handle);
    CHECK(result.ResultCode > 0);
    int versionNumber = workflow_get_update_manifest_version(handle);
    CHECK(versionNumber == -1);
    workflow_free(handle);
}

// clang-format off
const char* manifest_missing_version =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 0, )"
    R"(         "id": "action_bundle" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00000\":{\"fileName\":\"contoso-motor-1.0-updatemanifest.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
    R"(     "updateManifestSignature": "" )"
    R"( } )";
// clang-format on

TEST_CASE("Get update manifest version - missing")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(manifest_missing_version, false /* validateManifest */, &handle);
    CHECK(result.ResultCode > 0);
    int versionNumber = workflow_get_update_manifest_version(handle);
    CHECK(versionNumber == -1);
    workflow_free(handle);
}

TEST_CASE("Mininum update manifest version check - 5")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_parent_update, false /* validateManifest */, &handle);
    CHECK(result.ResultCode > 0);
    int versionNumber = workflow_get_update_manifest_version(handle);
    CHECK(versionNumber == 5);
    workflow_free(handle);
}

// clang-format off
const char* manifest_old_1_0 =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 0, )"
    R"(         "id": "old_manifest" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"1.0\",\"updateId\":{\"manufacturer\":\"Microsoft\",\"name\":\"OldManifest\",\"version\":\"1.0\"},\"updateType\":\"microsoft/swupdate:1\",\"installedCriteria\":\"1.0\",\"files\":{\"fec176c1cb389b2e4\":{\"fileName\":\"aduperftest-xsmall-5KB.json\",\"sizeInBytes\":4328,\"hashes\":{\"sha256\":\"RmjGU92yWs3H91/nGekzu9zvq0bFTHMnFMJI3A+YEOY=\"}}},\"createdDateTime\":\"2021-09-20T22:09:03.648Z\"}", )"
    R"(     "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpNVVJbjAuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pZVdaNVpUZ3pRMFl2TVZKYVUybHhaMFZ4UldJNWVIY3ZTeTlUSzFGa1pHaFRSbWsxYjNSbU9TdFJNMlV2TTJOVlJWbzJkRVkyZEhOQ2FITm9UbGxJUjBWV1dpOUlVRTh3Tms5WmNVUm1NMEZ6ZEdaQlZERmFPV0k1VmxORWEzWkxTR1ZKZGtGR01qVllNM1Z4YmpKeFJFZ3dkM0ZIUzBsM1EwOHlhV1J0ZVRWeFRVeHBaVFJHT0ZacVJXZFlTMlJRUzBnemNrdHpRbFpqTXpOaVVrUkhhVlVyYW05M1QyOVNkVkJYVXpVM1FYbGlOR3BIVTBOemRIUTVaSGxLTmpsQk1YZDZUMXBLUTNvell6TktPRU16TWs5aVVuaDNTM0IwY0dOUVlVZFBWbk4zWjNVM1pEUkhjMm93T0dsbFR6a3pZaTl4ZEROc1dWbzBVbGRTYWpsMk1uVXhjV1ZxTjBkcE9YaFpSWE5LVTJGeFExbzJjVWxOVDNNM1NDOTFjMVZPYVM5VmFscGtPR280ZVVwUmRtbEhVVXMzU200MFZsRXJUM2xLZEVNMU5uRlBTazFGWmxwMU1HczJhMWRGUkdGTWJWTndXa3hrU205bWMzWlRPV2hvZEU5WmJYZHpaVEZJV1VSUmRqVlJhVFlyTWxnME9GcE1TSE5CYkZocVkxcDVPRWRJYkRsclJVWm1aWFY1U21WeVJXOW1jM2gxV1dKcVEyVnRWV1pzWmpBeVYzbzJXR3RtYjNCMFFXbHJkSEJWTWtzdlZWZExVQzkwTkZWcU5uQjBjVGRXYkRGdmN6TXlha1IwZVRsaVVYZExZbmh6ZEVKM05IWXhLMEpFVG1Vd2RVcFhVSGcwYVRGRFEyaDVLekE1Tm1sVFJtRkdTazFQVmswcmRHWnVNbW9pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxOVUluMC5lTFJ2N21TVEdycFpPcXJUM0NTX0VXSkFEdzE4UUxzM0lzMUlnSHFKS0pLRTFEVlFxdEEyS3ZBaEJlV3VMVkVKMGplNXA5ZUsyejFya1YzaHJFMFRGc0NRU05JSXFOWTVpMU9pNDNWbTlkelFWVFhHcUVUWUFfNVV4SzBqYWhBRE5zOHdETDFBMTlBTDc5SS1NaUlYVXZFeUtWNnliUnVDR3NucExUV1RMYWRNaTlMNzB4VXVpUjUzSVhsVmFFZ0psMWRwSktkUWd4NjdOMTFFME1VUGVWWEVPZmI3am1lQ2V3TkxzeF9WOUNqREtZcmF6ckhYV2pETVh0T0NOZW5RMHNvSnhiVUNDTmdWTTQtMl8wVXljVC1uN09YeWNSQldRUFctbHV6M0xNekNEMHRPRF9qV1oxUDdFNEQzTnIwRHVPb2lMVklSMGd3TWh5ZTRIaEN3RURMOWFaTlZEUTExX0ZYX2tFZnlybzcwVUtYVGFCNGJLX0EwTy12ZThxd1NqRGJYVWZxZThIZnRxTFFJSE9hSE56T2M4OG9qLWowRF9oREhfNF9oTlFrdTNhaGlKa0hpcjZwNWNDRTlPd2pheU8wUXNYUmo4U2lWYV9BU1hvVUJ2RUdVLU1KVTlNa3ZCeE9HWnVIeXNnRVhLYlpFQ24zWG50c29rTUVaMzlLVSJ9.eyJzaGEyNTYiOiJBRDFtZmhwS1JUWjJiTUVQNFhoRzJ3QVVvZ2dOKzVPeGtybzlzUHlpbVVZPSJ9.sdoYZxDuBPkvdN-U362smwm4CqYXQQ2NVt1zAlTyGQ4G6PTYQ2xIHJtW_QeKj5lbnjvSRV3yAaYVymwID_zFyCLf_lpkbq5Mkf2eO5LdU6Ske0s_Nzj98rZP2Io10B6zIcTLE9Rh_NWJyc3PCdIXv6k4sdkL3J2ioc6i8kUAtjwsyoF_-nv1xdEtlajNkxneaX8iOAGAmaM-NdVR6yHfXAAHoJHYEtfRqGw_z2ETG4wSEyuWsoLRgJPNbku9HqpJAQgo76dH0h6N97SY3unDJcVUW8St6V2uu7_ov1I5I_RQ1JQ1UaNPMYPdw48n3arkPsMQLZZrZ5HQg2cOvJdF_kLe6h0KtknLtwlk5r3K_jsUSRRzg3IZGcgh_Uje5s9EX3AM_S_iUshXENDSG6MRKH1u8pTl2Udzc_gkqybfFHLg0rymML-IDitHaEBhBIdvlZg-OIsmJPAQ8WHU4byFOfjGCCTf-rfoxbjS-s182U0QP0NHmRHmj7KVb_ds_WOY" )"
    R"( } )";
// clang-format on

TEST_CASE("Mininum update manifest version check - 1")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(manifest_old_1_0, true /* validateManifest */, &handle);
    CHECK(result.ResultCode == 0);
    CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_UNSUPPORTED_UPDATE_MANIFEST_VERSION);
    workflow_free(handle);
}

// clang-format off
#define WORKFLOW_ID_COMPARE_0_UUID "aaaaaaaa-bfc9-47b7-b7ed-617feba1e6c4"
const char* manifest_workflow_id_compare_0 =
    R"( {                       )"
    R"(     "workflow": {       )"
    R"(            "action": 3, )"
    R"(            "id": "aaaaaaaa-bfc9-47b7-b7ed-617feba1e6c4" )"
    R"(        },   )"
    R"(        "updateManifest": "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"20.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/apt:1\",\"files\":[\"f483750ebb885d32c\"],\"handlerProperties\":{\"installedCriteria\":\"apt-update-tree-1.0\"}},{\"type\":\"reference\",\"detachedManifestFileId\":\"f222b9ffefaaac577\"}]},\"files\":{\"f483750ebb885d32c\":{\"fileName\":\"apt-manifest-tree-1.0.json\",\"sizeInBytes\":136,\"hashes\":{\"sha256\":\"Uk1vsEL/nT4btMngo0YSJjheOL2aqm6/EAFhzPb0rXs=\"}},\"f222b9ffefaaac577\":{\"fileName\":\"contoso.contoso-virtual-motors.1.1.updatemanifest.json\",\"sizeInBytes\":1031,\"hashes\":{\"sha256\":\"9Rnjw7ThZhGacOGn3uvvVq0ccQTHc/UFSL9khR2oKsc=\"}}},\"createdDateTime\":\"2022-01-27T13:45:05.8993329Z\"}",  )"
    R"(        "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJqSW12eGpsc2pqZ29JeUJuYThuZTk2d0RYYlVsU3N6eGFoM0NibkF6STFJPSJ9.PzpvU13h6VhN8VHXUTYKAlpDW5t3JaQ-gs895_Q10XshKPYpeZUtViXGHGC-aQSQAYPhhYV-lLia9niXzZz4Qs4ehwFLHJfkmKR8eRwWvoOgJtAY0IIUA_8SeShmoOc9cdpC35N3OeaM4hV9shxvvrphDib5sLpkrv3LQrt3DHvK_L2n0HsybC-pwS7MzaSUIYoU-fXwZo6x3z7IbSaSNwS0P-50qeV99Mc0AUSIvB26GjmjZ2gEH5R3YD9kp0DOrYvE5tIymVHPTqkmunv2OrjKu2UOhNj8Om3RoVzxIkVM89cVGb1u1yB2kxEmXogXPz64cKqQWm22tV-jalS4dAc_1p9A9sKzZ632HxnlavOBjTKDGFgM95gg8M5npXBP3QIvkwW3yervCukViRUKIm-ljpDmnBJsZTMx0uzTaAk5XgoCUCADuLLol8EXB-0V4m2w-6tV6kAzRiwkqw1PRrGqplf-gmfU7TuFlQ142-EZLU5rK_dAiQRXx-f7LxNH",  )"
    R"(        "fileUrls": {    )"
    R"(            "f483750ebb885d32c": "http://some_host/path/to/e5cc19d5e9174c93ada35cc315f1fb1d/apt-manifest-tree-1.0.json",      )"
    R"(            "f222b9ffefaaac577": "http://some_host/path/to/31c38c3340a84e38ae8d30ce340f4a49/contoso.contoso-virtual-motors.1.1.updatemanifest.json",  )"
    R"(            "f2c5d1f3b0295db0f": "http://some_host/path/to/9ff068f7c2bf43eb9561da14a7cbcecd/motor-firmware-1.1.json",         )"
    R"(            "f13b5435aab7c18da": "http://some_host/path/to/c02058a476a242d7bc0e3c576c180051/contoso-motor-installscript.sh"   )"
    R"(        }    )"
    R"( } )";

#define WORKFLOW_ID_COMPARE_1_UUID "dcb112da-bfc9-47b7-b7ed-617feba1e6c4"
const char* manifest_workflow_id_compare_1 =
    R"( {                       )"
    R"(     "workflow": {       )"
    R"(            "action": 3, )"
    R"(            "id": "dcb112da-bfc9-47b7-b7ed-617feba1e6c4" )"
    R"(        },   )"
    R"(        "updateManifest": "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Virtual-Vacuum\",\"version\":\"20.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/apt:1\",\"files\":[\"f483750ebb885d32c\"],\"handlerProperties\":{\"installedCriteria\":\"apt-update-tree-1.0\"}},{\"type\":\"reference\",\"detachedManifestFileId\":\"f222b9ffefaaac577\"}]},\"files\":{\"f483750ebb885d32c\":{\"fileName\":\"apt-manifest-tree-1.0.json\",\"sizeInBytes\":136,\"hashes\":{\"sha256\":\"Uk1vsEL/nT4btMngo0YSJjheOL2aqm6/EAFhzPb0rXs=\"}},\"f222b9ffefaaac577\":{\"fileName\":\"contoso.contoso-virtual-motors.1.1.updatemanifest.json\",\"sizeInBytes\":1031,\"hashes\":{\"sha256\":\"9Rnjw7ThZhGacOGn3uvvVq0ccQTHc/UFSL9khR2oKsc=\"}}},\"createdDateTime\":\"2022-01-27T13:45:05.8993329Z\"}",  )"
    R"(        "updateManifestSignature": "somesignature",  )"
    R"(        "fileUrls": {    )"
    R"(            "f483750ebb885d32c": "http://some_host.com/path/to/e5cc19d5e9174c93ada35cc315f1fb1d/apt-manifest-tree-1.0.json",      )"
    R"(            "f222b9ffefaaac577": "http://some_host.com/path/to/31c38c3340a84e38ae8d30ce340f4a49/contoso.contoso-virtual-motors.1.1.updatemanifest.json",  )"
    R"(            "f2c5d1f3b0295db0f": "http://some_host.com/path/to/9ff068f7c2bf43eb9561da14a7cbcecd/motor-firmware-1.1.json",         )"
    R"(            "f13b5435aab7c18da": "http://some_host.com/path/to/c02058a476a242d7bc0e3c576c180051/contoso-motor-installscript.sh"   )"
    R"(        }    )"
    R"( } )";

// clang-format on

TEST_CASE("workflow_id_compare")
{
    ADUC_WorkflowHandle handle0 = nullptr;
    ADUC_WorkflowHandle handle1 = nullptr;
    ADUC_WorkflowHandle handleNull = nullptr;

    ADUC_Result result = workflow_init(manifest_workflow_id_compare_0, false /* validateManifest */, &handle0);
    REQUIRE(result.ResultCode > 0);

    result = workflow_init(manifest_workflow_id_compare_1, false /* validateManifest */, &handle1);
    REQUIRE(result.ResultCode > 0);

    // different
    CHECK(workflow_id_compare(handle0, handle1) != 0);

    // same
    CHECK(workflow_id_compare(handle0, handle0) == 0);

    workflow_free(handle0);
    workflow_free(handle1);
}

TEST_CASE("workflow_isequal_id")
{
    ADUC_WorkflowHandle handle0 = nullptr;

    ADUC_Result result = workflow_init(manifest_workflow_id_compare_0, false /* validateManifest */, &handle0);
    REQUIRE(result.ResultCode > 0);

    // NULL
    CHECK_FALSE(workflow_isequal_id(handle0, nullptr));

    // different
    CHECK_FALSE(workflow_isequal_id(handle0, WORKFLOW_ID_COMPARE_1_UUID));

    // same
    CHECK(workflow_isequal_id(handle0, WORKFLOW_ID_COMPARE_0_UUID));

    workflow_free(handle0);
}

TEST_CASE("result success erc")
{
    SECTION("Not set is zero")
    {
        ADUC_WorkflowHandle h = nullptr;

        ADUC_Result result = workflow_init(manifest_workflow_id_compare_0, false /* validateManifest */, &h);
        REQUIRE(result.ResultCode == ADUC_Result_Success);

        ADUC_Result_t erc = workflow_get_success_erc(h);
        CHECK(erc == 0);
    }
    SECTION("set and get")
    {
        ADUC_WorkflowHandle h = nullptr;

        ADUC_Result result = workflow_init(manifest_workflow_id_compare_0, false /* validateManifest */, &h);
        REQUIRE(result.ResultCode == ADUC_Result_Success);

        workflow_set_success_erc(h, ADUC_ERC_NOMEM);
        ADUC_Result_t erc = workflow_get_success_erc(h);
        CHECK(erc == ADUC_ERC_NOMEM);
    }
}

TEST_CASE("Request workflow cancellation")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_parent_update, false /* validateManifest */, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_WorkflowHandle childWorkflow[3];

    char name[40];
    for (int i = 0; i < ARRAY_SIZE(childWorkflow); i++)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        result = workflow_init(action_child_update_0, false /* validateManifest */, &childWorkflow[i]);
        CHECK(result.ResultCode != 0);
        CHECK(result.ExtendedResultCode == 0);

        sprintf(name, "leaf%d", i);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        bool nameOk = workflow_set_id(childWorkflow[i], name);
        CHECK(nameOk);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        bool insertOk = workflow_insert_child(handle, -1, childWorkflow[i]);
        CHECK(insertOk);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        auto p = workflow_get_parent(childWorkflow[i]);
        CHECK(p == handle);

        int childCount = workflow_get_children_count(handle);
        CHECK(childCount == (i + 1));

        CHECK(!workflow_is_cancel_requested(childWorkflow[i]));
    }

    // Rquest cancel on parent and children
    CHECK(!workflow_is_cancel_requested(handle));
    CHECK(workflow_request_cancel(handle));
    CHECK(workflow_is_cancel_requested(handle));

    for (int i = 0; i < ARRAY_SIZE(childWorkflow); i++)
    {
        CHECK(workflow_is_cancel_requested(childWorkflow[i]));
    }

    for (int i = ARRAY_SIZE(childWorkflow) - 1; i >= 0; i--)
    {
        workflow_remove_child(handle, i);
        workflow_free(childWorkflow[i]);
    }

    workflow_free(handle);
}
