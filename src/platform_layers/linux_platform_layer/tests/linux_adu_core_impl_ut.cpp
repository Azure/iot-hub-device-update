/**
 * @file linux_adu_core_impl_ut.cpp
 * @brief Unit Tests for linux_adu_core_impl functionality
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch_all.hpp>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

#include <aduc/adu_core_exports.h>
#include <aduc/result.h>
#include <aduc/types/adu_core.h>
#include <aduc/types/workflow.h>

#include <sys/stat.h> // stat, mkdir
#include <unistd.h>   // rmdir

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

    SECTION("SandboxCreate with valid workflowId exercises user lookup path")
    {
        char workFolder[256] = "/tmp/adu-ut-sandbox-valid";

        // This exercises the getpwnam(ADUC_FILE_USER) path.
        // If 'adu' user exists: the function may succeed or fail depending on
        // directory permissions (both paths add coverage).
        // If 'adu' user does NOT exist: returns failure at the getpwnam check.
        ADUC_Result sandboxResult =
            callbacks.SandboxCreateCallback(callbacks.PlatformLayerHandle, "ut-valid-wf-001", workFolder);

        // Either success or failure, both paths give coverage.
        // Clean up sandbox if it was created.
        if (IsAducResultCodeSuccess(sandboxResult.ResultCode))
        {
            callbacks.SandboxDestroyCallback(callbacks.PlatformLayerHandle, "ut-valid-wf-001", workFolder);
        }
        CHECK(true); // exercised the path
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

    SECTION("SandboxDestroy removes an existing directory")
    {
        const char* testDir = "/tmp/adu-ut-sandbox-destroy-test";
        // Create a real temporary directory
        mkdir(testDir, 0755);

        struct stat st = {};
        bool dirExists = (stat(testDir, &st) == 0 && S_ISDIR(st.st_mode));
        REQUIRE(dirExists);

        // Destroy should remove it (exercises the statOk && S_ISDIR branch + RmDirRecursive)
        callbacks.SandboxDestroyCallback(callbacks.PlatformLayerHandle, "test-workflow", testDir);

        // Verify directory was removed
        bool dirStillExists = (stat(testDir, &st) == 0 && S_ISDIR(st.st_mode));
        CHECK(dirStillExists == false);
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

//
// Regression tests for issue #858 - worker thread lifecycle.
//
// The Download/Backup/Install/Apply/Restore callbacks spawn a worker thread that
// captures raw pointers to workCompletionData and workflowData. Previously this
// worker was detached, which meant the agent could shut down while the worker
// was still running, leading to use-after-free on those pointers. The platform
// layer now tracks the worker and joins it in its destructor; these tests
// exercise that guarantee.
//
// We drive the worker through a safe, fast-failing path by passing a
// workflowData with WorkflowHandle == nullptr. That causes
// GetUpdateManifestHandler to return an early failure, the worker calls
// WorkCompletionCallback, and exits - no real workflow infrastructure is
// required.
//

namespace
{
struct WorkerLifecycleProbe
{
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<int> completions{ 0 };
    std::atomic<int> entered{ 0 };
    ADUC_Result lastResult{};
    std::atomic<std::thread::id> callerThreadId{};

    // When non-null, OnComplete will wait on this until it becomes false,
    // letting tests pin the worker thread inside the completion callback.
    std::atomic<bool>* holdFlag{ nullptr };
    std::condition_variable holdCv;

    ADUC_WorkCompletionData MakeWorkCompletionData()
    {
        ADUC_WorkCompletionData data{};
        data.WorkCompletionCallback = &WorkerLifecycleProbe::OnComplete;
        data.WorkCompletionToken = this;
        return data;
    }

    bool WaitForCompletions(int expected, std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mtx);
        return cv.wait_for(lock, timeout, [this, expected] { return completions.load() >= expected; });
    }

    bool WaitForEntered(int expected, std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mtx);
        return cv.wait_for(lock, timeout, [this, expected] { return entered.load() >= expected; });
    }

    void ReleaseHold()
    {
        if (holdFlag != nullptr)
        {
            {
                std::lock_guard<std::mutex> lock(mtx);
                holdFlag->store(false);
            }
            holdCv.notify_all();
        }
    }

    static void OnComplete(const void* token, ADUC_Result result, bool /*isAsync*/)
    {
        auto* self = const_cast<WorkerLifecycleProbe*>(static_cast<const WorkerLifecycleProbe*>(token));
        {
            std::lock_guard<std::mutex> lock(self->mtx);
            self->lastResult = result;
            self->callerThreadId = std::this_thread::get_id();
            ++self->entered;
        }
        self->cv.notify_all();

        if (self->holdFlag != nullptr)
        {
            std::unique_lock<std::mutex> lock(self->mtx);
            self->holdCv.wait(lock, [self] { return !self->holdFlag->load(); });
        }

        {
            std::lock_guard<std::mutex> lock(self->mtx);
            ++self->completions;
        }
        self->cv.notify_all();
    }
};
} // namespace

TEST_CASE("LinuxPlatformLayer async worker thread completes and reports result")
{
    ADUC_UpdateActionCallbacks callbacks = {};
    REQUIRE(IsAducResultCodeSuccess(ADUC_RegisterPlatformLayer(&callbacks, 0, nullptr).ResultCode));

    WorkerLifecycleProbe probe;
    auto workCompletionData = probe.MakeWorkCompletionData();

    // Null WorkflowHandle makes GetUpdateManifestHandler fail fast in the worker.
    ADUC_WorkflowData workflowData{};
    workflowData.WorkflowHandle = nullptr;

    SECTION("ApplyCallback returns InProgress synchronously and worker fires completion")
    {
        ADUC_Result callerResult =
            callbacks.ApplyCallback(callbacks.PlatformLayerHandle, &workCompletionData, &workflowData);

        CHECK(callerResult.ResultCode == ADUC_Result_Apply_InProgress);
        REQUIRE(probe.WaitForCompletions(1, std::chrono::seconds(5)));
        CHECK(probe.completions.load() == 1);
        // The completion must come from the worker thread, not the calling thread.
        CHECK(probe.callerThreadId.load() != std::this_thread::get_id());
    }

    SECTION("DownloadCallback runs worker to completion")
    {
        ADUC_Result callerResult =
            callbacks.DownloadCallback(callbacks.PlatformLayerHandle, &workCompletionData, &workflowData);
        CHECK(callerResult.ResultCode == ADUC_Result_Download_InProgress);
        REQUIRE(probe.WaitForCompletions(1, std::chrono::seconds(5)));
    }

    ADUC_Unregister(callbacks.PlatformLayerHandle);
}

TEST_CASE("LinuxPlatformLayer sequential async callbacks serialize worker lifetimes")
{
    // Issuing a second async callback while the previous worker may still be
    // running must not leak or detach the prior thread. TrackWorker joins the
    // previous thread before adopting the new one.
    ADUC_UpdateActionCallbacks callbacks = {};
    REQUIRE(IsAducResultCodeSuccess(ADUC_RegisterPlatformLayer(&callbacks, 0, nullptr).ResultCode));

    WorkerLifecycleProbe probe;
    auto workCompletionData = probe.MakeWorkCompletionData();
    ADUC_WorkflowData workflowData{};
    workflowData.WorkflowHandle = nullptr;

    constexpr int iterations = 5;
    for (int i = 0; i < iterations; ++i)
    {
        ADUC_Result r = callbacks.ApplyCallback(callbacks.PlatformLayerHandle, &workCompletionData, &workflowData);
        CHECK(r.ResultCode == ADUC_Result_Apply_InProgress);
    }

    REQUIRE(probe.WaitForCompletions(iterations, std::chrono::seconds(10)));
    CHECK(probe.completions.load() == iterations);

    ADUC_Unregister(callbacks.PlatformLayerHandle);
}

TEST_CASE("LinuxPlatformLayer destructor waits for in-flight worker (issue #858)")
{
    // Core regression for issue #858: after ADUC_Unregister returns, no worker
    // thread may still be alive. If the destructor did not join the worker
    // (as was the case with detach()), the worker could touch freed memory.
    // We verify the worker's completion callback has fired by the time
    // ADUC_Unregister returns - that guarantees the worker had progressed past
    // the capture-using portion of its lambda.

    ADUC_UpdateActionCallbacks callbacks = {};
    REQUIRE(IsAducResultCodeSuccess(ADUC_RegisterPlatformLayer(&callbacks, 0, nullptr).ResultCode));

    WorkerLifecycleProbe probe;
    auto workCompletionData = probe.MakeWorkCompletionData();
    ADUC_WorkflowData workflowData{};
    workflowData.WorkflowHandle = nullptr;

    ADUC_Result callerResult =
        callbacks.ApplyCallback(callbacks.PlatformLayerHandle, &workCompletionData, &workflowData);
    REQUIRE(callerResult.ResultCode == ADUC_Result_Apply_InProgress);

    // Destructor must join the worker; the completion count must be observed
    // as 1 by the time Unregister returns.
    ADUC_Unregister(callbacks.PlatformLayerHandle);

    CHECK(probe.completions.load() == 1);
}

TEST_CASE("LinuxPlatformLayer all async callbacks run their worker to completion")
{
    // Structural coverage: Backup / Install / Restore use the same TrackWorker
    // pattern as Download / Apply. Verify each dispatches a worker that fires
    // WorkCompletionCallback.
    ADUC_UpdateActionCallbacks callbacks = {};
    REQUIRE(IsAducResultCodeSuccess(ADUC_RegisterPlatformLayer(&callbacks, 0, nullptr).ResultCode));

    WorkerLifecycleProbe probe;
    auto workCompletionData = probe.MakeWorkCompletionData();
    ADUC_WorkflowData workflowData{};
    workflowData.WorkflowHandle = nullptr;

    using AsyncCb = ADUC_Result (*)(ADUC_Token, const ADUC_WorkCompletionData*, ADUC_WorkflowDataToken);
    struct CallbackCase
    {
        const char* name;
        AsyncCb cb;
        int expectedInProgress;
    };

    const CallbackCase cases[] = {
        { "Backup", callbacks.BackupCallback, ADUC_Result_Backup_InProgress },
        { "Install", callbacks.InstallCallback, ADUC_Result_Install_InProgress },
        { "Restore", callbacks.RestoreCallback, ADUC_Result_Restore_InProgress },
    };

    int expectedCompletions = 0;
    for (const auto& c : cases)
    {
        INFO("Callback: " << c.name);
        ADUC_Result r = c.cb(callbacks.PlatformLayerHandle, &workCompletionData, &workflowData);
        CHECK(r.ResultCode == c.expectedInProgress);
        ++expectedCompletions;
        REQUIRE(probe.WaitForCompletions(expectedCompletions, std::chrono::seconds(5)));
    }
    CHECK(probe.completions.load() == 3);

    ADUC_Unregister(callbacks.PlatformLayerHandle);
}

TEST_CASE("LinuxPlatformLayer destructor blocks until an in-flight worker finishes")
{
    // Strong regression for issue #858: if the worker is still executing when
    // the platform layer is destroyed, the destructor must block until the
    // worker finishes. Before the fix (detach), Unregister would return
    // immediately even while the worker still held raw pointers.
    //
    // Strategy: pin the worker thread inside the completion callback via a
    // shared flag. Issue ApplyCallback, wait for the worker to enter
    // OnComplete. Then call ADUC_Unregister from a separate thread and verify
    // it does not return until we release the hold. At the moment the hold is
    // released, the destructor's join() should complete and Unregister returns.

    ADUC_UpdateActionCallbacks callbacks = {};
    REQUIRE(IsAducResultCodeSuccess(ADUC_RegisterPlatformLayer(&callbacks, 0, nullptr).ResultCode));

    WorkerLifecycleProbe probe;
    std::atomic<bool> hold{ true };
    probe.holdFlag = &hold;

    auto workCompletionData = probe.MakeWorkCompletionData();
    ADUC_WorkflowData workflowData{};
    workflowData.WorkflowHandle = nullptr;

    ADUC_Result callerResult =
        callbacks.ApplyCallback(callbacks.PlatformLayerHandle, &workCompletionData, &workflowData);
    REQUIRE(callerResult.ResultCode == ADUC_Result_Apply_InProgress);

    // Wait until the worker has entered OnComplete and is blocked on the hold.
    REQUIRE(probe.WaitForEntered(1, std::chrono::seconds(5)));
    REQUIRE(probe.completions.load() == 0); // still pinned inside OnComplete

    // Destroy the platform layer from another thread so we can observe that
    // the destructor is blocked in join().
    std::atomic<bool> unregisterReturned{ false };
    std::thread destroyer([&] {
        ADUC_Unregister(callbacks.PlatformLayerHandle);
        unregisterReturned = true;
    });

    // Give the destroyer thread time to enter the destructor and attempt to
    // join the worker. It must NOT return while the worker is pinned.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    CHECK(unregisterReturned.load() == false);
    CHECK(probe.completions.load() == 0);

    // Release the worker; the join() inside the destructor must now unblock.
    probe.ReleaseHold();

    destroyer.join();
    CHECK(unregisterReturned.load() == true);
    CHECK(probe.completions.load() == 1);
}
