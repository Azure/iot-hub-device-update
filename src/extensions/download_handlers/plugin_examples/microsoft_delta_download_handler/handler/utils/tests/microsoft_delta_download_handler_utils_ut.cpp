
/**
 * @file microsoft_delta_download_handler_ut.cpp
 * @brief Unit Tests for microsoft_delta_download_handler library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>
using Catch::Matchers::Equals;

#include "aduc/microsoft_delta_download_handler_utils.h"
#include <aduc/parser_utils.h>
#include <aduc/result.h> // ADUC_Result_*
#include <aduc/types/adu_core.h> // ADUC_Result_*
#include <aduc/types/update_content.h> // ADUC_RelatedFile, ADUC_FileEntity
#include <aduc/types/workflow.h> // ADUC_WorkflowHandle
#include <aduc/workflow_utils.h>
#include <regex>
#include <string>

#define TEST_WORKFLOW_ID "7e3e7d32de4db3ef1337bac7341ab347"
#define TEST_PAYLOAD_FILE_ID "ac47d3bab772454283ae95f0bbb1a1de"
#define TEST_DELTA_FILE_ID "312d0351155037c4900d76473d371c35"

const std::string kUpdateManifest{ R"({"manifestVersion":"5","updateId":{"provider":"contoso","name":"toaster_firmware","version":"0.1"},"instructions":{"steps":[{"handler":"microsoft/swupdate:1","files":["PAYLOAD_FILE_ID"],"handlerProperties":{"installedCriteria":"ok"}}]},"files":{"PAYLOAD_FILE_ID":{"fileName":"target_update.swu","sizeInBytes":10,"hashes":{"sha256":"PAYLOAD_HASH"},"downloadHandler":{"id":"microsoft/delta:1"},"relatedFiles":{"DELTA_FILE_ID":{"fileName":"delta_update.delta","sizeInBytes":5,"hashes":{"sha256":"DELTA_UPDATE_HASH"},"properties":{"microsoft.sourceFileHash":"SOURCE_UPDATE_HASH","microsoft.sourceFileHashAlgorithm":"sha256"}}}}}})" };

const std::string kDesiredTemplate{ R"({"workflow":{"action":3,"id":"WORKFLOW_ID_GUID"},"updateManifest":"UPDATE_MANIFEST","updateManifestSignature":"SIGNATURE","fileUrls":{"PAYLOAD_FILE_ID":"http://example/target.swu","DELTA_FILE_ID":"http://example/delta.delta"}})" };

static ADUC_WorkflowHandle CreateMinimalWorkflowHandle()
{
    const char* workflowJson =
        R"({"workflow":{"action":3,"id":"aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"},"updateManifest":"{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"contoso\",\"name\":\"delta-utils-test\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[]},\"files\":{},\"createdDateTime\":\"2022-01-01T00:00:00Z\"}","updateManifestSignature":"dummy","fileUrls":{}})";

    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(workflowJson, false, &handle);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    REQUIRE(handle != nullptr);
    return handle;
}

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
    JSON_Value* updateManifestTemplate = json_parse_string(kUpdateManifest.c_str());
    REQUIRE(updateManifestTemplate != nullptr);

    char* serialized = json_serialize_to_string(updateManifestTemplate);
    REQUIRE(serialized != nullptr);

    std::string serializedUpdateManifest = serialized;
    json_free_serialized_string(serialized);
    serialized = nullptr;
    serializedUpdateManifest = std::regex_replace(serializedUpdateManifest, std::regex("\""), "\\\"");

    std::string desired = std::regex_replace(kDesiredTemplate, std::regex("UPDATE_MANIFEST"), serializedUpdateManifest);

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

    workflow_free_file_entity(&fileEntity);
    ADUC_FileEntity_Uninit(&fileEntity);
    workflow_free(handle);
    json_value_free(updateManifestTemplate);
}

// =====================================================================
// MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile bad-arg tests
// =====================================================================

TEST_CASE("ProcessRelatedFile returns failure when workflowHandle is nullptr")
{
    ADUC_RelatedFile relatedFile;
    memset(&relatedFile, 0, sizeof(relatedFile));

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile(
        nullptr, &relatedFile, "/tmp/payload", nullptr, MockProcessDeltaUpdateFn, MockDownloadDeltaUpdateFn);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

TEST_CASE("ProcessRelatedFile returns failure when relatedFile is nullptr")
{
    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile(
        wfHandle, nullptr, "/tmp/payload", nullptr, MockProcessDeltaUpdateFn, MockDownloadDeltaUpdateFn);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

TEST_CASE("ProcessRelatedFile returns failure when payloadFilePath is nullptr")
{
    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);

    ADUC_RelatedFile relatedFile;
    memset(&relatedFile, 0, sizeof(relatedFile));

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile(
        wfHandle, &relatedFile, nullptr, nullptr, MockProcessDeltaUpdateFn, MockDownloadDeltaUpdateFn);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

TEST_CASE("ProcessRelatedFile returns failure when processDeltaUpdateFn is nullptr")
{
    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);

    ADUC_RelatedFile relatedFile;
    memset(&relatedFile, 0, sizeof(relatedFile));

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile(
        wfHandle, &relatedFile, "/tmp/payload", nullptr, nullptr /* processDeltaUpdateFn */, MockDownloadDeltaUpdateFn);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

TEST_CASE("ProcessRelatedFile returns failure when all args are nullptr")
{
    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_ProcessRelatedFile(
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

// =====================================================================
// MicrosoftDeltaDownloadHandlerUtils_GetSourceUpdateProperties bad-arg tests
// =====================================================================

TEST_CASE("GetSourceUpdateProperties returns failure when relatedFile is nullptr")
{
    STRING_HANDLE outHash = nullptr;
    STRING_HANDLE outAlg = nullptr;

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_GetSourceUpdateProperties(
        nullptr, &outHash, &outAlg);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

TEST_CASE("GetSourceUpdateProperties returns failure when outHash is nullptr")
{
    ADUC_RelatedFile relatedFile;
    memset(&relatedFile, 0, sizeof(relatedFile));
    STRING_HANDLE outAlg = nullptr;

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_GetSourceUpdateProperties(
        &relatedFile, nullptr, &outAlg);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

TEST_CASE("GetSourceUpdateProperties returns failure when outAlg is nullptr")
{
    ADUC_RelatedFile relatedFile;
    memset(&relatedFile, 0, sizeof(relatedFile));
    STRING_HANDLE outHash = nullptr;

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_GetSourceUpdateProperties(
        &relatedFile, &outHash, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

TEST_CASE("GetSourceUpdateProperties returns failure when relatedFile has no hash properties")
{
    ADUC_RelatedFile relatedFile;
    memset(&relatedFile, 0, sizeof(relatedFile));
    relatedFile.PropertiesCount = 0;
    relatedFile.Properties = nullptr;

    STRING_HANDLE outHash = nullptr;
    STRING_HANDLE outAlg = nullptr;

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_GetSourceUpdateProperties(
        &relatedFile, &outHash, &outAlg);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_RELATEDFILE_BAD_OR_MISSING_HASH_PROPERTIES);
}

TEST_CASE("GetSourceUpdateProperties returns source hash and algorithm for valid relatedFile")
{
    ADUC_Property props[2] = {
        { const_cast<char*>("microsoft.sourceFileHash"), const_cast<char*>("hash-value-123") },
        { const_cast<char*>("microsoft.sourceFileHashAlgorithm"), const_cast<char*>("sha256") },
    };

    ADUC_RelatedFile relatedFile{};
    relatedFile.Properties = props;
    relatedFile.PropertiesCount = 2;

    STRING_HANDLE outHash = nullptr;
    STRING_HANDLE outAlg = nullptr;

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_GetSourceUpdateProperties(&relatedFile, &outHash, &outAlg);

    CHECK(result.ResultCode == ADUC_Result_Success);
    REQUIRE(outHash != nullptr);
    REQUIRE(outAlg != nullptr);
    CHECK(std::string(STRING_c_str(outHash)) == "hash-value-123");
    CHECK(std::string(STRING_c_str(outAlg)) == "sha256");

    STRING_delete(outHash);
    STRING_delete(outAlg);
}

TEST_CASE("GetDeltaUpdateDownloadSandboxPath returns sandbox path with filename")
{
    ADUC_WorkflowHandle workflowHandle = CreateMinimalWorkflowHandle();
    REQUIRE(workflow_set_workfolder(workflowHandle, "/tmp") == true);

    ADUC_RelatedFile relatedFile{};
    relatedFile.FileName = const_cast<char*>("delta_update.delta");

    STRING_HANDLE outPath = nullptr;
    ADUC_Result result =
        MicrosoftDeltaDownloadHandlerUtils_GetDeltaUpdateDownloadSandboxPath(workflowHandle, &relatedFile, &outPath);

    CHECK(result.ResultCode == ADUC_Result_Success);
    REQUIRE(outPath != nullptr);
    CHECK(std::string(STRING_c_str(outPath)) == "/tmp/delta_update.delta");

    STRING_delete(outPath);
    workflow_free(workflowHandle);
}

TEST_CASE("DownloadDeltaUpdate returns failure when content downloader is unavailable")
{
    ADUC_WorkflowHandle workflowHandle = CreateMinimalWorkflowHandle();

    ADUC_Hash hash{};
    hash.type = const_cast<char*>("sha256");
    hash.value = const_cast<char*>("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa=");

    ADUC_RelatedFile relatedFile{};
    relatedFile.DownloadUri = const_cast<char*>("http://example/delta_update.delta");
    relatedFile.FileId = const_cast<char*>("delta-file-id");
    relatedFile.FileName = const_cast<char*>("delta_update.delta");
    relatedFile.Hash = &hash;
    relatedFile.HashCount = 1;
    relatedFile.SizeInBytes = 42;

    ADUC_Result result = MicrosoftDeltaDownloadHandlerUtils_DownloadDeltaUpdate(workflowHandle, &relatedFile);

    CHECK(result.ResultCode == ADUC_Result_Failure);

    workflow_free(workflowHandle);
}
