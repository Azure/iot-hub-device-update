/**
 * @file process_ipc_report.c
 * @brief Child-to-parent IPC reporting over Unix domain socket.
 */

#include "aduc/process_ipc_report.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

// Wire format: [type:u8][len:u16 big-endian][payload]
#define IPC_HEADER_SIZE 3
#define IPC_CANCEL_MSG_TYPE 0x80

#ifdef _WIN32
/*
 * Windows stub implementations — IPC reporting uses Unix domain sockets
 * which require different handling on Windows. These stubs compile cleanly.
 */

ADUC_Result2 ADUC_IpcClient_Connect(const char* socketPath, ADUC_IpcClientHandle* outHandle)
{
    (void)socketPath;
    (void)outHandle;
    return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 1);
}

ADUC_Result2 ADUC_IpcClient_ReportProgress(ADUC_IpcClientHandle handle, const ADUC_IpcProgressMsg* msg)
{
    (void)handle;
    (void)msg;
    return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 1);
}

ADUC_Result2 ADUC_IpcClient_ReportResult(ADUC_IpcClientHandle handle, const ADUC_IpcResultMsg* msg)
{
    (void)handle;
    (void)msg;
    return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 1);
}

bool ADUC_IpcClient_IsCancelRequested(ADUC_IpcClientHandle handle)
{
    (void)handle;
    return false;
}

ADUC_Result2 ADUC_IpcClient_AckCancel(ADUC_IpcClientHandle handle)
{
    (void)handle;
    return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_IO, 1);
}

void ADUC_IpcClient_Disconnect(ADUC_IpcClientHandle handle)
{
    (void)handle;
}

#else /* !_WIN32 */

struct ADUC_IpcClient
{
    int sockfd;
    bool connected;
};

static bool send_all(int fd, const void* buf, size_t len)
{
    const uint8_t* ptr = (const uint8_t*)buf;
    size_t remaining = len;

    while (remaining > 0)
    {
        ssize_t sent = send(fd, ptr, remaining, MSG_NOSIGNAL);
        if (sent <= 0)
        {
            return false;
        }
        ptr += sent;
        remaining -= (size_t)sent;
    }
    return true;
}

static ADUC_Result2 send_message(ADUC_IpcClientHandle handle, uint8_t msgType, const void* payload, uint16_t payloadLen)
{
    if (handle == NULL || !handle->connected)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1);
    }

    uint8_t header[IPC_HEADER_SIZE];
    header[0] = msgType;
    header[1] = (uint8_t)(payloadLen >> 8);
    header[2] = (uint8_t)(payloadLen & 0xFF);

    if (!send_all(handle->sockfd, header, IPC_HEADER_SIZE))
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 2);
    }

    if (payloadLen > 0 && payload != NULL)
    {
        if (!send_all(handle->sockfd, payload, payloadLen))
        {
            return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 3);
        }
    }

    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_IpcClient_Connect(const char* socketPath, ADUC_IpcClientHandle* outHandle)
{
    if (socketPath == NULL || outHandle == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1);
    }

    *outHandle = NULL;

    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 2);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    size_t pathLen = strlen(socketPath);
    if (pathLen >= sizeof(addr.sun_path))
    {
        close(sockfd);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 3);
    }
    memcpy(addr.sun_path, socketPath, pathLen + 1);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        close(sockfd);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 4);
    }

    struct ADUC_IpcClient* client = (struct ADUC_IpcClient*)calloc(1, sizeof(struct ADUC_IpcClient));
    if (client == NULL)
    {
        close(sockfd);
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 1);
    }

    client->sockfd = sockfd;
    client->connected = true;
    *outHandle = client;

    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_IpcClient_ReportProgress(ADUC_IpcClientHandle handle, const ADUC_IpcProgressMsg* msg)
{
    if (msg == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1);
    }

    return send_message(handle, (uint8_t)ADUC_IPC_MSG_PROGRESS, msg, (uint16_t)sizeof(ADUC_IpcProgressMsg));
}

ADUC_Result2 ADUC_IpcClient_ReportResult(ADUC_IpcClientHandle handle, const ADUC_IpcResultMsg* msg)
{
    if (msg == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_IO, 1);
    }

    return send_message(handle, (uint8_t)ADUC_IPC_MSG_RESULT, msg, (uint16_t)sizeof(ADUC_IpcResultMsg));
}

bool ADUC_IpcClient_IsCancelRequested(ADUC_IpcClientHandle handle)
{
    if (handle == NULL || !handle->connected)
    {
        return false;
    }

    struct pollfd pfd;
    pfd.fd = handle->sockfd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    int ret = poll(&pfd, 1, 0);
    if (ret > 0 && (pfd.revents & POLLIN))
    {
        // Peek at incoming data to check for cancel message
        uint8_t header[IPC_HEADER_SIZE];
        ssize_t n = recv(handle->sockfd, header, IPC_HEADER_SIZE, MSG_PEEK);
        if (n == IPC_HEADER_SIZE && header[0] == IPC_CANCEL_MSG_TYPE)
        {
            return true;
        }
    }

    return false;
}

ADUC_Result2 ADUC_IpcClient_AckCancel(ADUC_IpcClientHandle handle)
{
    return send_message(handle, (uint8_t)ADUC_IPC_MSG_CANCEL_ACK, NULL, 0);
}

void ADUC_IpcClient_Disconnect(ADUC_IpcClientHandle handle)
{
    if (handle == NULL)
    {
        return;
    }

    if (handle->connected && handle->sockfd >= 0)
    {
        close(handle->sockfd);
    }

    free(handle);
}

#endif /* !_WIN32 */