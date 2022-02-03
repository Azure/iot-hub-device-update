/**
 * @file adushell_action.cpp
 * @brief Implements the ADU Shell Actions helper functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "adushell_action.hpp"
#include "aduc/logging.h"

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
            { "apply", ADUShellAction::Apply },
            { "cancel", ADUShellAction::Cancel },
            { "download", ADUShellAction::Download },
            { "execute", ADUShellAction::Execute },
            { "initialize", ADUShellAction::Initialize },
            { "install", ADUShellAction::Install },
            { "reboot", ADUShellAction::Reboot },
            { "remove", ADUShellAction::Remove },
            { "rollback", ADUShellAction::Rollback }
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
