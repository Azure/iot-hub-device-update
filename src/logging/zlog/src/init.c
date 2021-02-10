/**
 * @file init.c
 * @brief Implements the logging init and uninit functions.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#include "aduc/logging.h"
#include <stdio.h> // printf
#include <sys/stat.h> // mkdir

/**
 * @brief Convert ADUC_LOG_SEVERITY to ZLOG_SEVERITY
 * @param logLevel An ADUC log level
 * @return A zlogging log level
 */
static enum ZLOG_SEVERITY AducLogSeverityToZLogLevel(ADUC_LOG_SEVERITY logLevel)
{
    switch (logLevel)
    {
    case ADUC_LOG_DEBUG:
        return ZLOG_DEBUG;

    case ADUC_LOG_INFO:
        return ZLOG_INFO;

    case ADUC_LOG_WARN:
        return ZLOG_WARN;

    default:
        return ZLOG_ERROR;
    }
}

/**
 * @brief Initialize logging.
 * @param logLevel log level.
 */
void ADUC_Logging_Init(ADUC_LOG_SEVERITY logLevel)
{
    // zlog_init doesn't create the log path, so attempt to create it here.
    // If it can't be created, zlogging will send output to console.
    (void)mkdir(ADUC_LOG_FOLDER, S_IRWXU);

    if (zlog_init(
            ADUC_LOG_FOLDER,
            "aduc",
            ZLOG_ENABLED /* enable console logging*/,
            ZLOG_ENABLED /* enable file logging*/,
            AducLogSeverityToZLogLevel(logLevel) /* set console log level*/,
            AducLogSeverityToZLogLevel(logLevel) /* set file log level*/
            )
        != 0)
    {
        printf("WARNING: Unable to start file logger. (Log folder: %s)\n", ADUC_LOG_FOLDER);
    }
}

/**
 * @brief Disable logging.
 */
void ADUC_Logging_Uninit()
{
    zlog_finish();
}
