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

#if defined(_WIN32)
// TODO(JeffMill): [PAL] S_
#    define S_ISVTX 01000
#    define S_ISUID 0004000

#    define S_IRUSR 00400
#    define S_IWUSR 00200
#    define S_IXUSR 00100

#    define S_IRGRP 00040
#    define S_IWGRP 00020
#    define S_IXGRP 00010

#    define S_IWOTH 00002
#else
#    include <sys/stat.h> // S_*
#endif

#if defined(_WIN32)
// TODO(JeffMill): [PAL] mkstemp
static int mkstemp(char* tmpl)
{
    __debugbreak();
    errno = ENOSYS;
    return -1;
}
#else
#    include <stdlib.h> //mkstemp
#endif

#if defined(_WIN32)
// TODO(JeffMill: [PAL] fchmod
static int fchmod(int fd, mode_t mode)
{
    __debugbreak();

    errno = ENOSYS;
    return -1;
}
#else
#    include <sys/stat.h> // fchmod
#endif

// #include <unistd.h>

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
        tmpfile_path, S_ISUID | S_ISVTX | S_IRWXU | S_IRGRP | S_IXGRP | S_IWOTH /* 05752 */));

    // cleanup
    unlink(tmpfile_path);
}
