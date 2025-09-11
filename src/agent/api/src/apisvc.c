/**
 * @file apisvc.c
 * @brief The service implementation for handling incoming cross-process API calls.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/apisvc.h"
#include "aduc/apisvc_internal.h"

#include <arpa/inet.h>
#include <stdbool.h>

#define MAX_CMD_QUEUE_LEN 32
#define g_AducApiVersion 1

pthread_mutex_t g_mutex_api_call_queue = PTHREAD_MUTEX_INITIALIZER; // !< Mutex for synchronizing the API call queue
pthread_t g_api_svc_thread; // !< The threads servicing incoming API calls
bool g_api_svc_thread_created = false; // !< Indicates if the API service thread has been created
bool g_api_svc_thread_running = false; // !< flag for the thread to keep running

static void _free_aduc_api_call(AducApiCall* call)
{
    if (call != NULL)
    {
        switch (call->req_args.request_arg_type)
        {
            case RequestArgType_GetState:
                free(call->req_args.get_state.resp_fifo_path);
                memset(call, 0, sizeof(*call));
                break;
            case RequestArgType_Unknown:
            default:
                break;
        }
    }
}

static AducApiCall* _parse_incoming_call(int fd_stream)
{
    AducApiCall call = {0};
    return NULL;
}

static bool _check_resp_fifo_security(const char* fifoPath)
{
    if (!(PermissionUtils_CheckOwnership(fifoPath, ADUC_FILE_USER, ADUC_FILE_GROUP))) {
        Log_Error("invalid ownership for resp fifo '%s'", fifoPath);
        return false;
    }
    return true;
}

static bool _write_apisvc_response(const char* response_path, ADUC_GetStateApiResponse resp)
{
    if (IsNullOrEmpty(response_path))
    {
        Log_Error("Null or Empty response path");
        return false;
    }

    Log_Info("Writing response to FIFO: %s", response_path);

    if (!_check_resp_fifo_security(response_path))
    {
        Log_Error("Security check failed for response FIFO: %s", response_path);
        return false;
    }

    int fd = open(response_path, O_WRONLY | O_NONBLOCK);
    if (fd < 0)
    {
        Log_Error("Failed to open response FIFO: %s", response_path);
        return false;
    }

    // wire format:
    // <RespCode><State>
    // where RespCode is num bytes of uint32_t
    // and State is num bytes of uint32_t
    pthread_mutex_lock(&g_mutex_api_call_queue);
    unsigned char resp_buf[sizeof(uint32_t) * 2] = {0};
    uint32_t code_net = htonl((uint32_t)resp.resp_code);
    uint32_t state_net = htonl((uint32_t)resp.state);
    memcpy(resp_buf, &code_net, sizeof(uint32_t));
    memcpy(resp_buf + sizeof(uint32_t), &state_net, sizeof(uint32_t));
    ssize_t bytes_written = write(fd, resp_buf, sizeof(resp_buf));
    pthread_mutex_unlock(&g_mutex_api_call_queue);
    close(fd);

    if (bytes_written != sizeof(resp_buf))
    {
        Log_Error("Failed to write response. bytes written: %zd", bytes_written);
        return false;
    }

    return true;
}

ApiServiceThreadProc ()
{
}

/////////////////////////////////////////////////
// BEGIN: Public API
//
/**
 * @brief Initializes the API service.
 * @return bool Returns true on success.
 */
bool InitializeApiService()
{
    if (g_api_svc_thread_created)
    {
        Log_Warn("API service thread already created.");
        return false;
    }

    Log_Info("Initializing API Service thread");

    g_api_svc_thread_running = true;
    int ret = pthread_create(&g_api_svc_thread, NULL, ApiServiceThreadProc, NULL);
    if (ret != 0)
    {
        Log_Error("Failed to create command listener thread: %d", ret);
        g_api_svc_thread_running = false;
        g_api_svc_thread_created = false;
        return false;
    }

    g_api_svc_thread_created = true;
    return true;
}

/**
 * @brief Uninitializes the API service thread.
 */
void UninitializeApiServiceThread()
{
    Log_Info("Uninitializing api service thread");
    g_api_svc_thread_running = false;
}
//
// END: Public API
/////////////////////////////////////////////////
