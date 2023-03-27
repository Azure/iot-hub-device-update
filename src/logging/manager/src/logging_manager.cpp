/**
 * @file logging_manager.cpp
 * @brief The logging manager implementation for accessors of global settings for logging.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "logging_manager.h"

class LoggingManager
{
public:
    static LoggingManager& GetInstance()
    {
        static LoggingManager instance{};
        return instance;
    }

    ADUC_LOG_SEVERITY get_log_level() const
    {
        return log_level;
    }

    void set_log_level(ADUC_LOG_SEVERITY sev)
    {
        log_level = sev;
    }

private:
    LoggingManager()
    {
    }

    ADUC_LOG_SEVERITY log_level = ADUC_LOG_INFO;
};

void LoggingManager_SetLogLevel(ADUC_LOG_SEVERITY log_level)
{
    LoggingManager::GetInstance().set_log_level(log_level);
}

ADUC_LOG_SEVERITY LoggingManager_GetLogLevel()
{
    return LoggingManager::GetInstance().get_log_level();
}
