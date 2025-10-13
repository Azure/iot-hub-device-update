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
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#define _XOPEN_SOURCE 700
#include <sys/types.h>
#include <unistd.h>

#define g_AducApiVersion 1
#define FIFO_FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) // rw-rw----

static const unsigned OnErrorDelayMicrosecs = 150000;

pthread_t g_api_svc_thread = { 0 };
bool g_api_svc_thread_running = false;
extern ViewStateManager g_vsm;

typedef struct tagFifoThreadRetVal
{
    ADUC_Result result;
} FifoThreadRetVal;

// fwd-decl
static void* aduc_apisvc_thread_proc(void*);

/////////////////////////////////////////////////
// BEGIN: Public API
//
bool init_api_svc(const char* fifoPath)
{
    if (g_api_svc_thread_running)
    {
        return false;
    }

    if (fifoPath == NULL)
    {
        fifoPath = ADUC_API_DEFAULT_FIFO_PATH;
    }

    Log_Info("Initializing API Service thread with fifo: '%s'", fifoPath);

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

bool uninit_api_svc()
{
    bool success = false;

    Log_Info("Uninitializing api service thread");

    void* threadRet = NULL;
    FifoThreadRetVal* retVal = NULL;

    g_api_svc_thread_running = false;
    int res = pthread_join(g_api_svc_thread, (void**)&threadRet);
    if (res != 0)
    {
        Log_Warn("pthread join error: %d\n", res);
    }
    else
    {
        Log_Info("api_svc pthread join succeeded.");
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
    }
    else
    {
        Log_Info("Fifo thread uninited and had no failure.");
        success = true;
    }

    free(retVal);
    return success;
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
    (void)unlink(fifoPath);
    if (mkfifo(fifoPath, FIFO_FILE_MODE) < 0)
    {
        if (errno == EEXIST)
        {
            Log_Info("FIFO '%s' already exists.");
        }
        else
        {
            Log_Error("Failed to create fifo '%s': %d", fifoPath, errno);
            return -1;
        }
    }

    if (chmod(fifoPath, FIFO_FILE_MODE) != 0)
    {
        Log_Warn("Failed to chmod 0660 on FIFO: '%s'", fifoPath);
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
    int rdfifo = -1;
    int writefifo = -1;
    int open_read_retries = 0, open_write_retries = 0;

    ADUC_Logging_Init(ADUC_LOG_DEBUG, "aducapi");

    // Ignore SIGPIPE so write() returns -1 with EPIPE instead of killing the thread
    signal(SIGPIPE, SIG_IGN);

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
        goto done;
    }
    Log_Info("API Request FIFO is verified: '%s'", fifo_path);

    while ((rdfifo == -1) && g_api_svc_thread_running && (open_read_retries < 30))
    {
        ++open_read_retries;
        rdfifo = open(fifo_path, O_RDONLY | O_NONBLOCK);
        if (rdfifo == -1)
        {
            Log_Warn("Failed to open'%s'. errno: %d", fifo_path, errno);
            (retval->result).ExtendedResultCode = ADUC_ERC_APISVC_OPEN_FIFO_FAILED;
            goto done;
        }
        (void)open(fifo_path, O_WRONLY | O_NONBLOCK, 0); // NOTE: never used. this avoids EOF return on read
        usleep(3 * OnErrorDelayMicrosecs);
    }
    if (rdfifo == -1)
    {
        Log_Error("Failed to open request fifo for read after %d retries", open_read_retries);
        (retval->result).ExtendedResultCode = ADUC_ERC_APISVC_OPEN_FIFO_FAILED;
        goto done;
    }

    Log_Info("Success opening API Request FIFO for read: '%s'", fifo_path);

    while (g_api_svc_thread_running)
    {
        ssize_t n;
        ApiWireRequestMsg msg = { 0 };

        open_write_retries = 0;

        if ((n = msg_recv_req(rdfifo, &msg)) < 0)
        {
            if (n == MSGREV_AGAIN)
            {
                // allow it to be interrupted and try again, checking for running flag
                continue;
            }
            Log_Error("msg_recv_req error: %zd", n);
            usleep(OnErrorDelayMicrosecs);
            continue;
        }

        // Null-terminate the data portion using the actual length field
        if (msg.len > 0 && msg.len < MAX_BUF_LEN)
        {
            msg.data[msg.len] = '\0';
        }
        else if (msg.len == 0)
        {
            msg.data[0] = '\0';
        }
        else
        {
            // Invalid length, skip this message
            Log_Error("Invalid message length: %u", msg.len);
            usleep(OnErrorDelayMicrosecs);
            continue;
        }

        if (msg.data[0] == '\0')
        {
            Log_Warn("Received empty message data");
            usleep(OnErrorDelayMicrosecs);
            continue;
        }

        if (!verify_fifo_security(msg.data))
        {
            usleep(OnErrorDelayMicrosecs);
            continue;
        }

        while (writefifo == -1 && g_api_svc_thread_running && open_write_retries < 30)
        {
            ++open_write_retries;
            writefifo = open(msg.data, O_WRONLY | O_NONBLOCK);
            if (writefifo < 0)
            {
                if (errno != ENXIO) // ENXIO is when other side did not open fifo for read yet
                {
                    Log_Error("Failed to open resp fifo '%s': %d", msg.data, errno);
                }

                usleep(3 * OnErrorDelayMicrosecs);
                continue;
            }
        }
        if (!g_api_svc_thread_running)
        {
            break;
        }

        if (writefifo == -1)
        {
            Log_Error("Failed to open response fifo for write after %d retries", open_write_retries);
            // Don't set extended result code here - just skip this request and continue
            usleep(OnErrorDelayMicrosecs);
            continue;
        }

        Log_Info("Success opening API Response FIFO for write: '%s'", msg.data);

        ApiWireResponseMsg resp = { 0 };
        if (ApiRequestType_NONE == msg.type)
        {
            Log_Error("Received invalid message type 0, skipping");
            if (writefifo != -1)
            {
                close(writefifo);
                writefifo = -1;
            }
            usleep(OnErrorDelayMicrosecs);
            continue;
        }
        else if (ApiRequestType_GETSTATE == msg.type)
        {
            Log_Info("recv GETSTATE request: %d", msg.type);
            ADUC_ServiceStatus status = ADUC_ServiceStatus_None;
            viewstatemgr_svcstatus_get(&g_vsm, &status);

            Log_Info("GETSTATE returning code %d ret_val %d", 1, status);
            resp.code = 1;
            resp.ret_val = status;

            ssize_t sent = msg_send_resp(writefifo, &resp);
            if (sent < 0)
            {
                if (errno == EPIPE)
                {
                    Log_Warn("msg_send_resp failed: client closed connection (EPIPE)");
                }
                else
                {
                    Log_Error("msg_send_resp failed, sent %zd bytes, errno: %d", sent, errno);
                }
            }
            else
            {
                Log_Info("Sent %d bytes for GETSTATE response status: %d", sent, status);
            }
        }

        // Close this side of the response FIFO after handling the request
        if (writefifo != -1)
        {
            close(writefifo);
            writefifo = -1;
        }
    }

    if (retval != NULL)
    {
        (retval->result).ResultCode = 1;
    }
done:
    if (rdfifo != -1)
    {
        close(rdfifo);
        rdfifo = -1;
    }
    if (writefifo != -1)
    {
        close(writefifo);
        writefifo = -1;
    }

    free(fifo_path);
    ADUC_Logging_Uninit();

    return retval;
}
