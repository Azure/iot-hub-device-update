/**
 * @file zlog-config.c
 * @brief Config header for Zlog utility
 * Adapted from the public domain "zlog" by Zhiqiang Ma
 * https://github.com/zma/zlog/
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ZLOG_CONFIG_H
#define ZLOG_CONFIG_H

// Force buffer flush for every log
// enabling this will slow down the log
// #define ZLOG_FORCE_FLUSH_BUFFER

#define ZLOG_BUFFER_LINE_MAXCHARS 512
#define ZLOG_BUFFER_MAXLINES 1024

#define ZLOG_FLUSH_INTERVAL_SEC 30
#define ZLOG_SLEEP_TIME_SEC 2
// In practice: flush size < .8 * BUFFER_SIZE
#define ZLOG_BUFFER_FLUSH_MAXLINES (0.8 * ZLOG_BUFFER_MAXLINES)

// Maximum number of log files to keep
#define ZLOG_MAX_FILE_COUNT 3

// Maximum size in KB per logfile.
#define ZLOG_FILE_MAX_SIZE_KB 50

#endif // ZLOG_CONFIG_H
