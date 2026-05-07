/**
 * @file ipc_transport_linux.c
 * @brief Unix domain socket implementation of the IPC transport interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef _WIN32

#define _GNU_SOURCE

#include "aduc/ipc_transport.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define DEFAULT_MAX_CONNECTIONS 5
#define DEFAULT_TIMEOUT_MS 5000
#define SOCKET_PATH_MAX (sizeof(((struct sockaddr_un*)0)->sun_path) - 1)

struct IpcTransport
{
    int fd;
    bool isServer;
    char endpoint[108]; // sun_path max
    int recvTimeoutMs;
    int sendTimeoutMs;
};

static IpcTransport* alloc_transport(void)
{
    IpcTransport* t = (IpcTransport*)calloc(1, sizeof(IpcTransport));
    if (t != NULL)
    {
        t->fd = -1;
        t->recvTimeoutMs = DEFAULT_TIMEOUT_MS;
        t->sendTimeoutMs = DEFAULT_TIMEOUT_MS;
    }
    return t;
}

static int set_socket_timeout(int fd, int timeoutMs, int optname)
{
    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    return setsockopt(fd, SOL_SOCKET, optname, &tv, sizeof(tv));
}

static int make_parent_dirs(const char* path)
{
    char tmp[256];
    size_t len = strlen(path);
    if (len >= sizeof(tmp))
    {
        return -1;
    }

    memcpy(tmp, path, len + 1);

    // Find last slash and null-terminate there
    char* last_slash = strrchr(tmp, '/');
    if (last_slash == NULL || last_slash == tmp)
    {
        return 0; // no parent or root
    }
    *last_slash = '\0';

    // mkdir -p equivalent
    for (char* p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = '\0';
            mkdir(tmp, 0750);
            *p = '/';
        }
    }
    mkdir(tmp, 0750);
    return 0;
}

IpcTransportResult ipc_server_create(const IpcServerConfig* config, IpcTransport** server)
{
    if (config == NULL || server == NULL || config->endpoint == NULL)
    {
        return IPC_ERR_INVALID_ARG;
    }

    if (strlen(config->endpoint) > SOCKET_PATH_MAX)
    {
        return IPC_ERR_INVALID_ARG;
    }

    IpcTransport* t = alloc_transport();
    if (t == NULL)
    {
        return IPC_ERR_OUT_OF_MEMORY;
    }

    t->isServer = true;
    strncpy(t->endpoint, config->endpoint, sizeof(t->endpoint) - 1);

    if (config->recvTimeoutMs > 0)
    {
        t->recvTimeoutMs = config->recvTimeoutMs;
    }
    if (config->sendTimeoutMs > 0)
    {
        t->sendTimeoutMs = config->sendTimeoutMs;
    }

    // Ensure parent directory exists
    make_parent_dirs(config->endpoint);

    // Remove stale socket file
    unlink(config->endpoint);

    t->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (t->fd < 0)
    {
        free(t);
        return IPC_ERR_PLATFORM;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, config->endpoint, sizeof(addr.sun_path) - 1);

    if (bind(t->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        close(t->fd);
        free(t);
        return IPC_ERR_BIND_FAILED;
    }

    // Set socket file permissions: rw-rw---- (owner + group only)
    chmod(config->endpoint, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

    int backlog = (config->maxConnections > 0) ? config->maxConnections : DEFAULT_MAX_CONNECTIONS;
    if (listen(t->fd, backlog) < 0)
    {
        close(t->fd);
        unlink(config->endpoint);
        free(t);
        return IPC_ERR_LISTEN_FAILED;
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

    // Use poll for accept timeout
    struct pollfd pfd = { .fd = server->fd, .events = POLLIN };
    int pollResult = poll(&pfd, 1, server->recvTimeoutMs);

    if (pollResult == 0)
    {
        return IPC_ERR_TIMEOUT;
    }
    if (pollResult < 0)
    {
        return IPC_ERR_PLATFORM;
    }

    int clientFd = accept(server->fd, NULL, NULL);
    if (clientFd < 0)
    {
        return IPC_ERR_ACCEPT_FAILED;
    }

    IpcTransport* ct = alloc_transport();
    if (ct == NULL)
    {
        close(clientFd);
        return IPC_ERR_OUT_OF_MEMORY;
    }

    ct->fd = clientFd;
    ct->isServer = false;
    ct->recvTimeoutMs = server->recvTimeoutMs;
    ct->sendTimeoutMs = server->sendTimeoutMs;

    set_socket_timeout(clientFd, ct->recvTimeoutMs, SO_RCVTIMEO);
    set_socket_timeout(clientFd, ct->sendTimeoutMs, SO_SNDTIMEO);

    *client = ct;
    return IPC_OK;
}

IpcTransportResult ipc_get_peer_credentials(IpcTransport* client, IpcPeerCredentials* creds)
{
    if (client == NULL || creds == NULL)
    {
        return IPC_ERR_INVALID_ARG;
    }

    struct ucred peerCred;
    socklen_t len = sizeof(peerCred);

    if (getsockopt(client->fd, SOL_SOCKET, SO_PEERCRED, &peerCred, &len) < 0)
    {
        return IPC_ERR_PLATFORM;
    }

    creds->uid = (uint32_t)peerCred.uid;
    creds->gid = (uint32_t)peerCred.gid;
    creds->pid = (uint32_t)peerCred.pid;

    return IPC_OK;
}

IpcTransportResult ipc_client_connect(const char* endpoint, int timeoutMs, IpcTransport** transport)
{
    if (endpoint == NULL || transport == NULL)
    {
        return IPC_ERR_INVALID_ARG;
    }

    if (strlen(endpoint) > SOCKET_PATH_MAX)
    {
        return IPC_ERR_INVALID_ARG;
    }

    IpcTransport* t = alloc_transport();
    if (t == NULL)
    {
        return IPC_ERR_OUT_OF_MEMORY;
    }

    if (timeoutMs > 0)
    {
        t->recvTimeoutMs = timeoutMs;
        t->sendTimeoutMs = timeoutMs;
    }

    t->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (t->fd < 0)
    {
        free(t);
        return IPC_ERR_PLATFORM;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, endpoint, sizeof(addr.sun_path) - 1);

    // Set non-blocking for connect timeout
    int flags = fcntl(t->fd, F_GETFL, 0);
    fcntl(t->fd, F_SETFL, flags | O_NONBLOCK);

    int result = connect(t->fd, (struct sockaddr*)&addr, sizeof(addr));
    if (result < 0 && errno != EINPROGRESS)
    {
        if (errno == EACCES)
        {
            close(t->fd);
            free(t);
            return IPC_ERR_PERMISSION_DENIED;
        }
        close(t->fd);
        free(t);
        return IPC_ERR_CONNECT_FAILED;
    }

    if (result < 0)
    {
        // Wait for connect to complete
        struct pollfd pfd = { .fd = t->fd, .events = POLLOUT };
        int pollRes = poll(&pfd, 1, (timeoutMs > 0) ? timeoutMs : DEFAULT_TIMEOUT_MS);
        if (pollRes <= 0)
        {
            close(t->fd);
            free(t);
            return (pollRes == 0) ? IPC_ERR_TIMEOUT : IPC_ERR_PLATFORM;
        }

        int err = 0;
        socklen_t errLen = sizeof(err);
        getsockopt(t->fd, SOL_SOCKET, SO_ERROR, &err, &errLen);
        if (err != 0)
        {
            close(t->fd);
            free(t);
            return IPC_ERR_CONNECT_FAILED;
        }
    }

    // Restore blocking mode
    fcntl(t->fd, F_SETFL, flags);
    set_socket_timeout(t->fd, t->recvTimeoutMs, SO_RCVTIMEO);
    set_socket_timeout(t->fd, t->sendTimeoutMs, SO_SNDTIMEO);

    *transport = t;
    return IPC_OK;
}

IpcTransportResult ipc_send(IpcTransport* transport, const void* buf, size_t len)
{
    if (transport == NULL || buf == NULL || len == 0)
    {
        return IPC_ERR_INVALID_ARG;
    }

    const uint8_t* ptr = (const uint8_t*)buf;
    size_t remaining = len;

    while (remaining > 0)
    {
        ssize_t n = send(transport->fd, ptr, remaining, MSG_NOSIGNAL);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return IPC_ERR_TIMEOUT;
            }
            if (errno == EPIPE || errno == ECONNRESET)
            {
                return IPC_ERR_PEER_DISCONNECTED;
            }
            return IPC_ERR_SEND_FAILED;
        }
        ptr += n;
        remaining -= (size_t)n;
    }

    return IPC_OK;
}

IpcTransportResult ipc_recv(IpcTransport* transport, void* buf, size_t len, size_t* bytesReceived)
{
    if (transport == NULL || buf == NULL || len == 0 || bytesReceived == NULL)
    {
        return IPC_ERR_INVALID_ARG;
    }

    uint8_t* ptr = (uint8_t*)buf;
    size_t totalRead = 0;

    while (totalRead < len)
    {
        ssize_t n = recv(transport->fd, ptr, len - totalRead, 0);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                *bytesReceived = totalRead;
                return (totalRead > 0) ? IPC_OK : IPC_ERR_TIMEOUT;
            }
            *bytesReceived = totalRead;
            return IPC_ERR_RECV_FAILED;
        }
        if (n == 0)
        {
            // Peer disconnected
            *bytesReceived = totalRead;
            return (totalRead > 0) ? IPC_OK : IPC_ERR_PEER_DISCONNECTED;
        }
        ptr += n;
        totalRead += (size_t)n;
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

    if (transport->fd >= 0)
    {
        shutdown(transport->fd, SHUT_RDWR);
        close(transport->fd);
    }

    if (transport->isServer && transport->endpoint[0] != '\0')
    {
        unlink(transport->endpoint);
    }

    free(transport);
}

const char* ipc_get_default_endpoint(void)
{
    return "/var/lib/adu/api/localapi.sock";
}

#endif // !_WIN32
