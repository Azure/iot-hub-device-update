/**
 * @file aducsdk.c
 * @brief Implementation of the ADU SDK API functions
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/aducsdk.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Minimal API protocol definitions (copied from apiproto.h to avoid circular dependencies)
#define PIPE_BUF 4096 // could properly use fcntl and F_GETPIPE_SZ and malloc, but this is simpler
#define MAX_BUF_LEN (PIPE_BUF - 3 * sizeof(uint16_t))

#define ApiRequestType_NONE 0x00
#define ApiRequestType_GETSTATE 0x01

typedef struct __attribute__((packed)) tagApiWireRequestMsg
{
    uint16_t ver;
    uint16_t type;
    uint16_t len;
    char data[MAX_BUF_LEN];
} ApiWireRequestMsg;

typedef struct tagApiWireResponseMsg
{
    uint16_t code;
    uint16_t ret_val;
} ApiWireResponseMsg;

#ifndef ADUC_API_DEFAULT_FIFO_PATH
#    define ADUC_API_DEFAULT_FIFO_PATH "/var/lib/adu/api/apireq.fifo"
#endif

#ifndef ADUC_DATA_FOLDER
#    define ADUC_DATA_FOLDER "/var/lib/adu"
#endif

#ifndef ADUC_SDK_REQUEST_FIFO_TIMEOUT_SECS
#    define ADUC_SDK_REQUEST_FIFO_TIMEOUT_SECS 10
#endif

#define FIFO_FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) // rw-rw----
#define RESPONSE_FIFO_NAME_LEN 32
static const char ALPHANUMERIC_CHARS[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static void get_rnd_suffix(char* str, size_t length)
{
    static bool seeded = false;
    if (!seeded)
    {
        srand((unsigned int)time(NULL) ^ (unsigned int)getpid());
        seeded = true;
    }

    const char* charset = ALPHANUMERIC_CHARS;

    for (size_t i = 0; i < length; ++i)
    {
        str[i] = charset[rand() % (int)sizeof(ALPHANUMERIC_CHARS)];
    }
    str[length] = '\0';
}

/**
 * @brief Check if a file exists and has the correct security properties for a FIFO
 * @param fifoPath Path to the FIFO file to check
 * @return true if the file exists and has correct security, false otherwise
 */
static bool verify_fifo_security(const char* fifoPath)
{
    if (fifoPath == NULL || fifoPath[0] == '\0')
    {
        return false;
    }

    struct stat st = { 0 };
    if (stat(fifoPath, &st) < 0)
    {
        return false;
    }

    if (!S_ISFIFO(st.st_mode))
    {
        return false;
    }

    if (st.st_uid != geteuid())
    {
        return false;
    }

    if (st.st_gid != getegid())
    {
        return false;
    }

    if ((st.st_mode & FIFO_FILE_MODE) != FIFO_FILE_MODE)
    {
        return false;
    }

    return true;
}

ADUC_ServiceStatus GetAduServiceStatus(void)
{
    const char* reqFifoPath = ADUC_API_DEFAULT_FIFO_PATH;

    if (!verify_fifo_security(reqFifoPath))
    {
        return ADUC_ServiceStatus_ERROR_AgentServiceNotRunning;
    }

    if (access(reqFifoPath, W_OK) != 0)
    {
        return ADUC_ServiceStatus_ERROR_AgentServicePermission;
    }

    char randomSuffix[13]; // 12 chars + null terminator
    get_rnd_suffix(randomSuffix, 12);

    char respFifoPath[512];
    snprintf(respFifoPath, sizeof(respFifoPath), ADUC_DATA_FOLDER "/api/resp_%s.fifo", randomSuffix);

    if (mkfifo(respFifoPath, FIFO_FILE_MODE) < 0)
    {
        if (errno != EEXIST)
        {
            return ADUC_ServiceStatus_ERROR_AgentServiceInternal;
        }
    }

    ADUC_ServiceStatus result = ADUC_ServiceStatus_ERROR_AgentServiceInternal;
    bool cleanup_fifo = true;

    int reqFifo = open(reqFifoPath, O_WRONLY | O_NONBLOCK);
    if (reqFifo == -1)
    {
        if (errno == ENXIO)
        {
            result = ADUC_ServiceStatus_ERROR_AgentServiceNotRunning;
        }
        else if (errno == EACCES)
        {
            result = ADUC_ServiceStatus_ERROR_AgentServicePermission;
        }
        else
        {
            result = ADUC_ServiceStatus_ERROR_AgentServiceBrokenPipe;
        }
        goto cleanup;
    }

    size_t respPathLen = strlen(respFifoPath);
    if (respPathLen >= MAX_BUF_LEN)
    {
        result = ADUC_ServiceStatus_ERROR_AgentServiceInternal;
        close(reqFifo);
        goto cleanup;
    }

    // Build message manually to avoid any struct padding issues
    uint16_t header[3];
    header[0] = htons(1); // version
    header[1] = htons(ApiRequestType_GETSTATE); // type
    header[2] = htons((uint16_t)respPathLen); // length

    // Write header first
    ssize_t n = write(reqFifo, header, sizeof(header));
    if (n == sizeof(header) && respPathLen > 0)
    {
        // Write data payload
        ssize_t data_written = write(reqFifo, respFifoPath, respPathLen);
        if (data_written != (ssize_t)respPathLen)
        {
            n = -1; // Indicate failure
        }
        else
        {
            n += data_written; // Total bytes written
        }
    }

    size_t msg_size = sizeof(header) + respPathLen;
    close(reqFifo);

    if (n != (ssize_t)msg_size)
    {
        result = ADUC_ServiceStatus_ERROR_AgentServiceBrokenPipe;
        goto cleanup;
    }

    int respFifo = open(respFifoPath, O_RDONLY | O_NONBLOCK);
    if (respFifo == -1)
    {
        result = ADUC_ServiceStatus_ERROR_AgentServiceInternal;
        goto cleanup;
    }

    // Wait for response with timeout
    fd_set read_fds;
    struct timeval timeout;
    FD_ZERO(&read_fds);
    FD_SET(respFifo, &read_fds);
    timeout.tv_sec = ADUC_SDK_REQUEST_FIFO_TIMEOUT_SECS;
    timeout.tv_usec = 0;

    int select_result = select(respFifo + 1, &read_fds, NULL, NULL, &timeout);
    if (select_result <= 0)
    {
        close(respFifo);
        result = (select_result == 0) ? ADUC_ServiceStatus_ERROR_AgentServiceTimeout
                                      : ADUC_ServiceStatus_ERROR_AgentServiceInternal;
        goto cleanup;
    }

    ApiWireResponseMsg resp = { 0 };
    n = read(respFifo, &resp, sizeof(resp));
    close(respFifo);

    if (n != sizeof(resp))
    {
        result = ADUC_ServiceStatus_ERROR_AgentServiceBrokenPipe;
        goto cleanup;
    }

    const uint16_t code = ntohs(resp.code);
    const uint16_t ret_val = ntohs(resp.ret_val);

    if (code != ApiRequestType_GETSTATE)
    {
        result = ADUC_ServiceStatus_ERROR_AgentServiceInternal;
        goto cleanup;
    }

    result = (ADUC_ServiceStatus)ret_val;
    cleanup_fifo = true;

cleanup:
    if (cleanup_fifo)
    {
        unlink(respFifoPath);
    }

    return result;
}

const char* ADUC_ServiceStatusToString(ADUC_ServiceStatus status)
{
    switch (status)
    {
    case ADUC_ServiceStatus_None:
        return "None";
    case ADUC_ServiceStatus_Initializing:
        return "Initializing";
    case ADUC_ServiceStatus_Downloading:
        return "Downloading";
    case ADUC_ServiceStatus_Installing:
        return "Installing";
    case ADUC_ServiceStatus_Rebooting:
        return "Rebooting";
    case ADUC_ServiceStatus_Reporting:
        return "Reporting";
    case ADUC_ServiceStatus_Paused:
        return "Paused";
    case ADUC_ServiceStatus_Idle:
        return "Idle";
    case ADUC_ServiceStatus_ERROR_UnsupportedApiVersion:
        return "Error: Unsupported API Version";
    case ADUC_ServiceStatus_ERROR_AgentServiceNotRunning:
        return "Error: Agent Service Not Running";
    case ADUC_ServiceStatus_ERROR_AgentServiceBrokenPipe:
        return "Error: Agent Service Broken Pipe";
    case ADUC_ServiceStatus_ERROR_AgentServicePermission:
        return "Error: Agent Service Permission";
    case ADUC_ServiceStatus_ERROR_AgentServiceTimeout:
        return "Error: Agent Service Timeout";
    case ADUC_ServiceStatus_ERROR_AgentServiceInternal:
        return "Error: Agent Service Internal";
    case ADUC_ServiceStatus_ERROR_Unknown:
        return "Error: Unknown";
    default:
        return "Unknown Status";
    }
}
