/**
 * @file extension_context.h
 * @brief Host-provided services passed to extensions during initialization.
 *
 * The ADUC_ExtensionContext is the extension's interface back to the host (agent).
 * It provides:
 *   - Logging (binary/text transparent to extension)
 *   - Configuration access
 *   - Secret retrieval
 *   - Process supervisor (for spawning child processes)
 *   - Workflow correlation context
 *
 * Extensions MUST use the provided log function (never write to stdout/files directly).
 * Extensions MUST NOT store the context pointer beyond Uninitialize().
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_EXTENSION_CONTEXT_H
#define ADUC_EXTENSION_CONTEXT_H

#include "aduc/extension_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Forward declaration of process supervisor.
 */
typedef struct ADUC_ProcessSupervisor ADUC_ProcessSupervisor;

/**
 * @brief Log function signature (host-provided).
 *
 * In production mode, the host extracts parameters and encodes as binary
 * (format string lives in manifest, never stored in log file).
 * In development mode, behaves like printf to stderr.
 *
 * @param ctx      Opaque host logging context.
 * @param level    Severity level.
 * @param component Extension component name (for log categorization).
 * @param eventId  Event identifier (index into extension's log manifest).
 * @param fmt      printf-style format string.
 * @param ...      Format arguments.
 */
typedef void (*ADUC_LogFn)(
    void* ctx,
    ADUC_LogLevel level,
    const char* component,
    uint16_t eventId,
    const char* fmt,
    ...) 
#ifndef _MSC_VER
    __attribute__((format(printf, 5, 6)))
#endif
    ;

/**
 * @brief Configuration access function signature.
 *
 * Retrieves a configuration value for this extension.
 * Keys are relative to the extension's config section.
 * Example: GetConfig(ctx, "pollIntervalSec") → "300"
 *
 * @param ctx  Opaque host config context.
 * @param key  Configuration key (dotted path for nested: "retry.maxRetries").
 * @return Value string (valid until next GetConfig call), or NULL if not found.
 */
typedef const char* (*ADUC_GetConfigFn)(void* ctx, const char* key);

/**
 * @brief Secret retrieval function signature.
 *
 * Retrieves a secret value from secure storage.
 * The buffer is zeroed after the extension calls ReleaseSecret().
 *
 * @param ctx     Opaque host secrets context.
 * @param name    Secret name/key.
 * @param buf     Output buffer for secret value.
 * @param bufLen  Buffer capacity.
 * @param outLen  Actual secret length written (excluding null terminator).
 * @return ADUC_Result2 Success or failure.
 */
typedef ADUC_Result2 (*ADUC_GetSecretFn)(void* ctx, const char* name, char* buf, size_t bufLen, size_t* outLen);

/**
 * @brief Release/zero a secret buffer previously filled by GetSecret.
 */
typedef void (*ADUC_ReleaseSecretFn)(void* ctx, char* buf, size_t bufLen);

/**
 * @brief Host-provided extension context.
 *
 * Passed to extension's Initialize(). Extensions should store this pointer
 * (or copy needed function pointers) for use throughout their lifetime.
 */
typedef struct ADUC_ExtensionContext
{
    /** ABI version of this struct (currently 1). */
    uint32_t structVersion;

    // ─── Logging ───────────────────────────────────────────────────────

    /** Host log function. Extensions MUST use this for all logging. */
    ADUC_LogFn Log;

    /** Opaque context for Log function (pass as first arg). */
    void* logCtx;

    // ─── Configuration ─────────────────────────────────────────────────

    /** Get a config value for this extension. */
    ADUC_GetConfigFn GetConfig;

    /** Opaque context for GetConfig. */
    void* configCtx;

    // ─── Secrets ───────────────────────────────────────────────────────

    /** Retrieve a secret from secure storage. */
    ADUC_GetSecretFn GetSecret;

    /** Zero and release a secret buffer. */
    ADUC_ReleaseSecretFn ReleaseSecret;

    /** Opaque context for secret functions. */
    void* secretsCtx;

    // ─── Process Management ────────────────────────────────────────────

    /** Process supervisor for spawning/managing child processes. May be NULL
     *  if the host doesn't provide process management (e.g., in test stubs). */
    ADUC_ProcessSupervisor* processSupervisor;

    // ─── Correlation ───────────────────────────────────────────────────

    /** Current workflow ID (NULL if no active workflow). Changes per deployment. */
    const char* workflowId;

    /** Correlation ID for distributed tracing. */
    const char* correlationId;

    // ─── Agent Info ────────────────────────────────────────────────────

    /** Agent version string. */
    const char* agentVersion;

    /** Device ID. */
    const char* deviceId;
} ADUC_ExtensionContext;

/**
 * @brief Convenience macros for logging from extensions.
 *
 * Usage:
 *   ADUC_EXT_LOG(ctx, ADUC_LOG_INFO, "my-handler", 4001, "Downloaded %s (%lu bytes)", name, size);
 */
#define ADUC_EXT_LOG(ctx, level, component, eventId, fmt, ...) \
    do { \
        if ((ctx) && (ctx)->Log) { \
            (ctx)->Log((ctx)->logCtx, (level), (component), (eventId), (fmt), ##__VA_ARGS__); \
        } \
    } while (0)

#define ADUC_EXT_LOG_INFO(ctx, comp, eid, fmt, ...)  ADUC_EXT_LOG(ctx, ADUC_LOG_INFO, comp, eid, fmt, ##__VA_ARGS__)
#define ADUC_EXT_LOG_ERROR(ctx, comp, eid, fmt, ...) ADUC_EXT_LOG(ctx, ADUC_LOG_ERROR, comp, eid, fmt, ##__VA_ARGS__)
#define ADUC_EXT_LOG_WARN(ctx, comp, eid, fmt, ...)  ADUC_EXT_LOG(ctx, ADUC_LOG_WARN, comp, eid, fmt, ##__VA_ARGS__)
#define ADUC_EXT_LOG_DEBUG(ctx, comp, eid, fmt, ...) ADUC_EXT_LOG(ctx, ADUC_LOG_DEBUG, comp, eid, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // ADUC_EXTENSION_CONTEXT_H
