/**
 * @file microsoft_delta_download_handler_exports_ut.cpp
 * @brief Non-mock unit tests for EXPORTS.c plugin export functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>

#include <aduc/contract_utils.h>
#include <aduc/logging.h>
#include <aduc/result.h>
#include <aduc/types/update_content.h>

extern "C" {
void Initialize(ADUC_LOG_SEVERITY logLevel);
void Cleanup(void);
ADUC_Result ProcessUpdate(
    const void* workflowHandle, const ADUC_FileEntity* fileEntity, const char* targetUpdateFilePath);
ADUC_Result OnUpdateWorkflowCompleted(const void* workflowHandle);
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo);
}

TEST_CASE("Initialize and Cleanup can be called")
{
    REQUIRE_NOTHROW(Initialize(ADUC_LOG_INFO));
    REQUIRE_NOTHROW(Cleanup());
}

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

TEST_CASE("ProcessUpdate returns bad-args failure for null workflow")
{
    ADUC_FileEntity entity{};
    ADUC_Result result = ProcessUpdate(nullptr, &entity, "/tmp/target");

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}

TEST_CASE("OnUpdateWorkflowCompleted returns bad-args failure for null workflow")
{
    ADUC_Result result = OnUpdateWorkflowCompleted(nullptr);

    CHECK(result.ResultCode == ADUC_GeneralResult_Failure);
    CHECK(result.ExtendedResultCode == ADUC_ERC_DDH_BAD_ARGS);
}
