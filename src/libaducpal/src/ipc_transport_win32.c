/**
 * @file ipc_transport_win32.c
 * @brief Named pipe implementation of the IPC transport interface for Windows.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifdef _WIN32

#include "aduc/ipc_transport.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <aclapi.h>
#include <sddl.h>

#define DEFAULT_MAX_CONNECTIONS 5
#define DEFAULT_TIMEOUT_MS 5000
#define PIPE_BUFFER_SIZE 4096
#define MAX_PIPE_NAME_LEN 256

struct IpcTransport
{
    HANDLE hPipe;
    bool isServer;
    char endpoint[MAX_PIPE_NAME_LEN];
    int recvTimeoutMs;
    int sendTimeoutMs;
    OVERLAPPED overlapped;
    HANDLE hEvent;
};

static IpcTransport* alloc_transport(void)
{
    IpcTransport* t = (IpcTransport*)calloc(1, sizeof(IpcTransport));
    if (t != NULL)
    {
        t->hPipe = INVALID_HANDLE_VALUE;
        t->hEvent = NULL;
        t->recvTimeoutMs = DEFAULT_TIMEOUT_MS;
        t->sendTimeoutMs = DEFAULT_TIMEOUT_MS;
    }
    return t;
}

/**
 * @brief Create a security descriptor that allows access to SYSTEM, Administrators, and the adu group.
 *
 * SDDL string breakdown:
 *   D: - DACL
 *   (A;;GA;;;SY) - Allow Generic All to SYSTEM
 *   (A;;GA;;;BA) - Allow Generic All to Administrators (adu-admin equivalent)
 *   (A;;GRGW;;;AU) - Allow Generic Read/Write to Authenticated Users (adu group equivalent)
 */
static PSECURITY_DESCRIPTOR create_pipe_security_descriptor(void)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
    const char* sddl = "D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GRGW;;;AU)";

    if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(
            sddl, SDDL_REVISION_1, &pSD, NULL))
    {
        return NULL;
    }
    return pSD;
}

IpcTransportResult ipc_server_create(const IpcServerConfig* config, IpcTransport** server)
{
    if (config == NULL || server == NULL || config->endpoint == NULL)
    {
        return IPC_ERR_INVALID_ARG;
    }

    if (strlen(config->endpoint) >= MAX_PIPE_NAME_LEN)
    {
        return IPC_ERR_INVALID_ARG;
    }

    IpcTransport* t = alloc_transport();
    if (t == NULL)
    {
        return IPC_ERR_OUT_OF_MEMORY;
    }

    t->isServer = true;
    strncpy_s(t->endpoint, sizeof(t->endpoint), config->endpoint, _TRUNCATE);

    if (config->recvTimeoutMs > 0)
    {
        t->recvTimeoutMs = config->recvTimeoutMs;
    }
    if (config->sendTimeoutMs > 0)
    {
        t->sendTimeoutMs = config->sendTimeoutMs;
    }

    // Create event for overlapped I/O
    t->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (t->hEvent == NULL)
    {
        free(t);
        return IPC_ERR_PLATFORM;
    }

    // Create security descriptor
    PSECURITY_DESCRIPTOR pSD = create_pipe_security_descriptor();
    SECURITY_ATTRIBUTES sa = { 0 };
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;

    int maxInstances = (config->maxConnections > 0) ? config->maxConnections : DEFAULT_MAX_CONNECTIONS;

    t->hPipe = CreateNamedPipeA(
        config->endpoint,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
        (DWORD)maxInstances,
        PIPE_BUFFER_SIZE,
        PIPE_BUFFER_SIZE,
        (DWORD)t->recvTimeoutMs,
        pSD != NULL ? &sa : NULL);

    if (pSD != NULL)
    {
        LocalFree(pSD);
    }

    if (t->hPipe == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        CloseHandle(t->hEvent);
        free(t);
        return (err == ERROR_ACCESS_DENIED) ? IPC_ERR_PERMISSION_DENIED : IPC_ERR_BIND_FAILED;
    }

    *server = t;
    return IPC_OK;
}

IpcTransportResult ipc_server_accept(IpcTransport* server, IpcTransport** client)
{
    if (server == NULL || client == NULL || !server->isServer)
    {
        return IPC_ERR_INVALID_ARG;
    }

    // Use overlapped ConnectNamedPipe for timeout support
    OVERLAPPED ov = { 0 };
    ov.hEvent = server->hEvent;
    ResetEvent(ov.hEvent);

    BOOL connected = ConnectNamedPipe(server->hPipe, &ov);
    if (!connected)
    {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING)
        {
            DWORD waitResult = WaitForSingleObject(ov.hEvent, (DWORD)server->recvTimeoutMs);
            if (waitResult == WAIT_TIMEOUT)
            {
                CancelIo(server->hPipe);
                return IPC_ERR_TIMEOUT;
            }
            if (waitResult != WAIT_OBJECT_0)
            {
                CancelIo(server->hPipe);
                return IPC_ERR_PLATFORM;
            }
        }
        else if (err != ERROR_PIPE_CONNECTED)
        {
            return IPC_ERR_ACCEPT_FAILED;
        }
    }

    // Client is connected. Create a transport for this connection.
    // On Windows, the same pipe handle is used for the accepted connection.
    // We'll transfer ownership and create a new pipe instance for the server.
    IpcTransport* ct = alloc_transport();
    if (ct == NULL)
    {
        DisconnectNamedPipe(server->hPipe);
        return IPC_ERR_OUT_OF_MEMORY;
    }

    ct->hPipe = server->hPipe;
    ct->isServer = false;
    ct->recvTimeoutMs = server->recvTimeoutMs;
    ct->sendTimeoutMs = server->sendTimeoutMs;

    // Create a new pipe instance for the server to accept next connection
    PSECURITY_DESCRIPTOR pSD = create_pipe_security_descriptor();
    SECURITY_ATTRIBUTES sa = { 0 };
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;

    server->hPipe = CreateNamedPipeA(
        server->endpoint,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
        PIPE_UNLIMITED_INSTANCES,
        PIPE_BUFFER_SIZE,
        PIPE_BUFFER_SIZE,
        (DWORD)server->recvTimeoutMs,
        pSD != NULL ? &sa : NULL);

    if (pSD != NULL)
    {
        LocalFree(pSD);
    }

    *client = ct;
    return IPC_OK;
}

IpcTransportResult ipc_get_peer_credentials(IpcTransport* client, IpcPeerCredentials* creds)
{
    if (client == NULL || creds == NULL)
    {
        return IPC_ERR_INVALID_ARG;
    }

    memset(creds, 0, sizeof(*creds));

    // Get client process ID
    ULONG clientPid = 0;
    if (!GetNamedPipeClientProcessId(client->hPipe, &clientPid))
    {
        return IPC_ERR_PLATFORM;
    }
    creds->processId = (uint32_t)clientPid;

    // Check if client is elevated by impersonating and checking token
    if (ImpersonateNamedPipeClient(client->hPipe))
    {
        HANDLE token = NULL;
        if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &token))
        {
            TOKEN_ELEVATION elevation = { 0 };
            DWORD size = 0;
            if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size))
            {
                creds->isElevated = (elevation.TokenIsElevated != 0);
            }
            CloseHandle(token);
        }
        RevertToSelf();
    }

    return IPC_OK;
}

IpcTransportResult ipc_client_connect(const char* endpoint, int timeoutMs, IpcTransport** transport)
{
    if (endpoint == NULL || transport == NULL)
    {
        return IPC_ERR_INVALID_ARG;
    }

    int timeout = (timeoutMs > 0) ? timeoutMs : DEFAULT_TIMEOUT_MS;

    // Wait for pipe to become available
    if (!WaitNamedPipeA(endpoint, (DWORD)timeout))
    {
        DWORD err = GetLastError();
        if (err == ERROR_SEM_TIMEOUT)
        {
            return IPC_ERR_TIMEOUT;
        }
        return IPC_ERR_CONNECT_FAILED;
    }

    HANDLE hPipe = CreateFileA(
        endpoint,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED)
        {
            return IPC_ERR_PERMISSION_DENIED;
        }
        return IPC_ERR_CONNECT_FAILED;
    }

    // Set pipe to byte-read mode
    DWORD mode = PIPE_READMODE_BYTE;
    SetNamedPipeHandleState(hPipe, &mode, NULL, NULL);

    IpcTransport* t = alloc_transport();
    if (t == NULL)
    {
        CloseHandle(hPipe);
        return IPC_ERR_OUT_OF_MEMORY;
    }

    t->hPipe = hPipe;
    t->recvTimeoutMs = timeout;
    t->sendTimeoutMs = timeout;

    *transport = t;
    return IPC_OK;
}

IpcTransportResult ipc_send(IpcTransport* transport, const void* buf, size_t len)
{
    if (transport == NULL || buf == NULL || len == 0)
    {
        return IPC_ERR_INVALID_ARG;
    }

    const BYTE* ptr = (const BYTE*)buf;
    size_t remaining = len;

    while (remaining > 0)
    {
        DWORD written = 0;
        if (!WriteFile(transport->hPipe, ptr, (DWORD)remaining, &written, NULL))
        {
            DWORD err = GetLastError();
            if (err == ERROR_BROKEN_PIPE || err == ERROR_NO_DATA)
            {
                return IPC_ERR_PEER_DISCONNECTED;
            }
            return IPC_ERR_SEND_FAILED;
        }
        ptr += written;
        remaining -= written;
    }

    return IPC_OK;
}

IpcTransportResult ipc_recv(IpcTransport* transport, void* buf, size_t len, size_t* bytesReceived)
{
    if (transport == NULL || buf == NULL || len == 0 || bytesReceived == NULL)
    {
        return IPC_ERR_INVALID_ARG;
    }

    BYTE* ptr = (BYTE*)buf;
    size_t totalRead = 0;

    while (totalRead < len)
    {
        DWORD read = 0;
        if (!ReadFile(transport->hPipe, ptr, (DWORD)(len - totalRead), &read, NULL))
        {
            DWORD err = GetLastError();
            if (err == ERROR_BROKEN_PIPE || err == ERROR_PIPE_NOT_CONNECTED)
            {
                *bytesReceived = totalRead;
                return (totalRead > 0) ? IPC_OK : IPC_ERR_PEER_DISCONNECTED;
            }
            *bytesReceived = totalRead;
            return IPC_ERR_RECV_FAILED;
        }
        if (read == 0)
        {
            *bytesReceived = totalRead;
            return (totalRead > 0) ? IPC_OK : IPC_ERR_PEER_DISCONNECTED;
        }
        ptr += read;
        totalRead += read;
    }

    *bytesReceived = totalRead;
    return IPC_OK;
}

void ipc_transport_close(IpcTransport* transport)
{
    if (transport == NULL)
    {
        return;
    }

    if (transport->hPipe != INVALID_HANDLE_VALUE)
    {
        if (!transport->isServer)
        {
            // For client connections, disconnect before close
            FlushFileBuffers(transport->hPipe);
            DisconnectNamedPipe(transport->hPipe);
        }
        CloseHandle(transport->hPipe);
    }

    if (transport->hEvent != NULL)
    {
        CloseHandle(transport->hEvent);
    }

    free(transport);
}

const char* ipc_get_default_endpoint(void)
{
    return "\\\\.\\pipe\\adu-agent-localapi";
}

#endif // _WIN32
