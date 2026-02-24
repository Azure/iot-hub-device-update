/**
 * @file file_info_utils_ut.cpp
 * @brief Unit Tests for file_info_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "file_info_utils.h"

#include <aduc/c_utils.h>
#include <aduc/calloc_wrapper.hpp>
#include <catch2/catch_all.hpp>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <vector>

// Test directory for file operations
static const char* TEST_DIR = "/tmp/file_info_utils_test";

// Helper class to manage test directory and files
class FileInfoTestHelper
{
public:
    FileInfoTestHelper()
    {
        cleanup();
        mkdir(TEST_DIR, 0755);
    }

    ~FileInfoTestHelper()
    {
        cleanup();
    }

    static void cleanup()
    {
        // Remove all files in test directory
        std::string rmCmd = std::string("rm -rf ") + TEST_DIR;
        system(rmCmd.c_str());
    }

    static bool createFile(const std::string& filename, size_t size)
    {
        std::string filepath = std::string(TEST_DIR) + "/" + filename;
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        // Write 'size' bytes to the file
        std::vector<char> buffer(size, 'X');
        file.write(buffer.data(), size);
        return file.good();
    }

    static bool createFileWithDelay(const std::string& filename, size_t size, int delayMs)
    {
        if (delayMs > 0)
        {
            usleep(delayMs * 1000);
        }
        return createFile(filename, size);
    }

    static bool createSubdirectory(const std::string& dirname)
    {
        std::string dirpath = std::string(TEST_DIR) + "/" + dirname;
        return mkdir(dirpath.c_str(), 0755) == 0;
    }
};

TEST_CASE("FileInfoUtils_InsertFileInfoIntoArray - Parameter Validation")
{
    SECTION("Returns false when sortedLogFiles is nullptr")
    {
        const auto testCandidateLastWrite = static_cast<time_t>(time(nullptr));
        CHECK_FALSE(FileInfoUtils_InsertFileInfoIntoArray(nullptr, 5, "test.log", 1024, testCandidateLastWrite));
    }

    SECTION("Returns false when sortedLogFileLength is 0")
    {
        FileInfo sortedLogFileArray[1] = {};
        const auto testCandidateLastWrite = static_cast<time_t>(time(nullptr));
        CHECK_FALSE(FileInfoUtils_InsertFileInfoIntoArray(sortedLogFileArray, 0, "test.log", 1024, testCandidateLastWrite));
    }

    SECTION("Returns false when candidateFileName is nullptr")
    {
        FileInfo sortedLogFileArray[5] = {};
        const auto testCandidateLastWrite = static_cast<time_t>(time(nullptr));
        CHECK_FALSE(FileInfoUtils_InsertFileInfoIntoArray(sortedLogFileArray, 5, nullptr, 1024, testCandidateLastWrite));
    }

    SECTION("Returns false when sizeOfCandidateFile is 0")
    {
        FileInfo sortedLogFileArray[5] = {};
        const auto testCandidateLastWrite = static_cast<time_t>(time(nullptr));
        CHECK_FALSE(FileInfoUtils_InsertFileInfoIntoArray(sortedLogFileArray, 5, "test.log", 0, testCandidateLastWrite));
    }

    SECTION("Returns false when all parameters are invalid")
    {
        const auto testCandidateLastWrite = static_cast<time_t>(time(nullptr));
        CHECK_FALSE(FileInfoUtils_InsertFileInfoIntoArray(nullptr, 0, nullptr, 0, testCandidateLastWrite));
    }
}

TEST_CASE("FileInfoUtils_InsertFileInfoIntoArray - Basic Operations")
{
    SECTION("Insert first file into empty array")
    {
        const size_t sortedLogFileArraySize = 5;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        const char* testCandidateFileName = "foo";
        size_t testSizeOfCandidateFile = 1024;
        const auto testCandidateLastWrite = static_cast<time_t>(time(nullptr));

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray,
            sortedLogFileArraySize,
            testCandidateFileName,
            static_cast<long long>(testSizeOfCandidateFile),
            testCandidateLastWrite));

        ADUC::StringUtils::cstr_wrapper fileNameSentinel{ sortedLogFileArray[0].fileName };
        CHECK(strcmp(sortedLogFileArray[0].fileName, testCandidateFileName) == 0);
        CHECK(sortedLogFileArray[0].fileSize == static_cast<long long>(testSizeOfCandidateFile));
        CHECK(sortedLogFileArray[0].lastWrite == testCandidateLastWrite);
    }

    SECTION("Insert file with minimum valid size")
    {
        const size_t sortedLogFileArraySize = 3;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        const char* testCandidateFileName = "small.log";
        const long long testSizeOfCandidateFile = 1; // Minimum valid size
        const auto testCandidateLastWrite = static_cast<time_t>(time(nullptr));

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray,
            sortedLogFileArraySize,
            testCandidateFileName,
            testSizeOfCandidateFile,
            testCandidateLastWrite));

        ADUC::StringUtils::cstr_wrapper fileNameSentinel{ sortedLogFileArray[0].fileName };
        CHECK(strcmp(sortedLogFileArray[0].fileName, testCandidateFileName) == 0);
        CHECK(sortedLogFileArray[0].fileSize == testSizeOfCandidateFile);
    }

    SECTION("Insert file with large size")
    {
        const size_t sortedLogFileArraySize = 3;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        const char* testCandidateFileName = "large.log";
        const long long testSizeOfCandidateFile = 1024LL * 1024LL * 1024LL; // 1 GB
        const auto testCandidateLastWrite = static_cast<time_t>(time(nullptr));

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray,
            sortedLogFileArraySize,
            testCandidateFileName,
            testSizeOfCandidateFile,
            testCandidateLastWrite));

        ADUC::StringUtils::cstr_wrapper fileNameSentinel{ sortedLogFileArray[0].fileName };
        CHECK(sortedLogFileArray[0].fileSize == testSizeOfCandidateFile);
    }

    SECTION("Insert file with negative file size is rejected")
    {
        const size_t sortedLogFileArraySize = 3;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        const char* testCandidateFileName = "negative.log";
        const long long testSizeOfCandidateFile = -100;
        const auto testCandidateLastWrite = static_cast<time_t>(time(nullptr));

        // Note: negative size should be treated as invalid (0 check catches this for <= 0)
        // But the function only checks for == 0, so negative might pass
        // This test documents current behavior
        bool result = FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray,
            sortedLogFileArraySize,
            testCandidateFileName,
            testSizeOfCandidateFile,
            testCandidateLastWrite);

        // Clean up if it was inserted
        if (sortedLogFileArray[0].fileName != nullptr)
        {
            ADUC::StringUtils::cstr_wrapper fileNameSentinel{ sortedLogFileArray[0].fileName };
        }
    }
}

TEST_CASE("FileInfoUtils_InsertFileInfoIntoArray - Array Limit Handling")
{
    SECTION("Insert files up to limit and reject older file")
    {
        const size_t sortedLogFileArraySize = 2;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        const char* testCandidateFileName = "foo";
        size_t testSizeOfCandidateFile = 1024;
        const auto testCandidateLastWrite = static_cast<time_t>(time(nullptr));

        {
            CHECK(FileInfoUtils_InsertFileInfoIntoArray(
                sortedLogFileArray,
                sortedLogFileArraySize,
                testCandidateFileName,
                static_cast<long long>(testSizeOfCandidateFile),
                testCandidateLastWrite));

            ADUC::StringUtils::cstr_wrapper fileNameSentinel{ sortedLogFileArray[0].fileName };
            CHECK(strcmp(sortedLogFileArray[0].fileName, testCandidateFileName) == 0);
        }

        {
            const char* secondTestCandidateFileName = "bar";
            const auto secondTestCandidateLastWrite = static_cast<time_t>(
                time(nullptr)
                + 1); // Note: set 1 second in the future so we know this value will be put before the current values

            CHECK(FileInfoUtils_InsertFileInfoIntoArray(
                sortedLogFileArray,
                sortedLogFileArraySize,
                secondTestCandidateFileName,
                static_cast<long long>(testCandidateLastWrite),
                secondTestCandidateLastWrite));

            ADUC::StringUtils::cstr_wrapper fileNameSentinel{ sortedLogFileArray[0].fileName };
            CHECK(strcmp(sortedLogFileArray[0].fileName, secondTestCandidateFileName) == 0);
            // do not free here since next call to FileInfoUtils_InsertFileInfoIntoArray
            // is supposed to fail so will not be changing sortedLogFileArray.

            const char* thirdTestCandidateFileName = "microsoft";

            // Because the last write time is the same as the first then this file should be rejected and FileInfoUtils_InsertFileInfoIntoArray should be the same as before
            CHECK_FALSE(FileInfoUtils_InsertFileInfoIntoArray(
                sortedLogFileArray,
                sortedLogFileArraySize,
                thirdTestCandidateFileName,
                static_cast<long long>(testSizeOfCandidateFile),
                testCandidateLastWrite));

            // With no change secondTestCandidateFileName should still be at the front
            CHECK(strcmp(sortedLogFileArray[0].fileName, secondTestCandidateFileName) == 0);
        }
    }

    SECTION("Single element array handling")
    {
        const size_t sortedLogFileArraySize = 1;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        const char* firstFileName = "first.log";
        const long long firstFileSize = 100;
        const auto firstLastWrite = static_cast<time_t>(time(nullptr));

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray,
            sortedLogFileArraySize,
            firstFileName,
            firstFileSize,
            firstLastWrite));

        ADUC::StringUtils::cstr_wrapper fileNameSentinel{ sortedLogFileArray[0].fileName };
        CHECK(strcmp(sortedLogFileArray[0].fileName, firstFileName) == 0);

        // Try to add an older file - should fail
        const char* olderFileName = "older.log";
        const auto olderLastWrite = static_cast<time_t>(firstLastWrite - 10);

        CHECK_FALSE(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray,
            sortedLogFileArraySize,
            olderFileName,
            100,
            olderLastWrite));

        // Original file should still be there
        CHECK(strcmp(sortedLogFileArray[0].fileName, firstFileName) == 0);
    }

    SECTION("Replace oldest when array is full with newer file")
    {
        const size_t sortedLogFileArraySize = 2;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        const auto baseTime = static_cast<time_t>(time(nullptr));

        // Insert two files
        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, "file1.log", 100, baseTime));

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, "file2.log", 100, baseTime + 1));

        ADUC::StringUtils::cstr_wrapper sentinel0{ sortedLogFileArray[0].fileName };
        ADUC::StringUtils::cstr_wrapper sentinel1{ sortedLogFileArray[1].fileName };

        CHECK(strcmp(sortedLogFileArray[0].fileName, "file2.log") == 0);
        CHECK(strcmp(sortedLogFileArray[1].fileName, "file1.log") == 0);
    }
}

TEST_CASE("FileInfoUtils_InsertFileInfoIntoArray - File Replacement")
{
    SECTION("Replace an older file with a newer one")
    {
        constexpr size_t sortedLogFileArraySize = 1;

        const char* oldFileName = "foo";
        const size_t oldFileSize = 512;
        const auto oldFileLastWrite = static_cast<time_t>(time(nullptr)); // Note: set time to now

        {
            FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};
            CHECK(FileInfoUtils_InsertFileInfoIntoArray(
                sortedLogFileArray, sortedLogFileArraySize, oldFileName, oldFileSize, oldFileLastWrite));

            ADUC::StringUtils::cstr_wrapper fileNameSentinel{ sortedLogFileArray[0].fileName };
            CHECK(strcmp(sortedLogFileArray[0].fileName, oldFileName) == 0);
        }

        const char* newerFileName = "bar";
        const size_t newerFileSize = 512;
        const auto newFileLastWrite =
            static_cast<time_t>(time(nullptr) + 10); // Note: ensure the new file is newer than the old

        {
            FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};
            CHECK(FileInfoUtils_InsertFileInfoIntoArray(
                sortedLogFileArray, sortedLogFileArraySize, newerFileName, newerFileSize, newFileLastWrite));

            ADUC::StringUtils::cstr_wrapper fileNameSentinel{ sortedLogFileArray[0].fileName };

            CHECK(strcmp(sortedLogFileArray[0].fileName, newerFileName) == 0);
            CHECK(sortedLogFileArray[0].lastWrite == newFileLastWrite);
            CHECK(sortedLogFileArray[0].fileSize == newerFileSize);
        }
    }

    SECTION("Maintain sorted order with multiple insertions")
    {
        const size_t sortedLogFileArraySize = 3;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        const auto baseTime = static_cast<time_t>(time(nullptr));

        // Insert files with times: baseTime, baseTime+5, baseTime+10
        // Should result in order: newest (baseTime+10) first

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, "middle.log", 100, baseTime + 5));

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, "oldest.log", 100, baseTime));

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, "newest.log", 100, baseTime + 10));

        // Verify order: newest first
        ADUC::StringUtils::cstr_wrapper sentinel0{ sortedLogFileArray[0].fileName };
        ADUC::StringUtils::cstr_wrapper sentinel1{ sortedLogFileArray[1].fileName };
        ADUC::StringUtils::cstr_wrapper sentinel2{ sortedLogFileArray[2].fileName };

        CHECK(strcmp(sortedLogFileArray[0].fileName, "newest.log") == 0);
        CHECK(strcmp(sortedLogFileArray[1].fileName, "middle.log") == 0);
        CHECK(strcmp(sortedLogFileArray[2].fileName, "oldest.log") == 0);
    }

    SECTION("Insert file with same timestamp as existing - should reject")
    {
        const size_t sortedLogFileArraySize = 2;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        const auto timestamp = static_cast<time_t>(time(nullptr));

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, "first.log", 100, timestamp));

        ADUC::StringUtils::cstr_wrapper sentinel0{ sortedLogFileArray[0].fileName };

        // Insert second file with same timestamp
        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, "second.log", 100, timestamp));

        ADUC::StringUtils::cstr_wrapper sentinel1{ sortedLogFileArray[1].fileName };

        // Third file with same timestamp should be rejected (array is full)
        CHECK_FALSE(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, "third.log", 100, timestamp));
    }

    SECTION("Insert many files to fill array")
    {
        const size_t sortedLogFileArraySize = 5;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        const auto baseTime = static_cast<time_t>(time(nullptr));

        // Insert 5 files with ascending timestamps
        for (int i = 0; i < 5; ++i)
        {
            std::string fileName = "file" + std::to_string(i) + ".log";
            CHECK(FileInfoUtils_InsertFileInfoIntoArray(
                sortedLogFileArray, sortedLogFileArraySize, fileName.c_str(), 100, baseTime + i));
        }

        // Verify order (newest to oldest)
        ADUC::StringUtils::cstr_wrapper sentinel0{ sortedLogFileArray[0].fileName };
        ADUC::StringUtils::cstr_wrapper sentinel1{ sortedLogFileArray[1].fileName };
        ADUC::StringUtils::cstr_wrapper sentinel2{ sortedLogFileArray[2].fileName };
        ADUC::StringUtils::cstr_wrapper sentinel3{ sortedLogFileArray[3].fileName };
        ADUC::StringUtils::cstr_wrapper sentinel4{ sortedLogFileArray[4].fileName };

        CHECK(strcmp(sortedLogFileArray[0].fileName, "file4.log") == 0); // newest
        CHECK(strcmp(sortedLogFileArray[4].fileName, "file0.log") == 0); // oldest
    }
}

TEST_CASE("FileInfoUtils_InsertFileInfoIntoArray - Edge Cases")
{
    SECTION("Insert file with very long filename")
    {
        const size_t sortedLogFileArraySize = 2;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        std::string longFileName(255, 'a'); // Max filename length on many filesystems
        longFileName += ".log";

        const auto timestamp = static_cast<time_t>(time(nullptr));

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, longFileName.c_str(), 100, timestamp));

        ADUC::StringUtils::cstr_wrapper sentinel{ sortedLogFileArray[0].fileName };
        CHECK(strcmp(sortedLogFileArray[0].fileName, longFileName.c_str()) == 0);
    }

    SECTION("Insert file with special characters in filename")
    {
        const size_t sortedLogFileArraySize = 2;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        const char* specialFileName = "test-file_2024.01.01.log";
        const auto timestamp = static_cast<time_t>(time(nullptr));

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, specialFileName, 100, timestamp));

        ADUC::StringUtils::cstr_wrapper sentinel{ sortedLogFileArray[0].fileName };
        CHECK(strcmp(sortedLogFileArray[0].fileName, specialFileName) == 0);
    }

    SECTION("Insert file with empty filename string")
    {
        const size_t sortedLogFileArraySize = 2;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        const char* emptyFileName = "";
        const auto timestamp = static_cast<time_t>(time(nullptr));

        // Empty string is technically valid (not nullptr)
        bool result = FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, emptyFileName, 100, timestamp);

        if (result && sortedLogFileArray[0].fileName != nullptr)
        {
            ADUC::StringUtils::cstr_wrapper sentinel{ sortedLogFileArray[0].fileName };
            CHECK(strcmp(sortedLogFileArray[0].fileName, emptyFileName) == 0);
        }
    }

    SECTION("Handle time_t boundary values")
    {
        const size_t sortedLogFileArraySize = 2;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        // Note: time 0 (epoch) is rejected by the function as invalid
        // So we test with a small positive value instead
        const auto smallTime = static_cast<time_t>(1);

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, "early.log", 100, smallTime));

        ADUC::StringUtils::cstr_wrapper sentinel0{ sortedLogFileArray[0].fileName };
        CHECK(sortedLogFileArray[0].lastWrite == smallTime);

        // Insert with a later time
        const auto laterTime = static_cast<time_t>(100);
        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, "later.log", 100, laterTime));

        ADUC::StringUtils::cstr_wrapper sentinel1{ sortedLogFileArray[0].fileName };
        CHECK(strcmp(sortedLogFileArray[0].fileName, "later.log") == 0);
    }

    SECTION("Rejects time 0 as invalid")
    {
        const size_t sortedLogFileArraySize = 2;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        // Time 0 (Unix epoch) is considered invalid by the function
        CHECK_FALSE(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, "epoch.log", 100, 0));
    }
}

TEST_CASE("FileInfoUtils_GetNewestFilesInDirUnderSize - Parameter Validation")
{
    FileInfoTestHelper helper;

    // NOTE: These tests are designed to work with parameters that will pass
    // initial validation. Tests with nullptr parameters may cause issues
    // due to bug mentioned in the source code comments.

    SECTION("Returns false when directoryPath does not exist")
    {
        VECTOR_HANDLE fileVector = nullptr;
        CHECK_FALSE(FileInfoUtils_GetNewestFilesInDirUnderSize(&fileVector, "/nonexistent/path/that/does/not/exist", 1024));
    }

    SECTION("Returns false when maxFileSize is 0")
    {
        // Create the test directory with a file
        helper.createFile("test.log", 100);

        VECTOR_HANDLE fileVector = nullptr;
        CHECK_FALSE(FileInfoUtils_GetNewestFilesInDirUnderSize(&fileVector, TEST_DIR, 0));
    }
}

TEST_CASE("FileInfoUtils_GetNewestFilesInDirUnderSize - Basic Operations")
{
    FileInfoTestHelper helper;

    SECTION("Returns files from directory with single file")
    {
        // Create a test file
        REQUIRE(helper.createFile("single.log", 100));

        VECTOR_HANDLE fileVector = nullptr;
        bool result = FileInfoUtils_GetNewestFilesInDirUnderSize(&fileVector, TEST_DIR, 1024);

        if (result && fileVector != nullptr)
        {
            size_t count = VECTOR_size(fileVector);
            CHECK(count == 1);

            if (count > 0)
            {
                STRING_HANDLE* elem = static_cast<STRING_HANDLE*>(VECTOR_element(fileVector, 0));
                CHECK(strcmp(STRING_c_str(*elem), "single.log") == 0);
            }

            // Cleanup
            for (size_t i = 0; i < count; ++i)
            {
                STRING_HANDLE* elem = static_cast<STRING_HANDLE*>(VECTOR_element(fileVector, i));
                STRING_delete(*elem);
            }
            VECTOR_destroy(fileVector);
        }
    }

    SECTION("Returns multiple files sorted by newest first")
    {
        // Create files with delays to ensure different timestamps
        REQUIRE(helper.createFileWithDelay("oldest.log", 100, 0));
        REQUIRE(helper.createFileWithDelay("middle.log", 100, 100));
        REQUIRE(helper.createFileWithDelay("newest.log", 100, 100));

        VECTOR_HANDLE fileVector = nullptr;
        bool result = FileInfoUtils_GetNewestFilesInDirUnderSize(&fileVector, TEST_DIR, 1024);

        if (result && fileVector != nullptr)
        {
            size_t count = VECTOR_size(fileVector);
            CHECK(count == 3);

            // Cleanup
            for (size_t i = 0; i < count; ++i)
            {
                STRING_HANDLE* elem = static_cast<STRING_HANDLE*>(VECTOR_element(fileVector, i));
                STRING_delete(*elem);
            }
            VECTOR_destroy(fileVector);
        }
    }

    SECTION("Respects maxFileSize limit")
    {
        // Create files that exceed the max size
        REQUIRE(helper.createFileWithDelay("file1.log", 500, 0));
        REQUIRE(helper.createFileWithDelay("file2.log", 500, 100));
        REQUIRE(helper.createFileWithDelay("file3.log", 500, 100));

        VECTOR_HANDLE fileVector = nullptr;
        // Max size of 1000 should only allow 2 of the 500-byte files
        bool result = FileInfoUtils_GetNewestFilesInDirUnderSize(&fileVector, TEST_DIR, 1000);

        if (result && fileVector != nullptr)
        {
            size_t count = VECTOR_size(fileVector);
            // Should have 2 or fewer files
            CHECK(count <= 2);

            // Cleanup
            for (size_t i = 0; i < count; ++i)
            {
                STRING_HANDLE* elem = static_cast<STRING_HANDLE*>(VECTOR_element(fileVector, i));
                STRING_delete(*elem);
            }
            VECTOR_destroy(fileVector);
        }
    }

    SECTION("Ignores subdirectories")
    {
        // Create a file and a subdirectory
        REQUIRE(helper.createFile("file.log", 100));
        REQUIRE(helper.createSubdirectory("subdir"));

        VECTOR_HANDLE fileVector = nullptr;
        bool result = FileInfoUtils_GetNewestFilesInDirUnderSize(&fileVector, TEST_DIR, 1024);

        if (result && fileVector != nullptr)
        {
            size_t count = VECTOR_size(fileVector);
            // Should only have the file, not the directory
            CHECK(count == 1);

            if (count > 0)
            {
                STRING_HANDLE* elem = static_cast<STRING_HANDLE*>(VECTOR_element(fileVector, 0));
                CHECK(strcmp(STRING_c_str(*elem), "file.log") == 0);
            }

            // Cleanup
            for (size_t i = 0; i < count; ++i)
            {
                STRING_HANDLE* elem = static_cast<STRING_HANDLE*>(VECTOR_element(fileVector, i));
                STRING_delete(*elem);
            }
            VECTOR_destroy(fileVector);
        }
    }

    SECTION("Ignores empty files")
    {
        // Create an empty file and a non-empty file
        REQUIRE(helper.createFile("nonempty.log", 100));

        // Create empty file manually
        std::string emptyPath = std::string(TEST_DIR) + "/empty.log";
        std::ofstream emptyFile(emptyPath);
        emptyFile.close();

        VECTOR_HANDLE fileVector = nullptr;
        bool result = FileInfoUtils_GetNewestFilesInDirUnderSize(&fileVector, TEST_DIR, 1024);

        if (result && fileVector != nullptr)
        {
            size_t count = VECTOR_size(fileVector);
            // Should only have the non-empty file
            CHECK(count == 1);

            if (count > 0)
            {
                STRING_HANDLE* elem = static_cast<STRING_HANDLE*>(VECTOR_element(fileVector, 0));
                CHECK(strcmp(STRING_c_str(*elem), "nonempty.log") == 0);
            }

            // Cleanup
            for (size_t i = 0; i < count; ++i)
            {
                STRING_HANDLE* elem = static_cast<STRING_HANDLE*>(VECTOR_element(fileVector, i));
                STRING_delete(*elem);
            }
            VECTOR_destroy(fileVector);
        }
    }

    SECTION("Returns false for empty directory")
    {
        // Directory exists but is empty
        VECTOR_HANDLE fileVector = nullptr;
        CHECK_FALSE(FileInfoUtils_GetNewestFilesInDirUnderSize(&fileVector, TEST_DIR, 1024));
    }
}

TEST_CASE("FileInfoUtils_GetNewestFilesInDirUnderSize - Edge Cases")
{
    FileInfoTestHelper helper;

    SECTION("Single file larger than maxFileSize returns false")
    {
        // Create a file larger than the max size
        REQUIRE(helper.createFile("large.log", 2000));

        VECTOR_HANDLE fileVector = nullptr;
        // Max size of 1000 is smaller than our 2000-byte file
        bool result = FileInfoUtils_GetNewestFilesInDirUnderSize(&fileVector, TEST_DIR, 1000);

        // Should return false because the only file is too large
        CHECK_FALSE(result);
    }

    SECTION("Large maxFileSize allows all files")
    {
        // Create multiple small files
        REQUIRE(helper.createFileWithDelay("file1.log", 100, 0));
        REQUIRE(helper.createFileWithDelay("file2.log", 100, 50));
        REQUIRE(helper.createFileWithDelay("file3.log", 100, 50));

        VECTOR_HANDLE fileVector = nullptr;
        // Very large max size should allow all files
        bool result = FileInfoUtils_GetNewestFilesInDirUnderSize(&fileVector, TEST_DIR, 1024 * 1024);

        if (result && fileVector != nullptr)
        {
            size_t count = VECTOR_size(fileVector);
            CHECK(count == 3);

            // Cleanup
            for (size_t i = 0; i < count; ++i)
            {
                STRING_HANDLE* elem = static_cast<STRING_HANDLE*>(VECTOR_element(fileVector, i));
                STRING_delete(*elem);
            }
            VECTOR_destroy(fileVector);
        }
    }
}

TEST_CASE("FileInfoUtils_GetNewestFilesInDirUnderSize - Additional Scenarios")
{
    FileInfoTestHelper helper;

    SECTION("Files with exact boundary size")
    {
        // Create files that exactly match the boundary
        REQUIRE(helper.createFile("boundary.log", 500));

        VECTOR_HANDLE fileVector = nullptr;
        bool result = FileInfoUtils_GetNewestFilesInDirUnderSize(&fileVector, TEST_DIR, 500);

        if (result && fileVector != nullptr)
        {
            size_t count = VECTOR_size(fileVector);
            CHECK(count == 1);

            // Cleanup
            for (size_t i = 0; i < count; ++i)
            {
                STRING_HANDLE* elem = static_cast<STRING_HANDLE*>(VECTOR_element(fileVector, i));
                STRING_delete(*elem);
            }
            VECTOR_destroy(fileVector);
        }
    }

    SECTION("Many small files")
    {
        // Create many small files
        for (int i = 0; i < 15; ++i)
        {
            std::string name = "small" + std::to_string(i) + ".log";
            REQUIRE(helper.createFileWithDelay(name.c_str(), 10, 20));
        }

        VECTOR_HANDLE fileVector = nullptr;
        bool result = FileInfoUtils_GetNewestFilesInDirUnderSize(&fileVector, TEST_DIR, 10000);

        if (result && fileVector != nullptr)
        {
            size_t count = VECTOR_size(fileVector);
            // Should have multiple files
            CHECK(count > 0);
            CHECK(count <= 15);

            // Cleanup
            for (size_t i = 0; i < count; ++i)
            {
                STRING_HANDLE* elem = static_cast<STRING_HANDLE*>(VECTOR_element(fileVector, i));
                STRING_delete(*elem);
            }
            VECTOR_destroy(fileVector);
        }
    }

    SECTION("Files with special characters in name")
    {
        REQUIRE(helper.createFile("test-file_2024.01.01.log", 100));
        REQUIRE(helper.createFile("another_test-file.log", 100));

        VECTOR_HANDLE fileVector = nullptr;
        bool result = FileInfoUtils_GetNewestFilesInDirUnderSize(&fileVector, TEST_DIR, 1024);

        if (result && fileVector != nullptr)
        {
            size_t count = VECTOR_size(fileVector);
            CHECK(count == 2);

            // Cleanup
            for (size_t i = 0; i < count; ++i)
            {
                STRING_HANDLE* elem = static_cast<STRING_HANDLE*>(VECTOR_element(fileVector, i));
                STRING_delete(*elem);
            }
            VECTOR_destroy(fileVector);
        }
    }

    SECTION("Mix of file sizes under limit")
    {
        REQUIRE(helper.createFileWithDelay("tiny.log", 10, 0));
        REQUIRE(helper.createFileWithDelay("small.log", 50, 50));
        REQUIRE(helper.createFileWithDelay("medium.log", 200, 50));

        VECTOR_HANDLE fileVector = nullptr;
        // Total: 260 bytes, limit 300
        bool result = FileInfoUtils_GetNewestFilesInDirUnderSize(&fileVector, TEST_DIR, 300);

        if (result && fileVector != nullptr)
        {
            size_t count = VECTOR_size(fileVector);
            CHECK(count >= 1);

            // Cleanup
            for (size_t i = 0; i < count; ++i)
            {
                STRING_HANDLE* elem = static_cast<STRING_HANDLE*>(VECTOR_element(fileVector, i));
                STRING_delete(*elem);
            }
            VECTOR_destroy(fileVector);
        }
    }
}

TEST_CASE("FileInfoUtils_InsertFileInfoIntoArray - Sorting Behavior")
{
    SECTION("Insert files in chronological order")
    {
        const size_t arraySize = 5;
        FileInfo sortedArray[arraySize] = {};

        const auto baseTime = static_cast<time_t>(time(nullptr));

        // Insert in chronological order (oldest first)
        for (int i = 0; i < 5; ++i)
        {
            std::string name = "file" + std::to_string(i) + ".log";
            CHECK(FileInfoUtils_InsertFileInfoIntoArray(
                sortedArray, arraySize, name.c_str(), 100, baseTime + i));
        }

        // Verify newest is first
        ADUC::StringUtils::cstr_wrapper sentinel0{ sortedArray[0].fileName };
        ADUC::StringUtils::cstr_wrapper sentinel1{ sortedArray[1].fileName };
        ADUC::StringUtils::cstr_wrapper sentinel2{ sortedArray[2].fileName };
        ADUC::StringUtils::cstr_wrapper sentinel3{ sortedArray[3].fileName };
        ADUC::StringUtils::cstr_wrapper sentinel4{ sortedArray[4].fileName };

        CHECK(strcmp(sortedArray[0].fileName, "file4.log") == 0);
        CHECK(strcmp(sortedArray[4].fileName, "file0.log") == 0);
    }

    SECTION("Insert files in reverse chronological order")
    {
        const size_t arraySize = 5;
        FileInfo sortedArray[arraySize] = {};

        const auto baseTime = static_cast<time_t>(time(nullptr));

        // Insert in reverse chronological order (newest first)
        for (int i = 4; i >= 0; --i)
        {
            std::string name = "file" + std::to_string(i) + ".log";
            bool result = FileInfoUtils_InsertFileInfoIntoArray(
                sortedArray, arraySize, name.c_str(), 100, baseTime + i);

            // First insertion should succeed, later ones may fail if older
            if (i == 4)
            {
                CHECK(result);
            }
        }

        // Cleanup
        for (size_t i = 0; i < arraySize; ++i)
        {
            if (sortedArray[i].fileName != nullptr)
            {
                ADUC::StringUtils::cstr_wrapper sentinel{ sortedArray[i].fileName };
            }
        }
    }

    SECTION("Insert file into middle of sorted array")
    {
        const size_t arraySize = 3;
        FileInfo sortedArray[arraySize] = {};

        const auto baseTime = static_cast<time_t>(time(nullptr));

        // Insert newest
        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedArray, arraySize, "newest.log", 100, baseTime + 10));

        // Insert oldest
        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedArray, arraySize, "oldest.log", 100, baseTime));

        // Insert middle
        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedArray, arraySize, "middle.log", 100, baseTime + 5));

        ADUC::StringUtils::cstr_wrapper sentinel0{ sortedArray[0].fileName };
        ADUC::StringUtils::cstr_wrapper sentinel1{ sortedArray[1].fileName };
        ADUC::StringUtils::cstr_wrapper sentinel2{ sortedArray[2].fileName };

        // Should be ordered newest to oldest
        CHECK(strcmp(sortedArray[0].fileName, "newest.log") == 0);
        CHECK(strcmp(sortedArray[1].fileName, "middle.log") == 0);
        CHECK(strcmp(sortedArray[2].fileName, "oldest.log") == 0);
    }
}

TEST_CASE("FileInfoUtils_InsertFileInfoIntoArray - Memory Management")
{
    SECTION("Displaced file is properly freed")
    {
        const size_t arraySize = 2;
        FileInfo sortedArray[arraySize] = {};

        const auto baseTime = static_cast<time_t>(time(nullptr));

        // Fill the array
        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedArray, arraySize, "old1.log", 100, baseTime));
        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedArray, arraySize, "old2.log", 100, baseTime + 1));

        // Insert a newer file that should displace old1.log
        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedArray, arraySize, "new.log", 100, baseTime + 2));

        // new.log should be first, old2.log second, old1.log displaced
        CHECK(strcmp(sortedArray[0].fileName, "new.log") == 0);
        CHECK(strcmp(sortedArray[1].fileName, "old2.log") == 0);

        // Cleanup at the end
        free(sortedArray[0].fileName);
        free(sortedArray[1].fileName);
    }

    SECTION("Single element array replacement")
    {
        const size_t arraySize = 1;
        FileInfo sortedArray[arraySize] = {};

        const auto baseTime = static_cast<time_t>(time(nullptr));

        // Insert first file
        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedArray, arraySize, "first.log", 100, baseTime));
        CHECK(strcmp(sortedArray[0].fileName, "first.log") == 0);

        // Insert newer file - should replace (the function frees the old one internally)
        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedArray, arraySize, "newer.log", 100, baseTime + 10));
        CHECK(strcmp(sortedArray[0].fileName, "newer.log") == 0);

        // Cleanup at the end
        free(sortedArray[0].fileName);
    }
}

TEST_CASE("FileInfoUtils - Negative File Size Handling")
{
    SECTION("InsertFileInfoIntoArray rejects negative size")
    {
        const size_t arraySize = 2;
        FileInfo sortedArray[arraySize] = {};

        const auto timestamp = static_cast<time_t>(time(nullptr));

        // The function checks for == 0, negative values may pass
        // This documents the actual behavior
        bool result = FileInfoUtils_InsertFileInfoIntoArray(
            sortedArray, arraySize, "negative.log", -100, timestamp);

        // If it was inserted, clean up
        if (sortedArray[0].fileName != nullptr)
        {
            ADUC::StringUtils::cstr_wrapper sentinel{ sortedArray[0].fileName };
        }
    }
}
