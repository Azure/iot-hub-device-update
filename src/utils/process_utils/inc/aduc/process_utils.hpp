/**
 * @file process_utils.hpp
 * @brief Contains utilities for managing child processes.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_PROCESS_UTILS_HPP
#define ADUC_PROCESS_UTILS_HPP

#include <azure_c_shared_utility/vector.h>
#include <functional>
#include <grp.h>
#include <pwd.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

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
int ADUC_LaunchChildProcess(const std::string& command, std::vector<std::string> args, std::string& output);

/**
 * @brief Ensure that the effective group of the process is the given group (or is root).
 * @remark This function is not thread-safe if called with the defaults for the optional args.
 * @param groupName The group that process group must match.
 * @param getegidFunc Optional. The function for getting the effective group id. Default is getegid, which is not thread-safe.
 * @param getgrnamFunc Optional. The function for getting the group record. Default is getgrnam, which is not thread-safe.
 * @return bool Return value. true for success; false otherwise.
 */
bool VerifyProcessEffectiveGroup(
    const char* groupName,
    const std::function<gid_t()>& getegidFunc = getegid,
    const std::function<struct group*(const char*)>& getgrnamFunc = getgrnam);

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
    const std::function<uid_t()>& geteuidFunc = geteuid,
    const std::function<struct passwd*(const char*)>& getpwnamFunc = getpwnam);

#endif // ADUC_PROCESS_UTILS_HPP
