/**
 * @file permission_utils_ut.cpp
 * @brief Unit Tests for permission_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/permission_utils.h>
#include <catch2/catch.hpp>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

//bool PermissionUtils_VerifyFilemodeBits(const char* path, mode_t expectedPermissions, bool isExactMatch);
TEST_CASE("PermissionUtils_VerifyFilemodeBit*")
{
    const mode_t file_permissions = S_ISUID | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IWOTH; // 04752

    // create temp file with all file permission bits set
    char tmpfile_path[32] = {};
    strncpy(tmpfile_path, "/tmp/permissionUtilsUT_XXXXXX", sizeof(tmpfile_path));
    int file_handle = mkstemp(tmpfile_path);
    REQUIRE(file_handle != -1);
    REQUIRE(0 == fchmod(file_handle, file_permissions));

    CHECK(PermissionUtils_VerifyFilemodeExact(tmpfile_path, file_permissions));
    CHECK_FALSE(PermissionUtils_VerifyFilemodeExact(tmpfile_path, file_permissions | S_IWGRP /* 04772 */));

    CHECK(PermissionUtils_VerifyFilemodeBitmask(tmpfile_path, file_permissions));
    CHECK(PermissionUtils_VerifyFilemodeBitmask(tmpfile_path, S_IXUSR | S_IRGRP | S_IWOTH /* 00142 */));
    CHECK_FALSE(PermissionUtils_VerifyFilemodeBitmask(
        tmpfile_path,
        S_ISUID | S_ISVTX | S_IRWXU | S_IRGRP | S_IXGRP | S_IWOTH /* 05752 */));

    // cleanup
    unlink(tmpfile_path);
}
