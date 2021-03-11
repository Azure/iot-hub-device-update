/**
 * @file aptget_tasks.hpp
 * @brief Implements functions related to microsoft/apt update type.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADU_SHELL_APTGET_TASKS_HPP
#define ADU_SHELL_APTGET_TASKS_HPP

#include "adushell.hpp"

namespace Adu
{
namespace Shell
{
namespace Tasks
{
namespace AptGet
{
/**
* @brief Run "apt-get update" command in a child process.
*
* @param launchArgs An adu-shell launch arguments.
* @return A result from child process.
*/
ADUShellTaskResult Update(const ADUShell_LaunchArguments& launchArgs);

/**
* @brief Run "apt-get -y install --auto-remove" command in a child process.
*
* --auto-remove option is used to remove packages that were automatically installed to
* satisfy dependencies for other packages and are now no longer needed.
*
* @param launchArgs An adu-shell launch arguments.
* @return A result from child process.
*/
ADUShellTaskResult RemoveUnusedDependencies(const ADUShell_LaunchArguments& launchArgs);

/**
* @brief Runs "apt-get install -y --allow-downgrades --download-only" command in  a child process.
*
* @param launchArgs An adu-shell launch arguments.
* @return A result from child process.
*/
ADUShellTaskResult Download(const ADUShell_LaunchArguments& launchArgs);

/**
* @brief Runs "apt-get -y --allow-downgrades install" command in  a child process.
*
* @param launchArgs An adu-shell launch arguments.
* @return A result from child process.
*/
ADUShellTaskResult Install(const ADUShell_LaunchArguments& launchArgs);

/**
* @brief Runs "apt-get -y remove" command in a child process.
*
* @param launchArgs An adu-shell launch arguments.
* @return A result from child process.
*/
ADUShellTaskResult Remove(const ADUShell_LaunchArguments& launchArgs);

/**
* @brief Runs "apt-get -y remove", and "apt-get -y install --auto-remove" commands in a child process.
*
* @param launchArgs An adu-shell launch arguments.
* @return A result from child process.
*/
ADUShellTaskResult Rollback(const ADUShell_LaunchArguments& launchArgs);

/**
* @brief Runs appropriate command based on an action and other arguments in launchArgs.
*
* This could resulted in one or more packaged installed or removed from the system.
*
* @param launchArgs The adu-shell launch command-line arguments that has been parsed.
* @return A result from child process.
*/
ADUShellTaskResult DoAptGetTask(const ADUShell_LaunchArguments& launchArgs);

} // namespace AptGet
} // namespace Tasks
} // namespace Shell
} // namespace Adu

#endif // ADU_SHELL_APTGET_TASKS_HPP
