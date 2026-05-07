/**
 * @file aducsdk.c
 * @brief Implementation of the ADU SDK API functions
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <aduc/aducsdk.h>
#include <aduc/ipc_transport.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <stdlib.h> // secure_getenv
#endif

// Local API wire protocol (must match localapi_server.c)
#define LOCALAPI_VERSION 1
#define LOCALAPI_REQ_GET_STATUS 0x01

#pragma pack(push, 1)
typedef struct SdkRequestMsg
{
    uint16_t ver;
    uint16_t type;
    uint16_t len;
} SdkRequestMsg;

typedef struct SdkResponseMsg
{
    uint16_t ver;
    uint16_t status;
    uint16_t len;
} SdkResponseMsg;
#pragma pack(pop)

ADUC_ServiceStatus GetAduServiceStatus(void)
{
    IpcTransport* transport = NULL;
    ADUC_ServiceStatus result = ADUC_ServiceStatus_ERROR_Unknown;

#ifndef _WIN32
    const char* debug_env = secure_getenv("ADUC_SDK_DEBUG");
#else
    char* debug_env = NULL;
    size_t envLen = 0;
    if (_dupenv_s(&debug_env, &envLen, "ADUC_SDK_DEBUG") != 0)
    {
        debug_env = NULL;
    }
#endif
    const bool debug_enabled = (debug_env != NULL && debug_env[0] != '\0');

    const char* endpoint = ipc_get_default_endpoint();

    // Connect to the Local API server
    IpcTransportResult rc = ipc_client_connect(endpoint, 5000, &transport);
    if (rc != IPC_OK)
    {
        if (debug_enabled)
        {
            fprintf(stderr, "[ADUC SDK] Failed to connect to agent at '%s': %d\n", endpoint, rc);
        }

        if (rc == IPC_ERR_CONNECT_FAILED)
        {
            result = ADUC_ServiceStatus_ERROR_AgentServiceNotRunning;
        }
        else if (rc == IPC_ERR_PERMISSION_DENIED)
        {
            result = ADUC_ServiceStatus_ERROR_AgentServicePermission;
        }
        else if (rc == IPC_ERR_TIMEOUT)
        {
            result = ADUC_ServiceStatus_ERROR_AgentServiceTimeout;
        }
        goto cleanup;
    }

    // Send GET_STATUS request (header only, no payload)
    SdkRequestMsg req = {
        .ver = LOCALAPI_VERSION,
        .type = LOCALAPI_REQ_GET_STATUS,
        .len = 0,
    };

    if (debug_enabled)
    {
        fprintf(stderr, "[ADUC SDK] Sending GET_STATUS request...\n");
    }

    rc = ipc_send(transport, &req, sizeof(req));
    if (rc != IPC_OK)
    {
        if (debug_enabled)
        {
            fprintf(stderr, "[ADUC SDK] Failed to send request: %d\n", rc);
        }
        result = ADUC_ServiceStatus_ERROR_AgentServiceBrokenPipe;
        goto cleanup;
    }

    // Receive response header
    SdkResponseMsg resp = { 0 };
    size_t bytesReceived = 0;
    rc = ipc_recv(transport, &resp, sizeof(resp), &bytesReceived);
    if (rc != IPC_OK || bytesReceived < sizeof(resp))
    {
        if (debug_enabled)
        {
            fprintf(stderr, "[ADUC SDK] Failed to receive response: rc=%d, bytes=%zu\n", rc, bytesReceived);
        }
        if (rc == IPC_ERR_TIMEOUT)
        {
            result = ADUC_ServiceStatus_ERROR_AgentServiceTimeout;
        }
        else if (rc == IPC_ERR_PEER_DISCONNECTED)
        {
            result = ADUC_ServiceStatus_ERROR_AgentServiceBrokenPipe;
        }
        else
        {
            result = ADUC_ServiceStatus_ERROR_RecvMsgFailed;
        }
        goto cleanup;
    }

    if (debug_enabled)
    {
        fprintf(stderr, "[ADUC SDK] Response: ver=%u, status=%u, len=%u\n",
                resp.ver, resp.status, resp.len);
    }

    // Check for error status codes from server (HTTP-style)
    if (resp.status >= 400)
    {
        if (resp.status == 403)
        {
            result = ADUC_ServiceStatus_ERROR_AgentServicePermission;
        }
        else if (resp.status == 429)
        {
            result = ADUC_ServiceStatus_ERROR_AgentServiceInternal;
        }
        else
        {
            result = ADUC_ServiceStatus_ERROR_AgentServiceInternal;
        }
        goto cleanup;
    }

    // Status field contains the ADUC_ServiceStatus value directly
    result = (ADUC_ServiceStatus)(resp.status);

    if (debug_enabled)
    {
        fprintf(stderr, "[ADUC SDK] Status: %s\n", ADUC_ServiceStatusToString(result));
    }

cleanup:
    if (transport != NULL)
    {
        ipc_transport_close(transport);
    }

#ifdef _WIN32
    free(debug_env);
#endif

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
    case ADUC_ServiceStatus_ERROR_AgentServiceSdkOpenRespFifoFailed:
        return "Error: Agent Service SDK Open Response FIFO Failed";
    case ADUC_ServiceStatus_ERROR_AgentServiceReqFifoSvcEndNotOpenedYet:
        return "Error: Agent Service Request FIFO Service End Not Opened Yet";
    case ADUC_ServiceStatus_ERROR_AgentServicePermission:
        return "Error: Agent Service Permission";
    case ADUC_ServiceStatus_ERROR_RecvMsgFailed:
        return "Error: Receive Message Failed";
    case ADUC_ServiceStatus_ERROR_AgentServiceTimeout:
        return "Error: Agent Service Timeout";
    case ADUC_ServiceStatus_ERROR_AgentServiceInternal:
        return "Error: Agent Service Internal";
    case ADUC_ServiceStatus_ERROR_AgentServiceMkFifoFailed:
        return "Error: Agent Service MkFifo Failed";
    case ADUC_ServiceStatus_ERROR_AgentServiceChmodFailed:
        return "Error: Agent Service Chmod Failed";
    case ADUC_ServiceStatus_ERROR_Unknown:
        return "Error: Unknown";
    default:
        return "Unknown Status";
    }
}
