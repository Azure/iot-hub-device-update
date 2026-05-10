/**
 * @file comm_dispatcher.c
 * @brief Communication dispatcher implementation.
 *
 * Routes operations through prioritized communication providers with failover.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/comm_dispatcher.h"

#include "aduc/communication_vtable.h"
#include "aduc/extension_descriptor.h"
#include "aduc/extension_loader.h"
#include "aduc/extension_types.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* ── Error codes (ADUC_FACILITY_COMM) ──────────────────────────────────────── */

#define COMM_ERR_INVALID_ARG    ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_CONFIG, 0x0001)
#define COMM_ERR_ALLOC          ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_RESOURCE, 0x0001)
#define COMM_ERR_EXT_NOT_FOUND  ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_CONFIG, 0x0002)
#define COMM_ERR_NO_PROVIDERS   ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_CONFIG, 0x0003)
#define COMM_ERR_ALL_FAILED     ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_PROTOCOL, 0x0001)
#define COMM_ERR_NONE_CONNECTED ADUC_RESULT2_MAKE(ADUC_FACILITY_COMM, ADUC_CATEGORY_STATE, 0x0001)

/* ── Internal types ────────────────────────────────────────────────────────── */

typedef struct ProviderEntry
{
    char* extensionId;
    uint32_t priority;
    bool reportOnly;
    const ADUC_CommunicationVtable* vtable;
    bool connected;
    ADUC_Result2 lastError;
} ProviderEntry;

struct ADUC_CommDispatcher
{
    ProviderEntry* entries;
    size_t count;
    uint32_t failoverTimeoutMs;
};

/* ── Helpers ───────────────────────────────────────────────────────────────── */

/**
 * @brief Comparison function for sorting providers by priority (ascending).
 */
static int ComparePriority(const void* a, const void* b)
{
    const ProviderEntry* ea = (const ProviderEntry*)a;
    const ProviderEntry* eb = (const ProviderEntry*)b;

    if (ea->priority < eb->priority)
    {
        return -1;
    }
    if (ea->priority > eb->priority)
    {
        return 1;
    }
    return 0;
}

static char* DuplicateString(const char* src)
{
    if (src == NULL)
    {
        return NULL;
    }
    size_t len = strlen(src);
    char* dup = (char*)malloc(len + 1);
    if (dup != NULL)
    {
        memcpy(dup, src, len + 1);
    }
    return dup;
}

/* ── Public API ────────────────────────────────────────────────────────────── */

ADUC_Result2 ADUC_CommDispatcher_Create(
    const ADUC_CommDispatcherConfig* config,
    ADUC_ExtensionRegistryHandle registry,
    ADUC_CommDispatcherHandle* outHandle)
{
    if (config == NULL || registry == NULL || outHandle == NULL)
    {
        return COMM_ERR_INVALID_ARG;
    }
    if (config->providerCount == 0 || config->providers == NULL)
    {
        return COMM_ERR_NO_PROVIDERS;
    }

    *outHandle = NULL;

    struct ADUC_CommDispatcher* disp =
        (struct ADUC_CommDispatcher*)calloc(1, sizeof(struct ADUC_CommDispatcher));
    if (disp == NULL)
    {
        return COMM_ERR_ALLOC;
    }

    disp->entries = (ProviderEntry*)calloc(config->providerCount, sizeof(ProviderEntry));
    if (disp->entries == NULL)
    {
        free(disp);
        return COMM_ERR_ALLOC;
    }

    disp->failoverTimeoutMs = config->failoverTimeoutMs;
    disp->count = 0;

    for (size_t i = 0; i < config->providerCount; i++)
    {
        const ADUC_CommProviderConfig* pc = &config->providers[i];

        if (pc->extensionId == NULL)
        {
            continue;
        }

        const ADUC_ExtensionDescriptor* desc =
            ADUC_ExtensionRegistry_FindById(registry, pc->extensionId);
        if (desc == NULL)
        {
            /* Extension not found — skip this provider. */
            continue;
        }

        if (desc->type != ADUC_EXT_TYPE_COMMUNICATION || desc->vtable == NULL)
        {
            continue;
        }

        ProviderEntry* entry = &disp->entries[disp->count];
        entry->extensionId = DuplicateString(pc->extensionId);
        if (entry->extensionId == NULL)
        {
            /* Clean up on allocation failure. */
            for (size_t j = 0; j < disp->count; j++)
            {
                free(disp->entries[j].extensionId);
            }
            free(disp->entries);
            free(disp);
            return COMM_ERR_ALLOC;
        }

        entry->priority = pc->priority;
        entry->reportOnly = pc->reportOnly;
        entry->vtable = (const ADUC_CommunicationVtable*)desc->vtable;
        entry->connected = false;
        entry->lastError = ADUC_RESULT2_SUCCESS;

        disp->count++;
    }

    if (disp->count == 0)
    {
        free(disp->entries);
        free(disp);
        return COMM_ERR_EXT_NOT_FOUND;
    }

    /* Sort providers by priority (ascending — lower value = higher priority). */
    qsort(disp->entries, disp->count, sizeof(ProviderEntry), ComparePriority);

    *outHandle = disp;
    return ADUC_RESULT2_SUCCESS;
}

ADUC_Result2 ADUC_CommDispatcher_ConnectAll(
    ADUC_CommDispatcherHandle handle,
    const ADUC_CommConfig* commConfig)
{
    if (handle == NULL || commConfig == NULL)
    {
        return COMM_ERR_INVALID_ARG;
    }

    size_t connectedCount = 0;

    for (size_t i = 0; i < handle->count; i++)
    {
        ProviderEntry* entry = &handle->entries[i];

        if (entry->vtable->Connect == NULL)
        {
            continue;
        }

        ADUC_Result2 result = entry->vtable->Connect(commConfig);
        if (ADUC_RESULT2_IS_SUCCESS(result))
        {
            entry->connected = true;
            entry->lastError = ADUC_RESULT2_SUCCESS;
            connectedCount++;
        }
        else
        {
            entry->connected = false;
            entry->lastError = result;
        }
    }

    return (connectedCount > 0) ? ADUC_RESULT2_SUCCESS : COMM_ERR_NONE_CONNECTED;
}

ADUC_Result2 ADUC_CommDispatcher_Poll(
    ADUC_CommDispatcherHandle handle,
    ADUC_CommMessage* outMsg,
    uint32_t timeoutMs)
{
    if (handle == NULL || outMsg == NULL)
    {
        return COMM_ERR_INVALID_ARG;
    }

    ADUC_Result2 lastError = COMM_ERR_ALL_FAILED;

    /* Entries are already sorted by priority. Try each non-reportOnly provider. */
    for (size_t i = 0; i < handle->count; i++)
    {
        ProviderEntry* entry = &handle->entries[i];

        if (entry->reportOnly)
        {
            continue;
        }

        if (!entry->connected)
        {
            continue;
        }

        if (entry->vtable->Poll == NULL)
        {
            continue;
        }

        ADUC_Result2 result = entry->vtable->Poll(outMsg, timeoutMs);
        if (ADUC_RESULT2_IS_SUCCESS(result))
        {
            return ADUC_RESULT2_SUCCESS;
        }

        /* Record error and try next provider (failover). */
        entry->lastError = result;
        lastError = result;
    }

    return lastError;
}

ADUC_Result2 ADUC_CommDispatcher_ReportState(
    ADUC_CommDispatcherHandle handle,
    const ADUC_AgentState* state)
{
    if (handle == NULL || state == NULL)
    {
        return COMM_ERR_INVALID_ARG;
    }

    size_t successCount = 0;
    ADUC_Result2 lastError = COMM_ERR_ALL_FAILED;

    for (size_t i = 0; i < handle->count; i++)
    {
        ProviderEntry* entry = &handle->entries[i];

        if (!entry->connected)
        {
            continue;
        }

        if (entry->vtable->ReportState == NULL)
        {
            continue;
        }

        ADUC_Result2 result = entry->vtable->ReportState(state);
        if (ADUC_RESULT2_IS_SUCCESS(result))
        {
            entry->lastError = ADUC_RESULT2_SUCCESS;
            successCount++;
        }
        else
        {
            entry->lastError = result;
            lastError = result;
        }
    }

    return (successCount > 0) ? ADUC_RESULT2_SUCCESS : lastError;
}

ADUC_Result2 ADUC_CommDispatcher_ReportResult(
    ADUC_CommDispatcherHandle handle,
    const ADUC_DeploymentResult2* result)
{
    if (handle == NULL || result == NULL)
    {
        return COMM_ERR_INVALID_ARG;
    }

    size_t successCount = 0;
    ADUC_Result2 lastError = COMM_ERR_ALL_FAILED;

    for (size_t i = 0; i < handle->count; i++)
    {
        ProviderEntry* entry = &handle->entries[i];

        if (!entry->connected)
        {
            continue;
        }

        if (entry->vtable->ReportResult == NULL)
        {
            continue;
        }

        ADUC_Result2 res = entry->vtable->ReportResult(result);
        if (ADUC_RESULT2_IS_SUCCESS(res))
        {
            entry->lastError = ADUC_RESULT2_SUCCESS;
            successCount++;
        }
        else
        {
            entry->lastError = res;
            lastError = res;
        }
    }

    return (successCount > 0) ? ADUC_RESULT2_SUCCESS : lastError;
}

ADUC_Result2 ADUC_CommDispatcher_HealthCheck(
    ADUC_CommDispatcherHandle handle,
    ADUC_CommHealthStatus* outStatus)
{
    if (handle == NULL || outStatus == NULL)
    {
        return COMM_ERR_INVALID_ARG;
    }

    size_t healthyCount = 0;
    size_t checkedCount = 0;

    for (size_t i = 0; i < handle->count; i++)
    {
        ProviderEntry* entry = &handle->entries[i];

        if (!entry->connected)
        {
            continue;
        }

        checkedCount++;

        if (entry->vtable->HealthCheck != NULL)
        {
            ADUC_CommHealthStatus providerStatus = ADUC_COMM_HEALTH_DISCONNECTED;
            ADUC_Result2 result = entry->vtable->HealthCheck(&providerStatus);

            if (ADUC_RESULT2_IS_SUCCESS(result) && providerStatus == ADUC_COMM_HEALTH_CONNECTED)
            {
                healthyCount++;
            }
        }
        else
        {
            /* No HealthCheck function — assume healthy if connected. */
            healthyCount++;
        }
    }

    if (checkedCount == 0)
    {
        *outStatus = ADUC_COMM_HEALTH_DISCONNECTED;
    }
    else if (healthyCount == checkedCount)
    {
        *outStatus = ADUC_COMM_HEALTH_CONNECTED;
    }
    else if (healthyCount > 0)
    {
        *outStatus = ADUC_COMM_HEALTH_DEGRADED;
    }
    else
    {
        *outStatus = ADUC_COMM_HEALTH_DISCONNECTED;
    }

    return ADUC_RESULT2_SUCCESS;
}

void ADUC_CommDispatcher_Destroy(ADUC_CommDispatcherHandle handle)
{
    if (handle == NULL)
    {
        return;
    }

    for (size_t i = 0; i < handle->count; i++)
    {
        ProviderEntry* entry = &handle->entries[i];

        if (entry->connected && entry->vtable->Disconnect != NULL)
        {
            entry->vtable->Disconnect();
            entry->connected = false;
        }

        free(entry->extensionId);
    }

    free(handle->entries);
    free(handle);
}
