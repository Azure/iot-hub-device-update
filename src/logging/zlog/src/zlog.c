/**
 * @file zlog.c
 * @brief Zlog utility
 * Adapted from the public domain "zlog" by Zhiqiang Ma
 * https://github.com/zma/zlog/
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for strcmp, memset, strlen, etc.
#include <sys/stat.h>
#include <sys/time.h> // for gettimeofday
#include <sys/types.h>
#include <time.h>
#include <unistd.h> // isatty

#include "zlog-config.h"
#include "zlog.h"

typedef enum tagCONSOLE_LOGGING_MODE
{
    ZLOG_CLM_DISABLED, // No console logging
    ZLOG_CLM_ENABLED, // Console logging (might be redirected)
    ZLOG_CLM_ENABLED_TTY, // Console logging to TTY
    ZLOG_CLM_ENABLED_TTYCOLOR, // Console logging to color-enabled TTY
} CONSOLE_LOGGING_MODE;

static struct
{
    enum ZLOG_SEVERITY console_level;
    CONSOLE_LOGGING_MODE console_logging_mode;

    enum ZLOG_SEVERITY file_level;

} log_setting;

static const char level_names[] = { 'D', 'I', 'W', 'E' }; // Must align with ZLOG_SEVERITY enum in zlog.h

static FILE* zlog_fout = NULL;
static char* zlog_file_log_dir = NULL;
static char* zlog_file_log_prefix = NULL;

char _zlog_buffer[ZLOG_BUFFER_MAXLINES][ZLOG_BUFFER_LINE_MAXCHARS];
static int _zlog_buffer_count = 0;
static pthread_mutex_t _zlog_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t _zlog_flush_thread;
static _Bool _is_flush_thread_initialized = false;

void zlog_init_flush_thread(void);
void zlog_stop_flush_thread(void);
struct tm* get_current_utctime();
_Bool get_current_utctime_filename(char* fullpath, size_t fullpath_len);
static inline void _zlog_buffer_lock(void);
static inline void _zlog_buffer_unlock(void);
static void _zlog_flush_buffer(void);
static inline char* zlog_lock_and_get_buffer(void);
static inline void zlog_finish_buffer_and_unlock(void);
void zlog_ensure_at_most_n_logfiles(int max_num);

static _Bool zlog_is_file_log_open()
{
    return zlog_fout != NULL;
}

static void zlog_close_file_log()
{
    if (zlog_is_file_log_open())
    {
        fclose(zlog_fout);
        zlog_fout = NULL;
    }
}

static _Bool zlog_is_stdout_a_tty()
{
    return (isatty(fileno(stdout)) != 0);
}

static _Bool zlog_term_supports_color()
{
    const char* term = getenv("TERM");
    if (term != NULL)
    {
        // clang-format off
        const char* color_terms[] =
        {
            "xterm",         "xterm-color",     "xterm-256color",
            "screen",        "screen-256color", "tmux",
            "tmux-256color", "rxvt-unicode",    "rxvt-unicode-256color",
            "linux",         "cygwin"
        };
        // clang-format on

        for (unsigned i = 0; i < sizeof(color_terms) / sizeof(color_terms[0]); ++i)
        {
            if (strcmp(term, color_terms[i]) == 0)
            {
                return true;
            }
        }
    }

    return false;
}

// ------------------------- Logging Utilities -------------------------

// Initialize zlog logging settings:
// Return true when the settings are initialized exactly as specified
// Otherwise leave zlog_fout = NULL and return false
int zlog_init(
    char const* log_dir,
    char const* log_file,
    int console_enable,
    int file_enable,
    enum ZLOG_SEVERITY console_level,
    enum ZLOG_SEVERITY file_level)
{
    memset(&log_setting, 0, sizeof(log_setting));
    log_setting.console_level = console_level;
    log_setting.file_level = file_level;

    CONSOLE_LOGGING_MODE console_logging_mode = ZLOG_CLM_DISABLED;

    if (console_enable != ZLOG_DISABLED)
    {
        // Console logging enabled - determine level.
        console_logging_mode = ZLOG_CLM_ENABLED;

        if (zlog_is_stdout_a_tty())
        {
            console_logging_mode = ZLOG_CLM_ENABLED_TTY;

            if (zlog_term_supports_color())
            {
                console_logging_mode = ZLOG_CLM_ENABLED_TTYCOLOR;
            }
        }
    }

    log_setting.console_logging_mode = console_logging_mode;

    if (file_enable == ZLOG_ENABLED)
    {
        zlog_file_log_dir = (char*)malloc(strlen(log_dir) + 1);
        if (zlog_file_log_dir == NULL)
        {
            return -1;
        }
        strcpy(zlog_file_log_dir, log_dir); // NOLINT(clang-analyzer-security.insecureAPI.strcpy)

        zlog_file_log_prefix = (char*)malloc(2 + strlen(log_file));
        if (zlog_file_log_prefix == NULL)
        {
            return -1;
        }
        strcpy(zlog_file_log_prefix, log_file); // NOLINT(clang-analyzer-security.insecureAPI.strcpy)
        strcat(zlog_file_log_prefix, "."); // NOLINT(clang-analyzer-security.insecureAPI.strcpy)

        // Timestamp the log file
        char zlog_file_log_fullpath[512];
        if (!get_current_utctime_filename(zlog_file_log_fullpath, sizeof(zlog_file_log_fullpath)))
        {
            // When error occurs to snprintf filepath, return false
            return -1;
        }

        zlog_fout = fopen(zlog_file_log_fullpath, "a+");
        if (zlog_fout == NULL)
        {
            return -1;
        }
        log_debug("Log file created: %s", zlog_file_log_fullpath);

        zlog_ensure_at_most_n_logfiles(ZLOG_MAX_FILE_COUNT);

#ifndef ZLOG_FORCE_FLUSH_BUFFER
        zlog_init_flush_thread();
#endif
    }
    return 0;
}

// Caller should NOT hold the lock
void zlog_flush_buffer(void)
{
    _zlog_buffer_lock();
    _zlog_flush_buffer();
    _zlog_buffer_unlock();
}

// Caller should NOT hold the lock
void zlog_finish(void)
{
#ifndef ZLOG_FORCE_FLUSH_BUFFER
    zlog_stop_flush_thread();
#endif
    zlog_flush_buffer();

    zlog_close_file_log();

    free(zlog_file_log_dir);
    free(zlog_file_log_prefix);
}

void zlog_log(enum ZLOG_SEVERITY msg_level, const char* func, const char* fmt, ...)
{
    const _Bool console_log_needed =
        (log_setting.console_logging_mode != ZLOG_CLM_DISABLED) && (msg_level >= log_setting.console_level);
    const _Bool file_log_needed = zlog_is_file_log_open() && (msg_level >= log_setting.file_level);

    if (!console_log_needed && !file_log_needed)
    {
        // If we're not logging to console or file, there's nothing to do.
        return;
    }

    char time_buffer[sizeof("2020-07-01T18:21:26.1234Z")];
    time_buffer[0] = '\0';

    struct timespec curtime;
    clock_gettime(CLOCK_REALTIME, &curtime);

    const time_t seconds = curtime.tv_sec;

    struct tm gmtval;
    struct tm* tmval = gmtime_r(&seconds, &gmtval);

    if (tmval != NULL)
    {
        // % 100 below to ensure the values fit in 2-digits template.
        int ret = snprintf(
            time_buffer,
            sizeof(time_buffer),
            "%04d-%02d-%02dT%02d:%02d:%02d.%04dZ",
            tmval->tm_year + 1900,
            tmval->tm_mon + 1,
            tmval->tm_mday % 100,
            tmval->tm_hour % 100,
            tmval->tm_min % 100,
            tmval->tm_sec % 100,
            (int)(curtime.tv_nsec / 100000));

        if (ret < 0)
        {
            return;
        }
    }

    char va_buffer[ZLOG_BUFFER_LINE_MAXCHARS];
    va_list va;
    va_start(va, fmt);
    vsnprintf(va_buffer, sizeof(va_buffer) / sizeof(va_buffer[0]), fmt, va);
    va_end(va);

    if (console_log_needed)
    {
        // Output to console

        const char* color_prefix;
        const char* color_suffix;

        if (log_setting.console_logging_mode != ZLOG_CLM_ENABLED_TTYCOLOR)
        {
            color_prefix = "";
            color_suffix = "";
        }
        else
        {
            // Use Bold Red for error, Bold Yellow for warn.
            color_prefix = (msg_level == ZLOG_ERROR) ? "\033[1;31m" : (msg_level == ZLOG_WARN) ? "\033[1;33m" : "";
            color_suffix = "\033[m";
        }

        fprintf(
            msg_level == ZLOG_ERROR ? stderr : stdout,
            "%s %s[%c]%s %s [%s]\n",
            time_buffer,
            color_prefix,
            level_names[msg_level],
            color_suffix,
            va_buffer,
            func);
    }

    if (file_log_needed)
    {
        // Add to zlog buffer.
        char* buffer = zlog_lock_and_get_buffer();

        // "%.400s" avoids error: '%s' directive output may be truncated writing up to 511 bytes into
        // a region of size between 444 and 507 [-Werror=format-truncation=]
        (void)snprintf(
            buffer,
            ZLOG_BUFFER_LINE_MAXCHARS,
            "%s [%c] %.400s [%s]\n",
            time_buffer,
            level_names[msg_level],
            va_buffer,
            func);

        zlog_finish_buffer_and_unlock();
    }

    if (msg_level == ZLOG_ERROR)
    {
        zlog_request_flush_buffer();
    }

}

bool g_flushRequested = false;

void zlog_request_flush_buffer(void)
{
    g_flushRequested = true;
}

// Buffer flushing thread
// Flush the thread every ZLOG_FLUSH_INTERVAL_SEC seconds
// or when buffer is 80% full
// or when g_flushRequested is true
//
// Caller should NOT hold the lock
static void* zlog_buffer_flush_thread()
{
    struct timeval tv;
    time_t lasttime;

    gettimeofday(&tv, NULL);
    lasttime = tv.tv_sec;

    do
    {
        time_t curtime;

        sleep(ZLOG_SLEEP_TIME_SEC);
        gettimeofday(&tv, NULL);
        curtime = tv.tv_sec;
        if (g_flushRequested || ((curtime - lasttime) >= ZLOG_FLUSH_INTERVAL_SEC))
        {
            g_flushRequested = false;
            zlog_flush_buffer();
            lasttime = curtime;
        }
        else
        {
            _zlog_buffer_lock();
            if (_zlog_buffer_count >= ZLOG_BUFFER_FLUSH_MAXLINES)
            {
                _zlog_flush_buffer();
            }
            _zlog_buffer_unlock();
        }
    } while (1);
    return NULL;
}

void zlog_init_flush_thread(void)
{
    if (pthread_create(&_zlog_flush_thread, NULL, zlog_buffer_flush_thread, NULL) == 0)
    {
        _is_flush_thread_initialized = true;
    }
}

// Caller should NOT hold the lock
void zlog_stop_flush_thread(void)
{
    // To prevent deadlock if flush thread calls zlog_flush_buffer but
    // gets terminated before calling unlock
    _zlog_buffer_lock();
    if (_is_flush_thread_initialized)
    {
        pthread_cancel(_zlog_flush_thread);
        pthread_join(_zlog_flush_thread, NULL);
        _is_flush_thread_initialized = false;
    }
    _zlog_buffer_unlock();
}

// ------------------------- Helper Functions ---------------------------
static int file_select(const struct dirent* logfile)
{
    // Filter: 1. File and 2. filename contains the log file pattern
    return (logfile->d_type == DT_REG && strstr(logfile->d_name, zlog_file_log_prefix) != NULL);
}

static inline void _zlog_buffer_lock(void)
{
    pthread_mutex_lock(&_zlog_buffer_mutex);
}

static inline void _zlog_buffer_unlock(void)
{
    pthread_mutex_unlock(&_zlog_buffer_mutex);
}

_Bool get_current_utctime_filename(char* fullpath, size_t fullpath_len)
{
    // Timestamp the log file
    char timebuf[sizeof("20200819-19181597864683")];
    const time_t current_time = time(NULL);
    const struct tm* tm = gmtime(&current_time);

    strftime(timebuf, sizeof(timebuf), "%Y%m%d-%H%M%S", tm);
    int res = snprintf(fullpath, fullpath_len, "%s/%s%s.log", zlog_file_log_dir, zlog_file_log_prefix, timebuf);
    if (res < 0 || res >= fullpath_len)
    {
        // When error occurs to snprintf filepath, return false
        return false;
    }
    return true;
}

// Caller should hold the lock
static void _zlog_flush_buffer()
{
    if (!zlog_is_file_log_open())
    {
        return;
    }

    // Flush buffer to file
    int i = 0;
    for (i = 0; i < _zlog_buffer_count; i++)
    {
        fputs(_zlog_buffer[i], zlog_fout);
    }
    fflush(zlog_fout);

    _zlog_buffer_count = 0;

    // Roll over to new log file once the current file size exceeds the limit
    if (ftell(zlog_fout) > (ZLOG_FILE_MAX_SIZE_KB * 1024))
    {
        zlog_close_file_log();

        // Clean up the log folder
        zlog_ensure_at_most_n_logfiles(ZLOG_MAX_FILE_COUNT);

        // Timestamp the new log file
        char zlog_file_log_fullpath[512];
        if (!get_current_utctime_filename(zlog_file_log_fullpath, sizeof(zlog_file_log_fullpath)))
        {
            return;
        }

        // INVARIANT: zlog_fout == NULL due to zlog_close_file_log() call above.
        zlog_fout = fopen(zlog_file_log_fullpath, "a");
    }
}

// Caller should NOT hold the lock
static inline char* zlog_lock_and_get_buffer(void)
{
    _zlog_buffer_lock();
    // Flush the buffer if it is full
    if (_zlog_buffer_count == ZLOG_BUFFER_MAXLINES)
    {
        _zlog_flush_buffer();
    }

    // allocate buffer
    _zlog_buffer_count++;
    return _zlog_buffer[_zlog_buffer_count - 1];
}

// Caller should hold the lock
static inline void zlog_finish_buffer_and_unlock()
{
#ifdef ZLOG_FORCE_FLUSH_BUFFER
    _zlog_flush_buffer();
#endif
    _zlog_buffer_unlock();
}

// Clean up until max of num old log files left
// Caller should hold the lock
void zlog_ensure_at_most_n_logfiles(int max_num)
{
    struct dirent** logfiles;

    // List the files specified by file_select in alphabetical order
    const int total = scandir(zlog_file_log_dir, &logfiles, file_select, alphasort);
    if (total == -1)
    {
        return;
    }

    if (total > max_num)
    {
        // Delete until only max_num of files left
        for (int i = 0; i < total - max_num; ++i)
        {
            char filepath[512];
            const unsigned int max_filepath = sizeof(filepath) / sizeof(filepath[0]);
            int res = snprintf(filepath, max_filepath, "%s/%s", zlog_file_log_dir, logfiles[i]->d_name);
            if (res > 0 && res < max_filepath)
            {
                remove(filepath);
            }
        }
    }

    // Free memory allocated by scandir.
    for (int i = 0; i < total; ++i)
    {
        free(logfiles[i]);
    }
    free(logfiles);
}
