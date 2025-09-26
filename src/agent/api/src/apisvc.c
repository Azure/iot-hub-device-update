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
#include "aduc/config_utils.h"
#include "aduc/logging.h"
#include "aduc/result.h"
#include "aduc/viewstatemgr.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <unistd.h>

#define g_AducApiVersion 1
#define FIFO_FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) // rw-rw----

static const unsigned OnErrorDelayMicrosecs = 2500;

pthread_t g_api_svc_thread = { 0 };
bool g_api_svc_thread_running = false;

typedef struct tagFifoThreadRetVal
{
    ADUC_Result result;
} FifoThreadRetVal;

// fwd-decl
static void* aduc_apisvc_thread_proc(void*);

// /**
//  * @brief Get the API request FIFO path from configuration, falling back to default if not configured
//  * @return const char* The FIFO path to use
//  */
// static const char* get_api_request_fifo_path()
// {
//     const char* path = ADUC_API_DEFAULT_FIFO_PATH;
//     const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
//     if (config != NULL)
//     {
//         if (config->apiRequestFifoPath != NULL)
//         {
//             path = config->apiRequestFifoPath;
//         }
//         ADUC_ConfigInfo_ReleaseInstance(config);
//     }

//     return path;
// }

/////////////////////////////////////////////////
// BEGIN: Public API
//
bool init_api_svc(const char* fifoPath)
{
    if (g_api_svc_thread_running)
    {
        return false;
    }

    Log_Info("Initializing API Service thread");

    if (fifoPath == NULL)
    {
        fifoPath = ADUC_API_DEFAULT_FIFO_PATH;
    }

    // NOTE: This thread will remain joinable and will be joined during uninit.
    char* arg = NULL;
    if (0 != mallocAndStrcpy_s(&arg, fifoPath == NULL ? (void*)ADUC_API_DEFAULT_FIFO_PATH : (void*)fifoPath))
    {
        Log_Error("Failed to copy fifo path");
        return false;
    }
    int ret = pthread_create(&g_api_svc_thread, NULL, aduc_apisvc_thread_proc, (void*)arg);
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

    void* threadRet = NULL;
    FifoThreadRetVal* retVal = NULL;

    g_api_svc_thread_running = false;
    int res = pthread_join(g_api_svc_thread, (void**)&retVal);
    if (res != 0)
    {
        Log_Warn("pthread join error: %d\n", res);
    }
    else
    {
        retVal = (FifoThreadRetVal*)threadRet;
    }
    usleep(3 * OnErrorDelayMicrosecs); // give it a moment to exit if it was in a delay loop
    memset(&g_api_svc_thread, 0, sizeof(pthread_t));

    if (retVal == NULL)
    {
        Log_Warn("Fifo thread failed, NULL retval\n");
    }
    else if (IsAducResultCodeFailure((retVal->result).ResultCode))
    {
        Log_Warn("Fifo thread failed with: 0x%08x\n", (retVal->result).ExtendedResultCode);
        free(retVal);
    }
    else
    {
        Log_Info("Fifo thread uninited and had no failure.");
        free(retVal);
    }
}
//
// END: Public API
/////////////////////////////////////////////////

static bool verify_fifo_security(const char* fifoPath)
{
    if (fifoPath == NULL || fifoPath[0] == '\0')
    {
        return false;
    }
    struct stat st = { 0 };
    if (stat(fifoPath, &st) < 0)
    {
        Log_Warn("Failed to stat fifo '%s': %d", fifoPath, errno);
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

static int create_and_verify_request_fifo(const char* fifoPath)
{
    if (mkfifo(fifoPath, FIFO_FILE_MODE) < 0 && errno != EEXIST)
    {
        Log_Error("Failed to create fifo '%s': %d", fifoPath, errno);
        return -1;
    }

    if (!verify_fifo_security(fifoPath))
    {
        Log_Error("Failed to verify fifo '%s' security", fifoPath);
        return -2;
    }

    return 0;
}

static void* aduc_apisvc_thread_proc(void* arg)
{
    char* fifo_path = (char*)arg;

    // The returned value from the implicit call to pthread_exit is from the following heap obj
    // that is freed by the main thread joining it.
    FifoThreadRetVal* retval = (FifoThreadRetVal*)calloc(1, sizeof(FifoThreadRetVal));
    if (retval == NULL)
    {
        return NULL;
    }

    if (create_and_verify_request_fifo(fifo_path) < 0)
    {
        Log_Error("Failed to create/verify request fifo");
        (retval->result).ExtendedResultCode = ADUC_ERC_APISVC_CREATE_FIFO_FAILED;
        return retval;
    }

    int rdfifo = open(fifo_path, O_RDONLY, 0);
    if (rdfifo == -1)
    {
        Log_Error("Failed to open'%s'. errno: %d", fifo_path, errno);
        (retval->result).ExtendedResultCode = ADUC_ERC_APISVC_OPEN_FIFO_FAILED;
        return retval;
    }
    (void)open(fifo_path, O_WRONLY, 0); // NOTE: never used, avoids EOF return on read

    while (g_api_svc_thread_running)
    {
        ssize_t n;
        ApiWireRequestMsg msg = { 0 };
        if ((n = msg_recv(rdfifo, &msg)) < 0)
        {
            if (n == MSGREV_AGAIN)
            {
                // allow it to be interrupted and try again, checking for running flag
                continue;
            }
            Log_Error("msg_recv: %zd", n);
            usleep(OnErrorDelayMicrosecs);
            continue;
        }
        if (n != 0)
        {
            msg.data[n] = '\0';
        }

        if (msg.data == NULL || msg.data[0] == '\0')
        {
            usleep(OnErrorDelayMicrosecs);
            continue;
        }

        if (!verify_fifo_security(msg.data))
        {
            usleep(OnErrorDelayMicrosecs);
            continue;
        }

        int writefifo = open(msg.data, O_WRONLY, 0);
        if (writefifo < 0)
        {
            Log_Error("open resp fifo '%s': %d", msg.data, errno);
            usleep(OnErrorDelayMicrosecs);
            continue;
        }

        ApiWireResponseMsg resp = { 0 };
        if (ApiRequestType_NONE == msg.type)
        {
            Log_Error("bad msg type 0");
            usleep(OnErrorDelayMicrosecs);
            continue;
        }
        else if (ApiRequestType_GETSTATE == msg.type)
        {
            Log_Info("recv GETSTATE request: %d", msg.type);
            ADUC_ServiceStatus status = ADUC_ServiceStatus_None;
            ADUC_Result res = viewstatemgr_svcstatus_get(g_viewstatemgr_handle, &status);
            if (IsAducResultCodeFailure(res.ResultCode))
            {
                Log_Error("failed get viewstate, erc: %d", res.ExtendedResultCode);
                (retval->result).ExtendedResultCode = res.ExtendedResultCode;
                status = ADUC_ServiceStatus_ERROR_AgentServiceInternal;
            }

            Log_Info("GETSTATE returning code %d ret_val %d", 1, status);
            resp.code = htons(1);
            resp.ret_val = htons(status);

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

    free(fifo_path);
    return retval;
}
