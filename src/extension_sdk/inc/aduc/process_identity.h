#ifndef ADUC_PROCESS_IDENTITY_H
#define ADUC_PROCESS_IDENTITY_H

#include "aduc/extension_types.h"
#include <stdbool.h>

#ifdef _WIN32
typedef unsigned int uid_t;
typedef unsigned int gid_t;
#else
#include <sys/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ADUC_ProcessIdentity {
    char username[64];
    uid_t uid;
    gid_t gid;
    bool resolved;     // true if uid/gid have been looked up
} ADUC_ProcessIdentity;

// Resolve username to uid/gid
ADUC_Result2 ADUC_ProcessIdentity_Resolve(const char* username, ADUC_ProcessIdentity* outIdentity);
// Check if a username is in the allowlist
bool ADUC_ProcessIdentity_IsAllowed(const char* username, const char** allowlist, size_t allowlistCount);
// Get current process identity
ADUC_Result2 ADUC_ProcessIdentity_GetCurrent(ADUC_ProcessIdentity* outIdentity);
// Validate that we have permission to switch to target identity
bool ADUC_ProcessIdentity_CanSwitchTo(const ADUC_ProcessIdentity* target);

#ifdef __cplusplus
}
#endif
#endif
