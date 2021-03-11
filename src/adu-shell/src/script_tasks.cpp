/**
 * @file script_tasks.cpp
 * @brief Implements tasks for microsoft/script actions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "script_tasks.hpp"
#include "aduc/logging.h"
#include "aduc/process_utils.hpp"
#include "common_tasks.hpp"

#include <sys/stat.h>
#include <unordered_map>

namespace Adu
{
namespace Shell
{
namespace Tasks
{
namespace Script
{
/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * @param launchArgs An adu-shell launch arguments.
 *                   For 'microsoft/script:1' handler, launchArgs.targetData is the script file to run.
 * @return A result from child process.
 */
ADUShellTaskResult Execute(const ADUShell_LaunchArguments& launchArgs)
{
    ADUShellTaskResult taskResult;

    // Constructing parameter for child process.
    Log_Info("Executing script. Path: %s", launchArgs.targetData);

    std::vector<std::string> args;
    for (const std::string& option : launchArgs.targetOptions)
    {
        Log_Debug("args: %s", option.c_str());
        args.emplace_back(option);
    }

    // Ensure that the script has the correct file permission.
    const char* path = launchArgs.targetData;
    struct stat st = {};
    bool filePermissionsChanged = false;
    bool statOk = stat(path, &st) == 0;
    int mode = S_IRWXU | S_IRGRP | S_IXGRP;
    if (statOk)
    {
        int perms = st.st_mode & ~S_IFMT;
        if (perms != mode)
        {
            // Fix the permissions.
            if (0 != chmod(path, mode))
            {
                filePermissionsChanged = true;
                stat(path, &st);
                Log_Warn(
                    "Failed to set '%s' file permissions (expected:%d, actual: %d)", path, mode, st.st_mode & ~S_IFMT);
            }
        }
    }

    taskResult.SetExitStatus(ADUC_LaunchChildProcess(launchArgs.targetData, args, taskResult.Output()));

    if (filePermissionsChanged)
    {
        // Restore the permissions.
        if (0 != chmod(path, mode))
        {
            stat(path, &st);
            Log_Warn("Failed restore '%s' file permissions", path);
        }
    }

    return taskResult;
}

/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult DoScriptTask(const ADUShell_LaunchArguments& launchArgs)
{
    ADUShellTaskResult taskResult;
    ADUShellTaskFuncType taskProc = nullptr;

    try
    {
        const std::unordered_map<ADUShellAction, ADUShellTaskFuncType> actionMap = { { ADUShellAction::Execute,
                                                                                       Execute } };

        taskProc = actionMap.at(launchArgs.action);
    }
    catch (const std::exception& ex)
    {
        Log_Error("Unsupported action: '%s'", launchArgs.updateAction);
        taskResult.SetExitStatus(ADUSHELL_EXIT_UNSUPPORTED);
    }

    if (taskProc != nullptr)
    {
        try
        {
            taskResult = taskProc(launchArgs);
        }
        catch (const std::exception& ex)
        {
            Log_Error("Exception occurred while running task: '%s'", ex.what());
            taskResult.SetExitStatus(EXIT_FAILURE);
        }
        catch (...)
        {
            Log_Error("Exception occurred while running task.");
            taskResult.SetExitStatus(EXIT_FAILURE);
        }
    }

    return taskResult;
}

} // namespace Script
} // namespace Tasks
} // namespace Shell
} // namespace Adu
