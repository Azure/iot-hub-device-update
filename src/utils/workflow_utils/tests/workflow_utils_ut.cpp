/**
 * @file workflow_utils_ut.cpp
 * @brief Unit Tests for workflow_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/parser_utils.h"
#include "aduc/workflow_utils.h"

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include <sstream>
#include <string>

/* Example of an Action PnP Data.
{
    "workflow": {
        "action": 3,
        "id": "someWorkflowId"
    }
    "updateManifest": "{\"manifestVersion\":\"2.0\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"files\":{\"00000\":{\"fileName\":\"contoso-motor-1.0-updatemanifest.json\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",
    "updateManifestSignature": "...",
    "fileUrls": {
        "00000": "file:///home/nox/tests/testfiles/contoso-motor-1.0-updatemanifest.json",
        "00001": "file:///home/nox/tests/testfiles/contoso-motor-1.0-fileinstaller",
        "00002": "file:///home/nox/tests/testfiles/component.json",
        "00003": "file:///home/nox/tests/testfiles/contoso-motor-1.0-instruction.json"
    }
*/

// clang-format off

const char* bundle_with_bundledUpdates =
    R"( {                    )"
    R"(     "workflow": {    )"
    R"(         "action": 3, )"
    R"(         "id": "1533dab9-183c-47b7-aabf-a076fd5ea74f" )"
    R"(      },  )"
    R"(     "updateManifest": "{\"manifestVersion\":\"3\",\"updateId\":{\"provider\":\"contoso\",\"name\":\"virtual-vacuum\",\"version\":\"1.1\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"v1\",\"compatibility\":[{\"DeviceManufacturer\":\"contoso\",\"DeviceModel\":\"virtual-vacuum\"}],\"bundledUpdates\":[{\"fileId\":\"c7f95b5a4b0b328a\",\"fileName\":\"contoso.virtual-motor.1.1.updatemanifest.json\",\"sizeInBytes\":800,\"hashes\":{\"sha256\":\"KBJ8BKKZn3c1/Yo4sslPiiHVqCAk+aFfHBg8uNuTjLs=\"}}],\"createdDateTime\":\"2021-06-24T01:51:55.5872972Z\"}", )"
    R"(     "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpNVVJbjAuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pZGtwYWJGRjFjM1J3T1RsUFRpOXJXWEJ4TlVveFRtdG1hMGxDTjNrdldVbzJNbWRrVjJWRFlteE5XWGt3WjNRd1VVVjZXVGR4TjI1UFJHMDJTbXQxYldoTVpuTTNaa1JCVTA4elJIaG5SVlE0Y1dkWmQyNVdhMFpsTVVkVVowZEVXR28xVTFGQ2VIQkxVRWxQTlVRclZ6TXdSSEpKTmxGSk4xRm1hVFpRWmxaT2FsVlVTRkI2U0VaNWFFMVRUVnByYWxreGVFeFBNMDFSWjBobmRXUjFNV1pzU0hGREszUmlhbms1WTI0emVtcDBNM2hUVWpKbU9URkRVa0p5VWxRelJ6Vm1WbFkxVjA1U1EwOWlWVU51YUdNeWFHaHlSRU15V0dreFFWaHliazlRZDNWRFJFbEZkSFIzYjBzd1QyNVFjV3BLVHk5SVVqRjJMelJPY3l0S1YwOU1VRTU0ZW1aRlNqSnBXVTlSWkdwbFpteEJPRVoyVFhWMGVYVndaVXRwUTJsclYzY3lOWFpwYjAxUldWSXdiMVozUjFwMU16aG1PSFJvTVVoRlZFSjJkVVZWTDFkeGVIQllXbVJQUm5wa09VTkhabVZsTW5FMGIzcFFNQzlQVUUxRWQxQlRNRzVZWm0xYVVHUlhhWHBwVDNWRlUwSndVRko1TjFwcFJYSjVSbVpNWjJnemNVWkVXQ3RYZDB3eFdXWmxaRzU1V1N0cFVWWjNiaXRpVTNWalVHeFRlRXdyUmxJNVNuZFFaRGhNZUU1bmFHUnZZbTFCTjA1MmNXSkNaakp6Wld0dE1HUkpTa0pRTm5wc2MxQmxURU5ZVW1VMU1IbE9SVk5rYUhWRlFuVnhNV1k0Y1ZZek1VVklSVmx6V0dRMGMwWm5WMVZ2Wm5kM1dIaFBTM0FpTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJd01EY3dNaTVTTGxOVUluMC5RS0ktckVJSXBIZTRPZVd1dkN4cFNYam1wbC05czNlN1d2ZGRPcFNnR2dJaUN5T1hPYThjSFdlbnpzSFVMTUZBZEF0WVBnM2ZVTnl3QUNJOEI2U044UGhUc3E5TFg4QjR2c0l2QlZ2S2FrTU1ucjRuZXRlVHN5V0NJbEhBeG4ydThzSlQ3amVzMmtFQ2ZrNnpBeS16cElSSXhYaF8yVEl5Y2RNLVU0UmJYVDhwSGpPUTVfZTRnSFRCZjRlODRaWTdsUEw0RGtTa3haR1dnaERhOTFGVjlXbjRtc050eDdhMEJ3NEtTMURVd1c2eXE0c2diTDFncVl6VG04bENaSDJOb1pDdjZZUUd4YVE5UkJ1ek5Eb0VMb0RXXzB4S3V1VzFaa3UxSDN6c3lZNlNJMVp0bkFkdDAxcmxIck5fTTlKdTBZM3EyNzJiRWxWSnVnN3F6cGF5WXFucllOMTBHRWhIUW9KTkRxZnlPSHNLRG9FbTNOR1pKY2FSWVl4eHY2SVZxTFJDaFZidnJ3ekdkTXdmME95aE52THFGWlhMS1RWMmhPX2t6MXZWaVNwRi1PVXVWbW5uRC1ZMXFQb28xSTBJeVFYallxaGNlME01UkhiQ1drVk1Qbk5jZHFMQzZlTWhiTjJGT1pHSUxSOVRqcDIyMmxucGFTbnVmMHlVbDF4MiJ9.eyJzaGEyNTYiOiJneUNWNm10bjd6ZDZhNGFpVzlGZ1c1c2hqaE5ubXVjNjVJbXJNbmo4ZDJrPSJ9.gz9abderaPSVXLP9AcsUS6OzabI-n2MLD8CZCP9cct0imQCLWZ0kEOX6G3wCSNgdvvTpocTYD0O5dZiDkzcYMzKJUr3sDOyjgxTYYLy51YCBcNKz3mcHA3bPZFBgMqmkZrdXvjp50sx612CBPbleVWF61tpqdbPzChOpCVL4EdHYgn8DfDx5qPa4EAgrnZH2WJrKuKo9JJeXuNNut3CMM7KVpHNgPgZ31LEYEZU__i904cqaMKUbY0s9KOVkHuuu8uiFB3Iisk9sgpv2tygCg-a0x7Dqaz9fI_wiT0D7Do9FRMCAqiEhiaYDAT6_3LEPgRfPJ4iesUG2NdDg4QWXzh3gy1QGOEACWSaswmIHg8moICJ3GH4VEjZ-AluqMU_FYI2KKcxyeU-96Y8SPlArYwxnGk0oIz-MBCgC9MvYuzJkr6DQuXaoNbMhxsRUaELnT7PdilWMr15EC7msDwmPWJdwMfX8Y2jh_MH5BNJZBgp76I_QgFK4WaLM6Y7GBcr9", )"
    R"(     "fileUrls": { )"
    R"(         "c7f95b5a4b0b328a": "http://dcsfe.int.adu.microsoft.com/westus2/intModuleIdTestInstance--intmoduleidtest/7349b36a88284d1d9daf850860ff1880/contoso.virtual-motor.1.1.updatemanifest.json", )"
    R"(         "3671c4d30950daa9": "http://dcsfe.int.adu.microsoft.com/westus2/intModuleIdTestInstance--intmoduleidtest/cc4beac35ec249748b3972a6a2e9e9f6/contoso-motor-1.1-instructions.json", )"
    R"(         "875773ba321fda6c": "http://dcsfe.int.adu.microsoft.com/westus2/intModuleIdTestInstance--intmoduleidtest/c78826e7b595400993ea2dc8995c5717/contoso-motor-fileinstaller", )"
    R"(         "a85dc1692a0ff90a": "http://dcsfe.int.adu.microsoft.com/westus2/intModuleIdTestInstance--intmoduleidtest/e95d264d56b84fd288699c8f37489f1a/firmware.json" )"
    R"(      }  )"
    R"( } )";


const char* action_bundle =
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

const char* action_leaf0 =
    R"( { )"
    R"(     "updateManifest": "{\"manifestVersion\":\"2.0\",\"updateId\":{\"provider\":\"fabrikam\",\"name\":\"motorUpdate\",\"version\":\"1.0\"},\"updateType\":\"microsoft/bundle:1\",\"installedCriteria\":\"1.0\",\"compatibility\":[{\"deviceManufacturer\":\"Contoso\",\"deviceModel\":\"VirtualVacuum\",\"componentGroup\":\"Motors\"}],\"files\":{\"00001\":{\"fileName\":\"contoso-motor-1.0-fileinstaller\",\"sizeInBytes\":1396,\"hashes\":{\"sha256\":\"E2o94XQss/K8niR1pW6OdaIS/y3tInwhEKMn/6Rw1Gw=\"}}},\"createdDateTime\":\"2021-06-07T07:25:59.0781905Z\"}",     )"
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
    R"(     "updateType": "contoso/fileinstaller:1", )"
    R"(     "files": [ )"
    R"(         { )"
    R"(             "fileName": "contoso-motor-1.0-fileinstaller", )"
    R"(             "arguments": "--pre-install" )"
    R"(         } )"
    R"(     ] )"
    R"( } )"
};

// clang-format on

TEST_CASE("Initialization test")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_bundle, true, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto action = workflow_get_action(handle);
    REQUIRE(action == ADUCITF_UpdateAction_ProcessDeployment);

    // Get and set id.
    char* id = workflow_get_id(handle);
    CHECK_THAT(id, Equals("action_bundle"));

    workflow_free_string(id);

    workflow_set_id(handle, "new_id_1");
    id = workflow_get_id(handle);

    CHECK(id != nullptr);

    CHECK_THAT(id, Equals("new_id_1"));
    workflow_free_string(id);
    id = nullptr;

    auto filecount = workflow_get_update_files_count(handle);
    REQUIRE(filecount == 1);

    ADUC_FileEntity* file0 = nullptr;
    bool success = workflow_get_update_file(handle, 0, &file0);
    CHECK(success);
    CHECK(file0 != nullptr);
    CHECK_THAT(file0->FileId, Equals("00000"));
    CHECK(file0->HashCount == 1);
    CHECK_THAT(file0->DownloadUri, Equals("file:///tmp/tests/testfiles/contoso-motor-1.0-updatemanifest.json"));

    ADUC_FileEntity_Uninit(file0);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc, hicpp-no-malloc)
    free(file0);

    char* installedCriteria = workflow_get_installed_criteria(handle);
    CHECK_THAT(installedCriteria, Equals("1.0"));
    workflow_free_string(installedCriteria);

    char* updateId = workflow_get_expected_update_id_string(handle);
    CHECK_THAT(updateId, Equals("{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"}"));
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
    ADUC_Result result = workflow_init(action_leaf0, false, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto action = workflow_get_action(handle);
    REQUIRE(action == ADUCITF_UpdateAction_Undefined);

    workflow_free(handle);
}

TEST_CASE("BundledUpdates array")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(bundle_with_bundledUpdates, false, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    int count = workflow_get_bundle_updates_count(handle);
    REQUIRE(count == 1);

    ADUC_FileEntity* entity = nullptr;
    bool fileOk = workflow_get_bundle_updates_file(handle, 0, &entity);
    CHECK(fileOk);
    REQUIRE(entity != nullptr);
    CHECK_THAT(entity->TargetFilename, Equals("contoso.virtual-motor.1.1.updatemanifest.json"));

    workflow_free_file_entity(entity);
    workflow_free(handle);
}

TEST_CASE("Get Compatibility")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_leaf0, false, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    const char* expectedValue =
        R"([{"deviceManufacturer":"Contoso","deviceModel":"VirtualVacuum","componentGroup":"Motors"}])";

    char* compats = workflow_get_compatibility(handle);
    CHECK_THAT(compats, Equals(expectedValue));

    workflow_free(handle);
}

// \"updateId\":{\"provider\":\"Contoso\",\"name\":\"VacuumBundleUpdate\",\"version\":\"1.0\"}
TEST_CASE("Update Id")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_bundle, false, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    ADUC_UpdateId* updateId = nullptr;
    result = workflow_get_expected_update_id(handle, &updateId);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    REQUIRE_THAT(updateId->Name, Equals("VacuumBundleUpdate"));
    REQUIRE_THAT(updateId->Provider, Equals("Contoso"));
    REQUIRE_THAT(updateId->Version, Equals("1.0"));

    workflow_free_update_id(updateId);

    workflow_free(handle);
}

TEST_CASE("Child workflow uses fileUrls from parent")
{
    ADUC_WorkflowHandle bundle = nullptr;
    ADUC_Result result = workflow_init(action_bundle, false, &bundle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto filecount = workflow_get_update_files_count(bundle);
    REQUIRE(filecount == 1);

    ADUC_WorkflowHandle leaf0 = nullptr;
    result = workflow_init(action_leaf0, false, &leaf0);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    workflow_insert_child(bundle, 0, leaf0);

    // Check that leaf 0 file has the right download uri.
    ADUC_FileEntity* file0 = nullptr;
    bool success = workflow_get_update_file(leaf0, 0, &file0);
    CHECK(success);
    CHECK(file0 != nullptr);
    CHECK_THAT(file0->FileId, Equals("00001"));
    CHECK(file0->HashCount == 1);
    CHECK_THAT(file0->DownloadUri, Equals("file:///tmp/tests/testfiles/contoso-motor-1.0-fileinstaller"));

    ADUC_WorkflowHandle leaf0_0 = nullptr;
    result = workflow_init(action_leaf0_0, false, &leaf0_0);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    workflow_insert_child(leaf0, 0, leaf0_0);

    // Check that leaf 0_0 file has the right download uri.
    ADUC_FileEntity* file0_0 = nullptr;
    success = workflow_get_update_file(leaf0_0, 0, &file0_0);
    CHECK(success);
    CHECK(file0_0 != nullptr);
    CHECK_THAT(file0_0->FileId, Equals("gw001"));
    CHECK(file0_0->HashCount == 1);
    CHECK_THAT(file0_0->DownloadUri, Equals("file:///tmp/tests/testfiles/behind-gateway-info.json"));

    ADUC_FileEntity_Uninit(file0);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc, hicpp-no-malloc)
    free(file0);
    ADUC_FileEntity_Uninit(file0_0);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc, hicpp-no-malloc)
    free(file0_0);

    workflow_free(bundle);
}

TEST_CASE("Create leaf instruction workflow")
{
    ADUC_WorkflowHandle bundle = nullptr;
    ADUC_Result result = workflow_init(action_bundle, false, &bundle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto filecount = workflow_get_update_files_count(bundle);
    REQUIRE(filecount == 1);

    ADUC_WorkflowHandle leaf0 = nullptr;
    result = workflow_init(action_leaf0, false, &leaf0);
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);
    workflow_set_id(leaf0, "leaf_0");

    workflow_insert_child(bundle, 0, leaf0);

    // Create leaf-0 instruction #0.
    ADUC_WorkflowHandle leaf0_inst_0 = nullptr;
    result = workflow_create_from_instruction(leaf0, leaf0_instruction_installItems[0], &leaf0_inst_0);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    workflow_set_id(leaf0_inst_0, "leaf0_inst_0");

    int fileCount = workflow_get_update_files_count(leaf0_inst_0);
    CHECK(fileCount == 1);

    // Leaf0-inst-0 now inherite installed-criteria from leaf-0.
    char* leaf_0_inst_0_installed_criteria = workflow_get_installed_criteria(leaf0_inst_0);
    CHECK_THAT(leaf_0_inst_0_installed_criteria, Equals("1.0"));
    workflow_free_string(leaf_0_inst_0_installed_criteria);
    leaf_0_inst_0_installed_criteria = nullptr;

    // At this point, the fileEntity contains blank downloadUri.

    ADUC_FileEntity* inst_file0 = nullptr;
    bool success = workflow_get_update_file(leaf0_inst_0, 0, &inst_file0);
    REQUIRE(success);
    CHECK(inst_file0 != nullptr);
    CHECK_THAT(inst_file0->FileId, Equals("00001"));
    CHECK(inst_file0->HashCount == 1);
    bool emptyDownloadUri = inst_file0->DownloadUri == nullptr || *inst_file0->DownloadUri == 0;
    CHECK(emptyDownloadUri);
    CHECK_THAT(inst_file0->Arguments, Equals("--pre-install"));

    ADUC_FileEntity_Uninit(inst_file0);
    inst_file0 = nullptr;

    // Insert file to a tree to inherite fileUrls list.
    workflow_insert_child(leaf0, 0, leaf0_inst_0);

    // Check that leaf 0 file has the right download uri.
    success = workflow_get_update_file(leaf0_inst_0, 0, &inst_file0);
    REQUIRE(success);
    CHECK(inst_file0 != nullptr);
    CHECK_THAT(inst_file0->FileId, Equals("00001"));
    CHECK(inst_file0->HashCount == 1);
    CHECK_THAT(inst_file0->DownloadUri, Equals("file:///tmp/tests/testfiles/contoso-motor-1.0-fileinstaller"));
    CHECK_THAT(inst_file0->Arguments, Equals("--pre-install"));

    //
    // Instance-level workflow should have the same workfolder as its parent's.
    //
    char* instanceWorkfolder = workflow_get_workfolder(leaf0_inst_0);
    char* componentsWorkfolder = workflow_get_workfolder(leaf0);
    CHECK(instanceWorkfolder != nullptr);
    CHECK(*instanceWorkfolder != 0);
    CHECK(componentsWorkfolder != nullptr);
    CHECK(*componentsWorkfolder != 0);
    CHECK_THAT(instanceWorkfolder, Equals(componentsWorkfolder));

    ADUC_FileEntity_Uninit(inst_file0);
    inst_file0 = nullptr;

    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory, cppcoreguidelines-no-malloc, hicpp-no-malloc)
    free(inst_file0);

    workflow_free(bundle);
}

TEST_CASE("Add and remove children")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_leaf0, false, &handle);

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto filecount = workflow_get_update_files_count(handle);
    REQUIRE(filecount == 1);

    ADUC_WorkflowHandle childWorkflow[12];

    char name[40];
    for (int i = 0; i < ARRAY_SIZE(childWorkflow); i++)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        result = workflow_init(action_leaf0, false, &childWorkflow[i]);
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
    ADUC_Result result = workflow_init(action_bundle, false, &bundle);
    workflow_set_id(bundle, "testWorkflow_001");
    workflow_set_workfolder(bundle, "/tmp/workflow_ut/testWorkflow_001");

    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    auto filecount = workflow_get_update_files_count(bundle);
    REQUIRE(filecount == 1);

    ADUC_WorkflowHandle leaf0 = nullptr;
    result = workflow_init(action_leaf0, false, &leaf0);
    workflow_set_id(leaf0, "testLeaf_0");
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    workflow_insert_child(bundle, 0, leaf0);

    ADUC_WorkflowHandle leaf0_0 = nullptr;
    result = workflow_init(action_leaf0_0, false, &leaf0_0);
    workflow_set_id(leaf0_0, "testLeaf0_0");
    CHECK(result.ResultCode != 0);
    CHECK(result.ExtendedResultCode == 0);

    workflow_insert_child(leaf0, 0, leaf0_0);

    // Test set result
    workflow_set_state(bundle, ADUCITF_State_DownloadStarted);

    CHECK(ADUCITF_State_DownloadStarted == workflow_get_root_state(leaf0_0));

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
    ADUC_Result result = workflow_init(manifest_1_0, false, &handle);
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
    ADUC_Result result = workflow_init(manifest_2_0, false, &handle);
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
    ADUC_Result result = workflow_init(manifest_2, false, &handle);
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
    ADUC_Result result = workflow_init(manifest_3, false, &handle);
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
    ADUC_Result result = workflow_init(manifest_x, false, &handle);
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
    ADUC_Result result = workflow_init(manifest_empty, false, &handle);
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
    ADUC_Result result = workflow_init(manifest_missing_version, false, &handle);
    CHECK(result.ResultCode > 0);
    int versionNumber = workflow_get_update_manifest_version(handle);
    CHECK(versionNumber == -1);
    workflow_free(handle);
}

TEST_CASE("Mininum update manifest version check - 2")
{
    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(action_bundle, true, &handle);
    CHECK(result.ResultCode > 0);
    int versionNumber = workflow_get_update_manifest_version(handle);
    CHECK(versionNumber == 2);
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
    ADUC_Result result = workflow_init(manifest_old_1_0, true, &handle);
    CHECK(result.ResultCode == 0);
    CHECK(result.ExtendedResultCode == ADUC_ERC_UTILITIES_UPDATE_DATA_PARSER_UNSUPPORTED_UPDATE_MANIFEST_VERSION);
    workflow_free(handle);
}
