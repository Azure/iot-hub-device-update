/**
 * @file process_identity.c
 * @brief Run-as-user support for child processes.
 */

#include "aduc/process_identity.h"

#include <errno.h>
#include <string.h>

#ifndef _WIN32
#include <pwd.h>
#include <unistd.h>
#endif

ADUC_Result2 ADUC_ProcessIdentity_Resolve(const char* username, ADUC_ProcessIdentity* outIdentity)
{
    if (username == NULL || outIdentity == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 1);
    }

    memset(outIdentity, 0, sizeof(*outIdentity));

#ifdef _WIN32
    /* On Windows, just store the username; uid/gid are not meaningful */
    size_t len = strlen(username);
    if (len >= sizeof(outIdentity->username))
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 3);
    }
    memcpy(outIdentity->username, username, len + 1);
    outIdentity->uid = 0;
    outIdentity->gid = 0;
    outIdentity->resolved = true;
    return ADUC_RESULT2_SUCCESS;
#else
    struct passwd* pw = getpwnam(username);
    if (pw == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 2);
    }

    size_t len = strlen(username);
    if (len >= sizeof(outIdentity->username))
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 3);
    }

    memcpy(outIdentity->username, username, len + 1);
    outIdentity->uid = pw->pw_uid;
    outIdentity->gid = pw->pw_gid;
    outIdentity->resolved = true;

    return ADUC_RESULT2_SUCCESS;
#endif
}

bool ADUC_ProcessIdentity_IsAllowed(const char* username, const char** allowlist, size_t allowlistCount)
{
    if (username == NULL || allowlist == NULL)
    {
        return false;
    }

    for (size_t i = 0; i < allowlistCount; i++)
    {
        if (allowlist[i] != NULL && strcmp(username, allowlist[i]) == 0)
        {
            return true;
        }
    }

    return false;
}

ADUC_Result2 ADUC_ProcessIdentity_GetCurrent(ADUC_ProcessIdentity* outIdentity)
{
    if (outIdentity == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 1);
    }

    memset(outIdentity, 0, sizeof(*outIdentity));

#ifdef _WIN32
    /* On Windows, return a placeholder identity */
    const char* user = "SYSTEM";
    memcpy(outIdentity->username, user, strlen(user) + 1);
    outIdentity->uid = 0;
    outIdentity->gid = 0;
    outIdentity->resolved = true;
    return ADUC_RESULT2_SUCCESS;
#else
    uid_t uid = getuid();
    struct passwd* pw = getpwuid(uid);
    if (pw == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_AGENT, ADUC_CATEGORY_RESOURCE, 2);
    }

    size_t len = strlen(pw->pw_name);
    if (len >= sizeof(outIdentity->username))
    {
        len = sizeof(outIdentity->username) - 1;
    }
    memcpy(outIdentity->username, pw->pw_name, len);
    outIdentity->username[len] = '\0';
    outIdentity->uid = pw->pw_uid;
    outIdentity->gid = pw->pw_gid;
    outIdentity->resolved = true;

    return ADUC_RESULT2_SUCCESS;
#endif
}

bool ADUC_ProcessIdentity_CanSwitchTo(const ADUC_ProcessIdentity* target)
{
    if (target == NULL || !target->resolved)
    {
        return false;
    }

#ifdef _WIN32
    /* On Windows, identity switching is not supported via this mechanism */
    return false;
#else
    // Only root can switch to another user
    if (geteuid() == 0)
    {
        return true;
    }

    // Non-root can only "switch" to themselves
    return (geteuid() == target->uid);
#endif
}
