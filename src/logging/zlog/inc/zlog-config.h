/**
 * @file zlog-config.h
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

// Determined maxchars and maxlines for zlog buffer based on analysis of log files
// in the log directory after a 1 GB update payload was downloaded and installed.
// The average line length was around 200 chars with a standard deviation of 100,
// so a value of 328 (41 * 8) was chosen to ensure adequate buffer size and avoid
// unnecessary flushing and direct FD writes. The majority of logs had between
// a dozen and 64 lines, with du-agent.* logs having 100-300 lines. A value of
// 128 was chosen to balance a smaller working set (41 KB) with log responsiveness
// when tailing logs and allowing for diagnostics upload.
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
