/**
 * @file adushell_action.hpp
 * @brief Private header for ADU Shell Actions enum and helper functions.
 * @copyright Copyright (c) 2020, Microsoft Corp.
 */
#ifndef ADU_SHELL_ACTION_HPP
#define ADU_SHELL_ACTION_HPP

enum class ADUShellAction
{
    Unknown,
    Initialize,
    Download,
    Install,
    Remove,
    Apply,
    Cancel,
    Rollback,
    Reboot,
    GetStatus
};

/**
 * @brief Convert string to ADUShellAction value.
 * @param actionString A string to convert.
 *
 * @return ADUShellAction enum value, or ADUShellAction::Unknown for unknown action string.
 */
ADUShellAction ADUShellActionFromString(const char* actionString);

#endif // ADU_SHELL_ACTION_HPP
