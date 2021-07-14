/**
 * @file pvcontrol_tasks.hpp
 * @brief Implements functions related to microsoft/pvcontrol update type.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 * @copyright Copyright (c) 2021, Pantacor Ltd.
 */
#ifndef ADU_SHELL_PVCONTROL_TASKS_HPP
#define ADU_SHELL_PVCONTROL_TASKS_HPP

#include <adushell.hpp>

namespace Adu
{
namespace Shell
{
namespace Tasks
{
namespace PVControl
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
ADUShellTaskResult GetStatus(const ADUShell_LaunchArguments& launchArgs);

/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 * 
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult DoPVControlTask(const ADUShell_LaunchArguments& launchArgs);

} // namespace PVControl
} // namespace Tasks
} // namespace Shell
} // namespace Adu

#endif // ADU_SHELL_PVCONTROL_TASKS_HPP
