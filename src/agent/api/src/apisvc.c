/**
 * @file apisvc.c
 * @brief The service implementation for handling incoming cross-process API calls.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/apisvc.h"

#include "aduc/aducsdk.h"
#include "aduc/apiproto.h"
#include "aduc/logging.h"
#include "aduc/result.h"
#include "aduc/viewstatemgr.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/stat.h>
#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <unistd.h>

#define g_AducApiVersion 1
#define FIFO_FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) // rw-rw----

static const unsigned OnErrorDelayMs = 250;

pthread_t g_api_svc_thread;
bool g_api_svc_thread_running = false;

// fwd-decl
static void* aduc_apisvc_thread_proc (void*);

/////////////////////////////////////////////////
// BEGIN: Public API
//
bool init_api_svc()
{
    if (g_api_svc_thread_running)
    {
        return false;
    }

    Log_Info("Initializing API Service thread");

    int ret = pthread_create(&g_api_svc_thread, NULL, aduc_apisvc_thread_proc, NULL);
    if (ret != 0)
    {
        Log_Error("Failed to create command listener thread: %d", ret);
        return false;
    }

    g_api_svc_thread_running = true;
    return true;
}

void uninit_api_svc()
{
    Log_Info("Uninitializing api service thread");
    g_api_svc_thread_running = false;
}
//
// END: Public API
/////////////////////////////////////////////////

static bool verify_fifo_security(const char* fifoPath)
{
    struct stat st = {0};
    if (stat(fifoPath, &st) < 0)
    {
        Log_Error("Failed to stat fifo '%s': %d", fifoPath, errno);
        return false;
    }

    if (!S_ISFIFO(st.st_mode))
    {
        Log_Error("File '%s' is not a fifo", fifoPath);
        return false;
    }

    if (st.st_uid != geteuid())
    {
        Log_Error("File '%s' is not owned by %d user", fifoPath, geteuid());
        return false;
    }

    if (st.st_gid != getegid())
    {
        Log_Error("File '%s' is not owned by %d group", fifoPath, getegid());
        return false;
    }

    if ((st.st_mode & FIFO_FILE_MODE) != FIFO_FILE_MODE)
    {
        Log_Error("File '%s' does not have 0%o permissions", fifoPath, FIFO_FILE_MODE);
        return false;
    }
    return true;
}

static int create_and_verify_request_fifo()
{
    if(mkfifo(ADUC_API_FIFO_PATH, FIFO_FILE_MODE) < 0 && errno != EEXIST)
    {
        Log_Error("Failed to create fifo '%s': %d", ADUC_API_FIFO_PATH, errno);
        return -1;
    }

    if (!verify_fifo_security(ADUC_API_FIFO_PATH))
    {
        Log_Error("Failed to verify fifo '%s' security", ADUC_API_FIFO_PATH);
        return -2;
    }

    return 0;
}

static void* aduc_apisvc_thread_proc (void*)
{
    if (create_and_verify_request_fifo() < 0)
    {
        Log_Error("Failed to create/verify request fifo");
        return NULL;
    }

    int rdfifo = open(ADUC_API_FIFO_PATH, O_RDONLY, 0);
    (void)open(ADUC_API_FIFO_PATH, O_WRONLY, 0); // NOTE(jewelden) - never used, avoids EOF on read side

    while (!g_api_svc_thread_running)
    {
        ssize_t n;
        ApiWireRequestMsg msg = {0};
        if ((n = msg_recv(rdfifo, &msg)) <= 0)
        {
            Log_Error("msg_recv failed: %zd", n);
            sleep(OnErrorDelayMs);
            continue;
        }
        msg.data[n] = '\0';

        if (!verify_fifo_security(msg.data))
        {
            Log_Error("security verification failed for response fifo '%s'", msg.data);
            sleep(OnErrorDelayMs);
            continue;
        }

        int writefifo = open(msg.data, O_WRONLY, 0);
        if (writefifo < 0)
        {
            Log_Error("Failed to open response fifo '%s': %d", msg.data, errno);
            sleep(OnErrorDelayMs);
            continue;
        }

        ApiWireResponseMsg resp = {0};
        if (ApiRequestType_NONE == msg.type)
        {
            resp.code = 1;
            Log_Error("bad msg type: %zd", n);
            sleep(OnErrorDelayMs);
            continue;
        }
        else if (ApiRequestType_GETSTATE == msg.type)
        {
            Log_Info("Received GETSTATE request");
            ADUC_ServiceStatus status = ADUC_ServiceStatus_None;
            ADUC_Result res = viewstatemgr_svcstatus_get(g_viewstatemgr_handle, &status);
            if (IsAducResultCodeFailure(res.ResultCode))
            {
                Log_Error("viewstatemgr_svcstatus_get failed, erc: %d", res.ExtendedResultCode);
                status = ADUC_ServiceStatus_ERROR_Unknown;
            }
            resp.ret_val = htonl(status);
            Log_Info("GETSTATE returning %d", status);

            ssize_t sent = msg_send(writefifo, &resp);
            if (sent < 0)
            {
                Log_Error("msg_send failed, sent %zd bytes", sent);
            }
            else
            {
                Log_Info("Sent GETSTATE response status: %d", status);
            }
        }
    }
    return NULL;
}
