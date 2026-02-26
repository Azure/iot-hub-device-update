/**
 * @file microsoft_delta_download_handler_ut.cpp
 * @brief Unit Tests for MicrosoftDeltaDownloadHandler_ProcessUpdate and
 *        MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted.
 *
 * Uses link-time mocks for ProcessRelatedFile, SourceUpdateCache_Move,
 * and workflow_add_erc to test all code paths without real delta files.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

extern "C" {
#include "aduc/microsoft_delta_download_handler.h"
}
#include "mock_delta_deps.h"
#include <aduc/result.h>
#include <aduc/types/adu_core.h>
#include <aduc/types/update_content.h>

#include <cstring>

// =====================================================================
// Helpers
// =====================================================================

/// Create a minimal ADUC_Property (needed to pass the Properties != NULL check).
static ADUC_Property s_dummyProp = { const_cast<char*>("key"), const_cast<char*>("val") };

/// Create a RelatedFile with valid Properties.
static ADUC_RelatedFile MakeRelatedFile()
{
    ADUC_RelatedFile rf;
    memset(&rf, 0, sizeof(rf));
    rf.Properties = &s_dummyProp;
    rf.PropertiesCount = 1;
    return rf;
}

/// Create a RelatedFile with null Properties (triggers early error).
static ADUC_RelatedFile MakeRelatedFileNoProps()
{
    ADUC_RelatedFile rf;
    memset(&rf, 0, sizeof(rf));
    rf.Properties = nullptr;
    rf.PropertiesCount = 0;
    return rf;
}

// =====================================================================
// ProcessUpdate — bad-arg tests (null / missing inputs)
// =====================================================================

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

TEST_CASE("ProcessUpdate returns failure when all args are nullptr")
{
    ADUC_Result result = MicrosoftDeltaDownloadHandler_ProcessUpdate(
        nullptr, nullptr, nullptr, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

// =====================================================================
// ProcessUpdate — RelatedFile has no Properties
// =====================================================================

TEST_CASE("ProcessUpdate returns failure when RelatedFile has null Properties")
{
    ResetAllDeltaMocks();

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

// =====================================================================
// ProcessUpdate — single RelatedFile success
// =====================================================================

TEST_CASE("ProcessUpdate returns SuccessSkipDownload on successful RelatedFile")
{
    ResetAllDeltaMocks();

    // Mock: ProcessRelatedFile returns success
    g_mockProcessRelatedFile.numEntries = 1;
    g_mockProcessRelatedFile.results[0].ResultCode = ADUC_Result_Success;
    g_mockProcessRelatedFile.results[0].ExtendedResultCode = 0;

    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = &dummy;

    ADUC_RelatedFile rf = MakeRelatedFile();

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.RelatedFiles = &rf;
    entity.RelatedFileCount = 1;

    ADUC_Result result = MicrosoftDeltaDownloadHandler_ProcessUpdate(
        wfHandle, &entity, "/tmp/payload", nullptr);

    CHECK(result.ResultCode == ADUC_Result_Download_Handler_SuccessSkipDownload);
    CHECK(g_mockProcessRelatedFile.callCount == 1);
}

// =====================================================================
// ProcessUpdate — all RelatedFiles fail
// =====================================================================

TEST_CASE("ProcessUpdate returns RequiredFullDownload when all RelatedFiles fail")
{
    ResetAllDeltaMocks();

    // Mock: ProcessRelatedFile returns failure
    g_mockProcessRelatedFile.numEntries = 1;
    g_mockProcessRelatedFile.results[0].ResultCode = ADUC_Result_Failure;
    g_mockProcessRelatedFile.results[0].ExtendedResultCode = 0x1234;

    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = &dummy;

    ADUC_RelatedFile rf = MakeRelatedFile();

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.RelatedFiles = &rf;
    entity.RelatedFileCount = 1;

    ADUC_Result result = MicrosoftDeltaDownloadHandler_ProcessUpdate(
        wfHandle, &entity, "/tmp/payload", nullptr);

    CHECK(result.ResultCode == ADUC_Result_Download_Handler_RequiredFullDownload);
    CHECK(g_mockWorkflowAddErc.callCount == 1);
    CHECK(g_mockWorkflowAddErc.lastErc == 0x1234);
}

// =====================================================================
// ProcessUpdate — cache miss then success
// =====================================================================

TEST_CASE("ProcessUpdate handles cache miss then succeeds on next RelatedFile")
{
    ResetAllDeltaMocks();

    // Mock: first call returns cache miss, second returns success
    g_mockProcessRelatedFile.numEntries = 2;
    g_mockProcessRelatedFile.results[0].ResultCode = ADUC_Result_Success_Cache_Miss;
    g_mockProcessRelatedFile.results[0].ExtendedResultCode = 0;
    g_mockProcessRelatedFile.results[1].ResultCode = ADUC_Result_Success;
    g_mockProcessRelatedFile.results[1].ExtendedResultCode = 0;

    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = &dummy;

    ADUC_RelatedFile rfs[2];
    rfs[0] = MakeRelatedFile();
    rfs[1] = MakeRelatedFile();

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.RelatedFiles = rfs;
    entity.RelatedFileCount = 2;

    ADUC_Result result = MicrosoftDeltaDownloadHandler_ProcessUpdate(
        wfHandle, &entity, "/tmp/payload", nullptr);

    CHECK(result.ResultCode == ADUC_Result_Download_Handler_SuccessSkipDownload);
    CHECK(g_mockProcessRelatedFile.callCount == 2);
    // Cache miss adds an ERC
    CHECK(g_mockWorkflowAddErc.callCount == 1);
}

// =====================================================================
// ProcessUpdate — cache miss then failure (full download required)
// =====================================================================

TEST_CASE("ProcessUpdate returns RequiredFullDownload when cache miss then failure")
{
    ResetAllDeltaMocks();

    g_mockProcessRelatedFile.numEntries = 2;
    g_mockProcessRelatedFile.results[0].ResultCode = ADUC_Result_Success_Cache_Miss;
    g_mockProcessRelatedFile.results[0].ExtendedResultCode = 0;
    g_mockProcessRelatedFile.results[1].ResultCode = ADUC_Result_Failure;
    g_mockProcessRelatedFile.results[1].ExtendedResultCode = 0xABCD;

    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = &dummy;

    ADUC_RelatedFile rfs[2];
    rfs[0] = MakeRelatedFile();
    rfs[1] = MakeRelatedFile();

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.RelatedFiles = rfs;
    entity.RelatedFileCount = 2;

    ADUC_Result result = MicrosoftDeltaDownloadHandler_ProcessUpdate(
        wfHandle, &entity, "/tmp/payload", nullptr);

    CHECK(result.ResultCode == ADUC_Result_Download_Handler_RequiredFullDownload);
    // Cache miss ERC + failure ERC
    CHECK(g_mockWorkflowAddErc.callCount == 2);
}

// =====================================================================
// ProcessUpdate — multiple failures
// =====================================================================

TEST_CASE("ProcessUpdate adds ERC for each failed RelatedFile")
{
    ResetAllDeltaMocks();

    g_mockProcessRelatedFile.numEntries = 3;
    g_mockProcessRelatedFile.results[0].ResultCode = ADUC_Result_Failure;
    g_mockProcessRelatedFile.results[0].ExtendedResultCode = 0x1111;
    g_mockProcessRelatedFile.results[1].ResultCode = ADUC_Result_Failure;
    g_mockProcessRelatedFile.results[1].ExtendedResultCode = 0x2222;
    g_mockProcessRelatedFile.results[2].ResultCode = ADUC_Result_Failure;
    g_mockProcessRelatedFile.results[2].ExtendedResultCode = 0x3333;

    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = &dummy;

    ADUC_RelatedFile rfs[3];
    rfs[0] = MakeRelatedFile();
    rfs[1] = MakeRelatedFile();
    rfs[2] = MakeRelatedFile();

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.RelatedFiles = rfs;
    entity.RelatedFileCount = 3;

    ADUC_Result result = MicrosoftDeltaDownloadHandler_ProcessUpdate(
        wfHandle, &entity, "/tmp/payload", nullptr);

    CHECK(result.ResultCode == ADUC_Result_Download_Handler_RequiredFullDownload);
    CHECK(g_mockProcessRelatedFile.callCount == 3);
    CHECK(g_mockWorkflowAddErc.callCount == 3);
    CHECK(g_mockWorkflowAddErc.lastErc == 0x3333);
}

// =====================================================================
// ProcessUpdate — success on first stops iteration
// =====================================================================

TEST_CASE("ProcessUpdate stops iterating after first successful RelatedFile")
{
    ResetAllDeltaMocks();

    g_mockProcessRelatedFile.numEntries = 2;
    g_mockProcessRelatedFile.results[0].ResultCode = ADUC_Result_Success;
    g_mockProcessRelatedFile.results[0].ExtendedResultCode = 0;
    g_mockProcessRelatedFile.results[1].ResultCode = ADUC_Result_Failure;
    g_mockProcessRelatedFile.results[1].ExtendedResultCode = 0xBEEF;

    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = &dummy;

    ADUC_RelatedFile rfs[2];
    rfs[0] = MakeRelatedFile();
    rfs[1] = MakeRelatedFile();

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.RelatedFiles = rfs;
    entity.RelatedFileCount = 2;

    ADUC_Result result = MicrosoftDeltaDownloadHandler_ProcessUpdate(
        wfHandle, &entity, "/tmp/payload", nullptr);

    CHECK(result.ResultCode == ADUC_Result_Download_Handler_SuccessSkipDownload);
    // Should have stopped after first success — only 1 call
    CHECK(g_mockProcessRelatedFile.callCount == 1);
    CHECK(g_mockWorkflowAddErc.callCount == 0);
}

// =====================================================================
// ProcessUpdate — with updateCacheBasePath
// =====================================================================

TEST_CASE("ProcessUpdate passes updateCacheBasePath to mock")
{
    ResetAllDeltaMocks();

    g_mockProcessRelatedFile.numEntries = 1;
    g_mockProcessRelatedFile.results[0].ResultCode = ADUC_Result_Success;
    g_mockProcessRelatedFile.results[0].ExtendedResultCode = 0;

    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = &dummy;

    ADUC_RelatedFile rf = MakeRelatedFile();

    ADUC_FileEntity entity;
    memset(&entity, 0, sizeof(entity));
    entity.RelatedFiles = &rf;
    entity.RelatedFileCount = 1;

    ADUC_Result result = MicrosoftDeltaDownloadHandler_ProcessUpdate(
        wfHandle, &entity, "/tmp/payload", "/cache/base/path");

    CHECK(result.ResultCode == ADUC_Result_Download_Handler_SuccessSkipDownload);
}

// =====================================================================
// OnUpdateWorkflowCompleted tests
// =====================================================================

TEST_CASE("OnUpdateWorkflowCompleted returns failure when workflowHandle is nullptr")
{
    ADUC_Result result = MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted(
        nullptr /* workflowHandle */, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

TEST_CASE("OnUpdateWorkflowCompleted delegates to SourceUpdateCache_Move on success")
{
    ResetAllDeltaMocks();

    g_mockSourceUpdateCacheMove.result.ResultCode = ADUC_GeneralResult_Success;
    g_mockSourceUpdateCacheMove.result.ExtendedResultCode = 0;

    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = &dummy;

    ADUC_Result result = MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted(wfHandle, nullptr);

    CHECK(result.ResultCode == ADUC_GeneralResult_Success);
    CHECK(result.ExtendedResultCode == 0);
    CHECK(g_mockSourceUpdateCacheMove.callCount == 1);
}

TEST_CASE("OnUpdateWorkflowCompleted returns failure from SourceUpdateCache_Move")
{
    ResetAllDeltaMocks();

    g_mockSourceUpdateCacheMove.result.ResultCode = ADUC_Result_Failure;
    g_mockSourceUpdateCacheMove.result.ExtendedResultCode = 0x99;

    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = &dummy;

    ADUC_Result result = MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted(wfHandle, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == 0x99);
    CHECK(g_mockSourceUpdateCacheMove.callCount == 1);
}

TEST_CASE("OnUpdateWorkflowCompleted with updateCacheBasePath")
{
    ResetAllDeltaMocks();

    g_mockSourceUpdateCacheMove.result.ResultCode = ADUC_GeneralResult_Success;
    g_mockSourceUpdateCacheMove.result.ExtendedResultCode = 0;

    int dummy = 0;
    ADUC_WorkflowHandle wfHandle = &dummy;

    ADUC_Result result = MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted(
        wfHandle, "/cache/base/path");

    CHECK(result.ResultCode == ADUC_GeneralResult_Success);
    CHECK(g_mockSourceUpdateCacheMove.callCount == 1);
}
