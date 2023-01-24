
/**
 * @file microsoft_delta_download_handler_ut.cpp
 * @brief Unit Tests for microsoft_delta_download_handler library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include "aduc/microsoft_delta_download_handler_utils.h"
#include <aduc/parser_utils.h>
#include <aduc/result.h> // ADUC_Result_*
#include <aduc/types/adu_core.h> // ADUC_Result_*
#include <aduc/types/update_content.h> // ADUC_RelatedFile, ADUC_FileEntity
#include <aduc/types/workflow.h> // ADUC_WorkflowHandle
#include <aduc/workflow_utils.h>
#include <regex>

#define TEST_WORKFLOW_ID "7e3e7d32de4db3ef1337bac7341ab347"
#define TEST_PAYLOAD_FILE_ID "ac47d3bab772454283ae95f0bbb1a1de"
#define TEST_DELTA_FILE_ID "312d0351155037c4900d76473d371c35"

const std::string updateManifest{ R"( {                                                                         )"
                                  R"(     "compatibility": [                                                    )"
                                  R"(         {                                                                 )"
                                  R"(             "deviceManufacturer": "contoso",                              )"
                                  R"(             "deviceModel": "toaster"                                      )"
                                  R"(         }                                                                 )"
                                  R"(     ],                                                                    )"
                                  R"(     "createdDateTime": "2022-03-12T12:22:37.2627901Z",                    )"
                                  R"(     "files": {                                                            )"
                                  R"(         "PAYLOAD_FILE_ID": {                                              )"
                                  R"(             "fileName": "target_update.swu",                              )"
                                  R"(             "hashes": {                                                   )"
                                  R"(                 "sha256": "PAYLOAD_HASH"                                  )"
                                  R"(             },                                                            )"
                                  R"(             "sizeInBytes": 98765,                                         )"
                                  R"(             "properties": {                                               )"
                                  R"(             },                                                            )"
                                  R"(             "downloadHandler": {                                          )"
                                  R"(                 "id": "microsoft/delta:1"                                 )"
                                  R"(             },                                                            )"
                                  R"(             "relatedFiles": {                                             )"
                                  R"(                 "DELTA_FILE_ID": {                                        )"
                                  R"(                     "fileName": "DELTA_UPDATE_FILE_NAME",                 )"
                                  R"(                     "sizeInBytes": 1234,                                  )"
                                  R"(                     "hashes": {                                           )"
                                  R"(                         "sha256": "DELTA_UPDATE_HASH"                     )"
                                  R"(                     },                                                    )"
                                  R"(                     "properties": {                                       )"
                                  R"(                         "microsoft.sourceFileHash": "SOURCE_UPDATE_HASH", )"
                                  R"(                         "microsoft.sourceFileHashAlgorithm": "sha256"     )"
                                  R"(                     }                                                     )"
                                  R"(                 }                                                         )"
                                  R"(             }                                                             )"
                                  R"(         }                                                                 )"
                                  R"(     },                                                                    )"
                                  R"(     "instructions": {                                                     )"
                                  R"(         "steps": [                                                        )"
                                  R"(             {                                                             )"
                                  R"(                 "files": [                                                )"
                                  R"(                     "f222b9ffefaaac577"                                   )"
                                  R"(                 ],                                                        )"
                                  R"(                 "handler": "microsoft/swupdate:1",                        )"
                                  R"(                 "handlerProperties": {                                    )"
                                  R"(                     "installedCriteria": "toaster_firmware-0.1"           )"
                                  R"(                 }                                                         )"
                                  R"(             }                                                             )"
                                  R"(         ]                                                                 )"
                                  R"(     },                                                                    )"
                                  R"(     "manifestVersion": "5",                                               )"
                                  R"(     "updateId": {                                                         )"
                                  R"(         "name": "toaster_firmware",                                       )"
                                  R"(         "provider": "contoso",                                            )"
                                  R"(         "version": "0.1"                                                  )"
                                  R"(     }                                                                     )"
                                  R"( }                                                                         )" };

const std::string desiredTemplate{
    R"( {                                                                                 )"
    R"(     "fileUrls": {                                                                 )"
    R"(         "PAYLOAD_FILE_ID": "http://hostname:port/path/to/full_target_update.swu", )"
    R"(         "DELTA_FILE_ID": "http://hostname:port/path/to/delta_update.delta"        )"
    R"(     },                                                                            )"
    R"(     "updateManifest": "UPDATE_MANIFEST",                                          )"
    R"(     "updateManifestSignature": "SIGNATURE",                                       )"
    R"(     "workflow": {                                                                 )"
    R"(         "action": 3,                                                              )"
    R"(         "id": "WORKFLOW_ID_GUID"                                                  )"
    R"(     }                                                                             )"
    R"( }                                                                                 )"
};

ADUC_Result MockProcessDeltaUpdateFn(
    const char* _sourceUpdateFilePath, const char* _deltaUpdateFilePath, const char* _targetUpdateFilePath)
{
    ADUC_Result result = { ADUC_Result_Success };
    return result;
}

ADUC_Result MockDownloadDeltaUpdateFn(const ADUC_WorkflowHandle _workflowHandle, const ADUC_RelatedFile* _relatedFile)
{
    ADUC_Result result = { ADUC_Result_Success };
    return result;
}

TEST_CASE("MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile Cache Miss")
{
    ADUC_Result result = {};

    //
    // Arrange
    //
    JSON_Value* updateManifestTemplate = json_parse_string(updateManifest.c_str());
    REQUIRE(updateManifestTemplate != nullptr);

    char* serialized = json_serialize_to_string(updateManifestTemplate);
    REQUIRE(serialized != nullptr);

    std::string serializedUpdateManifest = serialized;
    json_free_serialized_string(serialized);
    serialized = nullptr;
    serializedUpdateManifest = std::regex_replace(serializedUpdateManifest, std::regex("\""), "\\\"");

    std::string desired = std::regex_replace(desiredTemplate, std::regex("UPDATE_MANIFEST"), serializedUpdateManifest);

    desired = std::regex_replace(desired, std::regex("WORKFLOW_ID_GUID"), TEST_WORKFLOW_ID);
    desired = std::regex_replace(desired, std::regex("PAYLOAD_FILE_ID"), TEST_PAYLOAD_FILE_ID);
    desired = std::regex_replace(desired, std::regex("DELTA_FILE_ID"), TEST_DELTA_FILE_ID);

    ADUC_WorkflowHandle handle = nullptr;
    result = workflow_init(desired.c_str(), false, &handle);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

    ADUC_FileEntity fileEntity;
    memset(&fileEntity, 0, sizeof(fileEntity));
    REQUIRE(workflow_get_update_file(handle, 0, &fileEntity));

    //
    // Act
    //
    result = MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile(
        handle, &fileEntity.RelatedFiles[0], "/foo/", "/bar/", MockProcessDeltaUpdateFn, MockDownloadDeltaUpdateFn);

    CHECK(result.ResultCode == ADUC_Result_Success_Cache_Miss);

    ADUC_FileEntity_Uninit(&fileEntity);
}
