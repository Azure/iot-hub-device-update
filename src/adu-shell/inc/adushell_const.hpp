/**
 * @file adushell_const.hpp
 * @brief Private header contains all constants used by adu-shell and its consumers.
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADU_SHELL_CONST_HPP
#define ADU_SHELL_CONST_HPP

namespace Adu
{
namespace Shell
{
namespace Const
{
const char* adu_shell = "/usr/lib/adu/adu-shell";

const char* update_type_opt = "--update-type";
const char* update_type_microsoft_apt = "microsoft/apt";
const char* update_type_microsoft_swupdate = "microsoft/swupdate";
const char* update_type_microsoft_script = "microsoft/script";
const char* update_type_common = "common";
const char* update_action_opt = "--update-action";
const char* update_action_initialize = "initialize";
const char* update_action_download = "download";
const char* update_action_install = "install";
const char* update_action_apply = "apply";
const char* update_action_cancel = "cancel";
const char* update_action_rollback = "rollback";
const char* update_action_reboot = "reboot";
const char* update_action_execute = "execute";

const char* target_data_opt = "--target-data";
const char* target_options_opt = "--target-options";
const char* target_log_folder_opt = "--target-log-folder";
} // namespace Const
} // namespace Shell
} // namespace Adu

#endif // ADU_SHELL_CONST_HPP
