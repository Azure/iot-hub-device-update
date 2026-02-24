/**
 * @file diagnostics_result_ut.cpp
 * @brief Unit Tests for the Diagnostics Result module.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "diagnostics_result.h"

#include <catch2/catch_all.hpp>
#include <cstring>

TEST_CASE("DiagnosticsResult_ToString")
{
    SECTION("Returns correct string for NoSasCredential")
    {
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_NoSasCredential), "NoSasCredential") == 0);
    }

    SECTION("Returns correct string for NoOperationId")
    {
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_NoOperationId), "NoOperationId") == 0);
    }

    SECTION("Returns correct string for NoDiagnosticsComponents")
    {
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_NoDiagnosticsComponents), "NoDiagnosticsComponents") == 0);
    }

    SECTION("Returns correct string for BadCredential")
    {
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_BadCredential), "BadCredential") == 0);
    }

    SECTION("Returns correct string for ContainerCreateFailed")
    {
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_ContainerCreateFailed), "ContainerCreateFailed") == 0);
    }

    SECTION("Returns correct string for UploadFailed")
    {
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_UploadFailed), "UploadFailed") == 0);
    }

    SECTION("Returns correct string for NoLogsFound")
    {
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_NoLogsFound), "NoLogsFound") == 0);
    }

    SECTION("Returns correct string for Failure")
    {
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_Failure), "Failure") == 0);
    }

    SECTION("Returns correct string for Success")
    {
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_Success), "Success") == 0);
    }

    SECTION("Returns Unknown for invalid result code")
    {
        // Cast an invalid value to test the default case
        Diagnostics_Result invalidResult = static_cast<Diagnostics_Result>(999);
        CHECK(strcmp(DiagnosticsResult_ToString(invalidResult), "<Unknown>") == 0);
    }

    SECTION("Returns Unknown for negative invalid result code")
    {
        // Cast an invalid negative value to test the default case
        Diagnostics_Result invalidResult = static_cast<Diagnostics_Result>(-100);
        CHECK(strcmp(DiagnosticsResult_ToString(invalidResult), "<Unknown>") == 0);
    }
}

TEST_CASE("DiagnosticsResult_ToString - Verify all enum values are covered")
{
    // This test ensures all known enum values return non-Unknown strings
    SECTION("All defined enum values return specific strings")
    {
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_NoSasCredential), "<Unknown>") != 0);
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_NoOperationId), "<Unknown>") != 0);
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_NoDiagnosticsComponents), "<Unknown>") != 0);
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_BadCredential), "<Unknown>") != 0);
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_ContainerCreateFailed), "<Unknown>") != 0);
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_UploadFailed), "<Unknown>") != 0);
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_NoLogsFound), "<Unknown>") != 0);
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_Failure), "<Unknown>") != 0);
        CHECK(strcmp(DiagnosticsResult_ToString(Diagnostics_Result_Success), "<Unknown>") != 0);
    }
}

TEST_CASE("DiagnosticsResult enum values")
{
    // Test that enum values match expected numeric values as documented
    SECTION("Verify enum numeric values")
    {
        CHECK(Diagnostics_Result_NoSasCredential == -7);
        CHECK(Diagnostics_Result_NoOperationId == -6);
        CHECK(Diagnostics_Result_NoDiagnosticsComponents == -5);
        CHECK(Diagnostics_Result_BadCredential == -4);
        CHECK(Diagnostics_Result_ContainerCreateFailed == -3);
        CHECK(Diagnostics_Result_UploadFailed == -2);
        CHECK(Diagnostics_Result_NoLogsFound == -1);
        CHECK(Diagnostics_Result_Failure == 0);
        CHECK(Diagnostics_Result_Success == 200);
    }

    SECTION("Verify enum values are distinct")
    {
        // Ensure all enum values are unique
        CHECK(Diagnostics_Result_NoSasCredential != Diagnostics_Result_NoOperationId);
        CHECK(Diagnostics_Result_NoOperationId != Diagnostics_Result_NoDiagnosticsComponents);
        CHECK(Diagnostics_Result_NoDiagnosticsComponents != Diagnostics_Result_BadCredential);
        CHECK(Diagnostics_Result_BadCredential != Diagnostics_Result_ContainerCreateFailed);
        CHECK(Diagnostics_Result_ContainerCreateFailed != Diagnostics_Result_UploadFailed);
        CHECK(Diagnostics_Result_UploadFailed != Diagnostics_Result_NoLogsFound);
        CHECK(Diagnostics_Result_NoLogsFound != Diagnostics_Result_Failure);
        CHECK(Diagnostics_Result_Failure != Diagnostics_Result_Success);
    }
}

TEST_CASE("DiagnosticsResult_ToString - Boundary Values")
{
    SECTION("Returns Unknown for values between defined enums")
    {
        // Test values in gaps between defined enum values
        CHECK(strcmp(DiagnosticsResult_ToString(static_cast<Diagnostics_Result>(1)), "<Unknown>") == 0);
        CHECK(strcmp(DiagnosticsResult_ToString(static_cast<Diagnostics_Result>(100)), "<Unknown>") == 0);
        CHECK(strcmp(DiagnosticsResult_ToString(static_cast<Diagnostics_Result>(199)), "<Unknown>") == 0);
        CHECK(strcmp(DiagnosticsResult_ToString(static_cast<Diagnostics_Result>(201)), "<Unknown>") == 0);
    }

    SECTION("Returns Unknown for boundary adjacent values")
    {
        // Test values just outside the valid range
        CHECK(strcmp(DiagnosticsResult_ToString(static_cast<Diagnostics_Result>(-8)), "<Unknown>") == 0);
    }

    SECTION("Returned strings are not null")
    {
        CHECK(DiagnosticsResult_ToString(Diagnostics_Result_NoSasCredential) != nullptr);
        CHECK(DiagnosticsResult_ToString(Diagnostics_Result_NoOperationId) != nullptr);
        CHECK(DiagnosticsResult_ToString(Diagnostics_Result_NoDiagnosticsComponents) != nullptr);
        CHECK(DiagnosticsResult_ToString(Diagnostics_Result_BadCredential) != nullptr);
        CHECK(DiagnosticsResult_ToString(Diagnostics_Result_ContainerCreateFailed) != nullptr);
        CHECK(DiagnosticsResult_ToString(Diagnostics_Result_UploadFailed) != nullptr);
        CHECK(DiagnosticsResult_ToString(Diagnostics_Result_NoLogsFound) != nullptr);
        CHECK(DiagnosticsResult_ToString(Diagnostics_Result_Failure) != nullptr);
        CHECK(DiagnosticsResult_ToString(Diagnostics_Result_Success) != nullptr);
        CHECK(DiagnosticsResult_ToString(static_cast<Diagnostics_Result>(999)) != nullptr);
    }

    SECTION("Returned strings have non-zero length")
    {
        CHECK(strlen(DiagnosticsResult_ToString(Diagnostics_Result_NoSasCredential)) > 0);
        CHECK(strlen(DiagnosticsResult_ToString(Diagnostics_Result_Success)) > 0);
        CHECK(strlen(DiagnosticsResult_ToString(static_cast<Diagnostics_Result>(999))) > 0);
    }
}
