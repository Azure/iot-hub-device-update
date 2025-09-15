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

#define g_AducApiVersion 1
#define CALL_BUF_LEN (3 * sizeof(uint16_t) + PATH_MAX) // version(2) + req_type(2) + resp_path_len(2) + resp_path(PATH_MAX)

pthread_mutex_t g_mutex_api_call_queue = PTHREAD_MUTEX_INITIALIZER; // !< Mutex for synchronizing the API call queue
pthread_t g_api_svc_thread; // !< The threads servicing incoming API calls
bool g_api_svc_thread_created = false; // !< Indicates if the API service thread has been created
bool g_api_svc_thread_running = false; // !< flag for the thread to keep running

const unsigned int DelaySecsBetwenFailedOperation = 10u; // !< delay allowed between failed operations

typedef struct tagAducApiCallWireState
{
    int fd;
    unsigned char call_buf[CALL_BUF_LEN];
    ssize_t tot_bytes_read;
} AducApiCallWireState;

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

static _reset_with_delay(int* fd, unsigned char* buf, size_t buf_len, ssize_t* tot_bytes_read)
{
    if (*fd > 0)
    {
        close(*fd);
        *fd = -1;
    }

    if (buf != NULL && buf_len > 0 && buf_len <= CALL_BUF_LEN)
    {
        memset(buf, 0, buf_len);
    }

    *tot_bytes_read = 0;

    sleep(DelaySecsBetwenFailedOperation);
}

static void ApiServiceThreadProc (void*)
{
    AducApiCallWireState wire_state = {0};
    while (!g_api_svc_thread_running)
    {
        if (wire_state.fd <= 0)
        {
            wire_state.fd = open(ADUC_API_FIFO_PATH, O_RDONLY);
            if (wire_state.fd <= 0)
            {
                Log_Error("Cannot open FIFO '%s' for read.", ADUC_API_FIFO_PATH);
                _reset_with_delay(&wire_state, &parse_state);
                continue;
            }
        }

        Log_Info("Wait for API Call ...");

        ssize_t bytes_read = read(wire_state.fd, &wire_state.call_buf, sizeof(wire_state.call_buf));
        if (bytes_read < 0)
        {
            Log_Warn("Read error (error:%d).", errno);
            _reset_with_delay(&wire_state, &parse_state);
            continue;
        }

        ssize_t tmp_read = tot_bytes_read + num_bytes_read;
        if (tmp_read > CALL_BUF_LEN) // overflow
        {
            Log_Warn("overflow ingress detected. Resetting.");
            _reset_with_delay(&wire_state, &parse_state);
            continue;
        }

        if (num_bytes_read == 0)
        {
            // EOF, in this case, no more data written to the pipe.
            _reset_with_delay(&wire_state, &parse_state);
            continue;
        }

        memcpy(&wire_state.call_buf[tot_bytes_read], &wire_state.call_buf, bytes_read);
        wire_state.tot_bytes_read += bytes_read;

        ssize_t new_byte_start_pos = wire_state.tot_bytes_read - bytes_read;
        AducApiParseStatus parse_status = _parse_call(wire_state.call_buf, new_byte_start_pos, wire_state.tot_bytes_read, &parse_state);
        switch (parse_status)
        {
        case AducApiParseStatus_Complete:
            _evaluate_parsed_call(&parse_state);
            _reset_with_delay(&fd, call_buf, sizeof(call_buf));
            break;
        case AducApiParseStatus_Continue:
            // need more bytes to reach a complete parse.
            sleep(100); // avoid tight loop
            break;
        case AducApiParseStatus_Fail:
            _reset_with_delay(&fd, call_buf, sizeof(call_buf));
            tot_bytes_read = 0;
            break;
        }
    }
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
