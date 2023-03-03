/**
 * @file permission_utils.c
 * @brief Implements utilities for working with user, group, and file permissions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/permission_utils.h"
#include "aduc/bit_ops.h" // for BitOps_AreAllBitsSet
#include <grp.h> // for getgrnam
#include <pwd.h> // for getpwnam
#include <string.h> // for stat, etc.
#include <sys/stat.h> // for stat, etc.
#include <unistd.h> // for access

//
// Internal functions
//

/**
 * @brief Checks the file mode bits on the file object.
 * @remark When isExactMatch is false, expectedPermissions is used as a bit mask (other bits could also be set).
 * @param path The path to the file object.
 * @param expectedPermissions The expected permissions, or permissions mask.
 * @param isExactMatch true to do exact match on permission bits of file; else, use expectedPermissions as a bit mask.
 * @returns true if file's permissions matched according to isExactMatch.
 */
static bool PermissionUtils_VerifyFilemodeBits(const char* path, mode_t expectedPermissions, bool isExactMatch)
{
    struct stat st;
    if (stat(path, &st) != 0)
    {
        return false;
    }

    // keep just the file permission bits (as opposed to file_type bits).
    mode_t permissionBits = st.st_mode & ~S_IFMT;

    return isExactMatch ? (permissionBits == expectedPermissions)
                        : BitOps_AreAllBitsSet(permissionBits, expectedPermissions);
}

//
// Externally visible functions
//

/**
 * @brief Checks the file mode bits on the file object are equal to the provided permissions bits.
 * @param path The path to the file object.
 * @param expectedPermissions The expected permissions of the file object.
 * @returns true if file's permission bits are equal.
 */
bool PermissionUtils_VerifyFilemodeExact(const char* path, mode_t expectedPermissions)
{
    return PermissionUtils_VerifyFilemodeBits(path, expectedPermissions, true /* isExactMatch */);
}

/**
 * @brief Checks the file mode bits on the file object have all high bits set for each high bit in the mask.
 * @param path The path to the file object.
 * @param bitmask The permissions bit mask.
 * @returns true if file's permissions bits have high bits for all corresponding high bits in the mask.
 */
bool PermissionUtils_VerifyFilemodeBitmask(const char* path, mode_t bitmask)
{
    return PermissionUtils_VerifyFilemodeBits(path, bitmask, false /* isExactMatch */);
}

/**
 * @brief Checks if user exists.
 * @param user The case-sensitive user.
 * @returns true if user exists.
 */
bool PermissionUtils_UserExists(const char* user)
{
    return getpwnam(user) != NULL;
}

/**
 * @brief Checks if group exists.
 * @param group The case-sensitive group.
 * @returns true if group exists.
 */
bool PermissionUtils_GroupExists(const char* group)
{
    return getgrnam(group) != NULL;
}

/**
 * @brief Checks if user is a user member entry in the /etc/groups DB.
 * @param user The case-sensitive user.
 * @param group The case-sensitive group.
 * @returns true if user was found to be a member of group.
 */
bool PermissionUtils_UserInSupplementaryGroup(const char* user, const char* group)
{
    bool result = false;

    struct group* groupEntry = getgrnam(group);
    if (groupEntry != NULL && groupEntry->gr_mem != NULL)
    {
        for (int i = 0; groupEntry->gr_mem[i] != NULL; ++i)
        {
            if (strcmp(groupEntry->gr_mem[i], user) == 0)
            {
                result = true;
                break;
            }
        }
    }

    return result;
}

/**
 * @brief Checks the user and/or group ownership on a file.
 * @param path The path of the file object.
 * @param user The expected user on the file, or NULL to opt out of checking user.
 * @param group The expected group on the file, or NULL to opt out of checking group.
 * @returns true if it is owned by user.
 */
bool PermissionUtils_CheckOwnership(const char* path, const char* user, const char* group)
{
    bool result = true;

    struct stat st;
    if (stat(path, &st) != 0)
    {
        return false;
    }

    if (user != NULL)
    {
        const struct passwd* pwd = getpwnam(user);
        if (pwd == NULL)
        {
            return false;
        }

        result = result && (st.st_uid == pwd->pw_uid);
    }

    if (group != NULL)
    {
        const struct group* grp = getgrnam(group);
        if (grp == NULL)
        {
            return false;
        }

        result = result && (st.st_gid == grp->gr_gid);
    }

    return result;
}

/**
 * @brief Checks that owner uid of the file object is the given uid.
 * @param path The path to the file object.
 * @returns true if the given uid is the owning uid of the file.
 */
bool PermissionUtils_CheckOwnerUid(const char* path, uid_t uid)
{
    struct stat st;
    if (stat(path, &st) != 0)
    {
        return false;
    }

    return st.st_uid == uid;
}

/**
 * @brief Checks that owner gid of the file object is the given gid.
 * @param path The path to the file object.
 * @returns true if the given gid is the owning gid of the file.
 */
bool PermissionUtils_CheckOwnerGid(const char* path, gid_t gid)
{
    struct stat st;
    if (stat(path, &st) != 0)
    {
        return false;
    }

    return st.st_gid == gid;
}

/**
 * @brief Set effective user of the calling process.
 * 
 * @param name The username
 * @return bool Returns true if user @p name exist and the effective user successfully set.
 *  If failed, additional error is stored in errno.
 */
bool PermissionUtils_SetProcessEffectiveUID(const char* name)
{
    struct passwd* p = getpwnam(name);
    return (p != NULL && seteuid(p->pw_uid) == 0);
}

/**
 * @brief Set effective group of the calling process.
 * 
 * @param name The username
 * @return bool Returns true if group @p name exist and the effective group successfully set.
 * If failed, additional error is stored in errno.
 */
bool PermissionUtils_SetProcessEffectiveGID(const char* name)
{
    struct group* grp = getgrnam(name);
    return (grp != NULL && setegid(grp->gr_gid) == 0);
}
