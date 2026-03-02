/**
 * @file microsoft_delta_download_handler_ut.cpp
 * @brief Non-mock unit tests for microsoft_delta_download_handler library.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

extern "C" {
#include "aduc/microsoft_delta_download_handler.h"
}

#include <aduc/result.h>
#include <aduc/types/update_content.h>
#include <aduc/workflow_utils.h>

#include <cstring>

static ADUC_WorkflowHandle CreateMinimalWorkflowHandle()
{
    const char* workflowJson =
        R"({"workflow":{"action":3,"id":"99999999-1111-2222-3333-444444444444"},"updateManifest":"{\"manifestVersion\":\"5\",\"updateId\":{\"provider\":\"contoso\",\"name\":\"delta-test\",\"version\":\"1.0\"},\"compatibility\":[{\"deviceManufacturer\":\"contoso\",\"deviceModel\":\"virtual-vacuum-v1\"}],\"instructions\":{\"steps\":[]},\"files\":{},\"createdDateTime\":\"2022-01-01T00:00:00Z\"}","updateManifestSignature":"dummy","fileUrls":{}})";

    ADUC_WorkflowHandle handle = nullptr;
    ADUC_Result result = workflow_init(workflowJson, false, &handle);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));
    REQUIRE(handle != nullptr);
    return handle;
}

static ADUC_RelatedFile MakeRelatedFileNoProps()
{
    ADUC_RelatedFile rf;
    memset(&rf, 0, sizeof(rf));
    rf.Properties = nullptr;
    rf.PropertiesCount = 0;
    return rf;
}

TEST_CASE("ProcessUpdate returns failure when workflowHandle is nullptr")
{
    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));

    ADUC_Result result = MicrosoftDeltaDownloadHandler_ProcessUpdate(
        nullptr /* workflowHandle */, &entity, "/tmp/payload", nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

TEST_CASE("ProcessUpdate returns failure when fileEntity is nullptr")
{
    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);

    ADUC_Result result = MicrosoftDeltaDownloadHandler_ProcessUpdate(
        wfHandle, nullptr /* fileEntity */, "/tmp/payload", nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

TEST_CASE("ProcessUpdate returns failure when payloadFilePath is nullptr")
{
    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));

    ADUC_Result result = MicrosoftDeltaDownloadHandler_ProcessUpdate(
        wfHandle, &entity, nullptr /* payloadFilePath */, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

TEST_CASE("ProcessUpdate returns failure when entity has no RelatedFiles")
{
    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = reinterpret_cast<ADUC_WorkflowHandle>(&dummy);

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.RelatedFiles = nullptr;
    entity.RelatedFileCount = 0;

    ADUC_Result result = MicrosoftDeltaDownloadHandler_ProcessUpdate(
        wfHandle, &entity, "/tmp/payload", nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

TEST_CASE("ProcessUpdate returns failure when RelatedFile has null Properties")
{
    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = &dummy;

    ADUC_RelatedFile rf = MakeRelatedFileNoProps();

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.RelatedFiles = &rf;
    entity.RelatedFileCount = 1;

    ADUC_Result result = MicrosoftDeltaDownloadHandler_ProcessUpdate(
        wfHandle, &entity, "/tmp/payload", nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_RELATEDFILE_NO_PROPERTIES);
}

TEST_CASE("OnUpdateWorkflowCompleted returns failure when workflowHandle is nullptr")
{
    ADUC_Result result = MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted(
        nullptr /* workflowHandle */, nullptr /* updateCacheBasePath */);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

TEST_CASE("ProcessUpdate returns RequiredFullDownload on source cache miss")
{
    ADUC_WorkflowHandle workflowHandle = CreateMinimalWorkflowHandle();

    ADUC_Property properties[2] = {
        { const_cast<char*>("microsoft.sourceFileHash"), const_cast<char*>("source-hash") },
        { const_cast<char*>("microsoft.sourceFileHashAlgorithm"), const_cast<char*>("sha256") },
    };

    ADUC_RelatedFile relatedFile{};
    relatedFile.FileName = const_cast<char*>("delta.patch");
    relatedFile.DownloadUri = const_cast<char*>("http://example/delta.patch");
    relatedFile.Properties = properties;
    relatedFile.PropertiesCount = 2;

    ADUC_FileEntity entity{};
    entity.RelatedFiles = &relatedFile;
    entity.RelatedFileCount = 1;

    ADUC_Result result = MicrosoftDeltaDownloadHandler_ProcessUpdate(
        workflowHandle,
        &entity,
        "/tmp/payload.swu",
        "/tmp/nonexistent-source-update-cache");

    CHECK(result.ResultCode == ADUC_Result_Download_Handler_RequiredFullDownload);

    workflow_free(workflowHandle);
}

TEST_CASE("OnUpdateWorkflowCompleted non-null workflow does not return bad-args error")
{
    ADUC_WorkflowHandle workflowHandle = CreateMinimalWorkflowHandle();

    ADUC_Result result =
        MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted(workflowHandle, "/tmp/nonexistent-source-update-cache");

    CHECK(result.ExtendedResultCode != ADUC_ERC_DDH_BAD_ARGS);

    workflow_free(workflowHandle);
}
