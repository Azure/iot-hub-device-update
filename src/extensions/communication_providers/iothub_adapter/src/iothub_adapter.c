/**
 * @file iothub_adapter.c
 * @brief IoT Hub backward-compatibility adapter (stub).
 *
 * Implements ADUC_CommunicationVtable as a placeholder for the Gen1
 * IoT Hub communication manager integration.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "iothub_adapter.h"

#include "aduc/communication_vtable.h"
#include "aduc/extension_context.h"
#include "aduc/extension_descriptor.h"
#include "aduc/extension_types.h"

#include <stddef.h>

#define IOTHUB_COMP "iothub-adapter"

static const ADUC_ExtensionContext* s_extCtx = NULL;

static ADUC_Result2 NotImplemented(void)
{
    return ADUC_RESULT2_MAKE(1, 1, 1);
}

/* ─── Vtable implementations ─────────────────────────────────────────────── */

static ADUC_Result2 IotHub_Connect(const ADUC_CommConfig* config)
{
    (void)config;
    if (s_extCtx != NULL)
    {
        ADUC_EXT_LOG_INFO(s_extCtx, IOTHUB_COMP, 2001, "Connect called (stub - not implemented)");
    }
    return NotImplemented();
}

static void IotHub_Disconnect(void)
{
    if (s_extCtx != NULL)
    {
        ADUC_EXT_LOG_INFO(s_extCtx, IOTHUB_COMP, 2002, "Disconnect called (stub)");
    }
}

static ADUC_CommConnectionState IotHub_GetConnectionState(void)
{
    return ADUC_COMM_STATE_DISCONNECTED;
}

static ADUC_Result2 IotHub_Poll(ADUC_CommMessage* outMsg, uint32_t timeoutMs)
{
    (void)outMsg;
    (void)timeoutMs;
    return NotImplemented();
}

static ADUC_Result2 IotHub_RegisterCallback(ADUC_CommEventType event, ADUC_CommCallback cb, void* ctx)
{
    (void)event;
    (void)cb;
    (void)ctx;
    return NotImplemented();
}

static ADUC_Result2 IotHub_ReportState(const ADUC_AgentState* state)
{
    (void)state;
    return NotImplemented();
}

static ADUC_Result2 IotHub_ReportResult(const ADUC_DeploymentResult2* result)
{
    (void)result;
    return NotImplemented();
}

static ADUC_Result2 IotHub_GetDownloadUrl(const char* fileId, char* urlBuf, size_t urlBufLen)
{
    (void)fileId;
    (void)urlBuf;
    (void)urlBufLen;
    return NotImplemented();
}

static ADUC_Result2 IotHub_SendDiagnostics(const void* payload, size_t payloadLen)
{
    (void)payload;
    (void)payloadLen;
    return NotImplemented();
}

static ADUC_Result2 IotHub_HealthCheck(ADUC_CommHealthStatus* outStatus)
{
    if (outStatus != NULL)
    {
        *outStatus = ADUC_COMM_HEALTH_DISCONNECTED;
    }
    return NotImplemented();
}

/* ─── Vtable ──────────────────────────────────────────────────────────────── */

static const ADUC_CommunicationVtable s_vtable = {
    .structVersion = 1,
    .Connect = IotHub_Connect,
    .Disconnect = IotHub_Disconnect,
    .GetConnectionState = IotHub_GetConnectionState,
    .Poll = IotHub_Poll,
    .RegisterCallback = IotHub_RegisterCallback,
    .ReportState = IotHub_ReportState,
    .ReportResult = IotHub_ReportResult,
    .GetDownloadUrl = IotHub_GetDownloadUrl,
    .SendDiagnostics = IotHub_SendDiagnostics,
    .HealthCheck = IotHub_HealthCheck,
};

/* ─── Extension lifecycle ─────────────────────────────────────────────────── */

ADUC_Result2 IotHubAdapter_Initialize(const ADUC_ExtensionContext* ctx)
{
    s_extCtx = ctx;
    ADUC_EXT_LOG_INFO(ctx, IOTHUB_COMP, 2000, "IoT Hub adapter initialized (stub)");
    return ADUC_RESULT2_SUCCESS;
}

void IotHubAdapter_Uninitialize(void)
{
    if (s_extCtx != NULL)
    {
        ADUC_EXT_LOG_INFO(s_extCtx, IOTHUB_COMP, 2099, "IoT Hub adapter uninitialized");
    }
    s_extCtx = NULL;
}

/* ─── Extension descriptor ────────────────────────────────────────────────── */

static const ADUC_ExtensionDescriptor s_descriptor = {
    .structVersion = ADUC_EXTENSION_DESCRIPTOR_VERSION,
    .id = "iothub-adapter",
    .name = "IoT Hub Backward-Compat Adapter",
    .version = "1.0.0",
    .type = ADUC_EXT_TYPE_COMMUNICATION,
    .minHostApiVersion = 1,
    .Initialize = IotHubAdapter_Initialize,
    .Uninitialize = IotHubAdapter_Uninitialize,
    .vtable = &s_vtable,
    .capabilities = NULL,
};

ADUC_DECLARE_EXTENSION(s_descriptor)
