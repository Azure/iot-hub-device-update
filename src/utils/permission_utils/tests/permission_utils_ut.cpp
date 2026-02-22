/**
 * @file permission_utils_ut.cpp
 * @brief Unit Tests for permission_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/permission_utils.h>

#include "aduc/system_utils.h"

#include <catch2/catch_all.hpp>
#include <stdio.h>
#include <string.h>
#include <string>

#include <fstream> // ofstream

#include <aducpal/stdio.h> // remove
#include <aducpal/sys_stat.h> // S_I*
#include <aducpal/grp.h>
#include <aducpal/pwd.h>
#include <aducpal/unistd.h>
#include "aduc/string_c_utils.h" // ADUC_Safe_StrCopyN

// keep this last to avoid interfering with system headers
#include "aduc/aduc_banned.h"

TEST_CASE("PermissionUtils_VerifyFilemodeBit*")
{
    mode_t file_permissions = S_ISUID | S_IRUSR | S_IWUSR | S_IRGRP | S_IWOTH;

    // Windows only has X on folders and .EXE
#if !defined(WIN32)
    file_permissions |= (S_IXUSR | S_IXGRP);
#endif

    // create temp file with all file permission bits set
    char tmpfile_path[30];
    std::string src_str{ "/tmp/permissionUtilsUT_XXXXXX" };
    ADUC_Safe_StrCopyN(tmpfile_path, src_str.c_str(), sizeof(tmpfile_path), src_str.length());
    ADUC_SystemUtils_MkTemp(tmpfile_path);
    std::ofstream file{ tmpfile_path };
    file.close();

    REQUIRE(0 == ADUCPAL_chmod(tmpfile_path, file_permissions));

    CHECK(PermissionUtils_VerifyFilemodeExact(tmpfile_path, file_permissions));

    // Windows doesn't support group.
#if !defined(WIN32)
    CHECK_FALSE(PermissionUtils_VerifyFilemodeExact(tmpfile_path, file_permissions | S_IWGRP /* 04772 */));
#endif

    CHECK(PermissionUtils_VerifyFilemodeBitmask(tmpfile_path, file_permissions));

    // Check some of the bits
    file_permissions = S_IRGRP | S_IWOTH;
#if !defined(WIN32)
    file_permissions |= S_IXUSR;
#endif
    CHECK(PermissionUtils_VerifyFilemodeBitmask(tmpfile_path, file_permissions));

    CHECK_FALSE(PermissionUtils_VerifyFilemodeBitmask(
        tmpfile_path, S_ISUID | S_ISVTX | S_IRWXU | S_IRGRP | S_IXGRP | S_IWOTH /* 05752 */));

    // cleanup
    ADUCPAL_remove(tmpfile_path);
}

TEST_CASE("PermissionUtils user/group/ownership helpers")
{
    struct passwd* currentUser = getpwuid(ADUCPAL_geteuid());
    REQUIRE(currentUser != nullptr);
    struct group* currentGroup = getgrgid(ADUCPAL_getegid());
    REQUIRE(currentGroup != nullptr);

    char tmpfile_path[30];
    std::string src_str{ "/tmp/permissionUtilsUserUT_XXXXXX" };
    ADUC_Safe_StrCopyN(tmpfile_path, src_str.c_str(), sizeof(tmpfile_path), src_str.length());
    ADUC_SystemUtils_MkTemp(tmpfile_path);
    std::ofstream file{ tmpfile_path };
    file << "payload";
    file.close();

    SECTION("UserExists and GroupExists")
    {
        CHECK(PermissionUtils_UserExists(currentUser->pw_name));
        CHECK(PermissionUtils_GroupExists(currentGroup->gr_name));
        CHECK_FALSE(PermissionUtils_UserExists("aduc_nonexistent_user_123456"));
        CHECK_FALSE(PermissionUtils_GroupExists("aduc_nonexistent_group_123456"));
    }

    SECTION("Ownership checks by name and uid/gid")
    {
        CHECK(PermissionUtils_CheckOwnership(tmpfile_path, currentUser->pw_name, nullptr));
        CHECK(PermissionUtils_CheckOwnership(tmpfile_path, nullptr, currentGroup->gr_name));
        CHECK(PermissionUtils_CheckOwnership(tmpfile_path, currentUser->pw_name, currentGroup->gr_name));

        CHECK_FALSE(PermissionUtils_CheckOwnership(tmpfile_path, "aduc_nonexistent_user_123456", nullptr));
        CHECK_FALSE(PermissionUtils_CheckOwnership(tmpfile_path, nullptr, "aduc_nonexistent_group_123456"));

        CHECK(PermissionUtils_CheckOwnerUid(tmpfile_path, currentUser->pw_uid));
        CHECK(PermissionUtils_CheckOwnerGid(tmpfile_path, currentGroup->gr_gid));
        CHECK_FALSE(PermissionUtils_CheckOwnerUid("/tmp/aduc_no_such_file", currentUser->pw_uid));
        CHECK_FALSE(PermissionUtils_CheckOwnerGid("/tmp/aduc_no_such_file", currentGroup->gr_gid));
    }

    SECTION("SetProcessEffective UID/GID validation")
    {
        CHECK(PermissionUtils_SetProcessEffectiveUID(currentUser->pw_name));
        CHECK(PermissionUtils_SetProcessEffectiveGID(currentGroup->gr_name));

        CHECK_FALSE(PermissionUtils_SetProcessEffectiveUID("aduc_nonexistent_user_123456"));
        CHECK_FALSE(PermissionUtils_SetProcessEffectiveGID("aduc_nonexistent_group_123456"));
    }

    SECTION("UserInSupplementaryGroup handles missing group")
    {
        CHECK_FALSE(PermissionUtils_UserInSupplementaryGroup(currentUser->pw_name, "aduc_nonexistent_group_123456"));
    }

    ADUCPAL_remove(tmpfile_path);
}
