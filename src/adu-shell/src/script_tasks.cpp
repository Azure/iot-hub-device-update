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

#include <cerrno>
#include <cstring> // strerror
#include <unordered_map>

#include <aducpal/sys_stat.h> // chmod

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
    bool fileOwnershipChanged = false;
    int mode = S_IRWXU | S_IRGRP | S_IXGRP;
    int originalMode = 0;
    uid_t originalUid = 0;
    gid_t originalGid = 0;

    // Do not attempt to execute a script that does not exist. Otherwise the
    // child process launch below would fail with a hard-to-diagnose error.
    if (stat(path, &st) != 0)
    {
        Log_Error("Cannot execute script '%s'. File not found (errno: %d, %s)", path, errno, strerror(errno));
        taskResult.SetExitStatus(ADUSHELL_EXIT_FILE_NOT_FOUND);
        return taskResult;
    }

    // Remember the original permissions and ownership so they can be restored after execution.
    originalMode = st.st_mode & ~S_IFMT;
    originalUid = st.st_uid;
    originalGid = st.st_gid;

    // Ensure that the script has the correct ownership.
    struct group* grp = ADUCPAL_getgrnam(ADUC_FILE_GROUP);
    struct passwd* p = ADUCPAL_getpwnam(ADUC_FILE_USER);

    if (p != NULL && grp != NULL)
    {
        if (originalUid != p->pw_uid || originalGid != grp->gr_gid)
        {
            // Fix the ownership.
            if (0 != ADUCPAL_chown(path, p->pw_uid, grp->gr_gid))
            {
                Log_Error("Failed to set '%s' file ownership to %d:%d", path, p->pw_uid, grp->gr_gid);
                taskResult.SetExitStatus(ADUSHELL_EXIT_BAD_FILE_OWNERSHIP);
                goto done;
            }

            // The ownership was successfully changed and must be restored after execution.
            fileOwnershipChanged = true;
        }
    }

    if (originalMode != mode)
    {
        // Fix the permissions.
        if (0 != ADUCPAL_chmod(path, mode))
        {
            stat(path, &st);
            Log_Error(
                "Failed to set '%s' file permissions (expected:%d, actual: %d)", path, mode, st.st_mode & ~S_IFMT);
            taskResult.SetExitStatus(ADUSHELL_EXIT_BAD_FILE_PERMS);
            goto done;
        }

        // The permissions were successfully changed and must be restored after execution.
        filePermissionsChanged = true;
    }

    taskResult.SetExitStatus(ADUC_LaunchChildProcess(launchArgs.targetData, args, taskResult.Output()));

done:
    // Restore the original ownership if it was changed. This is done before
    // restoring the permissions because chown() may clear the set-user-ID and
    // set-group-ID permission bits.
    if (fileOwnershipChanged)
    {
        if (0 != ADUCPAL_chown(path, originalUid, originalGid))
        {
            Log_Warn("Failed to restore '%s' file ownership", path);
        }
    }

    // Restore the original permissions if they were changed.
    if (filePermissionsChanged)
    {
        if (0 != ADUCPAL_chmod(path, originalMode))
        {
            Log_Warn("Failed to restore '%s' file permissions", path);
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
    catch (const std::exception& /* ex*/)
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
