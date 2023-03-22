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

// The following values for line maxchars and maxlines for zlog buffer were
// derived by looking at arithmetic mean and standard deviation of chars/lines
// across all log files in the log dir after a script-based download and
// install of a 1 GB update payload.
// Average line length usually did not exceed 200 chars and adding in std
// deviation, it was roughly around 300 chars, so 328 (41 * 8) adds some safety
// margin to avoid unnecessary flushing + direct FD write in most cases.
// Most logs had lines between a dozen to 64 lines, but du-agent.* logs were
// in the range of 100-300, so 128 was a nice balance between keeping working
// set smaller (41 KB) and not flushing too often but often enough that the log
// traces on disk felt responsive when tailing the logs and being available for
// diagnostics to upload.
#define ZLOG_BUFFER_LINE_MAXCHARS 328
#define ZLOG_BUFFER_MAXLINES 128

#define ZLOG_FLUSH_INTERVAL_SEC 30
#define ZLOG_SLEEP_TIME_SEC 2
// In practice: flush size < .8 * BUFFER_SIZE
#define ZLOG_BUFFER_FLUSH_MAXLINES (0.8 * ZLOG_BUFFER_MAXLINES)

// Maximum number of log files to keep
#define ZLOG_MAX_FILE_COUNT 3

// Maximum size in KB per logfile.
#define ZLOG_FILE_MAX_SIZE_KB 50

#endif // ZLOG_CONFIG_H
