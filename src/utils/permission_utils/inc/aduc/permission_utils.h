/**
 * @file permission_utils.h
 * @brief Utilities for working with user, group, and file permissions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef _PERMISSION_UTILS_H_
#define _PERMISSION_UTILS_H_

#include <aduc/c_utils.h> // for EXTERN_C_*
#include <stdbool.h> // for bool
#include <sys/types.h> // for uid_t, gid_t, mode_t

EXTERN_C_BEGIN

bool PermissionUtils_VerifyFilemodeExact(const char* path, mode_t expectedPermissions);
bool PermissionUtils_VerifyFilemodeBitmask(const char* path, mode_t bitmask);
bool PermissionUtils_UserExists(const char* user);
bool PermissionUtils_GroupExists(const char* group);
bool PermissionUtils_UserInSupplementaryGroup(const char* user, const char* group);
bool PermissionUtils_CheckOwnership(const char* path, const char* user, const char* group);
bool PermissionUtils_CheckOwnerUid(const char* path, uid_t uid);
bool PermissionUtils_CheckOwnerGid(const char* path, gid_t gid);
bool PermissionUtils_SetProcessEffectiveUID(const char* name);
bool PermissionUtils_SetProcessEffectiveGID(const char* name);

EXTERN_C_END

#endif // PERMISSION_UTILS_H
