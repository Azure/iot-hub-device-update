/**
 * @file aptget_tasks.cpp
 * @brief Implements functions related to microsoft/apt update tasks.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <unordered_map>

#include "aptget_tasks.h"
#include "common_tasks.hpp"

#include "aduc/logging.h"
#include "aduc/process_utils.hpp"
#include "aduc/string_utils.hpp"

namespace Adu
{
namespace Shell
{
namespace Tasks
{
namespace AptGet
{
const char* aptget_command = "apt-get";
const char* apt_option_allow_downgrades = "--allow-downgrades";
const char* apt_option_auto_remove = "--auto-remove";
const char* apt_option_download = "download";
const char* apt_option_download_only = "--download-only";
const char* apt_option_install = "install";
const char* apt_option_remove = "remove";
const char* apt_option_update = "update";
const char* apt_option_y = "-y";

/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * This could resulted in one or more packaged installed or removed from the system.
 *
 * @param launchArgs The adu-shell launch command-line arguments that has been parsed.
 * @return A result from child process.
 */
ADUShellTaskResult DoAptGetTask(const ADUShell_LaunchArguments& launchArgs)
{
    ADUShellTaskResult taskResult;
    ADUShellTaskFuncType taskProc = nullptr;

    try
    {
        // clang-format off

        static const std::unordered_map<ADUShellAction, ADUShellTaskFuncType> actionMap = {
            { ADUShellAction::Initialize, Update },
            { ADUShellAction::Download, Download },
            { ADUShellAction::Install, Install },
            { ADUShellAction::Remove, Remove },
            { ADUShellAction::Rollback, Remove }
        };

        // clang-format on

        taskProc = actionMap.at(launchArgs.action);
    }
    catch (const std::exception& ex)
    {
        Log_Error("Unsupported action: '%s'", launchArgs.action);
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

/**
 * @brief Add supported target options to the arguments list.
 *
 * @param[in] targetOptions A string contains list of options to be added to the args.
 * @param args An argument list to which option items are added.
 */
void AddOptionsToArgs(const std::string& targetOptions, std::vector<std::string>* args)
{
    if (targetOptions.empty())
    {
        return;
    }

    //  targetOptions contains additional options to be passed to apt-get.
    //
    //  e.g., "-o Dpkg::Options::=--force-confdef -o Dpkg::Options::=--force-confold"
    //
    std::vector<std::string> options = ADUC::StringUtils::Split(std::string(targetOptions), ' ');
    for (const std::string& option : options)
    {
        if (!option.empty()
            && ((option == "-o") || (option == "Dpkg::Options::=--force-confdef")
                || (option == "Dpkg::Options::=--force-confold")))
        {
            args->emplace_back(option);
        }
        else
        {
            Log_Warn("Unsupported target option '%s'", option.c_str());
        }
    }
}

/**
 * @brief Removes enclosing single-quotes in targetData, if exist. Then add splitted package names to
 * the given output argument list.
 *
 * @param targetData A string contains space delimited package names.
 *
 * e.g., 'package1=#.#.# package2=#.#.# package3-'
 *
 * If a hyphen is appended to the package name (with no intervening space),
 * the identified package will be removed if it is installed.
 * @param args An argument list to be updated.
 */
void AddPackagesToArgs(const std::string& targetData, std::vector<std::string>* args)
{
    if (targetData.empty())
    {
        return;
    }

    std::string packages = targetData;
    std::replace(packages.begin(), packages.end(), '\'', ' ');

    std::vector<std::string> packageList = ADUC::StringUtils::Split(packages, ' ');
    for (const std::string& package : packageList)
    {
        if (!package.empty())
        {
            args->emplace_back(package);
        }
    }
}

/**
 * @brief Run "apt-get update" command in a child process.
 *
 * @param launchArgs An adu-shell launch arguments.
 *
 * @return A result from child process.
 */
ADUShellTaskResult Update(const ADUShell_LaunchArguments& /*launchArgs*/)
{
    ADUShellTaskResult taskResult;

    const std::vector<std::string> aptArgs = { apt_option_update };
    taskResult.SetExitStatus(ADUC_LaunchChildProcess(aptget_command, aptArgs, taskResult.Output()));
    if (taskResult.ExitStatus() != 0)
    {
        Log_Warn("apt-get update failed. (Exit code: %d)", taskResult.ExitStatus());
    }

    return taskResult;
}

/**
 * @brief Runs "apt-get install -y --allow-downgrades --download-only" command in  a child process.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult Download(const ADUShell_LaunchArguments& launchArgs)
{
    ADUShellTaskResult taskResult;
    std::vector<std::string> aptArgs = { apt_option_y, apt_option_allow_downgrades, apt_option_download_only };

    // NOTE: Only support the first target option.
    if (!launchArgs.targetOptions.empty())
    {
        AddOptionsToArgs(launchArgs.targetOptions.front(), &aptArgs);
    }

    aptArgs.emplace_back(apt_option_install);

    size_t argsCount = aptArgs.size();
    if (launchArgs.targetData != nullptr)
    {
        AddPackagesToArgs(launchArgs.targetData, &aptArgs);
    }

    if (aptArgs.size() == argsCount)
    {
        Log_Error("Aborting download. No packages specified. --target-data: %s", launchArgs.targetData);
        taskResult.SetExitStatus(EXIT_FAILURE);
        return taskResult;
    }

    taskResult.SetExitStatus(ADUC_LaunchChildProcess(aptget_command, aptArgs, taskResult.Output()));
    return taskResult;
}

/**
 * @brief Runs "apt-get -y --allow-downgrades install" command in  a child process.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult Install(const ADUShell_LaunchArguments& launchArgs)
{
    ADUShellTaskResult taskResult;
    std::vector<std::string> aptArgs = { apt_option_y, apt_option_allow_downgrades };

    if (!launchArgs.targetOptions.empty())
    {
        AddOptionsToArgs(launchArgs.targetOptions.front(), &aptArgs);
    }

    aptArgs.emplace_back(apt_option_install);

    size_t argsCount = aptArgs.size();
    if (launchArgs.targetData != nullptr)
    {
        AddPackagesToArgs(launchArgs.targetData, &aptArgs);
    }

    if (aptArgs.size() == argsCount)
    {
        taskResult.SetExitStatus(EXIT_FAILURE);
        return taskResult;
    }

    taskResult.SetExitStatus(ADUC_LaunchChildProcess(aptget_command, aptArgs, taskResult.Output()));
    return taskResult;
}

/**
 * @brief Runs "apt-get -y --allow-downgrades remove" command in a child process.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult Remove(const ADUShell_LaunchArguments& launchArgs)
{
    ADUShellTaskResult taskResult;
    std::vector<std::string> aptArgs = { apt_option_y, apt_option_allow_downgrades };

    if (!launchArgs.targetOptions.empty())
    {
        AddOptionsToArgs(launchArgs.targetOptions.front(), &aptArgs);
    }

    size_t argsCount = aptArgs.size();
    aptArgs.emplace_back(apt_option_remove);
    if (launchArgs.targetData != nullptr)
    {
        AddPackagesToArgs(launchArgs.targetData, &aptArgs);
    }

    if (aptArgs.size() == argsCount)
    {
        Log_Error("Aborting remove. No packages specified. --target-data: %s", launchArgs.targetData);
        taskResult.SetExitStatus(EXIT_FAILURE);
        return taskResult;
    }

    taskResult.SetExitStatus(ADUC_LaunchChildProcess(aptget_command, aptArgs, taskResult.Output()));
    return taskResult;
}

/**
 * @brief Run "apt-get -y install --auto-remove" command in a child process.
 *
 * --auto-remove option is used to remove packages that were automatically installed to
 * satisfy dependencies for other packages and are now no longer needed.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult RemoveUnusedDependencies(const ADUShell_LaunchArguments& /*launchArgs*/)
{
    ADUShellTaskResult taskResult;
    std::vector<std::string> aptArgs = { apt_option_y, apt_option_install, apt_option_auto_remove };
    taskResult.SetExitStatus(ADUC_LaunchChildProcess(aptget_command, aptArgs, taskResult.Output()));
    return taskResult;
}

/**
 * @brief Runs "apt-get -y remove" and "apt-get -y install --auto-remove" commands in a child process.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult Rollback(const ADUShell_LaunchArguments& launchArgs)
{
    ADUShellTaskResult taskResult = Remove(launchArgs);
    if (taskResult.ExitStatus() == 0)
    {
        // Note: it's acceptable if we can't remove unused dependencies.
        ADUShellTaskResult taskResult2 = RemoveUnusedDependencies(launchArgs);
        if (taskResult2.ExitStatus() != 0)
        {
            Log_Warn("Failed to remove unused dependencies (Exit code: %d)", taskResult2.ExitStatus());
        }
        taskResult.Output() += taskResult2.Output();
    }
    return taskResult;
}

} // namespace AptGet
} // namespace Tasks
} // namespace Shell
} // namespace Adu
