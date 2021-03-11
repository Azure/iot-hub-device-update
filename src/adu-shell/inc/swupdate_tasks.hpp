/**
 * @file swupdate_tasks.hpp
 * @brief Implements functions related to microsoft/swupdate update type.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADU_SHELL_SWUPDATE_TASKS_HPP
#define ADU_SHELL_SWUPDATE_TASKS_HPP

#include <adushell.hpp>

namespace Adu
{
namespace Shell
{
namespace Tasks
{
namespace SWUpdate
{
/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult Install(const ADUShell_LaunchArguments& launchArgs);

/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult Apply(const ADUShell_LaunchArguments& launchArgs);

/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult Rollback(const ADUShell_LaunchArguments& launchArgs);

/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult Cancel(const ADUShell_LaunchArguments& launchArgs);

/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult DoSWUpdateTask(const ADUShell_LaunchArguments& launchArgs);

} // namespace SWUpdate
} // namespace Tasks
} // namespace Shell
} // namespace Adu

#endif // ADU_SHELL_SWUPDATE_TASKS_HPP
