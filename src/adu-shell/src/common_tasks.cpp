/**
 * @file common_tasks.cpp
 * @brief Implements set of common tasks for most update type.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "common_tasks.hpp"
#include "aduc/process_utils.hpp"

#include <unordered_map>
namespace Adu
{
namespace Shell
{
namespace Tasks
{
namespace Common
{
/**
 * @brief Reboots the system.
 *
 * @param launchArgs The adu-shell launch command-line arguments that has been parsed.
 * @return A result from child process.
 */
ADUShellTaskResult Reboot(const ADUShell_LaunchArguments& /*launchArgs*/)
{
    Log_Info("Calling reboot wrapper to synchronize with agent cleanup.");
    ADUShellTaskResult taskResult;
    // Use adu-reboot-wrapper.sh which waits for lock file removal
    // The agent creates /var/run/adu-agent-reboot.lock before calling this
    // and removes it after caching and reporting status to cloud
    std::vector<std::string> args{};
    std::string output;
    int exitCode = ADUC_LaunchChildProcess("/usr/lib/adu/adu-reboot-wrapper.sh", args, output);

    if (exitCode == 0)
    {
        Log_Info("Reboot wrapper completed successfully.");
    }
    else
    {
        Log_Error("Reboot wrapper failed, exit code: %d", exitCode);
    }

    taskResult.SetExitStatus(exitCode);

    if (!output.empty())
    {
        Log_Info(output.c_str());
    }
    return taskResult;
}

/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult DoCommonTask(const ADUShell_LaunchArguments& launchArgs)
{
    ADUShellTaskResult taskResult;
    ADUShellTaskFuncType taskProc = nullptr;

    try
    {
        // clang-format off

        const std::unordered_map<ADUShellAction, ADUShellTaskFuncType> actionMap = {
            { ADUShellAction::Reboot, Reboot }
        };

        // clang-format on

        taskProc = actionMap.at(launchArgs.action);
    }
    catch (const std::exception& /* ex */)
    {
        Log_Error("Unsupported action: '%s'", launchArgs.updateAction);
        taskResult.SetExitStatus(ADUSHELL_EXIT_UNSUPPORTED);
    }

    try
    {
        taskResult = taskProc(launchArgs);
    }
    catch (const std::exception& ex)
    {
        Log_Error("Exception occurred while running task: '%s'", ex.what());
        taskResult.SetExitStatus(EXIT_FAILURE);
    }

    return taskResult;
}

} // namespace Common
} // namespace Tasks
} // namespace Shell
} // namespace Adu
