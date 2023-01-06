/**
 * @file process_utils.hpp
 * @brief Contains utilities for managing child processes.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <errno.h>

#include <aducpal/grp.h> // getgrnam
#include <aducpal/pwd.h> // getpwnam

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <aduc/c_utils.h>
#include <aduc/config_utils.h>
#include <aduc/logging.h>
#include <aduc/string_utils.hpp>

#include <aducpal/stdio.h> // popen,pclose

#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>

#include <functional> // for std::function
#include <iostream>
#include <sstream>
#include <string>

#include <chrono>

#include <fcntl.h>
#include <sys/types.h>

/**
 * @brief Runs specified command in a new process and captures output, error messages, and exit code.
 *        The captured output and error messages will be written to ADUC_LOG_FILE.
 *
 * @param comman Name of a command to run. If command doesn't contain '/', this function will
 *               search for the specified command in PATH.
 * @param args List of arguments for the command.
 * @param output A standard output from the command.
 *
 * @return 0 on success.
 */
int ADUC_LaunchChildProcess(
    const std::string& command,
    std::vector<std::string> args,
    std::string& output) // NOLINT(google-runtime-references)
{
    int ret = 0;

    std::string redirected_command{ command };
    redirected_command += " ";

    for (const std::string& arg : args)
    {
        redirected_command += arg;
        redirected_command += " ";
    }

    // We want to capture stderr as well, so we need to append "2>&1"
    redirected_command += "2>&1";

    FILE* fp = ADUCPAL_popen(redirected_command.c_str(), "r");
    if (fp == NULL)
    {
        return errno;
    }

    char buffer[1024];

    while (fgets(buffer, sizeof(buffer), fp) != nullptr)
    {
        output += buffer;
    }
    // Returns 0 if no error occurred.
    ret = ferror(fp);
    if (ret != 0)
    {
        return ret;
    }

    return ADUCPAL_pclose(fp);
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
