/**
 * @file workflow_get_update_file_ut.cpp
 * @brief Unit Tests for the workflow_get_update_file function in workflow_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/workflow_utils.h"

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include <aduc/workflow_utils.h>
#include <fstream>
#include <parson.h>
#include <regex>
#include <sstream>
#include <string>

static std::string get_update_manifest_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/workflow_get_update_file/updateManifest_downloadhandlerid_relatedfile.json";
    return path;
}

static std::string get_twin_desired_json_path()
{
    std::string path{ ADUC_TEST_DATA_FOLDER };
    path += "/workflow_get_update_file/desired_template.json";
    return path;
}

static std::string slurp(const std::string& path)
{
    std::ifstream f(path);
    std::stringstream stream;
    stream << f.rdbuf();
    return stream.str();
}

TEST_CASE("workflow_get_update_file with download handler")
{
    const char* targetUpdateFileId = "f222b9ffefaaac577";
    const char* deltaUpdateFileId = "f223bac3efa01c2df";
    const char* deltaUpdateFileUrl =
        "http://testinstance.b.nlu.dl.adu.microsoft.com/westus2/testinstance/e5cc19d5e9174c93ada35cc315f1fb1d/delta_update-0.2.delta";

    JSON_Value* updateManifestJson = json_parse_file(get_update_manifest_json_path().c_str());
    REQUIRE(updateManifestJson != nullptr);

    char* serialized = json_serialize_to_string(updateManifestJson);
    REQUIRE(serialized != nullptr);

    std::string serializedUpdateManifest = serialized;
    json_free_serialized_string(serialized);
    serialized = nullptr;

    serializedUpdateManifest =
        std::regex_replace(serializedUpdateManifest, std::regex("TARGET_UPDATE_FILE_ID"), targetUpdateFileId);
    serializedUpdateManifest =
        std::regex_replace(serializedUpdateManifest, std::regex("DELTA_UPDATE_FILE_ID"), deltaUpdateFileId);
    serializedUpdateManifest = std::regex_replace(serializedUpdateManifest, std::regex("\""), "\\\"");

    std::string desired = slurp(get_twin_desired_json_path());
    desired = std::regex_replace(desired, std::regex("UPDATE_MANIFEST_SIGNATURE"), "foo");
    desired = std::regex_replace(desired, std::regex("TARGET_UPDATE_FILE_ID"), targetUpdateFileId);
    desired = std::regex_replace(desired, std::regex("DELTA_UPDATE_FILE_ID"), deltaUpdateFileId);
    desired = std::regex_replace(desired, std::regex("DELTA_UPDATE_FILE_URL"), deltaUpdateFileUrl);
    desired = std::regex_replace(desired, std::regex("UPDATE_MANIFEST"), serializedUpdateManifest);

    ADUC_WorkflowHandle handle = nullptr;

    ADUC_Result result = workflow_init(desired.c_str(), false /* validateManifest */, &handle);
    REQUIRE(result.ResultCode > 0);

    size_t filecount = workflow_get_update_files_count(handle);
    REQUIRE(filecount == 1);

    ADUC_FileEntity* file = nullptr;
    REQUIRE(workflow_get_update_file(handle, 0, &file));

    CHECK_THAT(file->DownloadHandlerId, Equals("microsoft/delta:1"));

    CHECK(file->RelatedFileCount == 1);
    CHECK(file->RelatedFiles != nullptr);

    const auto& relatedFile = file->RelatedFiles[0];

    CHECK_THAT(relatedFile.FileId, Equals(deltaUpdateFileId));
    CHECK_THAT(relatedFile.DownloadUri, Equals(deltaUpdateFileUrl));
    CHECK_THAT(relatedFile.FileName, Equals("DELTA_UPDATE_FILE_NAME"));

    CHECK(relatedFile.HashCount == 1);
    CHECK(relatedFile.Hash != NULL);
    CHECK_THAT(relatedFile.Hash[0].type, Equals("sha256"));
    CHECK_THAT(relatedFile.Hash[0].value, Equals("DELTA_UPDATE_HASH"));

    CHECK(relatedFile.PropertiesCount == 2);
    CHECK(relatedFile.Properties != NULL);
    CHECK_THAT(relatedFile.Properties[0].Name, Equals("microsoft.sourceFileHash"));
    CHECK_THAT(relatedFile.Properties[0].Value, Equals("SOURCE_UPDATE_HASH"));
    CHECK_THAT(relatedFile.Properties[1].Name, Equals("microsoft.sourceFileHashAlgorithm"));
    CHECK_THAT(relatedFile.Properties[1].Value, Equals("sha256"));
}

// clang-format off
const char* manifest_missing_related_file_file_url =
    R"( {                                                    )"
    R"(     "workflow": {                                    )"
    R"(         "action": 3,                                 )"
    R"(         "id": "77232e26-97a5-440c-8bac-1e4c9652bd77" )"
    R"(     },                                               )"
    R"(     "updateManifest": "{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"DeltaUpdateTestManufacturer\",\"name\":\"deltaupdatetestupdate\",\"version\":\"0.2.0\"},\"compatibility\":[{\"DeviceManufacturer\":\"DeltaUpdateTestManufacturer\",\"DeviceModel\":\"DeltaUpdateTestModel\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:1\",\"files\":[\"f54d16eca3561a1e0\"],\"handlerProperties\":{\"InstalledCriteria\":\"0.2.0\"}}]},\"files\":{\"f54d16eca3561a1e0\":{\"fileName\":\"in2.FIT_RECOMPRESSED_and_RE-SIGNED.swu\",\"sizeInBytes\":105945088,\"hashes\":{\"sha256\":\"/16bQOP9P71DeGlyBYYIZGywsfaZknVY9LY3z1i6CXU=\"},\"relatedFiles\":{\"f512477968fd69644\":{\"fileName\":\"in1_in2_deltaupdate.dat\",\"sizeInBytes\":102910752,\"hashes\":{\"sha256\":\"2MIldV8LkdKenjJasgTHuYi+apgtNQ9FeL2xsV3ikHY=\"},\"properties\":{\"microsoft.sourceFileHashAlgorithm\":\"sha256\",\"microsoft.sourceFileHash\":\"YmFYwnEUddq2nZsBAn5v7gCRKdHx+TUntMz5tLwU+24=\"}}},\"downloadHandler\":{\"id\":\"microsoft/delta:1\"}}},\"createdDateTime\":\"2022-04-27T03:18:29.4289383Z\"}", )"
    R"(     "updateManifestSignature": "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJRQ0dELzM1Z2pPVFNqWWxIdnVrTzdOS09xSGw0SjJiR05nZ21QMzhKbG9vPSJ9.Py7yeKctqt5JkUrnEPlPfyqSzwVdq8AfrhazzRKhkQhG45G7MNJQHWDoxjDxLgIDHtUdi-MdoCJ8W0ABGvCI9Mm3vxNj7btktVdpzNZ0Wm7kR5dL-k_ZHvC2LCax5Wk5ngOYYnTeeGKsQgfxJhCrNpBavxR43WjJjC1R6K_MZZooCFLLH3WVgUrjqIL-AR7gnAlSVEOoeKJXp-Qw575uYv0JSwu4fgBYas8Kjnb72GPVh-PgpExbu0hTWl2n91kfUyHYcaBtydbpjKRq4CpKwtlxyRzZzlVf_XzbMzOOWNHZEV_YCZ99-JbLgWZ7uDMOT_b1lQ-dPn00_Ek73-RPEAVrbSBD_7WTloIuMCiNXJoGNkH0OhvI0VbV4OQnRNiqhlGPnQm__id5yr-Ss0z3fIDQTNHYQbe-EXhDR96E-8QSNddFiTV8vJL1Cyp4Ro1jxo7kua6lyYfdjjYV49iAiLEl8QdulsD8RU7-BN87H9C0w-Z1ysPShL3CGK7tQQoK", )"
    R"(     "fileUrls": { )"
    R"(         "f54d16eca3561a1e0": "http://some_host.com/path/to/0ab7cf50124548c188dca6f4da0ceff2/in2.FIT_RECOMPRESSED_and_RE-SIGNED.swu" )"
    R"(     } )"
    R"( } )";
// clang-format on

TEST_CASE("workflow_get_update_file - upd metadata missing relatedFile URL")
{
    SECTION("Fail missing url for fileId")
    {
        ADUC_WorkflowHandle handle = nullptr;

        ADUC_Result result = workflow_init(manifest_missing_related_file_file_url, true /* validateManifest */, &handle);
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        ADUC_FileEntity* file = nullptr;
        CHECK_FALSE(workflow_get_update_file(handle, 0, &file));
    }
}
