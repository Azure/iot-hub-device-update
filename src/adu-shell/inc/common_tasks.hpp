/**
 * @file common_tasks.hpp
 *
 * @brief Implements a set of common tasks for most update type.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADU_SHELL_COMMON_TASKS_HPP
#define ADU_SHELL_COMMON_TASKS_HPP

#include <adushell.hpp>

namespace Adu
{
namespace Shell
{
namespace Tasks
{
namespace Common
{
/**
 * @brief Reboot the system.
 *
 * @param launchArgs The adu-shell launch command-line arguments that has been parsed.
 * @return A result from child process.
 */
ADUShellTaskResult Reboot(const ADUShell_LaunchArguments& launchArgs);

/**
* @brief Runs appropriate command based on an action and other arguments in launchArgs.
*
* This could resulted in one or more packaged installed or removed from the system.
*
* @param launchArgs The adu-shell launch command-line arguments that has been parsed.
* @return A result from child process.
*/
ADUShellTaskResult DoCommonTask(const ADUShell_LaunchArguments& launchArgs);

} // namespace Common
} // namespace Tasks
} // namespace Shell
} // namespace Adu

#endif // ADU_SHELL_COMMON_TASKS_HPP
