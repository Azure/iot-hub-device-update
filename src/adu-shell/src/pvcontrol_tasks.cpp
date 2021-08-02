/**
 * @file pvcontrol_tasks.cpp
 * @brief Implements tasks for microsoft/pvcontrol actions. 
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 * @copyright Copyright (c) 2021, Pantacor Ltd.
 */
#include "pvcontrol_tasks.hpp"
#include "aduc/logging.h"
#include "aduc/process_utils.hpp"
#include "common_tasks.hpp"

#include <unordered_map>

namespace CommonTasks = Adu::Shell::Tasks::Common;

namespace Adu
{
namespace Shell
{
namespace Tasks
{
namespace PVControl
{
/**
 * @brief Bash script to install the image file, apply the install, or revert the apply.
 */
static const char* PVControlCommand = "/usr/lib/adu/pvcontrol";
static const char* RmCommand = "/bin/rm";

/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult Install(const ADUShell_LaunchArguments& launchArgs)
{
    ADUShellTaskResult taskResult;

    // pvcontrol install path

    Log_Info("Installing image. Path: %s", launchArgs.targetData);

    std::vector<std::string> args;

    args.emplace_back("-s");
    args.emplace_back("/var/run/pv-ctrl");
    args.emplace_back("-f");
    args.emplace_back(ADUC_PVINSTALLED_FILE_PATH);
    args.emplace_back("steps");
    args.emplace_back("install");
    args.emplace_back(launchArgs.targetData);
    taskResult.SetExitStatus(ADUC_LaunchChildProcess(PVControlCommand, args, taskResult.Output()));

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

    // pvcontrol commands run

    Log_Info("Applying image. Revision: %s", launchArgs.targetData);

    std::vector<std::string> args;

    args.emplace_back("-s");
    args.emplace_back("/var/run/pv-ctrl");
    args.emplace_back("commands");
    args.emplace_back("run");
    args.emplace_back(launchArgs.targetData);
    taskResult.SetExitStatus(ADUC_LaunchChildProcess(PVControlCommand, args, taskResult.Output()));

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

    taskResult.SetExitStatus(1);
    return taskResult;
}

/**
 * @brief Gets all info related to a given version into the data folder.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult GetStatus(const ADUShell_LaunchArguments& launchArgs)
{
    ADUShellTaskResult taskResult;
    std::vector<std::string> args;

    Log_Info("Retrieving status info from version %s", launchArgs.targetData);

    // pvcontrol steps show-progress
    if (launchArgs.targetData != nullptr)
    {
        args.emplace_back("-s");
        args.emplace_back("/var/run/pv-ctrl");
        args.emplace_back("-f");
        args.emplace_back(ADUC_PVPROGRESS_FILE_PATH);
        args.emplace_back("steps");
        args.emplace_back("show-progress");
        args.emplace_back(std::string("locals/") + launchArgs.targetData);

        taskResult.SetExitStatus(ADUC_LaunchChildProcess(PVControlCommand, args, taskResult.Output()));
        args.clear();
    }

    return taskResult;
}

/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult DoPVControlTask(const ADUShell_LaunchArguments& launchArgs)
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
            { ADUShellAction::Reboot, CommonTasks::Reboot },
            { ADUShellAction::GetStatus, GetStatus }
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

} // namespace PVControl
} // namespace Tasks
} // namespace Shell
} // namespace Adu
