/**
 * @file logging_manager.h
 * @brief The logging manager interface for accessors of global settings for logging.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef LOGGING_MANAGER_H
#define LOGGING_MANAGER_H

#include <aduc/c_utils.h>
#include <aduc/logging.h>

EXTERN_C_BEGIN

/**
 * @brief Sets the log level.
 *
 * @param log_level The log level.
 */
void LoggingManager_SetLogLevel(ADUC_LOG_SEVERITY log_level);

/**
 * @brief Gets the log level.
 *
 * @return ADUC_LOG_SEVERITY The log level.
 */
ADUC_LOG_SEVERITY LoggingManager_GetLogLevel();

EXTERN_C_END

#endif // LOGGING_MANAGER_H
