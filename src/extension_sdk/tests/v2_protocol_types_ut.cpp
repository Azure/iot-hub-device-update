/**
 * @file v2_protocol_types_ut.cpp
 * @brief Unit tests for v3 protocol types: Outcome, FailureOrigin, DeploymentResult2, StepResultDetail.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_test_macros.hpp>
#include <cstring>

extern "C"
{
#include "aduc/extension_types.h"
#include "aduc/communication_vtable.h"
#include "aduc/step_result.h"
}

TEST_CASE("ADUC_Outcome enum values", "[v3protocol]")
{
    CHECK(ADUC_Outcome_Succeeded == 0);
    CHECK(ADUC_Outcome_Failed == 1);
    CHECK(ADUC_Outcome_Canceled == 2);
    CHECK(ADUC_Outcome_Skipped == 3);
    // Backward compat alias
    CHECK(ADUC_Outcome_Cancelled == ADUC_Outcome_Canceled);
}

TEST_CASE("ADUC_Outcome_ToString uses v3 spelling", "[v3protocol]")
{
    CHECK(std::strcmp(ADUC_Outcome_ToString(ADUC_Outcome_Succeeded), "SUCCEEDED") == 0);
    CHECK(std::strcmp(ADUC_Outcome_ToString(ADUC_Outcome_Failed), "FAILED") == 0);
    CHECK(std::strcmp(ADUC_Outcome_ToString(ADUC_Outcome_Canceled), "CANCELED") == 0);
    CHECK(std::strcmp(ADUC_Outcome_ToString(ADUC_Outcome_Skipped), "SKIPPED") == 0);
    CHECK(std::strcmp(ADUC_Outcome_ToString(static_cast<ADUC_Outcome>(99)), "FAILED") == 0);
}

TEST_CASE("ADUC_FailureOrigin enum values", "[v3protocol]")
{
    CHECK(ADUC_FailureOrigin_NotApplicable == 0);
    CHECK(ADUC_FailureOrigin_AduCloudService == 1);
    CHECK(ADUC_FailureOrigin_AduManagedResource == 2);
    CHECK(ADUC_FailureOrigin_AgentCore == 3);
    CHECK(ADUC_FailureOrigin_AgentExtension == 4);
    CHECK(ADUC_FailureOrigin_AgentDependency == 5);
    CHECK(ADUC_FailureOrigin_Device == 6);
}

TEST_CASE("ADUC_FailureOrigin_ToString v3 wire strings", "[v3protocol][v4protocol]")
{
    CHECK(std::strcmp(ADUC_FailureOrigin_ToString(ADUC_FailureOrigin_NotApplicable), "NOT_APPLICABLE") == 0);
    CHECK(std::strcmp(ADUC_FailureOrigin_ToString(ADUC_FailureOrigin_AduCloudService), "ADU_CLOUD_SERVICE") == 0);
    CHECK(std::strcmp(ADUC_FailureOrigin_ToString(ADUC_FailureOrigin_AduManagedResource), "ADU_MANAGED_RESOURCE") == 0);
    CHECK(std::strcmp(ADUC_FailureOrigin_ToString(ADUC_FailureOrigin_AgentCore), "AGENT_CORE") == 0);
    CHECK(std::strcmp(ADUC_FailureOrigin_ToString(ADUC_FailureOrigin_AgentExtension), "AGENT_EXTENSION") == 0);
    CHECK(std::strcmp(ADUC_FailureOrigin_ToString(ADUC_FailureOrigin_AgentDependency), "AGENT_DEPENDENCY") == 0);
    CHECK(std::strcmp(ADUC_FailureOrigin_ToString(ADUC_FailureOrigin_Device), "DEVICE") == 0);
    CHECK(std::strcmp(ADUC_FailureOrigin_ToString(ADUC_FailureOrigin_Other), "OTHER") == 0);
    /* v4: unknown values default to OTHER (was AGENT_CORE in v3) */
    CHECK(std::strcmp(ADUC_FailureOrigin_ToString(static_cast<ADUC_FailureOrigin>(99)), "OTHER") == 0);
}

TEST_CASE("Backward compat aliases for ADUC_Origin", "[v3protocol]")
{
    // Old ADUC_Origin names still work via #define aliases
    CHECK(ADUC_Origin_AduService == ADUC_FailureOrigin_AduCloudService);
    CHECK(ADUC_Origin_AduResource == ADUC_FailureOrigin_AduManagedResource);
    CHECK(ADUC_Origin_AgentCore == ADUC_FailureOrigin_AgentCore);
    CHECK(ADUC_Origin_AgentExtension == ADUC_FailureOrigin_AgentExtension);
    CHECK(ADUC_Origin_Device == ADUC_FailureOrigin_Device);

    // ADUC_Origin_ToString is aliased to ADUC_FailureOrigin_ToString
    CHECK(std::strcmp(ADUC_Origin_ToString(ADUC_Origin_AgentCore), "AGENT_CORE") == 0);
}

TEST_CASE("ADUC_DeploymentResult2 can be populated with v3 fields", "[v3protocol]")
{
    ADUC_DeploymentResult2 dr;
    std::memset(&dr, 0, sizeof(dr));

    dr.workflowId = "wf-123";
    dr.outcome = ADUC_Outcome_Succeeded;
    dr.failureOrigin = ADUC_FailureOrigin_NotApplicable;
    dr.resultCode = 700;
    dr.extendedResultCodes = "00000000";
    dr.resultDetails = "Install succeeded";
    dr.installedUpdateId = "{\"provider\":\"Contoso\",\"name\":\"Toaster\",\"version\":\"1.0\"}";
    dr.stepResultsJson = "{\"step_0\":{\"outcome\":\"SUCCEEDED\",\"failureOrigin\":\"NOT_APPLICABLE\"}}";

    CHECK(std::strcmp(dr.workflowId, "wf-123") == 0);
    CHECK(dr.outcome == ADUC_Outcome_Succeeded);
    CHECK(dr.failureOrigin == ADUC_FailureOrigin_NotApplicable);
    CHECK(dr.resultCode == 700);
    CHECK(std::strcmp(dr.extendedResultCodes, "00000000") == 0);
    CHECK(std::strcmp(dr.installedUpdateId, "{\"provider\":\"Contoso\",\"name\":\"Toaster\",\"version\":\"1.0\"}") == 0);
}

TEST_CASE("ADUC_DeploymentResult2 failure with v3 fields", "[v3protocol]")
{
    ADUC_DeploymentResult2 dr;
    std::memset(&dr, 0, sizeof(dr));

    dr.workflowId = "wf-fail-456";
    dr.outcome = ADUC_Outcome_Failed;
    dr.failureOrigin = ADUC_FailureOrigin_Device;
    dr.resultCode = 0;
    dr.extendedResultCodes = "3000001C";
    dr.resultDetails = "Not enough space to extract payload";
    dr.installedUpdateId = nullptr;

    CHECK(dr.outcome == ADUC_Outcome_Failed);
    CHECK(dr.failureOrigin == ADUC_FailureOrigin_Device);
    CHECK(dr.resultCode == 0);
    CHECK(dr.installedUpdateId == nullptr);
}

TEST_CASE("StepResultDetail failure includes outcome and failureOrigin", "[v3protocol]")
{
    ADUC_StepResultDetail detail = ADUC_StepResult_Failure(
        ADUC_STEP_PHASE_EXECUTE,
        ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1),
        "download failed",
        "curl");

    CHECK(detail.outcome == ADUC_Outcome_Failed);
    CHECK(detail.failureOrigin == ADUC_FailureOrigin_AgentCore);
    CHECK(std::strcmp(detail.resultDetails, "download failed") == 0);
    CHECK(std::strcmp(detail.errorSource, "curl") == 0);
    CHECK(detail.signal == ADUC_SIGNAL_ABORT_DEPLOYMENT);

    ADUC_StepResult_Free(&detail);
}

TEST_CASE("StepResultDetail success has default outcome", "[v3protocol]")
{
    ADUC_StepResultDetail detail = ADUC_StepResult_Success(ADUC_STEP_PHASE_VALIDATE);

    CHECK(detail.outcome == ADUC_Outcome_Succeeded);
    // failureOrigin should be 0 (NOT_APPLICABLE) from memset
    CHECK(detail.failureOrigin == ADUC_FailureOrigin_NotApplicable);
    CHECK(detail.resultDetails == nullptr);
    CHECK(ADUC_StepResult_IsSuccess(&detail));

    ADUC_StepResult_Free(&detail);
}

TEST_CASE("extendedResultCodes hex formatting - no 0x prefix", "[v3protocol]")
{
    // v3 spec: unsigned hex representation with NO 0x prefix
    char buf[32];
    uint32_t code = 0x3000001C;
    std::snprintf(buf, sizeof(buf), "%08X", code);
    CHECK(std::strcmp(buf, "3000001C") == 0);

    // Multiple ERCs comma-separated
    char multiBuf[64];
    uint32_t code1 = 0x3000001C;
    uint32_t code2 = 0x80004005;
    std::snprintf(multiBuf, sizeof(multiBuf), "%08X,%08X", code1, code2);
    CHECK(std::strcmp(multiBuf, "3000001C,80004005") == 0);

    // Zero ERC (success)
    std::snprintf(buf, sizeof(buf), "%08X", 0u);
    CHECK(std::strcmp(buf, "00000000") == 0);
}
