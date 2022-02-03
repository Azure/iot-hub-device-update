/**
 * @file zlog.h
 * @brief Header for zlog logging utility
 * Adapted from the public domain "zlog" by Zhiqiang Ma
 * https://github.com/zma/zlog/
 *
 * @copyright Copyright (c) Microsoft Corp.
 * Licensed under the MIT License.
 */

#ifndef ZLOG_H
#define ZLOG_H

#define ZLOG_ENABLED 0
#define ZLOG_DISABLED 1

// Note that the order of severity levels is critical
enum ZLOG_SEVERITY
{
    ZLOG_DEBUG,
    ZLOG_INFO,
    ZLOG_WARN,
    ZLOG_ERROR
};

// Start API
// clang-format off
#define log_debug(...) zlog_log(ZLOG_DEBUG, __FUNCTION__, __VA_ARGS__) // NOLINT(misc-lambda-function-name)
#define log_info(...)  zlog_log(ZLOG_INFO, __FUNCTION__, __VA_ARGS__) // NOLINT(misc-lambda-function-name)
#define log_warn(...)  zlog_log(ZLOG_WARN, __FUNCTION__, __VA_ARGS__) // NOLINT(misc-lambda-function-name)
#define log_error(...) zlog_log(ZLOG_ERROR, __FUNCTION__, __VA_ARGS__) // NOLINT(misc-lambda-function-name)
// clang-format on

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

// initialize zlog log settings
int zlog_init(
    const char* log_dir,
    const char* log_file,
    int console_enable,
    int file_enable,
    enum ZLOG_SEVERITY console_level,
    enum ZLOG_SEVERITY file_level);
// finish using the zlog; clean up
void zlog_finish(void);
// explicitly flush the buffer in memory
void zlog_flush_buffer(void);
// request to flush the buffer.
void zlog_request_flush_buffer(void);
// log an entry with the function scope and timestamp
void zlog_log(enum ZLOG_SEVERITY msg_level, const char* func, const char* fmt, ...);

// End API

EXTERN_C_END

#endif // ZLOG_H
