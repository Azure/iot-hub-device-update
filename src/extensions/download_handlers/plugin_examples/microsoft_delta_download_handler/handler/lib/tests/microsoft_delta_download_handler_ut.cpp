/**
 * @file microsoft_delta_download_handler_ut.cpp
 * @brief Unit Tests for MicrosoftDeltaDownloadHandler_ProcessUpdate and
 *        MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted null-arg paths.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

extern "C" {
#include "aduc/microsoft_delta_download_handler.h"
}
#include <aduc/result.h>
#include <aduc/types/adu_core.h>
#include <aduc/types/update_content.h>

#include <cstring>

// =====================================================================
// MicrosoftDeltaDownloadHandler_ProcessUpdate bad-arg tests
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
// MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted bad-arg tests
// =====================================================================

TEST_CASE("OnUpdateWorkflowCompleted returns failure when workflowHandle is nullptr")
{
    ADUC_Result result = MicrosoftDeltaDownloadHandler_OnUpdateWorkflowCompleted(
        nullptr /* workflowHandle */, nullptr);

    CHECK(result.ResultCode == ADUC_Result_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}
