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
