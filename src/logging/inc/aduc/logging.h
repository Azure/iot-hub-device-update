/**
 * @file logging.h
 * @brief Maps logging macros to the appropriate logging library functions.
 *
 * Uses ADUC_LOGGING_LIBRARY to determine which logging library to use.
 * Currently supported libraries are zlog and xlogging.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_LOGGING_H
#define ADUC_LOGGING_H

#ifdef __cplusplus
#    define EXTERN_C_BEGIN \
        extern "C"         \
        {
#    define EXTERN_C_END }
#else
#    define EXTERN_C_BEGIN
#    define EXTERN_C_END
#endif

EXTERN_C_BEGIN

// Note that the order of severity levels is critical
typedef enum tagADUC_LOG_SEVERITY
{
    ADUC_LOG_DEBUG,
    ADUC_LOG_INFO,
    ADUC_LOG_WARN,
    ADUC_LOG_ERROR
} ADUC_LOG_SEVERITY;

#if ADUC_USE_ZLOGGING

#    include "zlog.h"

// Logging Init and Uninit helper function forward declarations.
// These are implemented for each logging library.
void ADUC_Logging_Init(ADUC_LOG_SEVERITY logLevel, const char* filePrefix);
void ADUC_Logging_Uninit();
ADUC_LOG_SEVERITY ADUC_Logging_GetLevel();

/**
 * @brief Detailed informational events that are useful to debug an application.
 */
#    define Log_Debug log_debug

/**
 * @brief Informational events that report the general progress of the application.
 */
#    define Log_Info log_info

/**
 * @brief Informational events about potentially harmful situations.
 */
#    define Log_Warn log_warn

/**
 * @brief Error events.
 */
#    define Log_Error log_error

/*
 * @brief Request a buffer flush.
 */
#    define Log_RequestFlush zlog_request_flush_buffer

#elif ADUC_USE_XLOGGING

#    include <azure_c_shared_utility/xlogging.h>

// Logging Init and Uninit helper function forward declarations.
// These are implemented for each logging library.
#    define ADUC_Logging_Init(...)
#    define ADUC_Logging_Uninit(...)
#    define ADUC_Logging_GetLevel(...) (0)

/**
 * @brief Detailed informational events that are useful to debug an application.
 * @remark Since XLogging doesn't implement debug category for logging, this will
 *         be mapped to LogInfo.
 */
#    define Log_Debug LogInfo

/**
 * @brief Informational events that report the general progress of the application.
 */
#    define Log_Info LogInfo

/**
 * @brief Informational events about potentially harmful situations.
 * XLogging doesn't have a warn level, so use info instead.
 */
#    define Log_Warn LogInfo

/**
 * @brief Error events.
 */
#    define Log_Error LogError

/*
 * @brief Request a buffer flush.
 */
#    define Log_RequestFlush(...)

#else

#    error "Unknown logger or logging type specified."

#endif

EXTERN_C_END

#endif // ADUC_LOGGING_H
