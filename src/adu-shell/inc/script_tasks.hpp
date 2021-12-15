/**
 * @file script_tasks.hpp
 * @brief Implements functions related to microsoft/script update type.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADU_SHELL_SCRIPT_TASKS_HPP
#define ADU_SHELL_SCRIPT_TASKS_HPP

#include <adushell.hpp>

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
 * @return A result from child process.
 */
ADUShellTaskResult Execute(const ADUShell_LaunchArguments& launchArgs);

/**
 * @brief Runs appropriate command based on an action and other arguments in launchArgs.
 *
 * @param launchArgs An adu-shell launch arguments.
 * @return A result from child process.
 */
ADUShellTaskResult DoScriptTask(const ADUShell_LaunchArguments& launchArgs);

} // namespace Script
} // namespace Tasks
} // namespace Shell
} // namespace Adu

#endif // ADU_SHELL_SWUPDATE_TASKS_HPP
