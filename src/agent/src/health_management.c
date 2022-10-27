/**
 * @file health_management.c
 * @brief Implements functions that determine whether ADU Agent can function properly.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/health_management.h"
#include "aduc/config_utils.h"
#include "aduc/logging.h"
#include "aduc/permission_utils.h" // for PermissionUtils_*
#include "aduc/string_c_utils.h"
#include "aduc/system_utils.h" // for SystemUtils_IsDir, SystemUtils_IsFile
#include <azure_c_shared_utility/strings.h> // for STRING_HANDLE, STRING_delete, STRING_c_str
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

/**
 * @brief The users that must exist on the system.
 */
static const char* aduc_required_users[] = { ADUC_FILE_USER };

/**
 * @brief The optional users.
 */
static const char* aduc_optional_users[] = { DO_FILE_USER };

/**
 * @brief The groups that must exist on the system.
 */
static const char* aduc_required_groups[] = { ADUC_FILE_GROUP };

/**
 * @brief The optional groups.
 */
static const char* aduc_optional_groups[] = { DO_FILE_GROUP };

/**
 * @brief The supplementary groups for ADUC_FILE_USER
 */
static const char* aduc_required_group_memberships[] = {};

/**
 * @brief The supplementary groups for ADUC_FILE_USER
 */
static const char* aduc_optional_group_memberships[] = {
    DO_FILE_GROUP // allows agent to set connection_string for DO
};

/**
 * @brief Get the Connection Info from Identity Service
 *
 * @return true if connection info can be obtained
 */
_Bool GetConnectionInfoFromIdentityService(ADUC_ConnectionInfo* info);

/**
 * @brief Get the Connection Info from connection string, if a connection string is provided in configuration file
 *
 * @return true if connection info can be obtained
 */
_Bool GetConnectionInfoFromConnectionString(ADUC_ConnectionInfo* info, const char* connectionString);

/**
 * @brief Checks whether we can obtain a device or module connection string.
 *
 * @return true if connection string can be obtained.
 */
_Bool IsConnectionInfoValid(const ADUC_LaunchArguments* launchArgs, ADUC_ConfigInfo* config)
{
    _Bool validInfo = false;

    ADUC_ConnectionInfo info = {};

    if (launchArgs->connectionString != NULL)
    {
        validInfo = true;
        goto done;
    }

    const ADUC_AgentInfo* agent = ADUC_ConfigInfo_GetAgent(config, 0);
    if (agent == NULL)
    {
        Log_Error("ADUC_ConfigInfo_GetAgent failed to get the agent information.");
        goto done;
    }
    if (strcmp(agent->connectionType, "AIS") == 0)
    {
        validInfo = GetConnectionInfoFromIdentityService(&info);
    }
    else if (strcmp(agent->connectionType, "string") == 0)
    {
        validInfo = GetConnectionInfoFromConnectionString(&info, agent->connectionData);
    }
    else
    {
        Log_Error("The connection type %s is not supported", agent->connectionType);
    }

done:
    ADUC_ConnectionInfo_DeAlloc(&info);
    return validInfo;
}

/**
 * @brief Helper function for simulating an unhealthy state.
 *
 * @return true if an ADU configuration file contains simulateUnhealthyState value (any value).
 */
_Bool IsSimulatingUnhealthyState(ADUC_ConfigInfo* config)
{
    return config->simulateUnhealthyState;
}

/**
 * @brief Reports which required users do not exist.
 * @remark Goes through the whole list of users to trace any that are missing.
 * @return true if all necessary users exist, or false if any do not exist.
 */
static _Bool ReportMissingRequiredUsers()
{
    _Bool result = true;

    for (int i = 0; i < ARRAY_SIZE(aduc_required_users); ++i)
    {
        if (!PermissionUtils_UserExists(aduc_required_users[i]))
        {
            Log_Error("Required user '%s' does not exist.", aduc_required_users[i]);
            result = false;
            // continue on to next user
        }
    }

    for (int i = 0; i < ARRAY_SIZE(aduc_optional_users); ++i)
    {
        if (!PermissionUtils_UserExists(aduc_optional_users[i]))
        {
            Log_Warn("Optional user '%s' does not exist.", aduc_optional_users[i]);
            // continue on to next user
        }
    }

    return result;
}

/**
 * @brief Reports which required groups do not exist.
 * @remark Goes through the whole list of groups to trace any that are missing.
 * @return true if necessary groups exist.
 */
static _Bool ReportMissingRequiredGroups()
{
    _Bool result = true;

    for (int i = 0; i < ARRAY_SIZE(aduc_required_groups); ++i)
    {
        if (!PermissionUtils_GroupExists(aduc_required_groups[i]))
        {
            Log_Error("Required group '%s' does not exist.", aduc_required_groups[i]);
            result = false;
            // continue on to next user
        }
    }

    for (int i = 0; i < ARRAY_SIZE(aduc_optional_groups); ++i)
    {
        if (!PermissionUtils_GroupExists(aduc_optional_groups[i]))
        {
            Log_Warn("Optional group '%s' does not exist.", aduc_optional_groups[i]);
            // continue on to next user
        }
    }

    return result;
}

/**
 * @brief Reports on any missing group memberships.
 * @remark Goes through all required user/group relationships and traces any that are missing.
 * @returns true if all necessary group membership entries exist, or false if any are missing.
 */
static _Bool ReportMissingGroupMemberships()
{
    _Bool result = true;

    // ADUC group memberships
    for (int i = 0; i < ARRAY_SIZE(aduc_required_group_memberships); ++i)
    {
        if (!PermissionUtils_UserInSupplementaryGroup(ADUC_FILE_USER, aduc_required_group_memberships[i]))
        {
            Log_Error("User '%s' is not a member of '%s' group.", ADUC_FILE_USER, aduc_required_group_memberships[i]);
            result = false;
            // continue evaluating next membership
        }
    }

    // ADUC group memberships
    for (int i = 0; i < ARRAY_SIZE(aduc_optional_group_memberships); ++i)
    {
        if (!PermissionUtils_UserInSupplementaryGroup(ADUC_FILE_USER, aduc_optional_group_memberships[i]))
        {
            Log_Warn("User '%s' is not a member of '%s' group.", ADUC_FILE_USER, aduc_optional_group_memberships[i]);
            // continue evaluating next membership
        }
    }

    // DO group memberships
    if (!PermissionUtils_UserInSupplementaryGroup(DO_FILE_USER, ADUC_FILE_GROUP))
    {
        Log_Warn("User '%s' is not a member of '%s' group.", DO_FILE_USER, ADUC_FILE_GROUP);
    }

    return result;
}

/**
 * @brief Reports on necessary user and group entries.
 * @return true if all necessary entries exist, or false if any are missing.
 */
static _Bool ReportUserAndGroupRequirements()
{
    _Bool result = false;

    // Call both to get logging side-effects.
    _Bool missingUser = !ReportMissingRequiredUsers();
    _Bool missingGroup = !ReportMissingRequiredGroups();
    if (missingUser || missingGroup)
    {
        // skip reporting group memberships if any user/groups are missing
        goto done;
    }

    if (!ReportMissingGroupMemberships())
    {
        goto done;
    }

    result = true;

done:

    return result;
}

/**
 * @brief Checks the conf directory has correct ownerships and permissions and logs when an issue is found.
 * @returns true if everything is correct.
 */
static _Bool CheckConfDirOwnershipAndPermissions()
{
    _Bool result = true;

    const char* path = ADUC_CONF_FOLDER;

    if (SystemUtils_IsDir(path, NULL))
    {
        if (!PermissionUtils_CheckOwnership(path, ADUC_FILE_USER, ADUC_FILE_GROUP))
        {
            Log_Error("'%s' has incorrect ownership.", path);
            result = false;
            // continue to allow tracing other related issues
        }

        // owning user can read, write, and list entries in dir.
        // group members can read and list entries in dir.
        const mode_t expected_permissions = S_IRWXU | S_IRGRP | S_IXGRP;

        if (!PermissionUtils_VerifyFilemodeExact(path, expected_permissions))
        {
            Log_Error(
                "Lookup failed or '%s' directory has incorrect permissions (expected: 0%o)",
                path,
                expected_permissions);
            result = false;
        }
    }
    else
    {
        Log_Error("'%s' does not exist or not a directory.", path);
        result = false;
    }

    return result;
}

/**
 * @brief Checks the conf file ownerships and permissions and logs issues when found.
 * @returns true if everything is correct.
 */
static _Bool CheckConfFile()
{
    _Bool result = true;

    const char* path = ADUC_CONF_FILE_PATH;

    if (SystemUtils_IsFile(path, NULL))
    {
        if (!PermissionUtils_CheckOwnership(path, ADUC_FILE_USER, ADUC_FILE_GROUP))
        {
            Log_Error("'%s' has incorrect ownership (expected: %s:%s)", path, ADUC_FILE_USER, ADUC_FILE_GROUP);
            result = false;
        }

        const mode_t bitmask = S_IRUSR | S_IRGRP;

        if (!PermissionUtils_VerifyFilemodeBitmask(path, bitmask))
        {
            Log_Error("Lookup failed or '%s' has incorrect permissions (bitmask: 0%o)", path, bitmask);
            result = false;
        }
    }
    else
    {
        Log_Error("'%s' does not exist or is not a file.", path);
        result = false;
    }

    return result;
}

/**
 * @brief Checks the log directory ownerships and permissions.
 * @returns true if everything is correct.
 */
static _Bool CheckLogDir()
{
    _Bool result = false;

    const char* dir = ADUC_LOG_FOLDER;

    int err;
    if (!SystemUtils_IsDir(dir, &err))
    {
        if (err != 0)
        {
            Log_Error("Cannot get '%s' status. (errno: %d)", dir, errno);
            goto done;
        }
        Log_Error("'%s' is not a directory", dir);
        goto done;
    }

    if (!PermissionUtils_CheckOwnership(dir, ADUC_FILE_USER, ADUC_FILE_GROUP))
    {
        Log_Error("'%s' has incorrect ownership (expected: %s:%s)", dir, ADUC_FILE_USER, ADUC_FILE_GROUP);
        goto done;
    }

    const mode_t bitmask = S_IRWXU | S_IRGRP | S_IXGRP;

    if (!PermissionUtils_VerifyFilemodeBitmask(dir, bitmask))
    {
        Log_Error("Lookup failed or '%s' has incorrect permissions (expected: 0%o)", dir, bitmask);
        goto done;
    }

    result = true;

done:

    return result;
}

/**
 * @brief Checks the user and/or group ownership on a file.
 * @param path The path of the file object.
 * @param user The expected user on the file, or NULL to opt out of checking user.
 * @param group The expected group on the file, or NULL to opt out of checking group.
 * @param expectedPermissions The expected permissions of the file object.
 * @returns true if everything is correct.
 */
static _Bool CheckDirOwnershipAndVerifyFilemodeExact(
    const char* path, const char* user, const char* group, const mode_t expected_permissions)
{
    _Bool result = false;

    int err;
    if (!SystemUtils_IsDir(path, &err))
    {
        if (err != 0)
        {
            Log_Error("Cannot get '%s' status. (errno: %d)", path, errno);
            goto done;
        }
        Log_Error("'%s' is not a directory", path);
        goto done;
    }

    if (!PermissionUtils_CheckOwnership(path, user, group))
    {
        Log_Error("'%s' has incorrect ownership (expected: %s:%s)", path, user, group);
        goto done;
    }

    if (!PermissionUtils_VerifyFilemodeExact(path, expected_permissions))
    {
        Log_Error("Lookup failed or '%s' has incorrect permissions (expected: 0%o)", path, expected_permissions);
        goto done;
    }

    result = true;

done:

    return result;
}

/**
 * @brief Checks the data directory ownerships and permissions.
 * @returns true if everything is correct.
 */
static _Bool CheckDataDir()
{
    // Note: "Other" bits are cleared to align with ADUC_SystemUtils_MkDirRecursiveDefault and packaging.
    const mode_t expected_permissions = S_IRWXU | S_IRWXG;
    return CheckDirOwnershipAndVerifyFilemodeExact(
        ADUC_DATA_FOLDER, ADUC_FILE_USER, ADUC_FILE_GROUP, expected_permissions);
}

/**
 * @brief Checks the downloads directory ownerships and permissions.
 * @returns true if everything is correct.
 */
static _Bool CheckDownloadsDir()
{
    // Note: "Other" bits are cleared to align with ADUC_SystemUtils_MkDirRecursiveDefault and packaging.
    const mode_t expected_permissions = S_IRWXU | S_IRWXG;
    return CheckDirOwnershipAndVerifyFilemodeExact(
        ADUC_DOWNLOADS_FOLDER, ADUC_FILE_USER, ADUC_FILE_GROUP, expected_permissions);
}

/**
 * @brief Checks the agent binary ownerships and permissions.
 * @returns true if everything is correct.
 */
static _Bool CheckAgentBinary()
{
    _Bool result = false;

    const char* path = ADUC_AGENT_FILEPATH;

    if (SystemUtils_IsFile(path, NULL))
    {
        if (!PermissionUtils_CheckOwnerUid(path, 0 /* root */))
        {
            Log_Error("'%s' has incorrect UID.", path);
        }

        if (!PermissionUtils_CheckOwnerGid(path, 0 /* root */))
        {
            Log_Error("'%s' has incorrect GID.", path);
            goto done;
        }

        const mode_t expected_permissions = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

        if (!PermissionUtils_VerifyFilemodeExact(path, expected_permissions))
        {
            Log_Error("Lookup failed or '%s' has incorrect permissions (expected: 0%o)", path, expected_permissions);
            goto done;
        }
    }

    result = true;

done:

    return result;
}

/**
 * @brief Checks the adu-shell binary ownerships and permissions.
 * @returns true if everything is correct.
 */
static _Bool CheckShellBinary()
{
    _Bool result = false;

    const char* path = ADUSHELL_FILE_PATH;

    if (SystemUtils_IsFile(path, NULL))
    {
        if (!PermissionUtils_CheckOwnerUid(path, 0 /* root */))
        {
            Log_Error("'%s' has incorrect UID.", path);
            goto done;
        }

        if (!PermissionUtils_CheckOwnership(path, NULL /* user */, ADUC_FILE_GROUP))
        {
            Log_Error("'%s' has incorrect group owner.", path);
            goto done;
        }

        // Needs set-uid, user read, and group read + execute.
        // Note: other has no permission bits set.
        const mode_t expected_permissions = S_ISUID | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP;

        if (!PermissionUtils_VerifyFilemodeExact(path, expected_permissions))
        {
            Log_Error("Lookup failed or '%s' has incorrect permissions (expected: 0%o)", path, expected_permissions);
            goto done;
        }
    }
    else
    {
        Log_Error("'%s' does not exist or not a file", ADUSHELL_FILE_PATH);
        goto done;
    }

    result = true;

done:

    return result;
}

/**
 * @brief Helper function for checking correct ownership and permissions for dirs and files.
 * @return true if dirs and files have correct ownership and permissions.
 */
static _Bool AreDirAndFilePermissionsValid()
{
    _Bool result = true;

    if (!ReportUserAndGroupRequirements())
    {
        result = false; // continue
    }

    if (!CheckConfDirOwnershipAndPermissions())
    {
        result = false; // continue
    }

    if (!CheckConfFile())
    {
        result = false; // continue
    }

    if (!CheckLogDir())
    {
        result = false; // continue
    }

    if (!CheckDataDir())
    {
        result = false; // continue
    }

    if (!CheckDownloadsDir())
    {
        result = false; // continue
    }

    if (!CheckAgentBinary())
    {
        result = false; // continue
    }

    if (!CheckShellBinary())
    {
        result = false; // continue
    }

    return result;
}

/**
 * @brief Performs necessary checks to determine whether ADU Agent can function properly.
 *
 * Currently, we are performing the following:
 *     - Implicitly check that agent process launched successfully.
 *     - Check that we can obtain the connection info.
 *
 * @return true if all checks passed.
 */
_Bool HealthCheck(const ADUC_LaunchArguments* launchArgs)
{
    _Bool isHealthy = false;

    ADUC_ConfigInfo config = {};
    if (!ADUC_ConfigInfo_Init(&config, ADUC_CONF_FILE_PATH))
    {
        Log_Error("Failed to initialize from config file: %s", ADUC_CONF_FILE_PATH);
        goto done;
    }

    if (!IsConnectionInfoValid(launchArgs, &config))
    {
        Log_Error("Invalid connection info.");
        goto done;
    }

    if (!AreDirAndFilePermissionsValid())
    {
        goto done;
    }

#ifdef ADUC_PLATFORM_SIMULATOR
    if (IsSimulatingUnhealthyState(&config))
    {
        Log_Error("Simulating an unhealthy state.");
        goto done;
    }
#endif

    isHealthy = true;

done:
    Log_Info("Health check %s.", isHealthy ? "passed" : "failed");
    ADUC_ConfigInfo_UnInit(&config);

    return isHealthy;
}
