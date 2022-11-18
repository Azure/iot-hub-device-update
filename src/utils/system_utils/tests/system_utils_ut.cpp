/**
 * @file system_utils_ut.cpp
 * @brief Unit Tests for c_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch.hpp>
using Catch::Matchers::Equals;

#include "aduc/system_utils.h"

#include <aduc/auto_opendir.hpp>
#include <algorithm> // std::find*
#include <dirent.h>
#include <sstream>
#include <sys/stat.h>
#include <vector>

static bool callback_called = false;
std::string actual;

static void perDirFn(void* context, const char* baseDir, const char* subdir_name)
{
    callback_called = true;
    std::stringstream ss;

    if (context != nullptr)
    {
        ss << *((int*)(context));
    }

    ss << ", " << baseDir << ", " << subdir_name << std::endl;

    actual += ss.str();
};

static void reset_test_state()
{
    callback_called = false;
    actual = "";
}

static int get_dirs_in_dir(std::string dir, std::vector<std::string>& outPaths)
{
    struct dirent* entry = nullptr;

    aduc::AutoOpenDir d{ dir };
    if (d.GetDirectoryStreamHandle() == nullptr)
    {
        return errno;
    }

    struct dirent* dirEntry = nullptr;
    do
    {
        errno = 0;
        dirEntry = d.NextDirEntry();
        if (dirEntry == nullptr)
        {
            if (errno != 0)
            {
                return errno;
            }
        }
        else
        {
            std::string entry_name{ dirEntry->d_name };
            if (entry_name == "." || entry_name == "..")
            {
                continue;
            }

            outPaths.push_back(entry_name);
        }
    } while (dirEntry != nullptr);

    return 0;
}

int val = 42;
ADUC_SystemUtils_ForEachDirFunctor functor = {
    &val, // context
    perDirFn, // callbackFn
};

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
        REQUIRE(ret == 0);

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
        REQUIRE(ret == 0);

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

TEST_CASE_METHOD(TestCaseFixture, "SystemUtils_ForEachDir")
{
    SECTION("All NULL should fail")
    {
        reset_test_state();

        int err =
            SystemUtils_ForEachDir(nullptr /* baseDir */, nullptr /* excludeDir */, nullptr /* perDirActionFunctor */);
        CHECK_FALSE(err == 0);
    }

    SECTION("NULL functor should fail")
    {
        reset_test_state();

        ADUC_SystemUtils_RmDirRecursive(TestPath());

        {
            int ret{ ADUC_SystemUtils_MkDirDefault(TestPath()) };
            REQUIRE(ret == 0);
        }

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 0);
        }

        int err = SystemUtils_ForEachDir(
            TestPath() /* baseDir */, nullptr /* excludeDir */, nullptr /* perDirActionFunctor */);
        CHECK_FALSE(err == 0);

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 0);
        }
    }

    SECTION("Non-existent base dir should fail with errno FileNotFound")
    {
        reset_test_state();

        ADUC_SystemUtils_RmDirRecursive(TestPath());

        int err = SystemUtils_ForEachDir(
            TestPath() /* baseDir */, nullptr /* excludeDir */, &functor /* perDirActionFunctor */);
        CHECK(err == 2); // FileNotFound
    }

    SECTION("Empty Dir, no excludeDir should succeed")
    {
        reset_test_state();

        ADUC_SystemUtils_RmDirRecursive(TestPath());

        {
            int ret{ ADUC_SystemUtils_MkDirDefault(TestPath()) };
            REQUIRE(ret == 0);
        }

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 0);
        }

        int err = SystemUtils_ForEachDir(
            TestPath() /* baseDir */, nullptr /* excludeDir */, &functor /* perDirActionFunctor */);
        REQUIRE(err == 0);

        CHECK_FALSE(callback_called);

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 0);
        }
    }

    SECTION("Non-Empty Dir, no excludeDir should callback")
    {
        reset_test_state();

        ADUC_SystemUtils_RmDirRecursive(TestPath());

        {
            int ret{ ADUC_SystemUtils_MkDirDefault(TestPath()) };
            REQUIRE(ret == 0);
        }

        const std::string test_subdir = "foo_subdir";

        std::stringstream ss;
        ss << "42, " << TestPath() << ", " << test_subdir << std::endl;
        std::string expected = ss.str();

        // create a directory under the test path;
        std::string test_path{ TestPath() };
        test_path += "/";
        test_path += test_subdir;

        REQUIRE(0 == ADUC_SystemUtils_MkDirRecursiveDefault(test_path.c_str()));

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 1);
            REQUIRE_THAT(paths[0], Equals(test_subdir));
        }

        int err = SystemUtils_ForEachDir(
            TestPath() /* baseDir */, nullptr /* excludeDir */, &functor /* perDirActionFunctor */);
        REQUIRE(err == 0);

        CHECK_THAT(actual, Equals(expected));

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 1);
            REQUIRE_THAT(paths[0], Equals(test_subdir));
        }
    }

    SECTION("Exclude the only existing subdir should not callback")
    {
        reset_test_state();
        std::string expected = "";

        ADUC_SystemUtils_RmDirRecursive(TestPath());

        {
            int ret{ ADUC_SystemUtils_MkDirDefault(TestPath()) };
            REQUIRE(ret == 0);
        }

        const std::string test_subdir = "foo_subdir";

        // create a directory under the test path;
        std::string test_path{ TestPath() };
        test_path += "/";
        test_path += test_subdir;

        REQUIRE(0 == ADUC_SystemUtils_MkDirRecursiveDefault(test_path.c_str()));

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 1);
            CHECK_THAT(paths[0], Equals(test_subdir));
        }

        int err = SystemUtils_ForEachDir(
            TestPath() /* baseDir */, test_subdir.c_str() /* excludeDir */, &functor /* perDirActionFunctor */);
        REQUIRE(err == 0);

        CHECK_FALSE(callback_called);
        CHECK_THAT(actual, Equals(expected));

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 1);
            CHECK_THAT(paths[0], Equals(test_subdir));
        }
    }

    SECTION("Empty Dir, exclude non-existent should not callback")
    {
        reset_test_state();
        std::string expected = "";

        ADUC_SystemUtils_RmDirRecursive(TestPath());

        {
            int ret{ ADUC_SystemUtils_MkDirDefault(TestPath()) };
            REQUIRE(ret == 0);
        }

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 0);
        }

        std::string excluded_subdir = "i_dont_exist";

        int err = SystemUtils_ForEachDir(
            TestPath() /* baseDir */, excluded_subdir.c_str() /* excludeDir */, &functor /* perDirActionFunctor */);
        REQUIRE(err == 0);

        CHECK_FALSE(callback_called);
        CHECK_THAT(actual, Equals(expected));

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 0);
        }
    }

    SECTION("two subdirs, exclude the 1st one")
    {
        reset_test_state();

        ADUC_SystemUtils_RmDirRecursive(TestPath());

        {
            int ret{ ADUC_SystemUtils_MkDirDefault(TestPath()) };
            REQUIRE(ret == 0);
        }

        const std::string first_subdir = "i_exist";
        const std::string second_subdir = "i_exist_too";

        std::stringstream ss;
        ss << "42, " << TestPath() << ", " << second_subdir << std::endl;
        std::string expected = ss.str();

        // create directories under the test path;
        std::string test_path{ TestPath() };
        test_path += "/";
        test_path += first_subdir;

        std::string test_path2{ TestPath() };
        test_path2 += "/";
        test_path2 += second_subdir;

        REQUIRE(0 == ADUC_SystemUtils_MkDirRecursiveDefault(test_path.c_str()));
        REQUIRE(0 == ADUC_SystemUtils_MkDirRecursiveDefault(test_path2.c_str()));

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 2);

            CHECK(std::end(paths) != std::find(begin(paths), end(paths), first_subdir));
            CHECK(std::end(paths) != std::find(begin(paths), end(paths), second_subdir));
        }

        int err = SystemUtils_ForEachDir(
            TestPath() /* baseDir */, first_subdir.c_str() /* excludeDir */, &functor /* perDirActionFunctor */);
        REQUIRE(err == 0);

        CHECK(callback_called);
        CHECK_THAT(actual, Equals(expected));

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 2);
            CHECK(std::end(paths) != std::find(begin(paths), end(paths), first_subdir));
            CHECK(std::end(paths) != std::find(begin(paths), end(paths), second_subdir));
        }
    }

    SECTION("multiple subdirs, exclude second one")
    {
        reset_test_state();

        ADUC_SystemUtils_RmDirRecursive(TestPath());

        {
            int ret{ ADUC_SystemUtils_MkDirDefault(TestPath()) };
            REQUIRE(ret == 0);
        }

        const std::string first_subdir = "i_exist";
        const std::string second_subdir = "i_exist_too";
        const std::string third_subdir = "i_exist_three";

        std::stringstream ss;
        ss << "42, " << TestPath() << ", " << first_subdir << std::endl;
        ss << "42, " << TestPath() << ", " << third_subdir << std::endl;
        std::string expected = ss.str();

        // create a directory under the test path;
        std::string test_path{ TestPath() };
        test_path += "/";
        test_path += first_subdir;

        std::string test_path2{ TestPath() };
        test_path2 += "/";
        test_path2 += second_subdir;

        std::string test_path3{ TestPath() };
        test_path3 += "/";
        test_path3 += third_subdir;

        REQUIRE(0 == ADUC_SystemUtils_MkDirRecursiveDefault(test_path.c_str()));
        REQUIRE(0 == ADUC_SystemUtils_MkDirRecursiveDefault(test_path2.c_str()));
        REQUIRE(0 == ADUC_SystemUtils_MkDirRecursiveDefault(test_path3.c_str()));

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 3);
            CHECK(std::end(paths) != std::find(begin(paths), end(paths), first_subdir));
            CHECK(std::end(paths) != std::find(begin(paths), end(paths), second_subdir));
            CHECK(std::end(paths) != std::find(begin(paths), end(paths), third_subdir));
        }

        int err = SystemUtils_ForEachDir(
            TestPath() /* baseDir */, second_subdir.c_str() /* excludeDir */, &functor /* perDirActionFunctor */);
        CHECK(err == 0); // not set

        CHECK(callback_called);
        CHECK_THAT(actual, Equals(expected));

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 3);
            CHECK(std::end(paths) != std::find(begin(paths), end(paths), first_subdir));
            CHECK(std::end(paths) != std::find(begin(paths), end(paths), second_subdir));
            CHECK(std::end(paths) != std::find(begin(paths), end(paths), third_subdir));
        }
    }

    SECTION("multiple subdirs, exclude the last one")
    {
        reset_test_state();

        ADUC_SystemUtils_RmDirRecursive(TestPath());

        {
            int ret{ ADUC_SystemUtils_MkDirDefault(TestPath()) };
            REQUIRE(ret == 0);
        }

        const std::string first_subdir = "i_exist";
        const std::string second_subdir = "i_exist_too";
        const std::string third_subdir = "i_exist_three";

        std::stringstream ss;
        ss << "42, " << TestPath() << ", " << first_subdir << std::endl;
        ss << "42, " << TestPath() << ", " << second_subdir << std::endl;
        std::string expected = ss.str();

        // create a directory under the test path;
        std::string test_path{ TestPath() };
        test_path += "/";
        test_path += first_subdir;

        std::string test_path2{ TestPath() };
        test_path2 += "/";
        test_path2 += second_subdir;

        std::string test_path3{ TestPath() };
        test_path3 += "/";
        test_path3 += third_subdir;

        REQUIRE(0 == ADUC_SystemUtils_MkDirRecursiveDefault(test_path.c_str()));
        REQUIRE(0 == ADUC_SystemUtils_MkDirRecursiveDefault(test_path2.c_str()));
        REQUIRE(0 == ADUC_SystemUtils_MkDirRecursiveDefault(test_path3.c_str()));

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 3);
            CHECK(std::end(paths) != std::find(begin(paths), end(paths), first_subdir));
            CHECK(std::end(paths) != std::find(begin(paths), end(paths), second_subdir));
            CHECK(std::end(paths) != std::find(begin(paths), end(paths), third_subdir));
        }

        int err = SystemUtils_ForEachDir(
            TestPath() /* baseDir */, third_subdir.c_str() /* excludeDir */, &functor /* perDirActionFunctor */);
        CHECK(err == 0); // not set

        CHECK(callback_called);
        CHECK_THAT(actual, Equals(expected));

        {
            std::vector<std::string> paths;
            REQUIRE(get_dirs_in_dir(TestPath(), paths) == 0);
            REQUIRE(paths.size() == 3);
            CHECK(std::end(paths) != std::find(begin(paths), end(paths), first_subdir));
            CHECK(std::end(paths) != std::find(begin(paths), end(paths), second_subdir));
            CHECK(std::end(paths) != std::find(begin(paths), end(paths), third_subdir));
        }
    }
}
