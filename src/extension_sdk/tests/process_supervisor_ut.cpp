/**
 * @file process_supervisor_ut.cpp
 * @brief Unit tests for the process supervisor module.
 */

#include <catch2/catch_all.hpp>

#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>

extern "C"
{
#include "aduc/process_identity.h"
#include "aduc/process_context.h"
#include "aduc/process_supervisor.h"
}

// --- Process Identity Tests ------------------------------------------------

TEST_CASE("ProcessIdentity: Resolve root gives uid 0", "[process_identity]")
{
    ADUC_ProcessIdentity identity = {};
    ADUC_Result2 result = ADUC_ProcessIdentity_Resolve("root", &identity);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(identity.uid == 0);
    CHECK(identity.resolved == true);
}

TEST_CASE("ProcessIdentity: Resolve non-existent user fails", "[process_identity]")
{
    ADUC_ProcessIdentity identity = {};
    ADUC_Result2 result = ADUC_ProcessIdentity_Resolve("nonexistent_user_xyz_99999", &identity);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

TEST_CASE("ProcessIdentity: GetCurrent succeeds", "[process_identity]")
{
    ADUC_ProcessIdentity identity = {};
    ADUC_Result2 result = ADUC_ProcessIdentity_GetCurrent(&identity);
    CHECK(ADUC_RESULT2_IS_SUCCESS(result));
    CHECK(identity.resolved == true);
    CHECK(strlen(identity.username) > 0);
}

// --- Process Context Tests -------------------------------------------------

static const char* TEST_CONTEXT_FILE = "test_process_context.toml";

class TempContextFile
{
public:
    TempContextFile() = default;
    ~TempContextFile() { remove(TEST_CONTEXT_FILE); }
};

TEST_CASE("ProcessContext: Write and read round-trip", "[process_context]")
{
    TempContextFile tmp;

    ADUC_ProcessContextData writeData = {};
    writeData.workflowId = "wf-001";
    writeData.deploymentId = "dep-123";
    writeData.stepId = "step-A";
    writeData.handlerType = "apt";
    writeData.workFolder = "/tmp/adu/work";
    writeData.contentDir = "/tmp/adu/content";
    writeData.ipcSocketPath = "/run/adu/ipc.sock";
    writeData.installedCriteria = "1.0.0";
    writeData.stepIndex = 0;
    writeData.totalSteps = 3;
    writeData.componentId = "comp1";
    writeData.componentGroup = "group1";

    ADUC_Result2 result = ADUC_ProcessContext_Write(TEST_CONTEXT_FILE, &writeData);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    ADUC_ProcessContextData readData = {};
    result = ADUC_ProcessContext_Read(TEST_CONTEXT_FILE, &readData);
    REQUIRE(ADUC_RESULT2_IS_SUCCESS(result));

    CHECK(std::string(readData.workflowId) == "wf-001");
    CHECK(std::string(readData.deploymentId) == "dep-123");
    CHECK(std::string(readData.stepId) == "step-A");
    CHECK(std::string(readData.handlerType) == "apt");
    CHECK(readData.stepIndex == 0);
    CHECK(readData.totalSteps == 3);

    ADUC_ProcessContext_Free(&readData);
}

TEST_CASE("ProcessContext: Read non-existent file fails", "[process_context]")
{
    ADUC_ProcessContextData data = {};
    ADUC_Result2 result = ADUC_ProcessContext_Read("/no/such/file_xyz.toml", &data);
    CHECK(ADUC_RESULT2_IS_FAILURE(result));
}

TEST_CASE("ProcessContext: Free NULL does not crash", "[process_context]")
{
    ADUC_ProcessContext_Free(nullptr); // Should not crash
}

// --- Process Supervisor Tests ----------------------------------------------

TEST_CASE("ProcessSupervisor: Spawn /bin/true - exit code 0", "[process_supervisor]")
{
    const char* argv[] = { "/bin/true", nullptr };

    ADUC_ProcessConfig config = {};
    config.executablePath = "/bin/true";
    config.argv = argv;
    config.timeoutSeconds = 10;

    ADUC_ProcessHandle handle = nullptr;
    int rc = ADUC_ProcessSupervisor_Spawn(&config, &handle);
    REQUIRE(rc == 0);
    REQUIRE(handle != nullptr);

    rc = ADUC_ProcessSupervisor_Wait(handle, 10000);
    CHECK(rc == 0);

    ADUC_ProcessResult result = {};
    rc = ADUC_ProcessSupervisor_GetResult(handle, &result);
    CHECK(rc == 0);
    CHECK(result.state == ADUC_PROC_STATE_COMPLETED);
    CHECK(result.exitCode == 0);

    ADUC_ProcessSupervisor_FreeResult(&result);
    ADUC_ProcessSupervisor_FreeHandle(handle);
}

TEST_CASE("ProcessSupervisor: Spawn /bin/false - exit code 1", "[process_supervisor]")
{
    const char* argv[] = { "/bin/false", nullptr };

    ADUC_ProcessConfig config = {};
    config.executablePath = "/bin/false";
    config.argv = argv;
    config.timeoutSeconds = 10;

    ADUC_ProcessHandle handle = nullptr;
    int rc = ADUC_ProcessSupervisor_Spawn(&config, &handle);
    REQUIRE(rc == 0);

    rc = ADUC_ProcessSupervisor_Wait(handle, 10000);
    CHECK(rc == 0);

    ADUC_ProcessResult result = {};
    rc = ADUC_ProcessSupervisor_GetResult(handle, &result);
    CHECK(rc == 0);
    CHECK(result.state == ADUC_PROC_STATE_COMPLETED);
    CHECK(result.exitCode == 1);

    ADUC_ProcessSupervisor_FreeResult(&result);
    ADUC_ProcessSupervisor_FreeHandle(handle);
}

TEST_CASE("ProcessSupervisor: Capture stdout from echo", "[process_supervisor]")
{
    const char* argv[] = { "/bin/echo", "hello", nullptr };

    ADUC_ProcessConfig config = {};
    config.executablePath = "/bin/echo";
    config.argv = argv;
    config.captureOutput = true;
    config.timeoutSeconds = 10;

    ADUC_ProcessResult result = {};
    int rc = ADUC_ProcessSupervisor_Run(&config, &result);
    CHECK(rc == 0);
    CHECK(result.state == ADUC_PROC_STATE_COMPLETED);
    CHECK(result.exitCode == 0);
    REQUIRE(result.stdoutData != nullptr);
    CHECK(std::string(result.stdoutData, result.stdoutLen) == "hello\n");

    ADUC_ProcessSupervisor_FreeResult(&result);
}

TEST_CASE("ProcessSupervisor: Capture stderr", "[process_supervisor]")
{
    const char* argv[] = { "/bin/sh", "-c", "echo errmsg >&2", nullptr };

    ADUC_ProcessConfig config = {};
    config.executablePath = "/bin/sh";
    config.argv = argv;
    config.captureOutput = true;
    config.timeoutSeconds = 10;

    ADUC_ProcessResult result = {};
    int rc = ADUC_ProcessSupervisor_Run(&config, &result);
    CHECK(rc == 0);
    CHECK(result.state == ADUC_PROC_STATE_COMPLETED);
    REQUIRE(result.stderrData != nullptr);
    CHECK(std::string(result.stderrData, result.stderrLen) == "errmsg\n");

    ADUC_ProcessSupervisor_FreeResult(&result);
}

TEST_CASE("ProcessSupervisor: Timeout kills process", "[process_supervisor]")
{
    const char* argv[] = { "/bin/sleep", "60", nullptr };

    ADUC_ProcessConfig config = {};
    config.executablePath = "/bin/sleep";
    config.argv = argv;
    config.timeoutSeconds = 1;

    ADUC_ProcessResult result = {};
    int rc = ADUC_ProcessSupervisor_Run(&config, &result);
    CHECK(rc == 0);
    CHECK(result.state == ADUC_PROC_STATE_TIMED_OUT);

    ADUC_ProcessSupervisor_FreeResult(&result);
}

TEST_CASE("ProcessSupervisor: Terminate with grace period", "[process_supervisor]")
{
    const char* argv[] = { "/bin/sleep", "60", nullptr };

    ADUC_ProcessConfig config = {};
    config.executablePath = "/bin/sleep";
    config.argv = argv;

    ADUC_ProcessHandle handle = nullptr;
    int rc = ADUC_ProcessSupervisor_Spawn(&config, &handle);
    REQUIRE(rc == 0);
    REQUIRE(handle != nullptr);

    CHECK(ADUC_ProcessSupervisor_IsRunning(handle) == true);

    rc = ADUC_ProcessSupervisor_Terminate(handle, 1000);
    CHECK(rc == 0);
    CHECK(ADUC_ProcessSupervisor_IsRunning(handle) == false);

    ADUC_ProcessSupervisor_FreeHandle(handle);
}

TEST_CASE("ProcessSupervisor: Kill immediately", "[process_supervisor]")
{
    const char* argv[] = { "/bin/sleep", "60", nullptr };

    ADUC_ProcessConfig config = {};
    config.executablePath = "/bin/sleep";
    config.argv = argv;

    ADUC_ProcessHandle handle = nullptr;
    int rc = ADUC_ProcessSupervisor_Spawn(&config, &handle);
    REQUIRE(rc == 0);

    rc = ADUC_ProcessSupervisor_Kill(handle);
    CHECK(rc == 0);
    CHECK(ADUC_ProcessSupervisor_IsRunning(handle) == false);

    ADUC_ProcessSupervisor_FreeHandle(handle);
}

TEST_CASE("ProcessSupervisor: Run synchronous helper", "[process_supervisor]")
{
    const char* argv[] = { "/bin/true", nullptr };

    ADUC_ProcessConfig config = {};
    config.executablePath = "/bin/true";
    config.argv = argv;
    config.timeoutSeconds = 10;

    ADUC_ProcessResult result = {};
    int rc = ADUC_ProcessSupervisor_Run(&config, &result);
    CHECK(rc == 0);
    CHECK(result.state == ADUC_PROC_STATE_COMPLETED);
    CHECK(result.exitCode == 0);
    CHECK(result.elapsedSeconds >= 0.0);

    ADUC_ProcessSupervisor_FreeResult(&result);
}

TEST_CASE("ProcessSupervisor: Invalid executable - FAILED_TO_START", "[process_supervisor]")
{
    const char* argv[] = { "/no/such/binary_xyz", nullptr };

    ADUC_ProcessConfig config = {};
    config.executablePath = "/no/such/binary_xyz";
    config.argv = argv;
    config.timeoutSeconds = 10;

    ADUC_ProcessResult result = {};
    int rc = ADUC_ProcessSupervisor_Run(&config, &result);
    CHECK(rc == 0);
    CHECK(result.state == ADUC_PROC_STATE_FAILED_TO_START);

    ADUC_ProcessSupervisor_FreeResult(&result);
}

TEST_CASE("ProcessSupervisor: Working directory test", "[process_supervisor]")
{
    const char* argv[] = { "/bin/pwd", nullptr };

    ADUC_ProcessConfig config = {};
    config.executablePath = "/bin/pwd";
    config.argv = argv;
    config.workingDir = "/tmp";
    config.captureOutput = true;
    config.timeoutSeconds = 10;

    ADUC_ProcessResult result = {};
    int rc = ADUC_ProcessSupervisor_Run(&config, &result);
    CHECK(rc == 0);
    CHECK(result.state == ADUC_PROC_STATE_COMPLETED);
    REQUIRE(result.stdoutData != nullptr);
    // /tmp may resolve to a symlink target, but should contain "tmp"
    std::string out(result.stdoutData, result.stdoutLen);
    CHECK(out.find("tmp") != std::string::npos);

    ADUC_ProcessSupervisor_FreeResult(&result);
}

TEST_CASE("ProcessSupervisor: Environment variable passing", "[process_supervisor]")
{
    const char* argv[] = { "/bin/sh", "-c", "echo $MY_TEST_VAR", nullptr };
    const char* envp[] = { "MY_TEST_VAR=hello_from_env", nullptr };

    ADUC_ProcessConfig config = {};
    config.executablePath = "/bin/sh";
    config.argv = argv;
    config.envp = envp;
    config.captureOutput = true;
    config.timeoutSeconds = 10;

    ADUC_ProcessResult result = {};
    int rc = ADUC_ProcessSupervisor_Run(&config, &result);
    CHECK(rc == 0);
    CHECK(result.state == ADUC_PROC_STATE_COMPLETED);
    REQUIRE(result.stdoutData != nullptr);
    CHECK(std::string(result.stdoutData, result.stdoutLen) == "hello_from_env\n");

    ADUC_ProcessSupervisor_FreeResult(&result);
}

TEST_CASE("ProcessSupervisor: Max output truncation", "[process_supervisor]")
{
    // Generate more than 64 bytes of output
    const char* argv[] = { "/bin/sh", "-c", "dd if=/dev/zero bs=128 count=1 2>/dev/null | tr '\\0' 'A'", nullptr };

    ADUC_ProcessConfig config = {};
    config.executablePath = "/bin/sh";
    config.argv = argv;
    config.captureOutput = true;
    config.maxOutputBytes = 64;
    config.timeoutSeconds = 10;

    ADUC_ProcessResult result = {};
    int rc = ADUC_ProcessSupervisor_Run(&config, &result);
    CHECK(rc == 0);
    CHECK(result.state == ADUC_PROC_STATE_COMPLETED);
    REQUIRE(result.stdoutData != nullptr);
    CHECK(result.stdoutLen <= 64);

    ADUC_ProcessSupervisor_FreeResult(&result);
}

TEST_CASE("ProcessSupervisor: FreeResult on zeroed struct is safe", "[process_supervisor]")
{
    ADUC_ProcessResult result = {};
    ADUC_ProcessSupervisor_FreeResult(&result); // Should not crash
    ADUC_ProcessSupervisor_FreeResult(nullptr);  // Should not crash
}

TEST_CASE("ProcessSupervisor: FreeHandle NULL is safe", "[process_supervisor]")
{
    ADUC_ProcessSupervisor_FreeHandle(nullptr); // Should not crash
}

TEST_CASE("ProcessSupervisor: CleanupOrphans on non-existent dir succeeds", "[process_supervisor]")
{
    int rc = ADUC_ProcessSupervisor_CleanupOrphans("/no/such/orphan_dir_xyz");
    CHECK(rc == 0);
}
