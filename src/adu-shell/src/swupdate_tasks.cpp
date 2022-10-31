/**
 * @file swupdate_tasks.cpp
 * @brief Implements tasks for microsoft/swupdate actions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "swupdate_tasks.hpp"
#include "aduc/logging.h"
#include "aduc/process_utils.hpp"
#include "common_tasks.hpp"

#include <unordered_map>

namespace Adu
{
namespace Shell
{
namespace Tasks
{
namespace SWUpdate
{
/**
 * @brief Bash script to install the image file, apply the install, or revert the apply.
 */
static const char* SWUpdateCommand = "/usr/lib/adu/adu-swupdate.sh";

/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult Install(const ADUShell_LaunchArguments& launchArgs)
{
    ADUShellTaskResult taskResult;

    // Constructing parameter for child process.
    // command << SWUpdateCommand << " -l " << _logFolder << " -i '" << _workFolder << "/" << filename << "'";

    Log_Info("Installing image. Path: %s, Log folder: %s", launchArgs.targetData, launchArgs.logFile);

    std::vector<std::string> args;
    if (launchArgs.logFile != nullptr)
    {
        args.emplace_back("-l");
        args.emplace_back(launchArgs.logFile);
    }

    args.emplace_back("-i");
    args.emplace_back(launchArgs.targetData);
    taskResult.SetExitStatus(ADUC_LaunchChildProcess(SWUpdateCommand, args, taskResult.Output()));

    return taskResult;
}

/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult Apply(const ADUShell_LaunchArguments& launchArgs)
{
    ADUShellTaskResult taskResult;

    // Constructing parameter for child process.
    // This is equivalent to : command << SWUpdateCommand << " -l " << _logFolder << " -a"

    std::vector<std::string> args;

    if (launchArgs.logFile != nullptr)
    {
        args.emplace_back("-l");
        args.emplace_back(launchArgs.logFile);
    }

    args.emplace_back("-a");

    taskResult.SetExitStatus(ADUC_LaunchChildProcess(SWUpdateCommand, args, taskResult.Output()));
    return taskResult;
}

/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult Rollback(const ADUShell_LaunchArguments& launchArgs)
{
    return Cancel(launchArgs);
}

/**
 * @brief Changes an active partition back to the previous one.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult Cancel(const ADUShell_LaunchArguments& launchArgs)
{
    ADUShellTaskResult taskResult;

    // Constructing parameter for child process.
    // command << SWUpdateCommand << " -l " << logFolder << " -r"
    std::vector<std::string> args;

    if (launchArgs.logFile != nullptr)
    {
        args.emplace_back("-l");
        args.emplace_back(launchArgs.logFile);
    }

    args.emplace_back("-r");

    taskResult.SetExitStatus(ADUC_LaunchChildProcess(SWUpdateCommand, args, taskResult.Output()));
    return taskResult;
}

/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult DoSWUpdateTask(const ADUShell_LaunchArguments& launchArgs)
{
    ADUShellTaskResult taskResult;
    ADUShellTaskFuncType taskProc = nullptr;

    try
    {
        const std::unordered_map<ADUShellAction, ADUShellTaskFuncType> actionMap = {
            { ADUShellAction::Install, Install },
            { ADUShellAction::Apply, Apply },
            { ADUShellAction::Cancel, Cancel },
            { ADUShellAction::Rollback, Rollback },
            { ADUShellAction::Reboot, Adu::Shell::Tasks::Common::Reboot }
        };

        taskProc = actionMap.at(launchArgs.action);
    }
    catch (const std::exception& ex)
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

} // namespace SWUpdate
} // namespace Tasks
} // namespace Shell
} // namespace Adu
