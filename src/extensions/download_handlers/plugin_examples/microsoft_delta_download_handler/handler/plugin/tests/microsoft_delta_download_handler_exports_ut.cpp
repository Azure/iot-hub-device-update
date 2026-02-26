/**
 * @file microsoft_delta_download_handler_exports_ut.cpp
 * @brief Unit Tests for EXPORTS.c — the download handler plugin export functions:
 *        Initialize, Cleanup, ProcessUpdate, OnUpdateWorkflowCompleted, GetContractInfo.
 *
 * Uses link-time mocks for ADUC_Logging_Init/Uninit,
 * MicrosoftDeltaDownloadHandler_ProcessUpdate, and
 * MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

#include "mock_exports_deps.h"
#include <aduc/contract_utils.h>
#include <aduc/logging.h>
#include <aduc/result.h>
#include <aduc/types/adu_core.h>
#include <aduc/types/update_content.h>

/* Declare the EXPORTED symbols from the EXPORTS.c source we compile in */
extern "C" {
void Initialize(ADUC_LOG_SEVERITY logLevel);
void Cleanup(void);
ADUC_Result ProcessUpdate(
    const void* workflowHandle, const ADUC_FileEntity* fileEntity, const char* targetUpdateFilePath);
ADUC_Result OnUpdateWorkflowCompleted(const void* workflowHandle);
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo);
}

// =====================================================================
// Initialize / Cleanup
// =====================================================================

TEST_CASE("Initialize calls ADUC_Logging_Init with correct log level")
{
    ResetExportsMocks();

    Initialize(static_cast<ADUC_LOG_SEVERITY>(3));  // ADUC_LOG_INFO = 3

    CHECK(g_mockExportsDeps.loggingInitCallCount == 1);
    CHECK(g_mockExportsDeps.lastLogLevel == 3);
}

TEST_CASE("Cleanup calls ADUC_Logging_Uninit")
{
    ResetExportsMocks();

    Cleanup();

    CHECK(g_mockExportsDeps.loggingUninitCallCount == 1);
}

TEST_CASE("Initialize and Cleanup can be called in sequence")
{
    ResetExportsMocks();

    Initialize(static_cast<ADUC_LOG_SEVERITY>(0));
    CHECK(g_mockExportsDeps.loggingInitCallCount == 1);

    Cleanup();
    CHECK(g_mockExportsDeps.loggingUninitCallCount == 1);
}

// =====================================================================
// ProcessUpdate
// =====================================================================

TEST_CASE("ProcessUpdate delegates to MicrosoftDeltaDownloadHandler_ProcessUpdate and returns success")
{
    ResetExportsMocks();

    g_mockExportsDeps.processUpdateResult.ResultCode = ADUC_Result_Download_Handler_SuccessSkipDownload;
    g_mockExportsDeps.processUpdateResult.ExtendedResultCode = 0;

    int dummy = 0;
    ADUC_FileEntity entity{};

    ADUC_Result result = ProcessUpdate(&dummy, &entity, "/tmp/target");

    CHECK(result.ResultCode == ADUC_Result_Download_Handler_SuccessSkipDownload);
    CHECK(result.ExtendedResultCode == 0);
    CHECK(g_mockExportsDeps.processUpdateCallCount == 1);
}

TEST_CASE("ProcessUpdate delegates and returns failure")
{
    ResetExportsMocks();

    g_mockExportsDeps.processUpdateResult.ResultCode = ADUC_Result_Download_Handler_RequiredFullDownload;
    g_mockExportsDeps.processUpdateResult.ExtendedResultCode = 0xAA;

    int dummy = 0;
    ADUC_FileEntity entity{};

    ADUC_Result result = ProcessUpdate(&dummy, &entity, "/tmp/target");

    CHECK(result.ResultCode == ADUC_Result_Download_Handler_RequiredFullDownload);
    CHECK(result.ExtendedResultCode == 0xAA);
    CHECK(g_mockExportsDeps.processUpdateCallCount == 1);
}

// =====================================================================
// OnUpdateWorkflowCompleted
// =====================================================================

TEST_CASE("OnUpdateWorkflowCompleted delegates and returns success")
{
    ResetExportsMocks();

    g_mockExportsDeps.onUpdateWorkflowCompletedResult.ResultCode = ADUC_GeneralResult_Success;
    g_mockExportsDeps.onUpdateWorkflowCompletedResult.ExtendedResultCode = 0;

    int dummy = 0;

    ADUC_Result result = OnUpdateWorkflowCompleted(&dummy);

    CHECK(result.ResultCode == ADUC_GeneralResult_Success);
    CHECK(result.ExtendedResultCode == 0);
    CHECK(g_mockExportsDeps.onUpdateWorkflowCompletedCallCount == 1);
}

TEST_CASE("OnUpdateWorkflowCompleted delegates and returns failure")
{
    ResetExportsMocks();

    g_mockExportsDeps.onUpdateWorkflowCompletedResult.ResultCode = ADUC_Result_Failure;
    g_mockExportsDeps.onUpdateWorkflowCompletedResult.ExtendedResultCode = 0xBB;

    int dummy = 0;

    ADUC_Result result = OnUpdateWorkflowCompleted(&dummy);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == 0xBB);
    CHECK(g_mockExportsDeps.onUpdateWorkflowCompletedCallCount == 1);
}

// =====================================================================
// GetContractInfo
// =====================================================================

TEST_CASE("GetContractInfo returns v1.0 contract")
{
    ADUC_ExtensionContractInfo ci{};
    ci.majorVer = 99;
    ci.minorVer = 99;

    ADUC_Result result = GetContractInfo(&ci);

    CHECK(result.ResultCode == ADUC_GeneralResult_Success);
    CHECK(result.ExtendedResultCode == 0);
    CHECK(ci.majorVer == ADUC_V1_CONTRACT_MAJOR_VER);
    CHECK(ci.minorVer == ADUC_V1_CONTRACT_MINOR_VER);
}
