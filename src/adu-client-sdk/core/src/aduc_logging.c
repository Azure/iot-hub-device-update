/**
 * @file aduc_logging.c
 * @brief Logging utilities (stub implementation)
 */

#include "aduc/logging.h"
#include <stdio.h>

void ADUC_Log(ADUC_LogLevel level, const char* message)
{
    const char* levelStr = "INFO";
    switch (level)
    {
        case ADUC_LOG_ERROR: levelStr = "ERROR"; break;
        case ADUC_LOG_WARN: levelStr = "WARN"; break;
        case ADUC_LOG_INFO: levelStr = "INFO"; break;
        case ADUC_LOG_DEBUG: levelStr = "DEBUG"; break;
    }
    
    printf("[%s] %s\n", levelStr, message ? message : "");
}