/**
 * @file communication_vtable.h
 * @brief Communication provider extension vtable.
 *
 * Communication providers connect the agent to an update service
 * (Azure IoT Hub, ADU Direct, or custom). The vtable defines the
 * contract for receiving deployments and reporting state.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_COMMUNICATION_VTABLE_H
#define ADUC_COMMUNICATION_VTABLE_H

#include "aduc/extension_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Communication event types (inbound from service).
 */
typedef enum ADUC_CommEventType
{
    ADUC_COMM_EVENT_DEPLOYMENT_AVAILABLE = 1,
    ADUC_COMM_EVENT_CANCEL = 2,
    ADUC_COMM_EVENT_CONFIG_CHANGED = 3,
    ADUC_COMM_EVENT_CONNECTION_CHANGED = 4,
} ADUC_CommEventType;

/**
 * @brief Communication health status.
 */
typedef enum ADUC_CommHealthStatus
{
    ADUC_COMM_HEALTH_CONNECTED = 1,
    ADUC_COMM_HEALTH_DISCONNECTED = 2,
    ADUC_COMM_HEALTH_DEGRADED = 3,
} ADUC_CommHealthStatus;

/**
 * @brief Connection state.
 */
typedef enum ADUC_CommConnectionState
{
    ADUC_COMM_STATE_DISCONNECTED = 0,
    ADUC_COMM_STATE_CONNECTING = 1,
    ADUC_COMM_STATE_CONNECTED = 2,
    ADUC_COMM_STATE_RECONNECTING = 3,
} ADUC_CommConnectionState;

/**
 * @brief Communication configuration (provider-specific settings).
 */
typedef struct ADUC_CommConfig
{
    const char* endpoint;
    const char* deviceId;
    const char* moduleId;
    const char* certPath;
    const char* keyPath;
    uint32_t pollIntervalSec;
    uint32_t reconnectDelaySec;
    uint32_t maxReconnectDelaySec;
} ADUC_CommConfig;

/**
 * @brief Inbound message from service.
 */
typedef struct ADUC_CommMessage
{
    ADUC_CommEventType type;
    const char* payload;         // JSON payload (deployment manifest, config, etc.)
    size_t payloadLen;
    const char* correlationId;
} ADUC_CommMessage;

/**
 * @brief Agent state reported to service.
 */
typedef struct ADUC_AgentState
{
    const char* stateJson;       // Serialized agent state (workflow, component inventory)
    size_t stateJsonLen;
} ADUC_AgentState;

/**
 * @brief Deployment result reported to service (v3 wire format).
 */
typedef struct ADUC_DeploymentResult2
{
    const char* workflowId;
    ADUC_Outcome outcome;              /**< Authoritative terminal outcome */
    ADUC_FailureOrigin failureOrigin;  /**< Advisory failure source hint (v3: "failureOrigin" on wire) */
    int64_t resultCode;                /**< Legacy result code (0=failed, -1=canceled, positive=success) */
    const char* extendedResultCodes;   /**< Comma-separated hex ERCs e.g. "3000001C,80004005" */
    const char* resultDetails;         /**< Human-readable diagnostic */
    const char* installedUpdateId;     /**< Opaque string (serialized JSON by convention) */
    const char* stepResultsJson;       /**< Per-step results as JSON */
} ADUC_DeploymentResult2;

/**
 * @brief Communication event callback.
 */
typedef void (*ADUC_CommCallback)(ADUC_CommEventType event, const ADUC_CommMessage* msg, void* ctx);

/**
 * @brief Communication provider vtable.
 *
 * Extension descriptor's `vtable` field points to this when type == ADUC_EXT_TYPE_COMMUNICATION.
 */
typedef struct ADUC_CommunicationVtable
{
    uint32_t structVersion;  // 1

    // ─── Lifecycle ─────────────────────────────────────────────────────

    /** Connect to the service endpoint. */
    ADUC_Result2 (*Connect)(const ADUC_CommConfig* config);

    /** Disconnect from service. */
    void (*Disconnect)(void);

    /** Query connection state. */
    ADUC_CommConnectionState (*GetConnectionState)(void);

    // ─── Inbound (service → agent) ────────────────────────────────────

    /**
     * @brief Poll for available messages (pull-based providers).
     * For push-based providers (IoT Hub), this may be a no-op that returns immediately.
     * @param outMsg Output message (caller owns payload lifetime until next Poll).
     * @param timeoutMs Maximum time to wait (0 = non-blocking).
     * @return Success if message available, failure code if timeout/error.
     */
    ADUC_Result2 (*Poll)(ADUC_CommMessage* outMsg, uint32_t timeoutMs);

    /**
     * @brief Register callback for async events (push-based providers).
     * Multiple callbacks can be registered for different event types.
     */
    ADUC_Result2 (*RegisterCallback)(ADUC_CommEventType event, ADUC_CommCallback cb, void* ctx);

    // ─── Outbound (agent → service) ───────────────────────────────────

    /** Report current agent state (inventory, health, status). */
    ADUC_Result2 (*ReportState)(const ADUC_AgentState* state);

    /** Report deployment result. */
    ADUC_Result2 (*ReportResult)(const ADUC_DeploymentResult2* result);

    // ─── Content ──────────────────────────────────────────────────────

    /**
     * @brief Get download URL for a content file.
     * @param fileId File identifier from deployment manifest.
     * @param urlBuf Output buffer for URL.
     * @param urlBufLen Buffer capacity.
     * @return Success with URL in buffer, or failure.
     */
    ADUC_Result2 (*GetDownloadUrl)(const char* fileId, char* urlBuf, size_t urlBufLen);

    // ─── Diagnostics ──────────────────────────────────────────────────

    /** Upload diagnostics payload to service. */
    ADUC_Result2 (*SendDiagnostics)(const void* payload, size_t payloadLen);

    // ─── Health ───────────────────────────────────────────────────────

    /** Perform a health check (lightweight connectivity test). */
    ADUC_Result2 (*HealthCheck)(ADUC_CommHealthStatus* outStatus);

} ADUC_CommunicationVtable;

#ifdef __cplusplus
}
#endif

#endif // ADUC_COMMUNICATION_VTABLE_H
