/**
 * @file linux_adu_core_exports_ut.cpp
 * @brief Unit Tests for linux_adu_core_exports functionality
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch_all.hpp>
#include <cstring>

#include <aduc/adu_core_exports.h>
#include <aduc/result.h>
#include <aduc/types/adu_core.h>

//
// Unit Tests for ADUC_RegisterPlatformLayer
//

TEST_CASE("ADUC_RegisterPlatformLayer basic tests")
{
    SECTION("Successful registration with valid callbacks structure")
    {
        ADUC_UpdateActionCallbacks callbacks = {};

        ADUC_Result result = ADUC_RegisterPlatformLayer(&callbacks, 0, nullptr);

        CHECK(IsAducResultCodeSuccess(result.ResultCode));
        CHECK(result.ResultCode == ADUC_Result_Register_Success);

        // Verify all callbacks are set
        CHECK(callbacks.IdleCallback != nullptr);
        CHECK(callbacks.DownloadCallback != nullptr);
        CHECK(callbacks.BackupCallback != nullptr);
        CHECK(callbacks.InstallCallback != nullptr);
        CHECK(callbacks.ApplyCallback != nullptr);
        CHECK(callbacks.RestoreCallback != nullptr);
        CHECK(callbacks.CancelCallback != nullptr);
        CHECK(callbacks.IsInstalledCallback != nullptr);
        CHECK(callbacks.SandboxCreateCallback != nullptr);
        CHECK(callbacks.SandboxDestroyCallback != nullptr);
        CHECK(callbacks.DoWorkCallback != nullptr);
        CHECK(callbacks.PlatformLayerHandle != nullptr);

        // Clean up
        ADUC_Unregister(callbacks.PlatformLayerHandle);
    }

    SECTION("Registration with argc=0 and argv=nullptr succeeds")
    {
        ADUC_UpdateActionCallbacks callbacks = {};

        ADUC_Result result = ADUC_RegisterPlatformLayer(&callbacks, 0, nullptr);

        CHECK(IsAducResultCodeSuccess(result.ResultCode));

        ADUC_Unregister(callbacks.PlatformLayerHandle);
    }

    SECTION("Multiple registrations create separate handles")
    {
        ADUC_UpdateActionCallbacks callbacks1 = {};
        ADUC_UpdateActionCallbacks callbacks2 = {};

        ADUC_Result result1 = ADUC_RegisterPlatformLayer(&callbacks1, 0, nullptr);
        ADUC_Result result2 = ADUC_RegisterPlatformLayer(&callbacks2, 0, nullptr);

        CHECK(IsAducResultCodeSuccess(result1.ResultCode));
        CHECK(IsAducResultCodeSuccess(result2.ResultCode));
        CHECK(callbacks1.PlatformLayerHandle != callbacks2.PlatformLayerHandle);

        ADUC_Unregister(callbacks1.PlatformLayerHandle);
        ADUC_Unregister(callbacks2.PlatformLayerHandle);
    }
}

//
// Unit Tests for ADUC_Unregister
//

TEST_CASE("ADUC_Unregister tests")
{
    SECTION("Unregister with valid token does not crash")
    {
        ADUC_UpdateActionCallbacks callbacks = {};

        ADUC_Result result = ADUC_RegisterPlatformLayer(&callbacks, 0, nullptr);
        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        // Should not throw or crash
        ADUC_Unregister(callbacks.PlatformLayerHandle);

        CHECK(true); // If we get here, no crash occurred
    }

    SECTION("Unregister with nullptr token does not crash")
    {
        // Should not throw or crash with nullptr
        ADUC_Unregister(nullptr);

        CHECK(true); // If we get here, no crash occurred
    }
}

//
// Unit Tests for ADUC_RestartAgent
//

TEST_CASE("ADUC_RestartAgent tests")
{
    SECTION("RestartAgent returns zero on success")
    {
        // Note: This test may have side effects in a real environment
        // In a unit test context, we verify it returns a valid result
        int result = ADUC_RestartAgent();

        // RestartAgent requests shutdown and returns 0 on success
        CHECK(result == 0);
    }
}

//
// Unit Tests for AducResultCodeIndicatesInProgress macro
//

TEST_CASE("AducResultCodeIndicatesInProgress macro tests")
{
    SECTION("Returns true for in-progress result codes")
    {
        CHECK(AducResultCodeIndicatesInProgress(ADUC_Result_Download_InProgress) == true);
        CHECK(AducResultCodeIndicatesInProgress(ADUC_Result_Backup_InProgress) == true);
        CHECK(AducResultCodeIndicatesInProgress(ADUC_Result_Install_InProgress) == true);
        CHECK(AducResultCodeIndicatesInProgress(ADUC_Result_Apply_InProgress) == true);
        CHECK(AducResultCodeIndicatesInProgress(ADUC_Result_Restore_InProgress) == true);
    }

    SECTION("Returns false for success result codes")
    {
        CHECK(AducResultCodeIndicatesInProgress(ADUC_Result_Download_Success) == false);
        CHECK(AducResultCodeIndicatesInProgress(ADUC_Result_Install_Success) == false);
        CHECK(AducResultCodeIndicatesInProgress(ADUC_Result_Apply_Success) == false);
        CHECK(AducResultCodeIndicatesInProgress(ADUC_Result_Success) == false);
        CHECK(AducResultCodeIndicatesInProgress(ADUC_Result_Register_Success) == false);
    }

    SECTION("Returns false for failure result codes")
    {
        CHECK(AducResultCodeIndicatesInProgress(ADUC_Result_Failure) == false);
        CHECK(AducResultCodeIndicatesInProgress(ADUC_Result_Failure_Cancelled) == false);
    }
}

//
// Unit Tests for Callback Function Pointers
//

TEST_CASE("Callback function pointer validation")
{
    ADUC_UpdateActionCallbacks callbacks = {};

    ADUC_Result result = ADUC_RegisterPlatformLayer(&callbacks, 0, nullptr);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

    SECTION("All callbacks are properly assigned")
    {
        // Verify all callback function pointers are set
        CHECK(callbacks.IdleCallback != nullptr);
        CHECK(callbacks.DownloadCallback != nullptr);
        CHECK(callbacks.BackupCallback != nullptr);
        CHECK(callbacks.InstallCallback != nullptr);
        CHECK(callbacks.ApplyCallback != nullptr);
        CHECK(callbacks.RestoreCallback != nullptr);
        CHECK(callbacks.CancelCallback != nullptr);
        CHECK(callbacks.IsInstalledCallback != nullptr);
        CHECK(callbacks.SandboxCreateCallback != nullptr);
        CHECK(callbacks.SandboxDestroyCallback != nullptr);
        CHECK(callbacks.DoWorkCallback != nullptr);
    }

    SECTION("DoWorkCallback can be invoked without crash")
    {
        // DoWorkCallback should be a no-op in the current implementation
        callbacks.DoWorkCallback(callbacks.PlatformLayerHandle, nullptr);
        CHECK(true);
    }

    ADUC_Unregister(callbacks.PlatformLayerHandle);
}

//
// Unit Tests for Result Code Validation
//

TEST_CASE("ADUC_ResultCode enum validation")
{
    SECTION("Failure codes are correctly defined")
    {
        CHECK(ADUC_Result_Failure == 0);
        CHECK(ADUC_Result_Failure_Cancelled == -1);
    }

    SECTION("Success codes are positive")
    {
        CHECK(ADUC_Result_Success > 0);
        CHECK(ADUC_Result_Register_Success > 0);
        CHECK(ADUC_Result_Download_Success > 0);
        CHECK(ADUC_Result_Install_Success > 0);
        CHECK(ADUC_Result_Apply_Success > 0);
    }

    SECTION("InProgress codes are distinguishable from success")
    {
        CHECK(ADUC_Result_Download_InProgress != ADUC_Result_Download_Success);
        CHECK(ADUC_Result_Install_InProgress != ADUC_Result_Install_Success);
        CHECK(ADUC_Result_Apply_InProgress != ADUC_Result_Apply_Success);
        CHECK(ADUC_Result_Backup_InProgress != ADUC_Result_Backup_Success);
        CHECK(ADUC_Result_Restore_InProgress != ADUC_Result_Restore_Success);
    }
}
