/**
 * @file localapi_security.h
 * @brief Security and authorization for the Agent Local API.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_LOCALAPI_SECURITY_H
#define ADUC_LOCALAPI_SECURITY_H

#include "aduc/ipc_transport.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Permission flags for Local API operations.
 */
typedef enum LocalApiPermission
{
    LOCALAPI_PERM_NONE = 0x00,
    LOCALAPI_PERM_READ_STATUS = 0x01,   /**< Read agent status (adu group members) */
    LOCALAPI_PERM_PAUSE_RESUME = 0x02,  /**< Pause/resume updates (adu-admin) */
    LOCALAPI_PERM_CANCEL = 0x04,        /**< Cancel current operation (adu-admin) */
    LOCALAPI_PERM_FORCE_CHECK = 0x08,   /**< Force update check (root/SYSTEM only) */
    LOCALAPI_PERM_ALL = 0x0F,
} LocalApiPermission;

/**
 * @brief Rate limit state for a single peer.
 */
typedef struct RateLimitEntry
{
    uint32_t peerId;       /**< UID on Linux, PID on Windows */
    uint32_t requestCount; /**< Requests in current window */
    uint64_t windowStart;  /**< Window start time (monotonic ms) */
} RateLimitEntry;

/**
 * @brief Security context for the Local API server.
 */
typedef struct LocalApiSecurityContext
{
    int maxRequestsPerSecond;  /**< Max requests per peer per second */
    int maxConcurrentClients;  /**< Max concurrent connections */
    int currentClientCount;    /**< Current active connections */
    RateLimitEntry* rateLimitTable;
    int rateLimitTableSize;
} LocalApiSecurityContext;

/**
 * @brief Initialize the security context.
 * @param ctx The security context to initialize.
 * @param maxReqPerSec Maximum requests per second per peer (0 = default 10).
 * @param maxClients Maximum concurrent clients (0 = default 5).
 * @return true on success.
 */
bool localapi_security_init(LocalApiSecurityContext* ctx, int maxReqPerSec, int maxClients);

/**
 * @brief Cleanup the security context.
 */
void localapi_security_uninit(LocalApiSecurityContext* ctx);

/**
 * @brief Check if a new connection is allowed (concurrent limit).
 */
bool localapi_security_allow_connection(LocalApiSecurityContext* ctx);

/**
 * @brief Notify that a connection was closed.
 */
void localapi_security_connection_closed(LocalApiSecurityContext* ctx);

/**
 * @brief Authorize a peer for a specific operation.
 *
 * @param creds Peer credentials obtained from ipc_get_peer_credentials().
 * @param requiredPerm The permission required for the operation.
 * @return true if the peer is authorized.
 */
bool localapi_security_authorize(const IpcPeerCredentials* creds, LocalApiPermission requiredPerm);

/**
 * @brief Check if a request from a peer is within rate limits.
 *
 * @param ctx The security context.
 * @param creds Peer credentials.
 * @return true if the request is allowed, false if rate-limited.
 */
bool localapi_security_check_rate_limit(LocalApiSecurityContext* ctx, const IpcPeerCredentials* creds);

#ifdef __cplusplus
}
#endif

#endif // ADUC_LOCALAPI_SECURITY_H
