/**
 * @file log_callback.c
 * @brief Host-side ADUC_LogFn implementation for the extension SDK.
 *
 * Routes extension log calls to the binary log writer and optionally
 * mirrors formatted output to stderr for development/debugging.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/log_callback.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef CRITICAL_SECTION adu_log_mutex_t;
static inline void adu_log_mutex_init(adu_log_mutex_t* m) { InitializeCriticalSection(m); }
#define adu_log_mutex_lock(m) EnterCriticalSection(m)
#define adu_log_mutex_unlock(m) LeaveCriticalSection(m)
#define adu_log_mutex_destroy(m) DeleteCriticalSection(m)
#else
#include <pthread.h>
typedef pthread_mutex_t adu_log_mutex_t;
static inline int adu_log_mutex_init(adu_log_mutex_t* m) { return pthread_mutex_init(m, NULL); }
#define adu_log_mutex_lock(m) pthread_mutex_lock(m)
#define adu_log_mutex_unlock(m) pthread_mutex_unlock(m)
#define adu_log_mutex_destroy(m) pthread_mutex_destroy(m)
#endif

#define MAX_COMPONENT_OVERRIDES 32
#define MAX_COMPONENT_NAME_LEN  64
#define MAX_CORRELATION_ID_LEN  64
#define MAX_MSG_LEN            1024
#define DEFAULT_TIMESTAMP_FMT  "%Y-%m-%d %H:%M:%S"

typedef struct ComponentLevelOverride
{
    char component[MAX_COMPONENT_NAME_LEN];
    ADUC_LogLevel level;
} ComponentLevelOverride;

struct ADUC_LogCallbackContext
{
    ADUC_LogCallbackConfig config;
    ADUC_LogWriter* writer; // Borrowed, not owned

    // Correlation
    char workflowId[MAX_CORRELATION_ID_LEN];
    char correlationId[MAX_CORRELATION_ID_LEN];

    // Per-component level overrides
    ComponentLevelOverride componentOverrides[MAX_COMPONENT_OVERRIDES];
    size_t overrideCount;

    // Thread safety
    adu_log_mutex_t mutex;
};

/**
 * @brief Map ADUC_LogLevel to a short label for stderr output.
 */
static const char* level_to_string(ADUC_LogLevel level)
{
    switch (level)
    {
        case ADUC_LOG_FATAL: return "FATAL";
        case ADUC_LOG_ERROR: return "ERROR";
        case ADUC_LOG_WARN:  return "WARN";
        case ADUC_LOG_INFO:  return "INFO";
        case ADUC_LOG_DEBUG: return "DEBUG";
        case ADUC_LOG_TRACE: return "TRACE";
        default:             return "?????";
    }
}

/**
 * @brief Determine the effective minimum log level for a component.
 *
 * Checks per-component overrides first, falls back to global minimum.
 * Caller must hold lctx->mutex.
 */
static ADUC_LogLevel get_effective_level(
    const ADUC_LogCallbackContext* lctx,
    const char* component)
{
    if (component != NULL)
    {
        for (size_t i = 0; i < lctx->overrideCount; i++)
        {
            if (strcmp(lctx->componentOverrides[i].component, component) == 0)
            {
                return lctx->componentOverrides[i].level;
            }
        }
    }

    return lctx->config.globalMinLevel;
}

/**
 * @brief Write a formatted log line to stderr.
 *
 * Caller must hold lctx->mutex.
 */
static void write_to_stderr(
    const ADUC_LogCallbackContext* lctx,
    ADUC_LogLevel level,
    const char* component,
    uint16_t eventId,
    const char* msg)
{
    if (lctx->config.includeTimestamp)
    {
        time_t now = time(NULL);
        struct tm tm_buf;
        localtime_r(&now, &tm_buf);

        const char* fmt = (lctx->config.timestampFormat != NULL)
                              ? lctx->config.timestampFormat
                              : DEFAULT_TIMESTAMP_FMT;

        char ts[64];
        strftime(ts, sizeof(ts), fmt, &tm_buf);

        fprintf(
            stderr,
            "[%s][%-5s][%s:%u] %s\n",
            ts,
            level_to_string(level),
            component ? component : "-",
            (unsigned)eventId,
            msg);
    }
    else
    {
        fprintf(
            stderr,
            "[%-5s][%s:%u] %s\n",
            level_to_string(level),
            component ? component : "-",
            (unsigned)eventId,
            msg);
    }
}

/**
 * @brief The actual log callback function matching the ADUC_LogFn signature.
 *
 * This function is passed to extensions via ADUC_ExtensionContext.Log.
 * The format attribute is on the ADUC_LogFn typedef, not on this definition.
 */
static void log_callback_fn(
    void* ctx,
    ADUC_LogLevel level,
    const char* component,
    uint16_t eventId,
    const char* fmt,
    ...)
{
    ADUC_LogCallbackContext* lctx = (ADUC_LogCallbackContext*)ctx;
    if (lctx == NULL || fmt == NULL)
    {
        return;
    }

    // Quick level check before taking the lock (globalMinLevel is the
    // loosest filter — per-component may be tighter but never looser
    // than the global setting... actually it can be, so we just check
    // against globalMinLevel as a fast-path and recheck under lock).
    // Higher numeric level = more verbose. Discard if too verbose.
    if (level > lctx->config.globalMinLevel)
    {
        // May still pass a per-component override; check under lock.
        // But if there are no overrides, skip entirely.
        if (lctx->overrideCount == 0)
        {
            return;
        }
    }

    va_list args;
    va_start(args, fmt);

    char msgBuf[MAX_MSG_LEN];
    vsnprintf(msgBuf, sizeof(msgBuf), fmt, args);

    va_end(args);

    adu_log_mutex_lock(&lctx->mutex);

    // Recheck effective level under lock (handles per-component overrides)
    ADUC_LogLevel minLevel = get_effective_level(lctx, component);
    if (level > minLevel)
    {
        adu_log_mutex_unlock(&lctx->mutex);
        return;
    }

    // Write to binary log writer
    if (lctx->writer != NULL)
    {
        ADUC_Log_WriteString(lctx->writer, level, eventId, msgBuf);
    }

    // Mirror to stderr if configured
    if (lctx->config.mirrorToStderr)
    {
        write_to_stderr(lctx, level, component, eventId, msgBuf);
    }

    adu_log_mutex_unlock(&lctx->mutex);
}

ADUC_Result2 ADUC_LogCallback_Create(
    const ADUC_LogCallbackConfig* config,
    ADUC_LogWriter* writer,
    ADUC_LogCallbackContext** outCtx)
{
    if (outCtx == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 1);
    }

    *outCtx = NULL;

    ADUC_LogCallbackContext* ctx = calloc(1, sizeof(ADUC_LogCallbackContext));
    if (ctx == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 2);
    }

    if (config != NULL)
    {
        ctx->config = *config;
    }
    else
    {
        // Defaults
        ctx->config.globalMinLevel = ADUC_LOG_INFO;
        ctx->config.mirrorToStderr = false;
        ctx->config.includeTimestamp = true;
        ctx->config.timestampFormat = NULL; // will use DEFAULT_TIMESTAMP_FMT
    }

    ctx->writer = writer;
    ctx->overrideCount = 0;

    #ifdef _WIN32
    adu_log_mutex_init(&ctx->mutex);
    if (0)
#else
    if (adu_log_mutex_init(&ctx->mutex) != 0)
#endif
    {
        free(ctx);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 3);
    }

    *outCtx = ctx;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_LogFn ADUC_LogCallback_GetLogFn(void)
{
    return log_callback_fn;
}

void* ADUC_LogCallback_GetContext(ADUC_LogCallbackContext* ctx)
{
    return (void*)ctx;
}

void ADUC_LogCallback_SetMinLevel(ADUC_LogCallbackContext* ctx, ADUC_LogLevel level)
{
    if (ctx == NULL)
    {
        return;
    }

    adu_log_mutex_lock(&ctx->mutex);
    ctx->config.globalMinLevel = level;
    adu_log_mutex_unlock(&ctx->mutex);
}

void ADUC_LogCallback_SetComponentLevel(
    ADUC_LogCallbackContext* ctx,
    const char* component,
    ADUC_LogLevel level)
{
    if (ctx == NULL || component == NULL)
    {
        return;
    }

    adu_log_mutex_lock(&ctx->mutex);

    // Update existing override if present
    for (size_t i = 0; i < ctx->overrideCount; i++)
    {
        if (strcmp(ctx->componentOverrides[i].component, component) == 0)
        {
            ctx->componentOverrides[i].level = level;
            adu_log_mutex_unlock(&ctx->mutex);
            return;
        }
    }

    // Add new override if space available
    if (ctx->overrideCount < MAX_COMPONENT_OVERRIDES)
    {
        strncpy(
            ctx->componentOverrides[ctx->overrideCount].component,
            component,
            MAX_COMPONENT_NAME_LEN - 1);
        ctx->componentOverrides[ctx->overrideCount].component[MAX_COMPONENT_NAME_LEN - 1] = '\0';
        ctx->componentOverrides[ctx->overrideCount].level = level;
        ctx->overrideCount++;
    }

    adu_log_mutex_unlock(&ctx->mutex);
}

void ADUC_LogCallback_SetCorrelation(
    ADUC_LogCallbackContext* ctx,
    const char* workflowId,
    const char* correlationId)
{
    if (ctx == NULL)
    {
        return;
    }

    adu_log_mutex_lock(&ctx->mutex);

    if (workflowId != NULL)
    {
        strncpy(ctx->workflowId, workflowId, MAX_CORRELATION_ID_LEN - 1);
        ctx->workflowId[MAX_CORRELATION_ID_LEN - 1] = '\0';
    }
    else
    {
        ctx->workflowId[0] = '\0';
    }

    if (correlationId != NULL)
    {
        strncpy(ctx->correlationId, correlationId, MAX_CORRELATION_ID_LEN - 1);
        ctx->correlationId[MAX_CORRELATION_ID_LEN - 1] = '\0';
    }
    else
    {
        ctx->correlationId[0] = '\0';
    }

    adu_log_mutex_unlock(&ctx->mutex);
}

void ADUC_LogCallback_Flush(ADUC_LogCallbackContext* ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    adu_log_mutex_lock(&ctx->mutex);

    if (ctx->writer != NULL)
    {
        ADUC_Log_Flush(ctx->writer);
    }

    if (ctx->config.mirrorToStderr)
    {
        fflush(stderr);
    }

    adu_log_mutex_unlock(&ctx->mutex);
}

void ADUC_LogCallback_Destroy(ADUC_LogCallbackContext* ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    // Flush before destroying
    if (ctx->writer != NULL)
    {
        ADUC_Log_Flush(ctx->writer);
    }

    adu_log_mutex_destroy(&ctx->mutex);
    free(ctx);
}
