/**
 * @file localapi_server.c
 * @brief Agent Local API server implementation.
 *
 * Replaces the legacy FIFO-based apisvc with a cross-platform IPC server
 * using Unix domain sockets (Linux) or Named Pipes (Windows).
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/localapi.h"
#include "aduc/ipc_transport.h"
#include "localapi_security.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

// Wire protocol definitions (compatible with Gen1)
#define LOCALAPI_VERSION 1
#define LOCALAPI_MAX_PAYLOAD 4090

typedef enum LocalApiRequestType
{
    LOCALAPI_REQ_NONE = 0x00,
    LOCALAPI_REQ_GET_STATUS = 0x01,
    LOCALAPI_REQ_PAUSE = 0x02,
    LOCALAPI_REQ_RESUME = 0x03,
    LOCALAPI_REQ_CANCEL = 0x04,
    LOCALAPI_REQ_FORCE_CHECK = 0x05,
} LocalApiRequestType;

#pragma pack(push, 1)
typedef struct LocalApiRequestMsg
{
    uint16_t ver;
    uint16_t type;
    uint16_t len;
    uint8_t payload[LOCALAPI_MAX_PAYLOAD];
} LocalApiRequestMsg;

typedef struct LocalApiResponseMsg
{
    uint16_t ver;
    uint16_t status;
    uint16_t len;
    uint8_t payload[LOCALAPI_MAX_PAYLOAD];
} LocalApiResponseMsg;
#pragma pack(pop)

// Forward declaration for status callback (to be provided by the agent)
extern int localapi_get_current_status(void);

// Server state
static IpcTransport* g_server = NULL;
static LocalApiSecurityContext g_secCtx;

#ifndef _WIN32
static pthread_t g_serverThread;
static atomic_bool g_running = false;
#else
static HANDLE g_serverThread = NULL;
static volatile LONG g_running = 0;
#endif

static bool is_running(void)
{
#ifndef _WIN32
    return atomic_load(&g_running);
#else
    return (InterlockedCompareExchange(&g_running, 0, 0) != 0);
#endif
}

static void set_running(bool val)
{
#ifndef _WIN32
    atomic_store(&g_running, val);
#else
    InterlockedExchange(&g_running, val ? 1 : 0);
#endif
}

static LocalApiPermission get_required_permission(LocalApiRequestType type)
{
    switch (type)
    {
    case LOCALAPI_REQ_GET_STATUS:
        return LOCALAPI_PERM_READ_STATUS;
    case LOCALAPI_REQ_PAUSE:
    case LOCALAPI_REQ_RESUME:
    case LOCALAPI_REQ_CANCEL:
        return LOCALAPI_PERM_PAUSE_RESUME;
    case LOCALAPI_REQ_FORCE_CHECK:
        return LOCALAPI_PERM_FORCE_CHECK;
    default:
        return LOCALAPI_PERM_ALL;
    }
}

static void handle_client(IpcTransport* client)
{
    IpcPeerCredentials creds = { 0 };
    LocalApiRequestMsg req = { 0 };
    LocalApiResponseMsg resp = { 0 };

    // Get peer credentials
    if (ipc_get_peer_credentials(client, &creds) != IPC_OK)
    {
        goto done;
    }

    // Rate limit check
    if (!localapi_security_check_rate_limit(&g_secCtx, &creds))
    {
        resp.ver = LOCALAPI_VERSION;
        resp.status = 429; // Too Many Requests
        resp.len = 0;
        ipc_send(client, &resp, 6);
        goto done;
    }

    // Receive request header (6 bytes: ver + type + len)
    size_t received = 0;
    IpcTransportResult rc = ipc_recv(client, &req, 6, &received);
    if (rc != IPC_OK || received < 6)
    {
        goto done;
    }

    // Validate version
    if (req.ver != LOCALAPI_VERSION)
    {
        resp.ver = LOCALAPI_VERSION;
        resp.status = 400; // Bad Request
        resp.len = 0;
        ipc_send(client, &resp, 6);
        goto done;
    }

    // Read payload if present
    if (req.len > 0 && req.len <= LOCALAPI_MAX_PAYLOAD)
    {
        rc = ipc_recv(client, req.payload, req.len, &received);
        if (rc != IPC_OK || received < req.len)
        {
            goto done;
        }
    }

    // Authorization check
    LocalApiPermission perm = get_required_permission((LocalApiRequestType)req.type);
    if (!localapi_security_authorize(&creds, perm))
    {
        resp.ver = LOCALAPI_VERSION;
        resp.status = 403; // Forbidden
        resp.len = 0;
        ipc_send(client, &resp, 6);
        goto done;
    }

    // Process request
    resp.ver = LOCALAPI_VERSION;

    switch ((LocalApiRequestType)req.type)
    {
    case LOCALAPI_REQ_GET_STATUS:
    {
        int status = localapi_get_current_status();
        resp.status = (uint16_t)status;
        resp.len = 0;
        break;
    }

    case LOCALAPI_REQ_PAUSE:
    case LOCALAPI_REQ_RESUME:
    case LOCALAPI_REQ_CANCEL:
    case LOCALAPI_REQ_FORCE_CHECK:
        // TODO: Wire to agent orchestration
        resp.status = 501; // Not Implemented
        resp.len = 0;
        break;

    default:
        resp.status = 400; // Bad Request
        resp.len = 0;
        break;
    }

    ipc_send(client, &resp, 6 + resp.len);

done:
    localapi_security_connection_closed(&g_secCtx);
    ipc_transport_close(client);
}

#ifndef _WIN32
static void* server_thread_proc(void* arg)
{
    (void)arg;
    signal(SIGPIPE, SIG_IGN);

    while (is_running())
    {
        IpcTransport* client = NULL;
        IpcTransportResult rc = ipc_server_accept(g_server, &client);

        if (rc == IPC_ERR_TIMEOUT)
        {
            continue;
        }
        if (rc != IPC_OK)
        {
            if (is_running())
            {
                usleep(100000); // 100ms backoff on error
            }
            continue;
        }

        if (!localapi_security_allow_connection(&g_secCtx))
        {
            ipc_transport_close(client);
            continue;
        }

        g_secCtx.currentClientCount++;
        handle_client(client);
    }

    return NULL;
}
#else
static DWORD WINAPI server_thread_proc(LPVOID arg)
{
    (void)arg;

    while (is_running())
    {
        IpcTransport* client = NULL;
        IpcTransportResult rc = ipc_server_accept(g_server, &client);

        if (rc == IPC_ERR_TIMEOUT)
        {
            continue;
        }
        if (rc != IPC_OK)
        {
            if (is_running())
            {
                Sleep(100);
            }
            continue;
        }

        if (!localapi_security_allow_connection(&g_secCtx))
        {
            ipc_transport_close(client);
            continue;
        }

        g_secCtx.currentClientCount++;
        handle_client(client);
    }

    return 0;
}
#endif

bool localapi_init(const LocalApiConfig* config)
{
    if (is_running())
    {
        return false;
    }

    const char* endpoint = (config != NULL && config->endpoint != NULL)
        ? config->endpoint
        : ipc_get_default_endpoint();

    int maxConns = (config != NULL && config->maxConnections > 0) ? config->maxConnections : 5;
    int maxReqPerSec = (config != NULL && config->maxRequestsPerSecond > 0) ? config->maxRequestsPerSecond : 10;
    int timeoutMs = (config != NULL && config->timeoutMs > 0) ? config->timeoutMs : 5000;

    // Initialize security context
    if (!localapi_security_init(&g_secCtx, maxReqPerSec, maxConns))
    {
        return false;
    }

    // Create IPC server
    IpcServerConfig srvConfig = {
        .endpoint = endpoint,
        .maxConnections = maxConns,
        .recvTimeoutMs = timeoutMs,
        .sendTimeoutMs = timeoutMs,
    };

    IpcTransportResult rc = ipc_server_create(&srvConfig, &g_server);
    if (rc != IPC_OK)
    {
        localapi_security_uninit(&g_secCtx);
        return false;
    }

    set_running(true);

    // Start server thread
#ifndef _WIN32
    if (pthread_create(&g_serverThread, NULL, server_thread_proc, NULL) != 0)
    {
        set_running(false);
        ipc_transport_close(g_server);
        g_server = NULL;
        localapi_security_uninit(&g_secCtx);
        return false;
    }
#else
    g_serverThread = CreateThread(NULL, 0, server_thread_proc, NULL, 0, NULL);
    if (g_serverThread == NULL)
    {
        set_running(false);
        ipc_transport_close(g_server);
        g_server = NULL;
        localapi_security_uninit(&g_secCtx);
        return false;
    }
#endif

    return true;
}

bool localapi_uninit(void)
{
    if (!is_running())
    {
        return false;
    }

    set_running(false);

#ifndef _WIN32
    pthread_join(g_serverThread, NULL);
    memset(&g_serverThread, 0, sizeof(g_serverThread));
#else
    WaitForSingleObject(g_serverThread, 10000);
    CloseHandle(g_serverThread);
    g_serverThread = NULL;
#endif

    ipc_transport_close(g_server);
    g_server = NULL;

    localapi_security_uninit(&g_secCtx);

    return true;
}

bool localapi_is_running(void)
{
    return is_running();
}

// Weak symbol default — agent must override with real status provider
#ifndef _WIN32
__attribute__((weak))
#endif
int localapi_get_current_status(void)
{
    return 9; // ADUC_ServiceStatus_Idle
}
