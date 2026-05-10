/**
 * @file log_writer.c
 * @brief Binary log writer implementation.
 *
 * Implements an ETW-inspired compact binary logging format with buffered I/O
 * and mutex-protected writes for thread safety.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/log_writer.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
/* Windows: use CRITICAL_SECTION instead of pthread_mutex */
typedef CRITICAL_SECTION adu_mutex_t;
static inline void adu_mutex_init(adu_mutex_t* m) { InitializeCriticalSection(m); }
#define adu_mutex_lock(m) EnterCriticalSection(m)
#define adu_mutex_unlock(m) LeaveCriticalSection(m)
#define adu_mutex_destroy(m) DeleteCriticalSection(m)
#else
#include <pthread.h>
typedef pthread_mutex_t adu_mutex_t;
#define adu_mutex_init(m) pthread_mutex_init(m, NULL)
#define adu_mutex_lock(m) pthread_mutex_lock(m)
#define adu_mutex_unlock(m) pthread_mutex_unlock(m)
#define adu_mutex_destroy(m) pthread_mutex_destroy(m)
#endif

// Buffer sizes
#define ADUC_LOG_BUFFER_SIZE 8192  // 8KB write buffer
#define ADUC_LOG_FLUSH_THRESHOLD 4096  // Flush when buffer exceeds 4KB

/**
 * @brief Internal log writer state.
 */
struct ADUC_LogWriter
{
    FILE* handle;
    uint8_t buffer[ADUC_LOG_BUFFER_SIZE];
    size_t bufferPos;
    size_t totalBytesWritten;
#ifdef _WIN32
    LARGE_INTEGER startTick;
    LARGE_INTEGER tickFreq;
#else
    struct timespec startTime;
#endif
    adu_mutex_t mutex;
};

// Module-level state
static ADUC_LogLevel s_maxLevel = ADUC_LOG_INFO;
ADUC_LogLevel g_logLevel = ADUC_LOG_INFO;

/**
 * @brief Get relative timestamp (seconds since writer creation).
 */
static uint32_t log_get_relative_timestamp(const ADUC_LogWriter* writer)
{
#ifdef _WIN32
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (uint32_t)((now.QuadPart - writer->startTick.QuadPart) / writer->tickFreq.QuadPart);
#else
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    time_t elapsed = now.tv_sec - writer->startTime.tv_sec;
    if (elapsed < 0)
    {
        elapsed = 0;
    }

    return (uint32_t)elapsed;
#endif
}

/**
 * @brief Flush internal buffer to file (caller must hold mutex).
 */
static ADUC_Result2 log_flush_locked(ADUC_LogWriter* writer)
{
    if (writer->bufferPos == 0)
    {
        return ADUC_RESULT2_SUCCESS;
    }

    size_t written = fwrite(writer->buffer, 1, writer->bufferPos, writer->handle);
    if (written != writer->bufferPos)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1);
    }

    if (fflush(writer->handle) != 0)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 2);
    }

    writer->bufferPos = 0;
    return ADUC_RESULT2_SUCCESS;
}

/**
 * @brief Append data to the write buffer, flushing if threshold exceeded (caller must hold mutex).
 */
static ADUC_Result2 log_buffer_append(ADUC_LogWriter* writer, const void* data, size_t len)
{
    // If data won't fit in buffer at all, flush first then write directly
    if (len > ADUC_LOG_BUFFER_SIZE)
    {
        ADUC_Result2 flushResult = log_flush_locked(writer);
        if (ADUC_RESULT2_IS_FAILURE(flushResult))
        {
            return flushResult;
        }

        size_t written = fwrite(data, 1, len, writer->handle);
        if (written != len)
        {
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1);
        }

        writer->totalBytesWritten += len;
        return ADUC_RESULT2_SUCCESS;
    }

    // Flush if adding this data would overflow the buffer
    if (writer->bufferPos + len > ADUC_LOG_BUFFER_SIZE)
    {
        ADUC_Result2 flushResult = log_flush_locked(writer);
        if (ADUC_RESULT2_IS_FAILURE(flushResult))
        {
            return flushResult;
        }
    }

    memcpy(writer->buffer + writer->bufferPos, data, len);
    writer->bufferPos += len;
    writer->totalBytesWritten += len;

    // Auto-flush when threshold exceeded
    if (writer->bufferPos >= ADUC_LOG_FLUSH_THRESHOLD)
    {
        return log_flush_locked(writer);
    }

    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_Log_Create(const char* logFilePath, ADUC_LogWriter** outWriter)
{
    if (logFilePath == NULL || outWriter == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 1);
    }

    *outWriter = NULL;

    ADUC_LogWriter* writer = (ADUC_LogWriter*)calloc(1, sizeof(ADUC_LogWriter));
    if (writer == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 1);
    }

    writer->handle = fopen(logFilePath, "ab");
    if (writer->handle == NULL)
    {
        free(writer);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 3);
    }

#ifdef _WIN32
    adu_mutex_init(&writer->mutex);
#else
    if (adu_mutex_init(&writer->mutex) != 0)
    {
        fclose(writer->handle);
        free(writer);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 2);
    }
#endif

    #ifdef _WIN32
    QueryPerformanceFrequency(&writer->tickFreq);
    QueryPerformanceCounter(&writer->startTick);
#else
    clock_gettime(CLOCK_MONOTONIC, &writer->startTime);
#endif
    writer->bufferPos = 0;
    writer->totalBytesWritten = 0;

    *outWriter = writer;
    return ADUC_RESULT2_SUCCESS;
}

void ADUC_Log_Destroy(ADUC_LogWriter* writer)
{
    if (writer == NULL)
    {
        return;
    }

    adu_mutex_lock(&writer->mutex);
    log_flush_locked(writer);
    fclose(writer->handle);
    adu_mutex_unlock(&writer->mutex);

    adu_mutex_destroy(&writer->mutex);
    free(writer);
}

ADUC_Result2 ADUC_Log_Write(
    ADUC_LogWriter* writer,
    ADUC_LogLevel level,
    uint16_t eventId,
    const void* payload,
    size_t payloadLen)
{
    if (writer == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 2);
    }

    // Level filter: discard entries above the configured max level
    if ((int)level > (int)s_maxLevel)
    {
        return ADUC_RESULT2_SUCCESS;
    }

    // Build 8-byte header
    uint8_t header[ADUC_LOG_HEADER_SIZE];
    uint32_t timestamp = log_get_relative_timestamp(writer);
    uint8_t flags = 0;

    if (payloadLen > 0 && payload != NULL)
    {
        flags |= ADUC_LOG_FLAG_HAS_PAYLOAD;
    }

    // Pack header in little-endian format
    header[0] = (uint8_t)(timestamp & 0xFF);
    header[1] = (uint8_t)((timestamp >> 8) & 0xFF);
    header[2] = (uint8_t)((timestamp >> 16) & 0xFF);
    header[3] = (uint8_t)((timestamp >> 24) & 0xFF);
    header[4] = (uint8_t)(eventId & 0xFF);
    header[5] = (uint8_t)((eventId >> 8) & 0xFF);
    header[6] = (uint8_t)level;
    header[7] = flags;

    adu_mutex_lock(&writer->mutex);

    ADUC_Result2 result = log_buffer_append(writer, header, ADUC_LOG_HEADER_SIZE);
    if (ADUC_RESULT2_IS_SUCCESS(result) && (flags & ADUC_LOG_FLAG_HAS_PAYLOAD))
    {
        result = log_buffer_append(writer, payload, payloadLen);
    }

    adu_mutex_unlock(&writer->mutex);
    return result;
}

void ADUC_Log_WriteText(
    ADUC_LogLevel level,
    const char* component,
    const char* format, ...)
{
    if ((int)level > (int)g_logLevel)
    {
        return;
    }

    // Get current wall-clock time for text output
#ifdef _WIN32
    time_t wall_sec = time(NULL);
    struct tm tm_buf;
    gmtime_s(&tm_buf, &wall_sec);
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm_buf;
    gmtime_r(&ts.tv_sec, &tm_buf);
#endif

    // Map level to string
    const char* levelStr;
    switch (level)
    {
        case ADUC_LOG_FATAL: levelStr = "FATAL"; break;
        case ADUC_LOG_ERROR: levelStr = "ERROR"; break;
        case ADUC_LOG_WARN:  levelStr = "WARN";  break;
        case ADUC_LOG_INFO:  levelStr = "INFO";  break;
        case ADUC_LOG_DEBUG: levelStr = "DEBUG"; break;
        case ADUC_LOG_TRACE: levelStr = "TRACE"; break;
        default:             levelStr = "?";     break;
    }

    // Print timestamp, level, and component prefix
    fprintf(
        stderr,
        "[%04d-%02d-%02dT%02d:%02d:%02dZ][%s][%s] ",
        tm_buf.tm_year + 1900,
        tm_buf.tm_mon + 1,
        tm_buf.tm_mday,
        tm_buf.tm_hour,
        tm_buf.tm_min,
        tm_buf.tm_sec,
        levelStr,
        component != NULL ? component : "unknown");

    // Print user message
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputc('\n', stderr);
}

ADUC_Result2 ADUC_Log_WriteString(
    ADUC_LogWriter* writer,
    ADUC_LogLevel level,
    uint16_t eventId,
    const char* str)
{
    if (str == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 3);
    }

    size_t strLen = strlen(str);
    if (strLen > UINT16_MAX)
    {
        strLen = UINT16_MAX;
    }

    // Encode as [uint16_t length][bytes]
    size_t payloadLen = sizeof(uint16_t) + strLen;
    uint8_t* payload = (uint8_t*)malloc(payloadLen);
    if (payload == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 1);
    }

    // Length prefix (little-endian)
    uint16_t len16 = (uint16_t)strLen;
    payload[0] = (uint8_t)(len16 & 0xFF);
    payload[1] = (uint8_t)((len16 >> 8) & 0xFF);
    memcpy(payload + sizeof(uint16_t), str, strLen);

    ADUC_Result2 result = ADUC_Log_Write(writer, level, eventId, payload, payloadLen);

    free(payload);
    return result;
}

ADUC_Result2 ADUC_Log_Flush(ADUC_LogWriter* writer)
{
    if (writer == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_CONFIG, 2);
    }

    adu_mutex_lock(&writer->mutex);
    ADUC_Result2 result = log_flush_locked(writer);
    adu_mutex_unlock(&writer->mutex);

    return result;
}

size_t ADUC_Log_GetBytesWritten(const ADUC_LogWriter* writer)
{
    if (writer == NULL)
    {
        return 0;
    }

    return writer->totalBytesWritten;
}

void ADUC_Log_SetLevel(ADUC_LogLevel maxLevel)
{
    s_maxLevel = maxLevel;
}

ADUC_LogLevel ADUC_Log_GetLevel(void)
{
    return s_maxLevel;
}
