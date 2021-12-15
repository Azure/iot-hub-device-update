/**
 * @file system_utils_ut.cpp
 * @brief Unit Tests for c_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch.hpp>

#include "aduc/system_utils.h"

#include <sys/stat.h>

TEST_CASE("ADUC_SystemUtils_GetTemporaryPathName")
{
    SECTION("Verify non-empty")
    {
        const std::string tempPath{ ADUC_SystemUtils_GetTemporaryPathName() };
        CHECK(!tempPath.empty());
    }
}

TEST_CASE("ADUC_SystemUtils_ExecuteShellCommand")
{
    SECTION("Run date")
    {
        const std::string command{ "/bin/date" };

        const int ret{ ADUC_SystemUtils_ExecuteShellCommand(command.c_str()) };
        CHECK(ret == 0);
    }

    SECTION("Run a directory")
    {
        const std::string command{ ADUC_SystemUtils_GetTemporaryPathName() };

        const int ret{ ADUC_SystemUtils_ExecuteShellCommand(command.c_str()) };
        CHECK_FALSE(ret == 0);
    }
}

class TestCaseFixture
{
public:
    TestCaseFixture() : _testPath{ ADUC_SystemUtils_GetTemporaryPathName() }
    {
        _testPath += "/system_utils_ut";

        (void)ADUC_SystemUtils_RmDirRecursive(_testPath.c_str());
    }

    ~TestCaseFixture()
    {
        (void)ADUC_SystemUtils_RmDirRecursive(_testPath.c_str());
    }

    const char* TestPath() const
    {
        return _testPath.c_str();
    }

    TestCaseFixture(const TestCaseFixture&) = delete;
    TestCaseFixture& operator=(const TestCaseFixture&) = delete;
    TestCaseFixture(TestCaseFixture&&) = delete;
    TestCaseFixture& operator=(TestCaseFixture&&) = delete;

private:
    std::string _testPath;
};

TEST_CASE_METHOD(TestCaseFixture, "ADUC_SystemUtils_MkDirDefault")
{
    SECTION("Make a directory under tmp")
    {
        int ret{ ADUC_SystemUtils_MkDirDefault(TestPath()) };
        CHECK(ret == 0);

        struct stat st = {};
        CHECK(stat(TestPath(), &st) == 0);
        CHECK(S_ISDIR(st.st_mode));
    }

    SECTION("Make recursive structure")
    {
        std::string dir{ TestPath() };
        dir += "/fail";

        const int ret{ ADUC_SystemUtils_MkDirDefault(dir.c_str()) };
        CHECK_FALSE(ret == 0);

        struct stat st = {};
        CHECK_FALSE(stat(TestPath(), &st) == 0);
        CHECK_FALSE(S_ISDIR(st.st_mode));
    }

    // We choose /sys because it will fail for root users and non-root users.
    SECTION("Make directory under /sys")
    {
        std::string dir{ "/sys/fail" };

        const int ret{ ADUC_SystemUtils_MkDirDefault(dir.c_str()) };
        CHECK_FALSE(ret == 0);

        struct stat st = {};
        CHECK_FALSE(stat(TestPath(), &st) == 0);
        CHECK_FALSE(S_ISDIR(st.st_mode));
    }
}

TEST_CASE_METHOD(TestCaseFixture, "ADUC_SystemUtils_MkDirRecursiveDefault")
{
    SECTION("Make directory")
    {
        std::string dir{ TestPath() };
        dir += "/a/b/c/d/e/f/g/h/i/j";

        const int ret{ ADUC_SystemUtils_MkDirRecursiveDefault(dir.c_str()) };
        CHECK(ret == 0);

        struct stat st = {};
        CHECK(stat(TestPath(), &st) == 0);
        CHECK(S_ISDIR(st.st_mode));
    }

    // We choose /sys because it will fail for root users and non-root users.
    SECTION("Make directory off /sys")
    {
        std::string dir{ "/sys/a/b/c/d/e/f/g/h/i/j" };

        const int ret{ ADUC_SystemUtils_MkDirRecursiveDefault(dir.c_str()) };
        CHECK_FALSE(ret == 0);

        struct stat st = {};
        CHECK_FALSE(stat(TestPath(), &st) == 0);
        CHECK_FALSE(S_ISDIR(st.st_mode));
    }
}

TEST_CASE_METHOD(TestCaseFixture, "ADUC_SystemUtils_RmDirRecursive")
{
    SECTION("Remove non-existent directory")
    {
        const std::string dir{ TestPath() };

        const int ret{ ADUC_SystemUtils_RmDirRecursive(dir.c_str()) };
        CHECK_FALSE(ret == 0);

        struct stat st = {};
        CHECK_FALSE(stat(TestPath(), &st) == 0);
        CHECK_FALSE(S_ISDIR(st.st_mode));
    }

    SECTION("Remove non-existent directory")
    {
        std::string dir{ TestPath() };
        dir += "/a/b/c/d/e/f/g/h/i/j";

        const int ret{ ADUC_SystemUtils_RmDirRecursive(dir.c_str()) };
        CHECK_FALSE(ret == 0);

        struct stat st = {};
        CHECK_FALSE(stat(TestPath(), &st) == 0);
        CHECK_FALSE(S_ISDIR(st.st_mode));
    }
}
