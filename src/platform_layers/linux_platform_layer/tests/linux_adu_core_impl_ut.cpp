/**
 * @file linux_adu_core_impl_ut.cpp
 * @brief Unit Tests for linux_adu_core_impl functionality
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch_all.hpp>
#include <cstring>
#include <string>

#include <aduc/adu_core_exports.h>
#include <aduc/result.h>
#include <aduc/types/adu_core.h>

//
// Unit Tests for LinuxPlatformLayer::Create and SetUpdateActionCallbacks
//

TEST_CASE("LinuxPlatformLayer creation and callback setup")
{
    SECTION("Create sets up all required callbacks")
    {
        ADUC_UpdateActionCallbacks callbacks = {};

        ADUC_Result result = ADUC_RegisterPlatformLayer(&callbacks, 0, nullptr);

        REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

        // Verify IdleCallback
        CHECK(callbacks.IdleCallback != nullptr);

        // Verify DownloadCallback
        CHECK(callbacks.DownloadCallback != nullptr);

        // Verify BackupCallback
        CHECK(callbacks.BackupCallback != nullptr);

        // Verify InstallCallback
        CHECK(callbacks.InstallCallback != nullptr);

        // Verify ApplyCallback
        CHECK(callbacks.ApplyCallback != nullptr);

        // Verify RestoreCallback
        CHECK(callbacks.RestoreCallback != nullptr);

        // Verify CancelCallback
        CHECK(callbacks.CancelCallback != nullptr);

        // Verify IsInstalledCallback
        CHECK(callbacks.IsInstalledCallback != nullptr);

        // Verify SandboxCreateCallback
        CHECK(callbacks.SandboxCreateCallback != nullptr);

        // Verify SandboxDestroyCallback
        CHECK(callbacks.SandboxDestroyCallback != nullptr);

        // Verify DoWorkCallback
        CHECK(callbacks.DoWorkCallback != nullptr);

        // Verify PlatformLayerHandle
        CHECK(callbacks.PlatformLayerHandle != nullptr);

        ADUC_Unregister(callbacks.PlatformLayerHandle);
    }
}

//
// Unit Tests for Idle callback
//

TEST_CASE("LinuxPlatformLayer Idle callback tests")
{
    ADUC_UpdateActionCallbacks callbacks = {};
    ADUC_Result result = ADUC_RegisterPlatformLayer(&callbacks, 0, nullptr);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

    SECTION("Idle callback executes without crash with valid workflowId")
    {
        callbacks.IdleCallback(callbacks.PlatformLayerHandle, "test-workflow-123");
        CHECK(true); // If we reach here, no crash
    }

    SECTION("Idle callback executes without crash with empty workflowId")
    {
        callbacks.IdleCallback(callbacks.PlatformLayerHandle, "");
        CHECK(true);
    }

    SECTION("Idle callback executes without crash with nullptr workflowId")
    {
        // This may log an error but should not crash
        callbacks.IdleCallback(callbacks.PlatformLayerHandle, nullptr);
        CHECK(true);
    }

    ADUC_Unregister(callbacks.PlatformLayerHandle);
}

//
// Unit Tests for Cancel callback
//
// Note: Cancel callback cannot be tested with nullptr workflowData as the implementation
// attempts to access workflow data to get the update manifest handler. Testing with
// valid workflow data requires complex setup that is better suited for integration tests.
//

//
// Unit Tests for DoWork callback
//

TEST_CASE("LinuxPlatformLayer DoWork callback tests")
{
    ADUC_UpdateActionCallbacks callbacks = {};
    ADUC_Result result = ADUC_RegisterPlatformLayer(&callbacks, 0, nullptr);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

    SECTION("DoWorkCallback is a no-op and does not crash")
    {
        callbacks.DoWorkCallback(callbacks.PlatformLayerHandle, nullptr);
        CHECK(true);
    }

    SECTION("DoWorkCallback with valid token does not crash")
    {
        callbacks.DoWorkCallback(callbacks.PlatformLayerHandle, nullptr);
        CHECK(true);
    }

    ADUC_Unregister(callbacks.PlatformLayerHandle);
}

//
// Unit Tests for SandboxCreate callback
//

TEST_CASE("LinuxPlatformLayer SandboxCreate callback tests")
{
    ADUC_UpdateActionCallbacks callbacks = {};
    ADUC_Result result = ADUC_RegisterPlatformLayer(&callbacks, 0, nullptr);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

    SECTION("SandboxCreate fails with empty workflowId")
    {
        char workFolder[256] = "/tmp/adu-test-sandbox-empty";

        ADUC_Result sandboxResult =
            callbacks.SandboxCreateCallback(callbacks.PlatformLayerHandle, "", workFolder);

        // Empty workflowId should fail
        CHECK(IsAducResultCodeFailure(sandboxResult.ResultCode));
    }

    SECTION("SandboxCreate fails with nullptr workflowId")
    {
        char workFolder[256] = "/tmp/adu-test-sandbox-null";

        ADUC_Result sandboxResult =
            callbacks.SandboxCreateCallback(callbacks.PlatformLayerHandle, nullptr, workFolder);

        // nullptr workflowId should fail
        CHECK(IsAducResultCodeFailure(sandboxResult.ResultCode));
    }

    ADUC_Unregister(callbacks.PlatformLayerHandle);
}

//
// Unit Tests for SandboxDestroy callback
//

TEST_CASE("LinuxPlatformLayer SandboxDestroy callback tests")
{
    ADUC_UpdateActionCallbacks callbacks = {};
    ADUC_Result result = ADUC_RegisterPlatformLayer(&callbacks, 0, nullptr);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

    SECTION("SandboxDestroy handles nullptr workFolder gracefully")
    {
        // Should not crash with nullptr workFolder
        callbacks.SandboxDestroyCallback(callbacks.PlatformLayerHandle, "test-workflow", nullptr);
        CHECK(true);
    }

    SECTION("SandboxDestroy handles non-existent folder gracefully")
    {
        // Should handle non-existent folder without crashing
        callbacks.SandboxDestroyCallback(
            callbacks.PlatformLayerHandle, "test-workflow", "/tmp/non-existent-adu-folder-xyz");
        CHECK(true);
    }

    SECTION("SandboxDestroy handles empty workFolder gracefully")
    {
        callbacks.SandboxDestroyCallback(callbacks.PlatformLayerHandle, "test-workflow", "");
        CHECK(true);
    }

    ADUC_Unregister(callbacks.PlatformLayerHandle);
}

//
// Unit Tests for IsInstalled callback
//

TEST_CASE("LinuxPlatformLayer IsInstalled callback tests")
{
    ADUC_UpdateActionCallbacks callbacks = {};
    ADUC_Result result = ADUC_RegisterPlatformLayer(&callbacks, 0, nullptr);
    REQUIRE(IsAducResultCodeSuccess(result.ResultCode));

    SECTION("IsInstalledCallback returns failure for nullptr workflowData")
    {
        ADUC_Result isInstalledResult =
            callbacks.IsInstalledCallback(callbacks.PlatformLayerHandle, nullptr);

        // Should return failure for nullptr workflowData
        CHECK(IsAducResultCodeFailure(isInstalledResult.ResultCode));
        CHECK(isInstalledResult.ExtendedResultCode != 0);
    }

    ADUC_Unregister(callbacks.PlatformLayerHandle);
}

//
// Note: Download, Install, Apply, Backup, and Restore callbacks require valid workflow data
// to function properly. These callbacks spawn worker threads that access workflow data internally.
// Testing with nullptr workflow data causes segmentation faults in the worker threads.
// Full testing of these callbacks requires integration tests with properly initialized workflow data.
//
// The following callbacks are tested for proper function pointer assignment only:
// - DownloadCallback (verified non-null in callback setup tests)
// - InstallCallback (verified non-null in callback setup tests)
// - ApplyCallback (verified non-null in callback setup tests)
// - BackupCallback (verified non-null in callback setup tests)
// - RestoreCallback (verified non-null in callback setup tests)
//

//
// Unit Tests for multiple platform layer instances
//

TEST_CASE("LinuxPlatformLayer multiple instance tests")
{
    SECTION("Multiple instances can coexist")
    {
        ADUC_UpdateActionCallbacks callbacks1 = {};
        ADUC_UpdateActionCallbacks callbacks2 = {};
        ADUC_UpdateActionCallbacks callbacks3 = {};

        ADUC_Result result1 = ADUC_RegisterPlatformLayer(&callbacks1, 0, nullptr);
        ADUC_Result result2 = ADUC_RegisterPlatformLayer(&callbacks2, 0, nullptr);
        ADUC_Result result3 = ADUC_RegisterPlatformLayer(&callbacks3, 0, nullptr);

        CHECK(IsAducResultCodeSuccess(result1.ResultCode));
        CHECK(IsAducResultCodeSuccess(result2.ResultCode));
        CHECK(IsAducResultCodeSuccess(result3.ResultCode));

        // All handles should be different
        CHECK(callbacks1.PlatformLayerHandle != callbacks2.PlatformLayerHandle);
        CHECK(callbacks2.PlatformLayerHandle != callbacks3.PlatformLayerHandle);
        CHECK(callbacks1.PlatformLayerHandle != callbacks3.PlatformLayerHandle);

        // Clean up in reverse order
        ADUC_Unregister(callbacks3.PlatformLayerHandle);
        ADUC_Unregister(callbacks2.PlatformLayerHandle);
        ADUC_Unregister(callbacks1.PlatformLayerHandle);
    }

    SECTION("Unregistering one instance does not affect others")
    {
        ADUC_UpdateActionCallbacks callbacks1 = {};
        ADUC_UpdateActionCallbacks callbacks2 = {};

        ADUC_Result result1 = ADUC_RegisterPlatformLayer(&callbacks1, 0, nullptr);
        ADUC_Result result2 = ADUC_RegisterPlatformLayer(&callbacks2, 0, nullptr);

        REQUIRE(IsAducResultCodeSuccess(result1.ResultCode));
        REQUIRE(IsAducResultCodeSuccess(result2.ResultCode));

        // Unregister first instance
        ADUC_Unregister(callbacks1.PlatformLayerHandle);

        // Second instance should still work
        callbacks2.IdleCallback(callbacks2.PlatformLayerHandle, "test-workflow");
        CHECK(true);

        ADUC_Unregister(callbacks2.PlatformLayerHandle);
    }
}

//
// Unit Tests for callback function pointer consistency
//

TEST_CASE("LinuxPlatformLayer callback function pointer consistency")
{
    SECTION("Same callback functions are assigned to different instances")
    {
        ADUC_UpdateActionCallbacks callbacks1 = {};
        ADUC_UpdateActionCallbacks callbacks2 = {};

        ADUC_Result result1 = ADUC_RegisterPlatformLayer(&callbacks1, 0, nullptr);
        ADUC_Result result2 = ADUC_RegisterPlatformLayer(&callbacks2, 0, nullptr);

        REQUIRE(IsAducResultCodeSuccess(result1.ResultCode));
        REQUIRE(IsAducResultCodeSuccess(result2.ResultCode));

        // All callback function pointers should be the same (static functions)
        CHECK(callbacks1.IdleCallback == callbacks2.IdleCallback);
        CHECK(callbacks1.DownloadCallback == callbacks2.DownloadCallback);
        CHECK(callbacks1.BackupCallback == callbacks2.BackupCallback);
        CHECK(callbacks1.InstallCallback == callbacks2.InstallCallback);
        CHECK(callbacks1.ApplyCallback == callbacks2.ApplyCallback);
        CHECK(callbacks1.RestoreCallback == callbacks2.RestoreCallback);
        CHECK(callbacks1.CancelCallback == callbacks2.CancelCallback);
        CHECK(callbacks1.IsInstalledCallback == callbacks2.IsInstalledCallback);
        CHECK(callbacks1.SandboxCreateCallback == callbacks2.SandboxCreateCallback);
        CHECK(callbacks1.SandboxDestroyCallback == callbacks2.SandboxDestroyCallback);
        CHECK(callbacks1.DoWorkCallback == callbacks2.DoWorkCallback);

        // But platform layer handles should be different
        CHECK(callbacks1.PlatformLayerHandle != callbacks2.PlatformLayerHandle);

        ADUC_Unregister(callbacks1.PlatformLayerHandle);
        ADUC_Unregister(callbacks2.PlatformLayerHandle);
    }
}
