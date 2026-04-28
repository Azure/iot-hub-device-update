/**
 * @file init.c
 * @brief Implements the logging init and uninit functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/logging.h"
#include "aduc/system_utils.h"
#ifdef _MSC_VER
#    include <windows.h>
// MSVC C99 does not support <stdatomic.h>; use Win32 Interlocked intrinsics instead.
typedef volatile LONG atomic_int_compat;
#    define ATOMIC_INIT_ZERO 0
#    define atomic_fetch_add_compat(ptr, val) InterlockedExchangeAdd((ptr), (val))
#    define atomic_fetch_sub_compat(ptr, val) InterlockedExchangeAdd((ptr), -(val))
#    define atomic_load_compat(ptr) InterlockedCompareExchange((ptr), 0, 0)
#else
#    include <stdatomic.h> // atomic_int, atomic_fetch_add, atomic_fetch_sub
typedef atomic_int atomic_int_compat;
#    define ATOMIC_INIT_ZERO 0
#    define atomic_fetch_add_compat(ptr, val) atomic_fetch_add((ptr), (val))
#    define atomic_fetch_sub_compat(ptr, val) atomic_fetch_sub((ptr), (val))
#    define atomic_load_compat(ptr) atomic_load((ptr))
#endif
#include <stdio.h> // printf
#include <sys/stat.h> // stat

#if ADUC_ENABLE_CONSOLE_LOG
#    define ZLOG_ENABLE_CONSOLE_LOG ZLOG_ENABLED
#else
#    define ZLOG_ENABLE_CONSOLE_LOG ZLOG_DISABLED
#endif

#if ADUC_ENABLE_FILE_LOG
#    define ZLOG_ENABLE_FILE_LOG ZLOG_ENABLED
#else
#    define ZLOG_ENABLE_FILE_LOG ZLOG_DISABLED
#endif

// Thread-safe reference count for logging init/uninit calls.
// Using atomic to prevent race conditions when multiple threads call init/uninit.
static atomic_int_compat ref_count = ATOMIC_INIT_ZERO;

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
 * @brief Convert ZLOG_SEVERITY to ADUC_LOG_SEVERITY
 * @param logLevel A zlogging log level
 * @return An ADUC log level
 */
ADUC_LOG_SEVERITY ZLogLevelToAducLogSeverity(enum ZLOG_SEVERITY logLevel)
{
    switch (logLevel)
    {
    case ZLOG_DEBUG:
        return ADUC_LOG_DEBUG;

    case ZLOG_INFO:
        return ADUC_LOG_INFO;

    case ZLOG_WARN:
        return ADUC_LOG_WARN;

    default:
        return ADUC_LOG_ERROR;
    }
}

ADUC_LOG_SEVERITY g_logLevel = ADUC_LOG_INFO;

/**
 * @brief Initialize logging.
 * @param logLevel log level.
 */
void ADUC_Logging_Init(ADUC_LOG_SEVERITY logLevel, const char* filePrefix)
{
    // atomic_fetch_add returns the previous value, so if it was > 0, logging is already initialized
    if (atomic_fetch_add_compat(&ref_count, 1) > 0)
    {
        return;
    }

    g_logLevel = logLevel;

    // zlog_init doesn't create the log path, so attempt to create it here if it does not exist.
    // If it can't be created, zlogging will send output to console.
    struct stat st;
    if (stat(ADUC_LOG_FOLDER, &st) != 0)
    {
        // NOTE: permissions must match those in CheckLogDir in health_management.c
        if (ADUC_SystemUtils_MkDirRecursive(
                ADUC_LOG_FOLDER, (uid_t)-1 /*userId*/, (gid_t)-1 /*groupId*/, (mode_t)S_IRWXU | S_IRGRP | S_IXGRP)
            != 0)
        {
            printf("WARNING: Cannot create a folder for logging file. ('%s')", ADUC_LOG_FOLDER);
        }
    }

    if (zlog_init(
            ADUC_LOG_FOLDER,
            filePrefix == NULL ? "aduc" : filePrefix,
            ZLOG_ENABLE_CONSOLE_LOG /* enable console logging*/,
            ZLOG_ENABLE_FILE_LOG /* enable file logging*/,
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
    // Guard against double-uninit: if already at 0 or below, nothing to do.
    if (atomic_load_compat(&ref_count) <= 0)
    {
        return;
    }

    // atomic_fetch_sub returns the previous value, so if it was > 1, there are still other users
    if (atomic_fetch_sub_compat(&ref_count, 1) > 1)
    {
        return;
    }

    zlog_finish();
}

ADUC_LOG_SEVERITY ADUC_Logging_GetLevel()
{
    return g_logLevel;
}
