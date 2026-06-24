/**
 * @file adushell_ut.cpp
 * @brief Unit tests for adu-shell public task result type.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "adushell.hpp"
#include "adushell_action.hpp"
#include "aptget_tasks.h"
#include "common_tasks.hpp"
#include "script_tasks.hpp"

#include <catch2/catch_all.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>

namespace
{
struct LaunchCapture
{
    std::string command;
    std::vector<std::string> args;
    int exitCode{ 0 };
    std::string output{ "mocked output" };

    void Reset()
    {
        command.clear();
        args.clear();
        exitCode = 0;
        output = "mocked output";
    }
};

LaunchCapture g_launchCapture;

// Tests in this file create, chmod/chown, and delete script files at runtime.
// They must write to a writable scratch directory under the build's configurable
// work folder (ADU_SHELL_TEST_TMP_DIR is derived from ADUC_TMP_DIR_PATH / the
// --work-folder), rather than the system temp directory (which may be
// unavailable) or the read-only test-data fixtures. Each ScopedTempDir is a
// unique per-run directory that is removed on destruction, keeping the tests
// hermetic and parallel-safe.
#ifndef ADU_SHELL_TEST_TMP_DIR
#    define ADU_SHELL_TEST_TMP_DIR "/tmp/adu/adushell-unit-tests"
#endif

class ScopedTempDir
{
public:
    ScopedTempDir()
    {
        static std::atomic<unsigned long long> counter{ 0 };
        const auto nonce =
            static_cast<unsigned long long>(std::chrono::steady_clock::now().time_since_epoch().count());
        _path = std::filesystem::path(ADU_SHELL_TEST_TMP_DIR)
            / ("adushell-ut-" + std::to_string(nonce) + "-" + std::to_string(counter.fetch_add(1)));

        std::error_code ec;
        std::filesystem::create_directories(_path, ec);
    }

    ~ScopedTempDir()
    {
        std::error_code ec;
        std::filesystem::remove_all(_path, ec);
    }

    ScopedTempDir(const ScopedTempDir&) = delete;
    ScopedTempDir& operator=(const ScopedTempDir&) = delete;

    std::filesystem::path file(const char* name) const
    {
        return _path / name;
    }

    const std::filesystem::path& path() const
    {
        return _path;
    }

private:
    std::filesystem::path _path;
};
}

int ADUC_LaunchChildProcess(const std::string& command, std::vector<std::string> args, std::string& output)
{
    g_launchCapture.command = command;
    g_launchCapture.args = args;
    output = g_launchCapture.output;
    return g_launchCapture.exitCode;
}

int ADUC_LaunchChildProcess(const std::string& command, std::vector<std::string> args, std::vector<std::string>& output)
{
    g_launchCapture.command = command;
    g_launchCapture.args = args;
    output = { g_launchCapture.output };
    return g_launchCapture.exitCode;
}

TEST_CASE("ADUShellTaskResult defaults to success and empty output")
{
    ADUShellTaskResult result;

    CHECK(result.ExitStatus() == EXIT_SUCCESS);
    CHECK(result.Output().empty());
}

TEST_CASE("ADUShellTaskResult stores updated exit status")
{
    ADUShellTaskResult result;

    result.SetExitStatus(ADUSHELL_EXIT_UNSUPPORTED);
    CHECK(result.ExitStatus() == ADUSHELL_EXIT_UNSUPPORTED);

    result.SetExitStatus(42);
    CHECK(result.ExitStatus() == 42);
}

TEST_CASE("ADUShellTaskResult output is mutable and preserved")
{
    ADUShellTaskResult result;

    result.Output() = "line1";
    result.Output() += "\nline2";

    CHECK(result.Output() == "line1\nline2");
}

TEST_CASE("ADUShellActionFromString maps known actions")
{
    CHECK(ADUShellActionFromString("initialize") == ADUShellAction::Initialize);
    CHECK(ADUShellActionFromString("download") == ADUShellAction::Download);
    CHECK(ADUShellActionFromString("install") == ADUShellAction::Install);
    CHECK(ADUShellActionFromString("remove") == ADUShellAction::Remove);
    CHECK(ADUShellActionFromString("apply") == ADUShellAction::Apply);
    CHECK(ADUShellActionFromString("cancel") == ADUShellAction::Cancel);
    CHECK(ADUShellActionFromString("rollback") == ADUShellAction::Rollback);
    CHECK(ADUShellActionFromString("reboot") == ADUShellAction::Reboot);
    CHECK(ADUShellActionFromString("execute") == ADUShellAction::Execute);
}

TEST_CASE("ADUShellActionFromString returns Unknown for unsupported action")
{
    CHECK(ADUShellActionFromString("unknown") == ADUShellAction::Unknown);
}

TEST_CASE("AptGet task routes initialize to apt-get update")
{
    g_launchCapture.Reset();

    ADUShell_LaunchArguments launchArgs{};
    launchArgs.action = ADUShellAction::Initialize;
    launchArgs.updateAction = const_cast<char*>("initialize");

    auto taskResult = Adu::Shell::Tasks::AptGet::DoAptGetTask(launchArgs);

    CHECK(taskResult.ExitStatus() == 0);
    CHECK(g_launchCapture.command == "apt-get");
    REQUIRE(g_launchCapture.args.size() == 1);
    CHECK(g_launchCapture.args[0] == "update");
}

TEST_CASE("AptGet download fails when no package list is provided")
{
    g_launchCapture.Reset();

    ADUShell_LaunchArguments launchArgs{};
    launchArgs.targetData = nullptr;

    auto taskResult = Adu::Shell::Tasks::AptGet::Download(launchArgs);

    CHECK(taskResult.ExitStatus() == EXIT_FAILURE);
}

TEST_CASE("AptGet install forwards supported options and packages")
{
    g_launchCapture.Reset();

    ADUShell_LaunchArguments launchArgs{};
    launchArgs.targetData = const_cast<char*>("'pkg-a=1.0 pkg-b-'");
    launchArgs.targetOptions.push_back(const_cast<char*>("-o Dpkg::Options::=--force-confdef bad-option"));

    auto taskResult = Adu::Shell::Tasks::AptGet::Install(launchArgs);

    CHECK(taskResult.ExitStatus() == 0);
    CHECK(g_launchCapture.command == "apt-get");
    CHECK(g_launchCapture.args.size() >= 5);
    CHECK(g_launchCapture.args[0] == "-y");
    CHECK(g_launchCapture.args[1] == "--allow-downgrades");
    CHECK(g_launchCapture.args[2] == "-o");
    CHECK(g_launchCapture.args[3] == "Dpkg::Options::=--force-confdef");
    CHECK(g_launchCapture.args[4] == "install");
}

TEST_CASE("AptGet do-task handles unsupported action")
{
    g_launchCapture.Reset();

    ADUShell_LaunchArguments launchArgs{};
    launchArgs.action = ADUShellAction::Apply;
    launchArgs.updateAction = const_cast<char*>("apply");

    auto taskResult = Adu::Shell::Tasks::AptGet::DoAptGetTask(launchArgs);

    CHECK(taskResult.ExitStatus() == EXIT_FAILURE);
}

TEST_CASE("AptGet rollback appends output from cleanup phase")
{
    g_launchCapture.Reset();
    g_launchCapture.output = "phase";

    ADUShell_LaunchArguments launchArgs{};
    launchArgs.targetData = const_cast<char*>("pkg-a-");

    auto taskResult = Adu::Shell::Tasks::AptGet::Rollback(launchArgs);

    CHECK(taskResult.ExitStatus() == 0);
    CHECK(taskResult.Output() == "phasephase");
}

TEST_CASE("Common reboot task dispatches reboot command")
{
    g_launchCapture.Reset();

    ADUShell_LaunchArguments launchArgs{};

    auto taskResult = Adu::Shell::Tasks::Common::Reboot(launchArgs);

    CHECK(taskResult.ExitStatus() == 0);
    CHECK(g_launchCapture.command == "/usr/lib/adu/adu-reboot-wrapper.sh");
    REQUIRE(g_launchCapture.args.size() == 0);
}

TEST_CASE("Common task handles unsupported action path")
{
    g_launchCapture.Reset();

    ADUShell_LaunchArguments launchArgs{};
    launchArgs.action = ADUShellAction::Apply;
    launchArgs.updateAction = const_cast<char*>("apply");

    auto taskResult = Adu::Shell::Tasks::Common::DoCommonTask(launchArgs);

    CHECK(taskResult.ExitStatus() == EXIT_FAILURE);
}

TEST_CASE("Script execute runs script with target options")
{
    g_launchCapture.Reset();

    ScopedTempDir tmp;
    const auto scriptPath = tmp.file("script.sh");
    {
        std::ofstream script(scriptPath);
        REQUIRE(script.good());
        script << "#!/bin/sh\n";
        script << "echo ok\n";
    }

    REQUIRE(::chmod(scriptPath.c_str(), S_IRWXU | S_IRGRP | S_IXGRP) == 0);

    ADUShell_LaunchArguments launchArgs{};
    launchArgs.targetData = const_cast<char*>(scriptPath.c_str());
    launchArgs.targetOptions.push_back(const_cast<char*>("--first"));
    launchArgs.targetOptions.push_back(const_cast<char*>("--second"));

    auto taskResult = Adu::Shell::Tasks::Script::Execute(launchArgs);

    CHECK(taskResult.ExitStatus() == 0);
    CHECK(g_launchCapture.command == scriptPath.string());
    REQUIRE(g_launchCapture.args.size() == 2);
    CHECK(g_launchCapture.args[0] == "--first");
    CHECK(g_launchCapture.args[1] == "--second");
}

TEST_CASE("Script execute does not run a script that does not exist")
{
    g_launchCapture.Reset();
    g_launchCapture.command = "__NOT_CALLED__";

    ScopedTempDir tmp;
    const auto missingPath = tmp.file("missing.sh");
    REQUIRE_FALSE(std::filesystem::exists(missingPath));
    const std::string missingStr = missingPath.string();

    ADUShell_LaunchArguments launchArgs{};
    launchArgs.targetData = const_cast<char*>(missingStr.c_str());

    auto taskResult = Adu::Shell::Tasks::Script::Execute(launchArgs);

    // The child process must NOT be launched when the script file is missing (issue #766).
    CHECK(g_launchCapture.command == "__NOT_CALLED__");
    // And the task must report a failure exit status.
    CHECK(taskResult.ExitStatus() != EXIT_SUCCESS);
}

TEST_CASE("Script execute restores original file permissions")
{
    g_launchCapture.Reset();

    ScopedTempDir tmp;
    const auto scriptPath = tmp.file("script.sh");
    {
        std::ofstream script(scriptPath);
        REQUIRE(script.good());
        script << "#!/bin/sh\n";
        script << "echo ok\n";
    }

    // Original permissions intentionally differ from the 0750 exec mode adu-shell applies.
    const mode_t originalMode = S_IRUSR | S_IWUSR; // 0600
    REQUIRE(::chmod(scriptPath.c_str(), originalMode) == 0);

    const std::string scriptStr = scriptPath.string();
    ADUShell_LaunchArguments launchArgs{};
    launchArgs.targetData = const_cast<char*>(scriptStr.c_str());

    auto taskResult = Adu::Shell::Tasks::Script::Execute(launchArgs);
    CHECK(taskResult.ExitStatus() == EXIT_SUCCESS);

    struct stat st = {};
    REQUIRE(::stat(scriptPath.c_str(), &st) == 0);
    // After execution the original permissions must be restored (issue #766).
    CHECK((st.st_mode & 07777) == originalMode);
}

TEST_CASE("Script execute leaves ownership unchanged when the configured user/group cannot be resolved")
{
    // The unit-test build configures ADUC_FILE_USER/ADUC_FILE_GROUP with names
    // that do not exist on the system, so the ownership-adjustment branch is
    // skipped and the file's ownership must be left untouched. (Exercising the
    // full chown-and-restore path requires a resolvable target user and root.)
    g_launchCapture.Reset();

    ScopedTempDir tmp;
    const auto scriptPath = tmp.file("script.sh");
    {
        std::ofstream script(scriptPath);
        REQUIRE(script.good());
        script << "#!/bin/sh\n";
        script << "echo ok\n";
    }

    // Use the exact exec mode so the permission path is a no-op and the test
    // isolates the ownership behavior.
    REQUIRE(::chmod(scriptPath.c_str(), S_IRWXU | S_IRGRP | S_IXGRP) == 0);

    struct stat before = {};
    REQUIRE(::stat(scriptPath.c_str(), &before) == 0);

    const std::string scriptStr = scriptPath.string();
    ADUShell_LaunchArguments launchArgs{};
    launchArgs.targetData = const_cast<char*>(scriptStr.c_str());

    auto taskResult = Adu::Shell::Tasks::Script::Execute(launchArgs);
    CHECK(taskResult.ExitStatus() == EXIT_SUCCESS);

    struct stat after = {};
    REQUIRE(::stat(scriptPath.c_str(), &after) == 0);
    CHECK(after.st_uid == before.st_uid);
    CHECK(after.st_gid == before.st_gid);
}

TEST_CASE("Script do-task handles unsupported action")
{
    g_launchCapture.Reset();

    ADUShell_LaunchArguments launchArgs{};
    launchArgs.action = ADUShellAction::Install;
    launchArgs.updateAction = const_cast<char*>("install");

    auto taskResult = Adu::Shell::Tasks::Script::DoScriptTask(launchArgs);

    CHECK(taskResult.ExitStatus() == ADUSHELL_EXIT_UNSUPPORTED);
}

TEST_CASE("adu-shell binary runs and returns an exit code")
{
    const std::string command = std::string("\"") + ADU_SHELL_BIN_PATH +
        "\" --update-type common --update-action reboot --config-folder /tmp/adu-shell-missing-config";

    const int rawStatus = std::system(command.c_str());

    CHECK(rawStatus != -1);
    CHECK(WIFEXITED(rawStatus));
}
