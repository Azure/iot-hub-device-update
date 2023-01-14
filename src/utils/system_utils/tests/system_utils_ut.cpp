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
#include <sys/stat.h>
#include <vector>

// fwd-decl
class TestCaseFixture;

/**
 * @brief A capture of the arguments passed in a call invocation to callback for each sub dir in base dir.
 *
 */
struct ForEachDirCallRecord
{
    TestCaseFixture* context;
    std::string baseDir;
    std::string subDir;
};

/**
 * @brief ForEachDirCallRecord equals operator needed for STL container.
 *
 * @param first The first call record object.
 * @param second The second call record object.

 * @return true when equal.
 */
bool operator==(const ForEachDirCallRecord& first, const ForEachDirCallRecord& second)
{
    return first.context == second.context && first.baseDir == second.baseDir && first.subDir == second.subDir;
}

/**
 * @brief ForEachDirCallRecord less-than operator for use with std::sort with std::less comparator.
 *
 * @param first The first call record object.
 * @param second The second call record object.
 * @return true When first call record is considered to be ordered less than the second.
 */
bool operator<(const ForEachDirCallRecord& first, const ForEachDirCallRecord& second)
{
    if (first.baseDir < second.baseDir)
    {
        return true;
    }

    return first.baseDir == second.baseDir ? first.subDir < second.subDir : false;
}

// forward declarations to successfully compile CreateCallbackFunctor function that references
// pointer to TestCaseFixture and the ForEachDir_Callback C-style callback, which has to
// call into TestCaseFixture class's static AddCallRecord method so that the TestCaseFixture
// instance can update it's per-testcase vector of callback call records.
static void ForEachDir_Callback(void* context, const char* baseDir, const char* subDir);

/**
 * @brief Create a Callback Functor object for the test case fixture object.
 *
 * @param fixture The test case fixture object instance.
 * @return ADUC_SystemUtils_ForEachDirFunctor The functor object.
 */

ADUC_SystemUtils_ForEachDirFunctor CreateCallbackFunctor(TestCaseFixture* fixture)
{
    return ADUC_SystemUtils_ForEachDirFunctor{ .context = fixture, .callbackFn = ForEachDir_Callback };
};

class TestCaseFixture
{
public:
    static TestCaseFixture* FromVoidPtr(void* vp)
    {
        return static_cast<TestCaseFixture*>(vp);
    }

    TestCaseFixture() : m_testPath{ ADUC_SystemUtils_GetTemporaryPathName() }
    {
        m_testPath += "/system_utils_ut";

        (void)ADUC_SystemUtils_RmDirRecursive(m_testPath.c_str());
    }

    ~TestCaseFixture()
    {
        (void)ADUC_SystemUtils_RmDirRecursive(m_testPath.c_str());
    }

    /**
     * @brief Gets the base test temp path as a c_str.
     *
     * @return const char* The test path.
     */
    const char* TestPath() const
    {
        return m_testPath.c_str();
    }

    std::string BaseDir_for_ForEachDir() const
    {
        return m_testPath + "/ForEachDirTest/";
    };

    /**
     * @brief provides the instance to this text fixture object for asserting
     * that the Context passed to the for-each callback matches the current
     * TestCaseFixture instance.
     *
     * @return TestCaseFixture* The pointer to this instance.
     */
    TestCaseFixture* Instance()
    {
        return this;
    }

    /**
     * @brief Create a Call Record object for use in test assertions.
     *
     * @param subdir The sub directory under the BaseDir.
     * @return ForEachDirCallRecord The expected call record for the given sub directory.
     */
    ForEachDirCallRecord CreateCallRecord(std::string subdir)
    {
        return { Instance(), BaseDir_for_ForEachDir(), subdir };
    }

    /**
     * @brief Wrapper to execute a test case for SystemUtils_ForEachDir utility function.
     *
     * @param subdirs The vector of subdirs under the base dir on the file system.
     * @param excludedSubDir The subdir to exclude. Empty string to not exclude any.
     * @param expectedRetCode The expected return code of the SystemUtils_ForEachDir call.
     * @param shouldDeleteBaseDirBeforeExecuteTest Optional, default of false. Deletes BaseDir before running test case if true.
     * @return std::vector<CallRecord> The sorted vector of call records to the C-style callback function.
     */
    std::vector<ForEachDirCallRecord> ExecuteForEachDirTestCase(
        std::vector<std::string> subdirs,
        std::string excludedSubDir,
        int expectedRetCode,
        bool shouldDeleteBaseDirBeforeExecuteTest = false)
    {
        std::string base_dir{ BaseDir_for_ForEachDir() };
        ADUC_SystemUtils_RmDirRecursive(base_dir.c_str());

        if (!shouldDeleteBaseDirBeforeExecuteTest)
        {
            REQUIRE(0 == ADUC_SystemUtils_MkDirRecursiveDefault(base_dir.c_str()));
        }

        for (const auto& subdir : subdirs)
        {
            REQUIRE(0 == ADUC_SystemUtils_MkDirRecursiveDefault((base_dir + subdir).c_str()));
        }

        auto functor = CreateCallbackFunctor(this);

        int err = SystemUtils_ForEachDir(
            base_dir.c_str() /* baseDir */,
            excludedSubDir.length() > 0 ? excludedSubDir.c_str() : nullptr /* excludeDir */,
            &functor /* perDirActionFunctor */);
        REQUIRE(err == expectedRetCode);

        std::sort(
            m_actual_foreach_callback_records.begin(),
            m_actual_foreach_callback_records.end(),
            std::less<ForEachDirCallRecord>());
        return m_actual_foreach_callback_records;
    }

    /**
     * @brief Adds a call record to the instance's collection.
     *
     * @param call_record The call record to save.
     */
    void AddCallRecord(const ForEachDirCallRecord& call_record)
    {
        m_actual_foreach_callback_records.push_back(call_record);
    }

    /**
     * @brief static AddCallRecord so that pure C callback function can call it.
     *
     * @param instance The TestCaseFixture instance on which to call the AddCallRecord instance method.
     * @param call_record The call record to save.
     */
    static void AddCallRecord(void* instance, const ForEachDirCallRecord& call_record)
    {
        static_cast<TestCaseFixture*>(instance)->AddCallRecord(call_record);
    }

private:
    TestCaseFixture(const TestCaseFixture&) = delete;
    TestCaseFixture& operator=(const TestCaseFixture&) = delete;
    TestCaseFixture(TestCaseFixture&&) = delete;
    TestCaseFixture& operator=(TestCaseFixture&&) = delete;

    // The test temporary file path.
    std::string m_testPath;

    // The captures an ordered list of call captures, one for each  timethe SystemUtils_ForEachDir calls
    // the C-style callback for each subdir under baseDir.
    std::vector<ForEachDirCallRecord> m_actual_foreach_callback_records;
};

/**
 * @brief The C-style callback for use when test case exercises SystemUtils_ForEachDir.
 *
 * @param context The context object, which is a TestCaseFixture pointer in the context of test case.
 * @param baseDir The base directory.
 * @param subDir The subdirectory under the base dir.
 * @details It simply forwards along the context and a call record to the AddCallRecord static class method.
 */
static void ForEachDir_Callback(void* context, const char* baseDir, const char* subDir)
{
    TestCaseFixture::AddCallRecord(
        context, ForEachDirCallRecord{ TestCaseFixture::FromVoidPtr(context), baseDir, subDir });
}

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
        CHECK(
            -1
            == SystemUtils_ForEachDir(
                nullptr /* baseDir */, nullptr /* excludeDir */, nullptr /* perDirActionFunctor */));
    }

    SECTION("NULL functor should fail")
    {
        CHECK(
            -1
            == SystemUtils_ForEachDir(
                BaseDir_for_ForEachDir().c_str() /* baseDir */,
                "subdir" /* excludeDir */,
                nullptr /* perDirActionFunctor */));
    }

    SECTION("Non-existent base dir should fail with errno FileNotFound")
    {
        int FileNotFound = 2;
        auto actualCallRecords = ExecuteForEachDirTestCase(
            std::vector<std::string>{} /* subdirs */,
            "" /* excludedSubDir */,
            FileNotFound /* expectedRetCode */,
            true /* shouldDeleteBaseDirBeforeExecuteTest */);
        REQUIRE(actualCallRecords.size() == 0);
    }

    SECTION("Empty Dir, no excludeDir should succeed")
    {
        auto actualCallRecords = ExecuteForEachDirTestCase(
            std::vector<std::string>{} /* subdirs */, "" /* excludedSubDir */, 0 /* expectedRetCode */);
        REQUIRE(actualCallRecords.size() == 0);
    }

    SECTION("Non-Empty Dir, no excludeDir should callback")
    {
        auto actualCallRecords = ExecuteForEachDirTestCase(
            std::vector<std::string>{ "subdir1" } /* subdirs */, "" /* excludedSubDir */, 0 /* expectedRetCode */);
        REQUIRE(actualCallRecords.size() == 1);

        CHECK(
            actualCallRecords
            == std::vector<ForEachDirCallRecord>{
                CreateCallRecord("subdir1"),
            });
    }

    SECTION("Exclude the only existing subdir should not callback")
    {
        auto actualCallRecords = ExecuteForEachDirTestCase(
            std::vector<std::string>{ "subdir1" } /* subdirs */,
            "subdir1" /* excludedSubDir */,
            0 /* expectedRetCode */);
        REQUIRE(actualCallRecords.size() == 0);
    }

    SECTION("Empty Dir, exclude non-existent should not callback")
    {
        auto actualCallRecords = ExecuteForEachDirTestCase(
            std::vector<std::string>{} /* subdirs */, "i_do_not_exist" /* excludedSubDir */, 0 /* expectedRetCode */);
        REQUIRE(actualCallRecords.size() == 0);
    }

    SECTION("two subdirs, exclude the 1st one")
    {
        auto actualCallRecords = ExecuteForEachDirTestCase(
            std::vector<std::string>{ "subdir1", "subdir2", "subdir3" } /* subdirs */,
            "subdir1" /* excludedSubDir */,
            0 /* expectedRetCode */);
        REQUIRE(actualCallRecords.size() == 2);

        CHECK(
            actualCallRecords
            == std::vector<ForEachDirCallRecord>{
                CreateCallRecord("subdir2"),
                CreateCallRecord("subdir3"),
            });
    }

    SECTION("multiple subdirs, exclude second one")
    {
        auto actualCallRecords = ExecuteForEachDirTestCase(
            std::vector<std::string>{ "subdir1", "subdir2", "subdir3" } /* subdirs */,
            "subdir2" /* excludedSubDir */,
            0 /* expectedRetCode */);
        REQUIRE(actualCallRecords.size() == 2);

        CHECK(
            actualCallRecords
            == std::vector<ForEachDirCallRecord>{
                CreateCallRecord("subdir1"),
                CreateCallRecord("subdir3"),
            });
    }

    SECTION("multiple subdirs, exclude the last one")
    {
        auto actualCallRecords = ExecuteForEachDirTestCase(
            std::vector<std::string>{ "subdir1", "subdir2", "subdir3" } /* subdirs */,
            "subdir3" /* excludedSubDir */,
            0 /* expectedRetCode */);
        REQUIRE(actualCallRecords.size() == 2);

        CHECK(
            actualCallRecords
            == std::vector<ForEachDirCallRecord>{
                CreateCallRecord("subdir1"),
                CreateCallRecord("subdir2"),
            });
    }
}
