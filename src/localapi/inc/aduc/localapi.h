/**
 * @file localapi.h
 * @brief Public interface for the Agent Local API server.
 *
 * The Local API provides a cross-platform IPC mechanism for external processes
 * to query agent status and send control commands.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_LOCALAPI_H
#define ADUC_LOCALAPI_H

#include "aduc/c_utils.h"
#include <stdbool.h>

EXTERN_C_BEGIN

/**
 * @brief Configuration for the Local API server.
 */
typedef struct LocalApiConfig
{
    const char* endpoint;       /**< IPC endpoint (NULL = platform default) */
    int maxConnections;         /**< Max concurrent clients (0 = default 5) */
    int maxRequestsPerSecond;   /**< Rate limit per peer (0 = default 10) */
    int timeoutMs;              /**< I/O timeout in ms (0 = default 5000) */
} LocalApiConfig;

/**
 * @brief Initialize and start the Local API server.
 *
 * Creates a background thread that listens for incoming IPC connections
 * and handles status queries and control commands.
 *
 * @param config Server configuration. Pass NULL for all defaults.
 * @return true on success, false on failure.
 */
bool localapi_init(const LocalApiConfig* config);

/**
 * @brief Shut down the Local API server.
 *
 * Stops the listener thread, disconnects all clients, and cleans up resources.
 *
 * @return true on success, false on failure.
 */
bool localapi_uninit(void);

/**
 * @brief Check if the Local API server is currently running.
 */
bool localapi_is_running(void);

EXTERN_C_END

#endif // ADUC_LOCALAPI_H
