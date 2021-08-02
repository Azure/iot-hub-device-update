/**
 * @file adushell_action.cpp
 * @brief Implements the ADU Shell Actions helper functions.
 *
 * @copyright Copyright (c) 2020, Microsoft Corporation.
 */
#include "aduc/logging.h"
#include "adushell_action.hpp"

#include <string>
#include <unordered_map>

/**
 * @brief Convert string to  value.
 * @param actionString A string to convert.
 *
 * @return ADUShellAction enum value, or ADUShellAction::Unknown for unknown action string.
 */
ADUShellAction ADUShellActionFromString(const char* actionString)
{
    ADUShellAction value;

    try
    {
        // clang-format off

        const std::unordered_map<std::string, ADUShellAction> actionMap = {
            { "initialize", ADUShellAction::Initialize }, 
            { "download", ADUShellAction::Download },
            { "install", ADUShellAction::Install },
            { "remove", ADUShellAction::Remove },
            { "apply", ADUShellAction::Apply },
            { "cancel", ADUShellAction::Cancel },
            { "rollback", ADUShellAction::Rollback },
            { "reboot", ADUShellAction::Reboot },
            { "getstatus", ADUShellAction::GetStatus }
        };

        // clang-format on

        value = actionMap.at(std::string(actionString));
    }
    catch (...)
    {
        Log_Error("Unknown action '%s'", actionString);
        value = ADUShellAction::Unknown;
    }

    return value;
}
