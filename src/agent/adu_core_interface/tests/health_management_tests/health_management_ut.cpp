/**
 * @file health_management_ut.cpp
 * @brief Unit Tests for health_management module
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <catch2/catch_all.hpp>
#include <cstring>
#include <string>

extern "C"
{
#include <aduc/adu_types.h>
#include <aduc/config_utils.h>
#include <stdbool.h>
#include <sys/stat.h>

    // Forward declare functions from health_management.c with correct signatures
    bool HealthCheck(const ADUC_LaunchArguments* launchArgs);
    bool IsConnectionInfoValid(const ADUC_LaunchArguments* launchArgs, const ADUC_ConfigInfo* config);

    // Mock control variables
    static bool mock_user_exists = true;
    static bool mock_group_exists = true;
    static bool mock_user_in_group = true;
    static bool mock_check_ownership = true;
    static bool mock_verify_filemode_exact = true;
    static bool mock_verify_filemode_bitmask = true;
    static bool mock_check_owner_uid = true;
    static bool mock_check_owner_gid = true;
    static bool mock_is_dir = true;
    static bool mock_is_file = true;
    static int mock_is_dir_err = 0;
    static bool mock_configInfo_available = true;
    static ADUC_ConfigInfo mock_configInfo;
    static ADUC_AgentInfo mock_agentInfo;
    static bool mock_connection_info_valid = true;

    // Mock permission utils
    bool PermissionUtils_UserExists(const char* user)
    {
        (void)user;
        return mock_user_exists;
    }

    bool PermissionUtils_GroupExists(const char* group)
    {
        (void)group;
        return mock_group_exists;
    }

    bool PermissionUtils_UserInSupplementaryGroup(const char* user, const char* group)
    {
        (void)user;
        (void)group;
        return mock_user_in_group;
    }

    bool PermissionUtils_CheckOwnership(const char* path, const char* expectedUser, const char* expectedGroup)
    {
        (void)path;
        (void)expectedUser;
        (void)expectedGroup;
        return mock_check_ownership;
    }

    bool PermissionUtils_VerifyFilemodeExact(const char* path, mode_t expectedPermissions)
    {
        (void)path;
        (void)expectedPermissions;
        return mock_verify_filemode_exact;
    }

    bool PermissionUtils_VerifyFilemodeBitmask(const char* path, mode_t bitmask)
    {
        (void)path;
        (void)bitmask;
        return mock_verify_filemode_bitmask;
    }

    bool PermissionUtils_CheckOwnerUid(const char* path, uid_t uid)
    {
        (void)path;
        (void)uid;
        return mock_check_owner_uid;
    }

    bool PermissionUtils_CheckOwnerGid(const char* path, gid_t gid)
    {
        (void)path;
        (void)gid;
        return mock_check_owner_gid;
    }

    bool PermissionUtils_SetProcessEffectiveGID(const char* groupName)
    {
        (void)groupName;
        return true;
    }

    bool PermissionUtils_SetProcessEffectiveUID(const char* userName)
    {
        (void)userName;
        return true;
    }

    // Mock system utils
    bool SystemUtils_IsDir(const char* path, int* err)
    {
        (void)path;
        if (err != NULL)
        {
            *err = mock_is_dir_err;
        }
        return mock_is_dir;
    }

    bool SystemUtils_IsFile(const char* path, int* err)
    {
        (void)path;
        if (err != NULL)
        {
            *err = 0;
        }
        return mock_is_file;
    }

    // Mock config utils
    const ADUC_ConfigInfo* ADUC_ConfigInfo_GetInstance()
    {
        if (!mock_configInfo_available)
        {
            return NULL;
        }
        return &mock_configInfo;
    }

    int ADUC_ConfigInfo_ReleaseInstance(const ADUC_ConfigInfo* configInfo)
    {
        (void)configInfo;
        return 0;
    }

    const ADUC_AgentInfo* ADUC_ConfigInfo_GetAgent(const ADUC_ConfigInfo* config, unsigned int index)
    {
        (void)config;
        (void)index;
        return &mock_agentInfo;
    }

    // Mock connection info functions
    bool GetConnectionInfoFromIdentityService(ADUC_ConnectionInfo* info)
    {
        (void)info;
        return mock_connection_info_valid;
    }

    bool GetConnectionInfoFromConnectionString(
        ADUC_ConnectionInfo* info,
        const char* connectionString,
        const char* const x509Cert,
        const char* const x509PrivateKey,
        const char* const opensslEngine,
        const char* const x509CaCert)
    {
        (void)info;
        (void)connectionString;
        (void)x509Cert;
        (void)x509PrivateKey;
        (void)opensslEngine;
        (void)x509CaCert;
        return mock_connection_info_valid;
    }

    void ADUC_ConnectionInfo_DeAlloc(ADUC_ConnectionInfo* info)
    {
        (void)info;
    }

// Undefine logging macros so we can provide mock function implementations
#undef log_debug
#undef log_info
#undef log_warn
#undef log_error
#undef Log_Debug
#undef Log_Info
#undef Log_Warn
#undef Log_Error

    // Mock logging functions
    void Log_Error(const char* fmt, ...)
    {
        (void)fmt;
    }
    void Log_Warn(const char* fmt, ...)
    {
        (void)fmt;
    }
    void Log_Info(const char* fmt, ...)
    {
        (void)fmt;
    }
    void Log_Debug(const char* fmt, ...)
    {
        (void)fmt;
    }
}

class HealthManagementTestFixture
{
public:
    HealthManagementTestFixture()
    {
        // Reset all mocks to success/true
        mock_user_exists = true;
        mock_group_exists = true;
        mock_user_in_group = true;
        mock_check_ownership = true;
        mock_verify_filemode_exact = true;
        mock_verify_filemode_bitmask = true;
        mock_check_owner_uid = true;
        mock_check_owner_gid = true;
        mock_is_dir = true;
        mock_is_file = true;
        mock_is_dir_err = 0;
        mock_configInfo_available = true;
        mock_connection_info_valid = true;

        memset(&mock_configInfo, 0, sizeof(mock_configInfo));
        memset(&mock_agentInfo, 0, sizeof(mock_agentInfo));
        memset(&launchArgs, 0, sizeof(launchArgs));

        // Set up default config values
        mock_configInfo.aduShellFilePath = strdup("/usr/bin/adu-shell");
        mock_configInfo.downloadsFolder = "/var/lib/adu/downloads";
        mock_agentInfo.connectionType = strdup("string");
        mock_agentInfo.connectionData = strdup("HostName=test.azure-devices.net;DeviceId=test;SharedAccessKey=test");
    }

    ~HealthManagementTestFixture()
    {
        free(mock_configInfo.aduShellFilePath);
        mock_configInfo.aduShellFilePath = NULL;
        free(mock_agentInfo.connectionType);
        mock_agentInfo.connectionType = NULL;
        free(mock_agentInfo.connectionData);
        mock_agentInfo.connectionData = NULL;
    }

protected:
    ADUC_LaunchArguments launchArgs;
};

TEST_CASE_METHOD(HealthManagementTestFixture, "HealthCheck", "[health_management]")
{
    SECTION("Returns true when everything is healthy")
    {
        bool result = HealthCheck(&launchArgs);
        REQUIRE(result == true);
    }

    SECTION("Returns false when ConfigInfo is not available")
    {
        mock_configInfo_available = false;
        bool result = HealthCheck(&launchArgs);
        REQUIRE(result == false);
    }

    SECTION("Returns false when connection info is invalid")
    {
        mock_connection_info_valid = false;
        bool result = HealthCheck(&launchArgs);
        REQUIRE(result == false);
    }

    SECTION("Returns false when required users don't exist")
    {
        mock_user_exists = false;
        bool result = HealthCheck(&launchArgs);
        REQUIRE(result == false);
    }

    SECTION("Returns false when required groups don't exist")
    {
        mock_group_exists = false;
        bool result = HealthCheck(&launchArgs);
        REQUIRE(result == false);
    }

    SECTION("Returns false when dir ownership check fails")
    {
        mock_check_ownership = false;
        bool result = HealthCheck(&launchArgs);
        REQUIRE(result == false);
    }

    SECTION("Returns false when dir permissions check fails")
    {
        mock_verify_filemode_exact = false;
        bool result = HealthCheck(&launchArgs);
        REQUIRE(result == false);
    }

    SECTION("Returns false when directory does not exist")
    {
        mock_is_dir = false;
        bool result = HealthCheck(&launchArgs);
        REQUIRE(result == false);
    }

    SECTION("Returns false when file does not exist")
    {
        mock_is_file = false;
        bool result = HealthCheck(&launchArgs);
        REQUIRE(result == false);
    }

    SECTION("Returns false when UID check fails")
    {
        mock_check_owner_uid = false;
        bool result = HealthCheck(&launchArgs);
        REQUIRE(result == false);
    }

    SECTION("Returns false when filemode bitmask check fails")
    {
        mock_verify_filemode_bitmask = false;
        bool result = HealthCheck(&launchArgs);
        REQUIRE(result == false);
    }
}

TEST_CASE_METHOD(HealthManagementTestFixture, "IsConnectionInfoValid", "[health_management]")
{
    SECTION("Returns true when connection string is provided via launch args")
    {
        launchArgs.connectionString = strdup("HostName=test.azure-devices.net;DeviceId=test;SharedAccessKey=test");
        bool result = IsConnectionInfoValid(&launchArgs, &mock_configInfo);
        REQUIRE(result == true);
        free(launchArgs.connectionString);
        launchArgs.connectionString = NULL;
    }

    SECTION("Returns true when connection type is 'string'")
    {
        bool result = IsConnectionInfoValid(&launchArgs, &mock_configInfo);
        REQUIRE(result == true);
    }

    SECTION("Returns true when connection type is 'AIS'")
    {
        free(mock_agentInfo.connectionType);
        mock_agentInfo.connectionType = strdup("AIS");
        bool result = IsConnectionInfoValid(&launchArgs, &mock_configInfo);
        REQUIRE(result == true);
    }

    SECTION("Returns true when connection type is 'X509'")
    {
        free(mock_agentInfo.connectionType);
        mock_agentInfo.connectionType = strdup("X509");
        mock_agentInfo.x509Cert = NULL;
        mock_agentInfo.x509PrivateKey = NULL;
        mock_agentInfo.opensslEngine = NULL;
        mock_agentInfo.x509CaCert = NULL;
        bool result = IsConnectionInfoValid(&launchArgs, &mock_configInfo);
        REQUIRE(result == true);
    }

    SECTION("Returns false when connection type is unsupported")
    {
        free(mock_agentInfo.connectionType);
        mock_agentInfo.connectionType = strdup("unsupported");
        bool result = IsConnectionInfoValid(&launchArgs, &mock_configInfo);
        REQUIRE(result == false);
    }

    SECTION("Returns false when connection info retrieval fails")
    {
        mock_connection_info_valid = false;
        bool result = IsConnectionInfoValid(&launchArgs, &mock_configInfo);
        REQUIRE(result == false);
    }
}
