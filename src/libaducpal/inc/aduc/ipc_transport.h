/**
 * @file ipc_transport.h
 * @brief Platform-agnostic IPC transport interface for the Agent Local API.
 *
 * Provides a connection-oriented, bidirectional IPC mechanism that abstracts
 * Unix domain sockets (Linux) and Named Pipes (Windows).
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_IPC_TRANSPORT_H
#define ADUC_IPC_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Opaque handle to an IPC transport instance.
 */
typedef struct IpcTransport IpcTransport;

/**
 * @brief Peer credentials obtained from a connected client.
 */
typedef struct IpcPeerCredentials
{
#ifdef _WIN32
    uint32_t processId; /**< Client process ID */
    // On Windows, SID-based authorization is done via token impersonation
    // rather than storing credentials in this struct.
    bool isElevated; /**< Whether the client process is running elevated */
#else
    uint32_t uid; /**< Client user ID */
    uint32_t gid; /**< Client group ID */
    uint32_t pid; /**< Client process ID */
#endif
} IpcPeerCredentials;

/**
 * @brief Result codes for IPC transport operations.
 */
typedef enum IpcTransportResult
{
    IPC_OK = 0,
    IPC_ERR_INVALID_ARG = -1,
    IPC_ERR_ALREADY_BOUND = -2,
    IPC_ERR_BIND_FAILED = -3,
    IPC_ERR_LISTEN_FAILED = -4,
    IPC_ERR_ACCEPT_FAILED = -5,
    IPC_ERR_CONNECT_FAILED = -6,
    IPC_ERR_SEND_FAILED = -7,
    IPC_ERR_RECV_FAILED = -8,
    IPC_ERR_TIMEOUT = -9,
    IPC_ERR_PEER_DISCONNECTED = -10,
    IPC_ERR_PERMISSION_DENIED = -11,
    IPC_ERR_OUT_OF_MEMORY = -12,
    IPC_ERR_PLATFORM = -13,
} IpcTransportResult;

/**
 * @brief Configuration for creating an IPC transport server.
 */
typedef struct IpcServerConfig
{
    const char* endpoint;    /**< Endpoint path (socket path on Linux, pipe name on Windows) */
    int maxConnections;      /**< Maximum concurrent connections (0 = default of 5) */
    int recvTimeoutMs;       /**< Receive timeout in milliseconds (0 = default of 5000) */
    int sendTimeoutMs;       /**< Send timeout in milliseconds (0 = default of 5000) */
} IpcServerConfig;

// ============================================================
// Server-side API
// ============================================================

/**
 * @brief Create and bind an IPC server transport.
 *
 * On Linux: Creates a Unix domain socket and binds to the given path.
 * On Windows: Creates a Named Pipe instance at the given pipe name.
 *
 * @param config Server configuration.
 * @param[out] server Receives the created server transport handle.
 * @return IPC_OK on success, error code on failure.
 */
IpcTransportResult ipc_server_create(const IpcServerConfig* config, IpcTransport** server);

/**
 * @brief Accept an incoming client connection.
 *
 * Blocks until a client connects or the transport is closed.
 *
 * @param server The server transport handle.
 * @param[out] client Receives the connected client transport handle.
 * @return IPC_OK on success, IPC_ERR_TIMEOUT if no client connects within timeout.
 */
IpcTransportResult ipc_server_accept(IpcTransport* server, IpcTransport** client);

/**
 * @brief Get peer credentials from a connected client transport.
 *
 * Must be called after ipc_server_accept(). Uses SO_PEERCRED on Linux
 * or GetNamedPipeClientProcessId()/token impersonation on Windows.
 *
 * @param client The connected client transport handle.
 * @param[out] creds Receives the peer credentials.
 * @return IPC_OK on success, error code on failure.
 */
IpcTransportResult ipc_get_peer_credentials(IpcTransport* client, IpcPeerCredentials* creds);

// ============================================================
// Client-side API
// ============================================================

/**
 * @brief Connect to an IPC server endpoint.
 *
 * @param endpoint Endpoint path (socket path on Linux, pipe name on Windows).
 * @param timeoutMs Connection timeout in milliseconds (0 = default of 5000).
 * @param[out] transport Receives the connected transport handle.
 * @return IPC_OK on success, error code on failure.
 */
IpcTransportResult ipc_client_connect(const char* endpoint, int timeoutMs, IpcTransport** transport);

// ============================================================
// Common send/receive API (used by both server and client)
// ============================================================

/**
 * @brief Send data over an established IPC connection.
 *
 * Sends exactly `len` bytes. Blocks until all data is sent or an error occurs.
 *
 * @param transport The transport handle (server-accepted or client-connected).
 * @param buf Pointer to the data to send.
 * @param len Number of bytes to send.
 * @return IPC_OK on success, error code on failure.
 */
IpcTransportResult ipc_send(IpcTransport* transport, const void* buf, size_t len);

/**
 * @brief Receive data from an established IPC connection.
 *
 * Blocks until exactly `len` bytes are received, the peer disconnects,
 * or the receive timeout expires.
 *
 * @param transport The transport handle.
 * @param buf Buffer to receive data into.
 * @param len Number of bytes to receive.
 * @param[out] bytesReceived Actual bytes received (may be less than len on disconnect).
 * @return IPC_OK on success, IPC_ERR_PEER_DISCONNECTED if peer closed, IPC_ERR_TIMEOUT on timeout.
 */
IpcTransportResult ipc_recv(IpcTransport* transport, void* buf, size_t len, size_t* bytesReceived);

// ============================================================
// Lifecycle
// ============================================================

/**
 * @brief Close and destroy an IPC transport handle.
 *
 * For server transports, this also unlinks the socket file (Linux)
 * or closes the pipe handle (Windows).
 *
 * @param transport The transport handle to destroy. May be NULL (no-op).
 */
void ipc_transport_close(IpcTransport* transport);

/**
 * @brief Get the platform-specific default endpoint for the Agent Local API.
 *
 * @return Default endpoint string. Do not free.
 *   Linux:   "/var/lib/adu/api/localapi.sock"
 *   Windows: "\\\\.\\pipe\\adu-agent-localapi"
 */
const char* ipc_get_default_endpoint(void);

#ifdef __cplusplus
}
#endif

#endif // ADUC_IPC_TRANSPORT_H
