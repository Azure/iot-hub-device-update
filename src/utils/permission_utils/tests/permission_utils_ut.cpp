/**
 * @file permission_utils_ut.cpp
 * @brief Unit Tests for permission_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/permission_utils.h>

#include "aduc/system_utils.h"

#include <catch2/catch.hpp>
#include <stdio.h>
#include <string.h>
#include <string>

#include <fstream> // ofstream

#include <aducpal/stdio.h> // remove
#include <aducpal/sys_stat.h> // S_I*
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
