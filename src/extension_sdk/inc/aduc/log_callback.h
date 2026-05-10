/**
 * @file log_callback.h
 * @brief Host-side log callback for the ADU Gen2 extension SDK.
 *
 * Provides the ADUC_LogFn implementation that gets passed to extensions
 * via ADUC_ExtensionContext. Routes extension log calls to the binary
 * log writer and optionally mirrors to stderr in development mode.
 *
 * Thread-safe: multiple extensions may log concurrently.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_LOG_CALLBACK_H
#define ADUC_LOG_CALLBACK_H

#include "aduc/extension_context.h"
#include "aduc/extension_types.h"
#include "aduc/log_writer.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Configuration for the log callback.
 */
typedef struct ADUC_LogCallbackConfig
{
    ADUC_LogLevel globalMinLevel; /**< Minimum severity to log (default: ADUC_LOG_INFO). */
    bool mirrorToStderr;          /**< Also write to stderr (dev mode). */
    bool includeTimestamp;        /**< Prefix stderr output with timestamp. */
    const char* timestampFormat;  /**< strftime format (default: "%Y-%m-%d %H:%M:%S"). */
} ADUC_LogCallbackConfig;

/**
 * @brief Opaque log callback context.
 */
typedef struct ADUC_LogCallbackContext ADUC_LogCallbackContext;

/**
 * @brief Create a log callback context bound to a log writer.
 *
 * @param config  Configuration (may be NULL for defaults).
 * @param writer  Binary log writer (may be NULL for stderr-only mode).
 * @param outCtx  Receives the created context.
 * @return ADUC_Result2  Success or error code.
 */
ADUC_Result2 ADUC_LogCallback_Create(
    const ADUC_LogCallbackConfig* config,
    ADUC_LogWriter* writer,
    ADUC_LogCallbackContext** outCtx);

/**
 * @brief Get the ADUC_LogFn function pointer for use in ADUC_ExtensionContext.
 *
 * @return Function pointer matching the ADUC_LogFn signature.
 */
ADUC_LogFn ADUC_LogCallback_GetLogFn(void);

/**
 * @brief Get the opaque context for use as logCtx in ADUC_ExtensionContext.
 *
 * @param ctx  Log callback context.
 * @return Opaque pointer to pass as the first argument to the log function.
 */
void* ADUC_LogCallback_GetContext(ADUC_LogCallbackContext* ctx);

/**
 * @brief Set the minimum log level filter (can be changed at runtime).
 *
 * Messages with level numerically greater than @p level (i.e. more verbose)
 * are discarded.
 *
 * @param ctx    Log callback context.
 * @param level  New minimum level.
 */
void ADUC_LogCallback_SetMinLevel(ADUC_LogCallbackContext* ctx, ADUC_LogLevel level);

/**
 * @brief Set a per-component log level override.
 *
 * Allows a specific component to log at a different verbosity than the
 * global minimum.
 *
 * @param ctx        Log callback context.
 * @param component  Component name (copied internally, max 63 chars).
 * @param level      Override level for this component.
 */
void ADUC_LogCallback_SetComponentLevel(
    ADUC_LogCallbackContext* ctx,
    const char* component,
    ADUC_LogLevel level);

/**
 * @brief Set correlation context (called when workflow changes).
 *
 * @param ctx            Log callback context.
 * @param workflowId     Current workflow ID (may be NULL).
 * @param correlationId  Correlation ID for distributed tracing (may be NULL).
 */
void ADUC_LogCallback_SetCorrelation(
    ADUC_LogCallbackContext* ctx,
    const char* workflowId,
    const char* correlationId);

/**
 * @brief Flush any buffered log data to disk.
 *
 * @param ctx  Log callback context.
 */
void ADUC_LogCallback_Flush(ADUC_LogCallbackContext* ctx);

/**
 * @brief Destroy the log callback context and release resources.
 *
 * Does not destroy the underlying log writer (it is borrowed, not owned).
 *
 * @param ctx  Log callback context (may be NULL).
 */
void ADUC_LogCallback_Destroy(ADUC_LogCallbackContext* ctx);

#ifdef __cplusplus
}
#endif

#endif // ADUC_LOG_CALLBACK_H
