/**
 * @file v2_protocol_types_ut.cpp
 * @brief Unit tests for v2 protocol types: Outcome, Origin, DeploymentResult2, StepResultDetail.
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

TEST_CASE("ADUC_Outcome enum values", "[v2protocol]")
{
    CHECK(ADUC_Outcome_Succeeded == 0);
    CHECK(ADUC_Outcome_Failed == 1);
    CHECK(ADUC_Outcome_Cancelled == 2);
    CHECK(ADUC_Outcome_Skipped == 3);
}

TEST_CASE("ADUC_Outcome_ToString", "[v2protocol]")
{
    CHECK(std::strcmp(ADUC_Outcome_ToString(ADUC_Outcome_Succeeded), "SUCCEEDED") == 0);
    CHECK(std::strcmp(ADUC_Outcome_ToString(ADUC_Outcome_Failed), "FAILED") == 0);
    CHECK(std::strcmp(ADUC_Outcome_ToString(ADUC_Outcome_Cancelled), "CANCELLED") == 0);
    CHECK(std::strcmp(ADUC_Outcome_ToString(ADUC_Outcome_Skipped), "SKIPPED") == 0);
    CHECK(std::strcmp(ADUC_Outcome_ToString(static_cast<ADUC_Outcome>(99)), "FAILED") == 0);
}

TEST_CASE("ADUC_Origin enum values", "[v2protocol]")
{
    CHECK(ADUC_Origin_AduService == 0);
    CHECK(ADUC_Origin_AduResource == 1);
    CHECK(ADUC_Origin_AgentCore == 2);
    CHECK(ADUC_Origin_AgentExtension == 3);
    CHECK(ADUC_Origin_Device == 4);
}

TEST_CASE("ADUC_Origin_ToString", "[v2protocol]")
{
    CHECK(std::strcmp(ADUC_Origin_ToString(ADUC_Origin_AduService), "ADU_SERVICE") == 0);
    CHECK(std::strcmp(ADUC_Origin_ToString(ADUC_Origin_AduResource), "ADU_RESOURCE") == 0);
    CHECK(std::strcmp(ADUC_Origin_ToString(ADUC_Origin_AgentCore), "AGENT_CORE") == 0);
    CHECK(std::strcmp(ADUC_Origin_ToString(ADUC_Origin_AgentExtension), "AGENT_EXTENSION") == 0);
    CHECK(std::strcmp(ADUC_Origin_ToString(ADUC_Origin_Device), "DEVICE") == 0);
    CHECK(std::strcmp(ADUC_Origin_ToString(static_cast<ADUC_Origin>(99)), "AGENT_CORE") == 0);
}

TEST_CASE("ADUC_DeploymentResult2 can be populated", "[v2protocol]")
{
    ADUC_DeploymentResult2 dr;
    std::memset(&dr, 0, sizeof(dr));

    dr.workflowId = "wf-123";
    dr.outcome = ADUC_Outcome_Succeeded;
    dr.origin = ADUC_Origin_AgentCore;
    dr.resultCode = 1;
    dr.extendedResultCodes = "3000001C,80004005";
    dr.resultDetails = "Install succeeded";
    dr.installedUpdateId = "{\"provider\":\"Contoso\",\"name\":\"Toaster\",\"version\":\"1.0\"}";
    dr.stepResultsJson = "[{\"stepIndex\":0,\"outcome\":\"SUCCEEDED\"}]";

    CHECK(std::strcmp(dr.workflowId, "wf-123") == 0);
    CHECK(dr.outcome == ADUC_Outcome_Succeeded);
    CHECK(dr.origin == ADUC_Origin_AgentCore);
    CHECK(dr.resultCode == 1);
    CHECK(std::strcmp(dr.extendedResultCodes, "3000001C,80004005") == 0);
    CHECK(std::strcmp(dr.installedUpdateId, "{\"provider\":\"Contoso\",\"name\":\"Toaster\",\"version\":\"1.0\"}") == 0);
}

TEST_CASE("StepResultDetail failure includes outcome and origin", "[v2protocol]")
{
    ADUC_StepResultDetail detail = ADUC_StepResult_Failure(
        ADUC_STEP_PHASE_EXECUTE,
        ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1),
        "download failed",
        "curl");

    CHECK(detail.outcome == ADUC_Outcome_Failed);
    CHECK(detail.origin == ADUC_Origin_AgentCore);
    CHECK(std::strcmp(detail.resultDetails, "download failed") == 0);
    CHECK(std::strcmp(detail.errorSource, "curl") == 0);
    CHECK(detail.signal == ADUC_SIGNAL_ABORT_DEPLOYMENT);

    ADUC_StepResult_Free(&detail);
}

TEST_CASE("StepResultDetail success has default outcome", "[v2protocol]")
{
    ADUC_StepResultDetail detail = ADUC_StepResult_Success(ADUC_STEP_PHASE_VALIDATE);

    // Success: outcome is zeroed (ADUC_Outcome_Succeeded == 0)
    CHECK(detail.outcome == ADUC_Outcome_Succeeded);
    CHECK(detail.resultDetails == nullptr);
    CHECK(ADUC_StepResult_IsSuccess(&detail));

    ADUC_StepResult_Free(&detail);
}

TEST_CASE("extendedResultCodes hex formatting", "[v2protocol]")
{
    // Verify the hex formatting pattern used in agent_main.c
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
}
