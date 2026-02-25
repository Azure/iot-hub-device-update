/**
 * @file system_utils_ut.cpp
 * @brief Unit Tests for c_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <catch2/catch_all.hpp>
using Catch::Matchers::Equals;

#include "aduc/system_utils.h"
#include <aduc/auto_opendir.hpp>
#include <aduc/string_handle_wrapper.hpp>
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
    return ADUC_SystemUtils_ForEachDirFunctor{ /*.context = */ fixture, /*.callbackFn = */ ForEachDir_Callback };
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
// Windows doesn't have any issue with creating root folders.
#if !defined(WIN32)
        std::string dir{ TestPath() };
        dir += "/fail";

        const int ret{ ADUC_SystemUtils_MkDirDefault(dir.c_str()) };
        CHECK_FALSE(ret == 0);

        struct stat st = {};
        CHECK_FALSE(stat(TestPath(), &st) == 0);
        CHECK_FALSE(S_ISDIR(st.st_mode));
#endif
    }

}

// TODO: Remove intrinsic !mayfail tag once windows pipeline is using self-hosted vmImage
TEST_CASE_METHOD(TestCaseFixture, "ADUC_SystemUtils_MkDirDefault negative", "[!mayfail]")
{
    // We choose /sys because it will fail for root users and non-root users.
    SECTION("Make directory under /sys")
    {
#if !defined(WIN32)
        std::string dir{ "/sys/fail" };
#else
        // For Windows, try creating a directory under %WINDIR% which will fail.
        std::string dir{ "c:/windows/fail" };
#endif

        const int ret{ ADUC_SystemUtils_MkDirDefault(dir.c_str()) };
        CHECK_FALSE(ret == 0);
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
#if !defined(WIN32)
        std::string dir{ "/sys/a/b/c/d/e/f/g/h/i/j" };
#else
        // For Windows, try creating a directory under %WINDIR% which will fail.
        std::string dir{ "c:/windows/a/b/c/d/e/f/g/h/i/j" };
#endif

        const int ret{ ADUC_SystemUtils_MkDirRecursiveDefault(dir.c_str()) };
        CHECK_FALSE(ret == 0);
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

TEST_CASE("ADUC_SystemUtils_FormatFilePathHelper")
{
    SECTION("ADUC_SystemUtils_FormatFilePathHelper without trailing forward slash")
    {
        const char* filePath = "/path/to/file.ext";
        const char* dirPath = "/path/to/folder";

        ADUC::StringUtils::STRING_HANDLE_wrapper newFilePath{ nullptr };

        REQUIRE(ADUC_SystemUtils_FormatFilePathHelper(newFilePath.address_of(), filePath, dirPath));
        REQUIRE_FALSE(newFilePath.is_null());

        CHECK_THAT(STRING_c_str(newFilePath.get()), Equals("/path/to/folder/file.ext"));
    }

    SECTION("ADUC_SystemUtils_FormatFilePathHelper with trailing forward slash")
    {
        const char* filePath = "/path/to/file.ext";
        const char* dirPath = "/path/to/folder/";

        ADUC::StringUtils::STRING_HANDLE_wrapper newFilePath{ nullptr };

        REQUIRE(ADUC_SystemUtils_FormatFilePathHelper(newFilePath.address_of(), filePath, dirPath));
        REQUIRE_FALSE(newFilePath.is_null());

        CHECK_THAT(STRING_c_str(newFilePath.get()), Equals("/path/to/folder/file.ext"));
    }
}

/**
 * @brief Regression test for feof() bug in ADUC_SystemUtils_CopyFileToDir
 *
 * Bug: Original had `feof(sourceFile) != 0` - impossible condition, loop never executed.
 * Fix: Changed to `feof(sourceFile) == 0` - correct "while not at EOF" condition.
 */
TEST_CASE_METHOD(TestCaseFixture, "ADUC_SystemUtils_CopyFileToDir")
{
    // Create test subdirectory (auto-cleaned by TestCaseFixture destructor)
    std::string copyTestDir = std::string(TestPath()) + "/copy_file_test";
    REQUIRE(ADUC_SystemUtils_MkDirRecursiveDefault(copyTestDir.c_str()) == 0);

    // Create a source file with known content
    std::string sourceFilePath = copyTestDir + "/source_file.txt";
    const char* testContent = "This is test content to verify file copying works correctly.\n"
                              "The bug caused files to be created with 0 bytes.\n"
                              "This content should be fully copied to the destination.\n"
                              "Multiple lines ensure we test the loop iterates properly.\n";
    size_t contentLength = strlen(testContent);

    // Write test content to source file
    FILE* sourceFile = fopen(sourceFilePath.c_str(), "wb");
    REQUIRE(sourceFile != nullptr);
    REQUIRE(fwrite(testContent, 1, contentLength, sourceFile) == contentLength);
    fclose(sourceFile);

    // Verify source file size
    struct stat sourceStat;
    REQUIRE(stat(sourceFilePath.c_str(), &sourceStat) == 0);
    REQUIRE(sourceStat.st_size == static_cast<off_t>(contentLength));

    SECTION("Copy file to directory - normal case with permissions check")
    {
        std::string destDir = copyTestDir + "/dest";
        REQUIRE(ADUC_SystemUtils_MkDirRecursiveDefault(destDir.c_str()) == 0);

        // Perform the copy
        int result = ADUC_SystemUtils_CopyFileToDir(sourceFilePath.c_str(), destDir.c_str(), false);
        REQUIRE(result == 0);

        // Verify destination file exists
        std::string destFilePath = destDir + "/source_file.txt";
        struct stat destStat;
        REQUIRE(stat(destFilePath.c_str(), &destStat) == 0);

        // Critical: Verify destination file is NOT 0 bytes (the bug symptom)
        REQUIRE(destStat.st_size > 0);
        CHECK(destStat.st_size == sourceStat.st_size);

        // Verify permissions are preserved
        CHECK((destStat.st_mode & 0777) == (sourceStat.st_mode & 0777));

        // Verify content matches
        FILE* destFile = fopen(destFilePath.c_str(), "rb");
        REQUIRE(destFile != nullptr);

        std::vector<char> destContent(contentLength + 1);
        size_t bytesRead = fread(destContent.data(), 1, contentLength, destFile);
        fclose(destFile);

        REQUIRE(bytesRead == contentLength);
        destContent[contentLength] = '\0';
        CHECK_THAT(destContent.data(), Equals(testContent));
    }

    SECTION("Copy empty file - edge case")
    {
        // Create an empty source file
        std::string emptySourcePath = copyTestDir + "/empty_file.txt";
        FILE* emptySource = fopen(emptySourcePath.c_str(), "wb");
        REQUIRE(emptySource != nullptr);
        fclose(emptySource);

        struct stat emptySourceStat;
        REQUIRE(stat(emptySourcePath.c_str(), &emptySourceStat) == 0);
        REQUIRE(emptySourceStat.st_size == 0);

        std::string emptyDestDir = copyTestDir + "/empty_dest";
        REQUIRE(ADUC_SystemUtils_MkDirRecursiveDefault(emptyDestDir.c_str()) == 0);

        // Copy the empty file
        int result = ADUC_SystemUtils_CopyFileToDir(emptySourcePath.c_str(), emptyDestDir.c_str(), false);
        REQUIRE(result == 0);

        // Verify empty file was created
        std::string emptyDestPath = emptyDestDir + "/empty_file.txt";
        struct stat emptyDestStat;
        REQUIRE(stat(emptyDestPath.c_str(), &emptyDestStat) == 0);
        CHECK(emptyDestStat.st_size == 0);
    }

    SECTION("Copy large file - stress test for loop iteration")
    {
        std::string largeSourcePath = copyTestDir + "/large_source.bin";
        FILE* largeSource = fopen(largeSourcePath.c_str(), "wb");
        REQUIRE(largeSource != nullptr);

        // Write 1MB of data (should iterate copy loop multiple times)
        const size_t largeSize = 1024 * 1024; // 1 MB
        std::vector<char> largeData(largeSize);
        for (size_t i = 0; i < largeSize; i++)
        {
            largeData[i] = static_cast<char>(i % 256);
        }
        REQUIRE(fwrite(largeData.data(), 1, largeSize, largeSource) == largeSize);
        fclose(largeSource);

        std::string largeDestDir = copyTestDir + "/large_dest";
        REQUIRE(ADUC_SystemUtils_MkDirRecursiveDefault(largeDestDir.c_str()) == 0);

        int result = ADUC_SystemUtils_CopyFileToDir(largeSourcePath.c_str(), largeDestDir.c_str(), false);
        REQUIRE(result == 0);

        std::string largeDestPath = largeDestDir + "/large_source.bin";
        struct stat largeStat;
        REQUIRE(stat(largeDestPath.c_str(), &largeStat) == 0);

        // Verify NOT 0 bytes (bug symptom)
        REQUIRE(largeStat.st_size > 0);
        CHECK(largeStat.st_size == static_cast<off_t>(largeSize));
    }

    SECTION("Copy file - overwrite existing file")
    {
        std::string overwriteDir = copyTestDir + "/overwrite";
        REQUIRE(ADUC_SystemUtils_MkDirRecursiveDefault(overwriteDir.c_str()) == 0);

        // Create existing file with different content
        std::string existingFilePath = overwriteDir + "/source_file.txt";
        FILE* existingFile = fopen(existingFilePath.c_str(), "wb");
        REQUIRE(existingFile != nullptr);
        const char* oldContent = "Old content";
        fwrite(oldContent, 1, strlen(oldContent), existingFile);
        fclose(existingFile);

        // Copy with overwrite enabled
        int result = ADUC_SystemUtils_CopyFileToDir(sourceFilePath.c_str(), overwriteDir.c_str(), true);
        REQUIRE(result == 0);

        // Verify file was overwritten with new content
        struct stat overwriteStat;
        REQUIRE(stat(existingFilePath.c_str(), &overwriteStat) == 0);
        REQUIRE(overwriteStat.st_size > 0);
        CHECK(overwriteStat.st_size == static_cast<off_t>(contentLength));

        // Verify new content
        FILE* verifyFile = fopen(existingFilePath.c_str(), "rb");
        REQUIRE(verifyFile != nullptr);
        std::vector<char> verifyContent(contentLength + 1);
        size_t bytesRead = fread(verifyContent.data(), 1, contentLength, verifyFile);
        fclose(verifyFile);

        REQUIRE(bytesRead == contentLength);
        verifyContent[contentLength] = '\0';
        CHECK_THAT(verifyContent.data(), Equals(testContent));
    }

    SECTION("Copy file exactly 1024 bytes - buffer boundary edge case")
    {
        // The internal buffer is 1024 bytes. When file is exactly 1024 bytes,
        // fread reads the full buffer AND sets EOF simultaneously.
        // This is the critical edge case for the feof() bug.
        std::string exactSourcePath = copyTestDir + "/exact_1024.bin";
        FILE* exactSource = fopen(exactSourcePath.c_str(), "wb");
        REQUIRE(exactSource != nullptr);

        const size_t exactSize = 1024;
        std::vector<char> exactData(exactSize);
        for (size_t i = 0; i < exactSize; i++)
        {
            exactData[i] = static_cast<char>('A' + (i % 26));
        }
        REQUIRE(fwrite(exactData.data(), 1, exactSize, exactSource) == exactSize);
        fclose(exactSource);

        std::string exactDestDir = copyTestDir + "/exact_1024_dest";
        REQUIRE(ADUC_SystemUtils_MkDirRecursiveDefault(exactDestDir.c_str()) == 0);

        int result = ADUC_SystemUtils_CopyFileToDir(exactSourcePath.c_str(), exactDestDir.c_str(), false);
        REQUIRE(result == 0);

        std::string exactDestPath = exactDestDir + "/exact_1024.bin";
        struct stat exactStat;
        REQUIRE(stat(exactDestPath.c_str(), &exactStat) == 0);
        REQUIRE(exactStat.st_size > 0);
        CHECK(exactStat.st_size == static_cast<off_t>(exactSize));

        // Verify content matches
        FILE* verifyFile = fopen(exactDestPath.c_str(), "rb");
        REQUIRE(verifyFile != nullptr);
        std::vector<char> verifyContent(exactSize);
        size_t bytesRead = fread(verifyContent.data(), 1, exactSize, verifyFile);
        fclose(verifyFile);
        REQUIRE(bytesRead == exactSize);
        CHECK(memcmp(verifyContent.data(), exactData.data(), exactSize) == 0);
    }

    SECTION("Copy file 1023 bytes - just under buffer size")
    {
        std::string underSourcePath = copyTestDir + "/under_1023.bin";
        FILE* underSource = fopen(underSourcePath.c_str(), "wb");
        REQUIRE(underSource != nullptr);

        const size_t underSize = 1023;
        std::vector<char> underData(underSize);
        for (size_t i = 0; i < underSize; i++)
        {
            underData[i] = static_cast<char>('a' + (i % 26));
        }
        REQUIRE(fwrite(underData.data(), 1, underSize, underSource) == underSize);
        fclose(underSource);

        std::string underDestDir = copyTestDir + "/under_1023_dest";
        REQUIRE(ADUC_SystemUtils_MkDirRecursiveDefault(underDestDir.c_str()) == 0);

        int result = ADUC_SystemUtils_CopyFileToDir(underSourcePath.c_str(), underDestDir.c_str(), false);
        REQUIRE(result == 0);

        std::string underDestPath = underDestDir + "/under_1023.bin";
        struct stat underStat;
        REQUIRE(stat(underDestPath.c_str(), &underStat) == 0);
        REQUIRE(underStat.st_size > 0);
        CHECK(underStat.st_size == static_cast<off_t>(underSize));
    }

    SECTION("Copy file 1025 bytes - just over buffer size (2 reads, second is partial)")
    {
        std::string overSourcePath = copyTestDir + "/over_1025.bin";
        FILE* overSource = fopen(overSourcePath.c_str(), "wb");
        REQUIRE(overSource != nullptr);

        const size_t overSize = 1025;
        std::vector<char> overData(overSize);
        for (size_t i = 0; i < overSize; i++)
        {
            overData[i] = static_cast<char>('0' + (i % 10));
        }
        REQUIRE(fwrite(overData.data(), 1, overSize, overSource) == overSize);
        fclose(overSource);

        std::string overDestDir = copyTestDir + "/over_1025_dest";
        REQUIRE(ADUC_SystemUtils_MkDirRecursiveDefault(overDestDir.c_str()) == 0);

        int result = ADUC_SystemUtils_CopyFileToDir(overSourcePath.c_str(), overDestDir.c_str(), false);
        REQUIRE(result == 0);

        std::string overDestPath = overDestDir + "/over_1025.bin";
        struct stat overStat;
        REQUIRE(stat(overDestPath.c_str(), &overStat) == 0);
        REQUIRE(overStat.st_size > 0);
        CHECK(overStat.st_size == static_cast<off_t>(overSize));

        // Verify all content including the single byte after buffer boundary
        FILE* verifyFile = fopen(overDestPath.c_str(), "rb");
        REQUIRE(verifyFile != nullptr);
        std::vector<char> verifyContent(overSize);
        size_t bytesRead = fread(verifyContent.data(), 1, overSize, verifyFile);
        fclose(verifyFile);
        REQUIRE(bytesRead == overSize);
        CHECK(memcmp(verifyContent.data(), overData.data(), overSize) == 0);
    }

    SECTION("Copy binary data with null bytes - ensure null bytes don't truncate copy")
    {
        std::string binarySourcePath = copyTestDir + "/binary_nulls.bin";
        FILE* binarySource = fopen(binarySourcePath.c_str(), "wb");
        REQUIRE(binarySource != nullptr);

        // Create binary data with embedded null bytes
        const size_t binarySize = 256;
        std::vector<char> binaryData(binarySize);
        for (size_t i = 0; i < binarySize; i++)
        {
            binaryData[i] = static_cast<char>(i); // Includes 0x00 at position 0
        }
        // Explicitly add more nulls in the middle
        binaryData[50] = '\0';
        binaryData[100] = '\0';
        binaryData[150] = '\0';

        REQUIRE(fwrite(binaryData.data(), 1, binarySize, binarySource) == binarySize);
        fclose(binarySource);

        std::string binaryDestDir = copyTestDir + "/binary_dest";
        REQUIRE(ADUC_SystemUtils_MkDirRecursiveDefault(binaryDestDir.c_str()) == 0);

        int result = ADUC_SystemUtils_CopyFileToDir(binarySourcePath.c_str(), binaryDestDir.c_str(), false);
        REQUIRE(result == 0);

        std::string binaryDestPath = binaryDestDir + "/binary_nulls.bin";
        struct stat binaryStat;
        REQUIRE(stat(binaryDestPath.c_str(), &binaryStat) == 0);
        REQUIRE(binaryStat.st_size > 0);
        CHECK(binaryStat.st_size == static_cast<off_t>(binarySize));

        // Verify all bytes including those after null bytes
        FILE* verifyFile = fopen(binaryDestPath.c_str(), "rb");
        REQUIRE(verifyFile != nullptr);
        std::vector<char> verifyContent(binarySize);
        size_t bytesRead = fread(verifyContent.data(), 1, binarySize, verifyFile);
        fclose(verifyFile);
        REQUIRE(bytesRead == binarySize);
        CHECK(memcmp(verifyContent.data(), binaryData.data(), binarySize) == 0);
    }

    SECTION("Null parameter handling - null filePath")
    {
        std::string destDir = copyTestDir + "/null_test";
        REQUIRE(ADUC_SystemUtils_MkDirRecursiveDefault(destDir.c_str()) == 0);

        int result = ADUC_SystemUtils_CopyFileToDir(nullptr, destDir.c_str(), false);
        CHECK(result != 0); // Should return error
    }

    SECTION("Null parameter handling - null dirPath")
    {
        int result = ADUC_SystemUtils_CopyFileToDir(sourceFilePath.c_str(), nullptr, false);
        CHECK(result != 0); // Should return error
    }

    SECTION("Null parameter handling - both null")
    {
        int result = ADUC_SystemUtils_CopyFileToDir(nullptr, nullptr, false);
        CHECK(result != 0); // Should return error
    }

    SECTION("Non-existent source file - should return error, not create empty dest")
    {
        std::string nonExistentPath = copyTestDir + "/this_file_does_not_exist.txt";
        std::string destDir = copyTestDir + "/nonexistent_src_dest";
        REQUIRE(ADUC_SystemUtils_MkDirRecursiveDefault(destDir.c_str()) == 0);

        int result = ADUC_SystemUtils_CopyFileToDir(nonExistentPath.c_str(), destDir.c_str(), false);
        CHECK(result != 0); // Should return error

        // Verify no destination file was created
        std::string wouldBeDestPath = destDir + "/this_file_does_not_exist.txt";
        struct stat destStat;
        CHECK(stat(wouldBeDestPath.c_str(), &destStat) != 0); // File should not exist
    }
}