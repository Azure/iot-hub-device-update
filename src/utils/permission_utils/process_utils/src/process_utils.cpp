/**
 * @file process_utils.hpp
 * @brief Contains utilities for managing child processes.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <errno.h>
#include <grp.h> // for getgrnam, struct group
#include <pwd.h> // for getpwnam
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <aduc/c_utils.h>
#include <aduc/config_utils.h>
#include <aduc/logging.h>
#include <aduc/string_utils.hpp>
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>

#include <functional> // for std::function
#include <iostream>
#include <sstream>
#include <string>

#include <chrono>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

/**
 * @brief Runs specified command in a new process and captures output, error messages, and exit code.
 *        The captured output and error messages will be written to ADUC_LOG_FILE.
 *
 * @param comman Name of a command to run. If command doesn't contain '/', this function will
 *               search for the specified command in PATH.
 * @param args List of arguments for the command.
 * @param output A standard output from the command.
 *
 * @return An exit code from the command.
 */
int ADUC_LaunchChildProcess(const std::string& command, std::vector<std::string> args, std::string& output) // NOLINT(google-runtime-references)
{
#define READ_END 0
#define WRITE_END 1

    int filedes[2];
    const int ret = pipe(filedes);
    if (ret != 0)
    {
        Log_Error("Cannot create output and error pipes. %s (errno %d).", strerror(errno), errno);
        return ret;
    }

    const int pid = fork();

    if (pid == 0)
    {
        // Running inside child process.

        // Redirect stdout and stderr to WRITE_END
        dup2(filedes[WRITE_END], STDOUT_FILENO);
        dup2(filedes[WRITE_END], STDERR_FILENO);

        close(filedes[READ_END]);
        close(filedes[WRITE_END]);

        std::vector<char*> argv;
        argv.reserve(args.size() + 2);
        argv.emplace_back(const_cast<char*>(command.c_str())); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        for (const std::string& arg : args)
        {
            argv.emplace_back(const_cast<char*>(arg.c_str())); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        }
        argv.emplace_back(nullptr);

        // The exec() functions only return if an error has occurred.
        // The return value is -1, and errno is set to indicate the error.
        int status = execvp(command.c_str(), &argv[0]);

        fprintf(stderr, "execvp failed, returned %d, error %d\n", status, errno);

        _exit(EXIT_FAILURE);
    }

    close(filedes[WRITE_END]);

    for (;;)
    {
        char buffer[1024];
        ssize_t count;
        count = read(filedes[READ_END], buffer, sizeof(buffer));

        if (count == -1)
        {
            Log_Error("Read failed, error %d", errno);
            break;
        }

        if (count <= 0)
        {
            break;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
        buffer[count] = 0;
        output += buffer;
    }

    int wstatus;
    int childExitStatus;

    waitpid(pid, &wstatus, 0);

    // Get the child process exit code.
    if (WIFEXITED(wstatus))
    {
        // Child process terminated normally.
        // e.g. by calling exit() or _exit(), or by returning from main().
        childExitStatus = WEXITSTATUS(wstatus);
    }
    else if (WIFSIGNALED(wstatus))
    {
        // Child process terminated by a signal.

        // Get the number of the signal that caused the child process to terminate.
        childExitStatus = WTERMSIG(wstatus);
        Log_Info("Child process terminated, signal %d", childExitStatus);
    }
    else if (WCOREDUMP(wstatus))
    {
        // Child process produced a core dump
        childExitStatus = WCOREDUMP(wstatus);
        Log_Error("Child process terminated, core dump %d", childExitStatus);
    }
    else
    {
        childExitStatus = EXIT_FAILURE;
        // Child process terminated abnormally.
        Log_Error("Child process terminated abnormally.", childExitStatus);
    }

    close(filedes[READ_END]);

    return childExitStatus;
}

/**
 * @brief Ensure that the effective group of the process is the given group (or is root).
 * @remark This function is not thread-safe if called with the defaults for the optional args.
 * @param groupName The group that process group must match.
 * @param getegidFunc Optional. The function for getting the effective group id. Default is getegid, which is not thread-safe.
 * @param getgrnamFunc Optional. The function for getting the group record. Default is getgrnam.
 * @return bool Return value. true for success; false otherwise.
 */
bool VerifyProcessEffectiveGroup(
    const char* groupName,
    const std::function<gid_t()>& getegidFunc /* = getegid */,
    const std::function<struct group*(const char*)>& getgrnamFunc /* = getgrnam */)
{
    const gid_t processEffectiveGroupId = getegidFunc();
    errno = 0;
    const struct group* aduGroupEntry = getgrnamFunc(groupName);
    if (aduGroupEntry == nullptr)
    {
        if (errno != 0)
        {
            Log_Error("lookup of group %s failed, errno: %d", groupName, errno);
            return false;
        }

        Log_Error("No group entry found for %s.", groupName);
        return false;
    }

    if (processEffectiveGroupId != 0 // root
        && processEffectiveGroupId != aduGroupEntry->gr_gid)
    {
        Log_Error(
            "effective group id [%d] did not match %s id of %d.",
            processEffectiveGroupId,
            groupName,
            aduGroupEntry->gr_gid);
        return false;
    }
    return true;
}

/**
 * @brief Ensure that the effective user of the process is one of the ADU Shell Trusted Users.
 * @remark This function is not thread-safe if called with the defaults for the optional args.
 * @param trustedUsersArray The list of adu shell trusted user account names (configured in the config file)
 * @param geteuidFunc Optional. The function for getting the effective group id. Default is geteuid, which is not thread-safe.
 * @param getpwnamFunc Optional. The function for getting the group record. Default is getpwnam, which is not thread-safe.
 * @return bool Return value. true for success; otherwise, returns false.
 */
bool VerifyProcessEffectiveUser(
    VECTOR_HANDLE trustedUsersArray,
    const std::function<uid_t()>& geteuidFunc /* = geteuid */,
    const std::function<struct passwd*(const char*)>& getpwnamFunc /* = getpwnam */)
{
    const uid_t processEffectiveUserId = geteuidFunc();
    // If user is root, it has the permission to run operations as an effective user.
    if (processEffectiveUserId == 0)
    {
        return true;
    }

    bool isTrusted = false;

    for (size_t i = 0; i < VECTOR_size(trustedUsersArray); i++)
    {
        auto user = static_cast<STRING_HANDLE>(VECTOR_element(trustedUsersArray, i));
        const struct passwd* trustedUserEntry = getpwnamFunc(STRING_c_str(user));
        if (trustedUserEntry != nullptr)
        {
            if (processEffectiveUserId == trustedUserEntry->pw_uid)
            {
                isTrusted = true;
                break;
            }
        }
    }

    if (!isTrusted)
    {
        Log_Error("effective user id [%d] is not one of the trusted users.", processEffectiveUserId);
    }
    return isTrusted;
}
