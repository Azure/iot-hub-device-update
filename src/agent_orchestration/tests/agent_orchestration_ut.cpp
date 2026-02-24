/**
 * @file agent_orchestration_ut.cpp
 * @brief Unit Tests for agent_orchestration library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

extern "C"
{
#include "aduc/agent_orchestration.h"
#include "aduc/types/update_content.h"
#include "aduc/types/workflow.h"
}

#include <catch2/catch_all.hpp>
#include <cstring>
#include <string>

using Catch::Matchers::Equals;

//
// Unit Tests for AgentOrchestration_GetWorkflowStep
//

TEST_CASE("AgentOrchestration_GetWorkflowStep")
{
    SECTION("ProcessDeployment action maps to ProcessDeployment step")
    {
        ADUCITF_WorkflowStep result = AgentOrchestration_GetWorkflowStep(ADUCITF_UpdateAction_ProcessDeployment);
        CHECK(result == ADUCITF_WorkflowStep_ProcessDeployment);
    }

    SECTION("Cancel action maps to Undefined step")
    {
        ADUCITF_WorkflowStep result = AgentOrchestration_GetWorkflowStep(ADUCITF_UpdateAction_Cancel);
        CHECK(result == ADUCITF_WorkflowStep_Undefined);
    }

    SECTION("Undefined action maps to Undefined step")
    {
        ADUCITF_WorkflowStep result = AgentOrchestration_GetWorkflowStep(ADUCITF_UpdateAction_Undefined);
        CHECK(result == ADUCITF_WorkflowStep_Undefined);
    }

    SECTION("Invalid_Download action maps to Undefined step")
    {
        ADUCITF_WorkflowStep result = AgentOrchestration_GetWorkflowStep(ADUCITF_UpdateAction_Invalid_Download);
        CHECK(result == ADUCITF_WorkflowStep_Undefined);
    }

    SECTION("Invalid_Install action maps to Undefined step")
    {
        ADUCITF_WorkflowStep result = AgentOrchestration_GetWorkflowStep(ADUCITF_UpdateAction_Invalid_Install);
        CHECK(result == ADUCITF_WorkflowStep_Undefined);
    }

    SECTION("Invalid_Apply action maps to Undefined step")
    {
        ADUCITF_WorkflowStep result = AgentOrchestration_GetWorkflowStep(ADUCITF_UpdateAction_Invalid_Apply);
        CHECK(result == ADUCITF_WorkflowStep_Undefined);
    }

    SECTION("Out of range positive value maps to Undefined step")
    {
        // Test with a value that's not defined in the enum
        ADUCITF_WorkflowStep result = AgentOrchestration_GetWorkflowStep(static_cast<ADUCITF_UpdateAction>(100));
        CHECK(result == ADUCITF_WorkflowStep_Undefined);
    }

    SECTION("Out of range negative value maps to Undefined step")
    {
        // Test with a negative value other than -1 (Undefined)
        ADUCITF_WorkflowStep result = AgentOrchestration_GetWorkflowStep(static_cast<ADUCITF_UpdateAction>(-10));
        CHECK(result == ADUCITF_WorkflowStep_Undefined);
    }
}

//
// Unit Tests for AgentOrchestration_IsWorkflowComplete
//

TEST_CASE("AgentOrchestration_IsWorkflowComplete")
{
    SECTION("Undefined step indicates workflow is complete")
    {
        bool result = AgentOrchestration_IsWorkflowComplete(ADUCITF_WorkflowStep_Undefined);
        CHECK(result == true);
    }

    SECTION("ProcessDeployment step indicates workflow is NOT complete")
    {
        bool result = AgentOrchestration_IsWorkflowComplete(ADUCITF_WorkflowStep_ProcessDeployment);
        CHECK(result == false);
    }

    SECTION("Download step indicates workflow is NOT complete")
    {
        bool result = AgentOrchestration_IsWorkflowComplete(ADUCITF_WorkflowStep_Download);
        CHECK(result == false);
    }

    SECTION("Backup step indicates workflow is NOT complete")
    {
        bool result = AgentOrchestration_IsWorkflowComplete(ADUCITF_WorkflowStep_Backup);
        CHECK(result == false);
    }

    SECTION("Install step indicates workflow is NOT complete")
    {
        bool result = AgentOrchestration_IsWorkflowComplete(ADUCITF_WorkflowStep_Install);
        CHECK(result == false);
    }

    SECTION("Apply step indicates workflow is NOT complete")
    {
        bool result = AgentOrchestration_IsWorkflowComplete(ADUCITF_WorkflowStep_Apply);
        CHECK(result == false);
    }

    SECTION("Restore step indicates workflow is NOT complete")
    {
        bool result = AgentOrchestration_IsWorkflowComplete(ADUCITF_WorkflowStep_Restore);
        CHECK(result == false);
    }

    SECTION("Arbitrary non-zero value indicates workflow is NOT complete")
    {
        // Test with a value outside normal enum range
        bool result = AgentOrchestration_IsWorkflowComplete(static_cast<ADUCITF_WorkflowStep>(999));
        CHECK(result == false);
    }
}

//
// Unit Tests for AgentOrchestration_ShouldNotReportToCloud
//

TEST_CASE("AgentOrchestration_ShouldNotReportToCloud")
{
    SECTION("DeploymentInProgress state SHOULD report to cloud")
    {
        bool shouldNotReport = AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State_DeploymentInProgress);
        CHECK(shouldNotReport == false);
    }

    SECTION("Idle state SHOULD report to cloud")
    {
        bool shouldNotReport = AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State_Idle);
        CHECK(shouldNotReport == false);
    }

    SECTION("Failed state SHOULD report to cloud")
    {
        bool shouldNotReport = AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State_Failed);
        CHECK(shouldNotReport == false);
    }

    SECTION("DownloadStarted state should NOT report to cloud")
    {
        bool shouldNotReport = AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State_DownloadStarted);
        CHECK(shouldNotReport == true);
    }

    SECTION("DownloadSucceeded state should NOT report to cloud")
    {
        bool shouldNotReport = AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State_DownloadSucceeded);
        CHECK(shouldNotReport == true);
    }

    SECTION("InstallStarted state should NOT report to cloud")
    {
        bool shouldNotReport = AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State_InstallStarted);
        CHECK(shouldNotReport == true);
    }

    SECTION("InstallSucceeded state should NOT report to cloud")
    {
        bool shouldNotReport = AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State_InstallSucceeded);
        CHECK(shouldNotReport == true);
    }

    SECTION("ApplyStarted state should NOT report to cloud")
    {
        bool shouldNotReport = AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State_ApplyStarted);
        CHECK(shouldNotReport == true);
    }

    SECTION("BackupStarted state should NOT report to cloud")
    {
        bool shouldNotReport = AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State_BackupStarted);
        CHECK(shouldNotReport == true);
    }

    SECTION("BackupSucceeded state should NOT report to cloud")
    {
        bool shouldNotReport = AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State_BackupSucceeded);
        CHECK(shouldNotReport == true);
    }

    SECTION("RestoreStarted state should NOT report to cloud")
    {
        bool shouldNotReport = AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State_RestoreStarted);
        CHECK(shouldNotReport == true);
    }

    SECTION("None state should NOT report to cloud")
    {
        bool shouldNotReport = AgentOrchestration_ShouldNotReportToCloud(ADUCITF_State_None);
        CHECK(shouldNotReport == true);
    }
}

//
// Unit Tests for AgentOrchestration_IsRetryApplicable
//

TEST_CASE("AgentOrchestration_IsRetryApplicable")
{
    SECTION("Both tokens NULL - no retry")
    {
        bool result = AgentOrchestration_IsRetryApplicable(nullptr, nullptr);
        CHECK(result == false);
    }

    SECTION("Current token NULL, new token non-NULL - canonical retry")
    {
        bool result = AgentOrchestration_IsRetryApplicable(nullptr, "2024-01-01T12:00:00Z");
        CHECK(result == true);
    }

    SECTION("Current token non-NULL, new token NULL - no retry")
    {
        bool result = AgentOrchestration_IsRetryApplicable("2024-01-01T12:00:00Z", nullptr);
        CHECK(result == false);
    }

    SECTION("Both tokens same - no retry")
    {
        const char* token = "2024-01-01T12:00:00Z";
        bool result = AgentOrchestration_IsRetryApplicable(token, token);
        CHECK(result == false);
    }

    SECTION("Both tokens equal strings - no retry")
    {
        bool result = AgentOrchestration_IsRetryApplicable("2024-01-01T12:00:00Z", "2024-01-01T12:00:00Z");
        CHECK(result == false);
    }

    SECTION("Tokens different - retry applicable")
    {
        bool result = AgentOrchestration_IsRetryApplicable("2024-01-01T12:00:00Z", "2024-01-01T13:00:00Z");
        CHECK(result == true);
    }

    SECTION("Empty current token, non-empty new token - retry applicable")
    {
        bool result = AgentOrchestration_IsRetryApplicable("", "2024-01-01T12:00:00Z");
        CHECK(result == true);
    }

    SECTION("Non-empty current token, empty new token - retry applicable")
    {
        bool result = AgentOrchestration_IsRetryApplicable("2024-01-01T12:00:00Z", "");
        CHECK(result == true);
    }

    SECTION("Both tokens empty strings - no retry")
    {
        bool result = AgentOrchestration_IsRetryApplicable("", "");
        CHECK(result == false);
    }

    SECTION("Tokens with different lengths - retry applicable")
    {
        bool result = AgentOrchestration_IsRetryApplicable("short", "much_longer_token_value");
        CHECK(result == true);
    }

    SECTION("Tokens case sensitive comparison - retry applicable")
    {
        bool result = AgentOrchestration_IsRetryApplicable("Token", "token");
        CHECK(result == true);
    }

    SECTION("Tokens with whitespace difference - retry applicable")
    {
        bool result = AgentOrchestration_IsRetryApplicable("token ", "token");
        CHECK(result == true);
    }

    SECTION("Complex timestamp tokens - different times")
    {
        bool result = AgentOrchestration_IsRetryApplicable(
            "2024-06-15T08:30:45.123Z",
            "2024-06-15T08:30:45.124Z"
        );
        CHECK(result == true);
    }

    SECTION("Complex timestamp tokens - same time")
    {
        bool result = AgentOrchestration_IsRetryApplicable(
            "2024-06-15T08:30:45.123Z",
            "2024-06-15T08:30:45.123Z"
        );
        CHECK(result == false);
    }
}

//
// Edge case and boundary tests
//

TEST_CASE("AgentOrchestration edge cases")
{
    SECTION("GetWorkflowStep with boundary Cancel value (255)")
    {
        // Cancel is defined as 255, verify it's handled correctly
        ADUCITF_WorkflowStep result = AgentOrchestration_GetWorkflowStep(static_cast<ADUCITF_UpdateAction>(255));
        CHECK(result == ADUCITF_WorkflowStep_Undefined);
    }

    SECTION("IsWorkflowComplete with zero value")
    {
        // Zero is ADUCITF_WorkflowStep_Undefined
        bool result = AgentOrchestration_IsWorkflowComplete(static_cast<ADUCITF_WorkflowStep>(0));
        CHECK(result == true);
    }

    SECTION("ShouldNotReportToCloud with boundary Failed value (255)")
    {
        // Failed is defined as 255
        bool result = AgentOrchestration_ShouldNotReportToCloud(static_cast<ADUCITF_State>(255));
        CHECK(result == false); // Failed should report
    }

    SECTION("IsRetryApplicable with very long tokens")
    {
        std::string longToken1(1000, 'a');
        std::string longToken2(1000, 'b');
        bool result = AgentOrchestration_IsRetryApplicable(longToken1.c_str(), longToken2.c_str());
        CHECK(result == true);
    }

    SECTION("IsRetryApplicable with identical very long tokens")
    {
        std::string longToken(1000, 'a');
        bool result = AgentOrchestration_IsRetryApplicable(longToken.c_str(), longToken.c_str());
        CHECK(result == false);
    }

    SECTION("IsRetryApplicable with special characters in tokens")
    {
        bool result = AgentOrchestration_IsRetryApplicable(
            "token!@#$%^&*()",
            "token!@#$%^&*()"
        );
        CHECK(result == false);
    }

    SECTION("IsRetryApplicable with unicode-like characters")
    {
        bool result = AgentOrchestration_IsRetryApplicable(
            "token\xC3\xA9", // UTF-8 encoded é
            "tokene"
        );
        CHECK(result == true);
    }
}

//
// Comprehensive workflow step mapping tests
//

TEST_CASE("AgentOrchestration_GetWorkflowStep comprehensive mapping")
{
    SECTION("All defined UpdateAction values are handled")
    {
        // Test all explicitly defined UpdateAction values
        struct TestCase {
            ADUCITF_UpdateAction action;
            ADUCITF_WorkflowStep expectedStep;
            const char* description;
        };

        TestCase testCases[] = {
            { ADUCITF_UpdateAction_Invalid_Download, ADUCITF_WorkflowStep_Undefined, "Invalid_Download" },
            { ADUCITF_UpdateAction_Invalid_Install, ADUCITF_WorkflowStep_Undefined, "Invalid_Install" },
            { ADUCITF_UpdateAction_Invalid_Apply, ADUCITF_WorkflowStep_Undefined, "Invalid_Apply" },
            { ADUCITF_UpdateAction_ProcessDeployment, ADUCITF_WorkflowStep_ProcessDeployment, "ProcessDeployment" },
            { ADUCITF_UpdateAction_Cancel, ADUCITF_WorkflowStep_Undefined, "Cancel" },
            { ADUCITF_UpdateAction_Undefined, ADUCITF_WorkflowStep_Undefined, "Undefined" },
        };

        for (const auto& tc : testCases)
        {
            INFO("Testing action: " << tc.description);
            ADUCITF_WorkflowStep result = AgentOrchestration_GetWorkflowStep(tc.action);
            CHECK(result == tc.expectedStep);
        }
    }
}

//
// Comprehensive state reporting tests
//

TEST_CASE("AgentOrchestration_ShouldNotReportToCloud comprehensive")
{
    SECTION("All defined State values are handled correctly")
    {
        struct TestCase {
            ADUCITF_State state;
            bool expectedShouldNotReport;
            const char* description;
        };

        TestCase testCases[] = {
            { ADUCITF_State_None, true, "None" },
            { ADUCITF_State_Idle, false, "Idle" },
            { ADUCITF_State_DownloadStarted, true, "DownloadStarted" },
            { ADUCITF_State_DownloadSucceeded, true, "DownloadSucceeded" },
            { ADUCITF_State_InstallStarted, true, "InstallStarted" },
            { ADUCITF_State_InstallSucceeded, true, "InstallSucceeded" },
            { ADUCITF_State_ApplyStarted, true, "ApplyStarted" },
            { ADUCITF_State_DeploymentInProgress, false, "DeploymentInProgress" },
            { ADUCITF_State_BackupStarted, true, "BackupStarted" },
            { ADUCITF_State_BackupSucceeded, true, "BackupSucceeded" },
            { ADUCITF_State_RestoreStarted, true, "RestoreStarted" },
            { ADUCITF_State_Failed, false, "Failed" },
        };

        for (const auto& tc : testCases)
        {
            INFO("Testing state: " << tc.description);
            bool result = AgentOrchestration_ShouldNotReportToCloud(tc.state);
            CHECK(result == tc.expectedShouldNotReport);
        }
    }
}

//
// Comprehensive workflow step completion tests
//

TEST_CASE("AgentOrchestration_IsWorkflowComplete comprehensive")
{
    SECTION("All defined WorkflowStep values are handled correctly")
    {
        struct TestCase {
            ADUCITF_WorkflowStep step;
            bool expectedComplete;
            const char* description;
        };

        TestCase testCases[] = {
            { ADUCITF_WorkflowStep_Undefined, true, "Undefined" },
            { ADUCITF_WorkflowStep_ProcessDeployment, false, "ProcessDeployment" },
            { ADUCITF_WorkflowStep_Download, false, "Download" },
            { ADUCITF_WorkflowStep_Backup, false, "Backup" },
            { ADUCITF_WorkflowStep_Install, false, "Install" },
            { ADUCITF_WorkflowStep_Apply, false, "Apply" },
            { ADUCITF_WorkflowStep_Restore, false, "Restore" },
        };

        for (const auto& tc : testCases)
        {
            INFO("Testing step: " << tc.description);
            bool result = AgentOrchestration_IsWorkflowComplete(tc.step);
            CHECK(result == tc.expectedComplete);
        }
    }
}
