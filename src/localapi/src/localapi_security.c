/**
 * @file localapi_security.c
 * @brief Security and authorization implementation for the Agent Local API.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "localapi_security.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#endif

#define DEFAULT_MAX_REQ_PER_SEC 10
#define DEFAULT_MAX_CLIENTS 5
#define RATE_LIMIT_TABLE_SLOTS 32
#define RATE_LIMIT_WINDOW_MS 1000

static uint64_t get_monotonic_ms(void)
{
#ifdef _WIN32
    return (uint64_t)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

bool localapi_security_init(LocalApiSecurityContext* ctx, int maxReqPerSec, int maxClients)
{
    if (ctx == NULL)
    {
        return false;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->maxRequestsPerSecond = (maxReqPerSec > 0) ? maxReqPerSec : DEFAULT_MAX_REQ_PER_SEC;
    ctx->maxConcurrentClients = (maxClients > 0) ? maxClients : DEFAULT_MAX_CLIENTS;
    ctx->rateLimitTableSize = RATE_LIMIT_TABLE_SLOTS;
    ctx->rateLimitTable = (RateLimitEntry*)calloc((size_t)RATE_LIMIT_TABLE_SLOTS, sizeof(RateLimitEntry));

    return (ctx->rateLimitTable != NULL);
}

void localapi_security_uninit(LocalApiSecurityContext* ctx)
{
    if (ctx == NULL)
    {
        return;
    }
    free(ctx->rateLimitTable);
    ctx->rateLimitTable = NULL;
}

bool localapi_security_allow_connection(LocalApiSecurityContext* ctx)
{
    if (ctx == NULL)
    {
        return false;
    }
    return (ctx->currentClientCount < ctx->maxConcurrentClients);
}

void localapi_security_connection_closed(LocalApiSecurityContext* ctx)
{
    if (ctx != NULL && ctx->currentClientCount > 0)
    {
        ctx->currentClientCount--;
    }
}

#ifndef _WIN32
/**
 * @brief Check if a UID belongs to the 'adu' group.
 */
static bool is_uid_in_adu_group(uint32_t uid)
{
    // Root always has access
    if (uid == 0)
    {
        return true;
    }

    struct passwd* pw = getpwuid((uid_t)uid);
    if (pw == NULL)
    {
        return false;
    }

    struct group* aduGroup = getgrnam("adu");
    if (aduGroup == NULL)
    {
        // If 'adu' group doesn't exist, allow owner's primary group match
        return true;
    }

    // Check if user's primary GID matches
    if (pw->pw_gid == aduGroup->gr_gid)
    {
        return true;
    }

    // Check supplementary groups
    for (char** member = aduGroup->gr_mem; *member != NULL; member++)
    {
        if (strcmp(*member, pw->pw_name) == 0)
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Check if a UID is root or in adu-admin group.
 */
static bool is_uid_admin(uint32_t uid)
{
    if (uid == 0)
    {
        return true;
    }

    struct passwd* pw = getpwuid((uid_t)uid);
    if (pw == NULL)
    {
        return false;
    }

    struct group* adminGroup = getgrnam("adu-admin");
    if (adminGroup == NULL)
    {
        // Fallback: only root is admin
        return false;
    }

    if (pw->pw_gid == adminGroup->gr_gid)
    {
        return true;
    }

    for (char** member = adminGroup->gr_mem; *member != NULL; member++)
    {
        if (strcmp(*member, pw->pw_name) == 0)
        {
            return true;
        }
    }

    return false;
}
#endif // !_WIN32

bool localapi_security_authorize(const IpcPeerCredentials* creds, LocalApiPermission requiredPerm)
{
    if (creds == NULL)
    {
        return false;
    }

#ifdef _WIN32
    // On Windows, pipe ACLs handle basic access control.
    // Elevated processes get full permissions, others get read-only.
    if (requiredPerm == LOCALAPI_PERM_READ_STATUS)
    {
        return true; // Pipe ACL already verified access
    }
    return creds->isElevated;
#else
    // Root has all permissions
    if (creds->uid == 0)
    {
        return true;
    }

    switch (requiredPerm)
    {
    case LOCALAPI_PERM_READ_STATUS:
        return is_uid_in_adu_group(creds->uid);

    case LOCALAPI_PERM_PAUSE_RESUME:
    case LOCALAPI_PERM_CANCEL:
        return is_uid_admin(creds->uid);

    case LOCALAPI_PERM_FORCE_CHECK:
        return (creds->uid == 0); // root only

    default:
        return false;
    }
#endif
}

bool localapi_security_check_rate_limit(LocalApiSecurityContext* ctx, const IpcPeerCredentials* creds)
{
    if (ctx == NULL || creds == NULL || ctx->rateLimitTable == NULL)
    {
        return false;
    }

#ifdef _WIN32
    uint32_t peerId = creds->processId;
#else
    uint32_t peerId = creds->uid;
#endif

    uint64_t now = get_monotonic_ms();

    // Find or allocate slot for this peer
    int freeSlot = -1;
    for (int i = 0; i < ctx->rateLimitTableSize; i++)
    {
        RateLimitEntry* entry = &ctx->rateLimitTable[i];

        if (entry->peerId == peerId && entry->windowStart > 0)
        {
            // Found existing entry
            if ((now - entry->windowStart) >= RATE_LIMIT_WINDOW_MS)
            {
                // Window expired, reset
                entry->requestCount = 1;
                entry->windowStart = now;
                return true;
            }

            entry->requestCount++;
            return (entry->requestCount <= (uint32_t)ctx->maxRequestsPerSecond);
        }

        if (freeSlot < 0 && entry->peerId == 0)
        {
            freeSlot = i;
        }
    }

    // New peer — allocate slot
    if (freeSlot >= 0)
    {
        ctx->rateLimitTable[freeSlot].peerId = peerId;
        ctx->rateLimitTable[freeSlot].requestCount = 1;
        ctx->rateLimitTable[freeSlot].windowStart = now;
        return true;
    }

    // Table full — evict oldest entry
    uint64_t oldest = UINT64_MAX;
    int oldestIdx = 0;
    for (int i = 0; i < ctx->rateLimitTableSize; i++)
    {
        if (ctx->rateLimitTable[i].windowStart < oldest)
        {
            oldest = ctx->rateLimitTable[i].windowStart;
            oldestIdx = i;
        }
    }

    ctx->rateLimitTable[oldestIdx].peerId = peerId;
    ctx->rateLimitTable[oldestIdx].requestCount = 1;
    ctx->rateLimitTable[oldestIdx].windowStart = now;
    return true;
}
