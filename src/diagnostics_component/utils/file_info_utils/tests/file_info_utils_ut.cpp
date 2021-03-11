/**
 * @file file_info_utils_ut.cpp
 * @brief Unit Tests for file_info_utils library
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "file_info_utils.h"

#include <aduc/c_utils.h>
#include <catch2/catch.hpp>
#include <ctime>
#include <time.h>
#include <umock_c/umock_c.h>

TEST_CASE("FileInfoUtils_FillFileInfoWithNewestFilesInDir")
{
    SECTION("Parameter validation- Positive")
    {
        const auto testCandidateLastWrite = static_cast<time_t>(time(nullptr));

        CHECK_FALSE(FileInfoUtils_InsertFileInfoIntoArray(nullptr, 0, nullptr, 0, testCandidateLastWrite));
    }
    SECTION("Insert first file")
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
            testSizeOfCandidateFile,
            testCandidateLastWrite));

        CHECK(strcmp(sortedLogFileArray[0].fileName, testCandidateFileName) == 0);
    }
    SECTION("Insert files up to limit and try and add another")
    {
        const size_t sortedLogFileArraySize = 2;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        const char* testCandidateFileName = "foo";
        size_t testSizeOfCandidateFile = 1024;
        const auto testCandidateLastWrite = static_cast<time_t>(time(nullptr));

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray,
            sortedLogFileArraySize,
            testCandidateFileName,
            testSizeOfCandidateFile,
            testCandidateLastWrite));

        CHECK(strcmp(sortedLogFileArray[0].fileName, testCandidateFileName) == 0);

        const char* secondTestCandidateFileName = "bar";
        const auto secondTestCandidateLastWrite = static_cast<time_t>(
            time(nullptr)
            + 1); // Note: set 1 second in the future so we know this value will be put before the current values

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray,
            sortedLogFileArraySize,
            secondTestCandidateFileName,
            testCandidateLastWrite,
            secondTestCandidateLastWrite));

        CHECK(strcmp(sortedLogFileArray[0].fileName, secondTestCandidateFileName) == 0);

        const char* thirdTestCandidateFileName = "microsoft";
        size_t thirTestSizeOfCandidateFile = 1024;

        // Because the last write time is the same as the first then this file should be rejected and FileInfoUtils_InsertFileInfoIntoArray should be the same as before
        CHECK_FALSE(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray,
            sortedLogFileArraySize,
            thirdTestCandidateFileName,
            testSizeOfCandidateFile,
            testCandidateLastWrite));

        // With no change secondTestCandidateFileName should still be at the front
        CHECK(strcmp(sortedLogFileArray[0].fileName, secondTestCandidateFileName) == 0);
    }
    SECTION("Replace an older file with a newer one")
    {
        const size_t sortedLogFileArraySize = 1;
        FileInfo sortedLogFileArray[sortedLogFileArraySize] = {};

        const char* oldFileName = "foo";
        const size_t oldFileSize = 512;
        const auto oldFileLastWrite = static_cast<time_t>(time(nullptr)); // Note: set time to now

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, oldFileName, oldFileSize, oldFileLastWrite));

        CHECK(strcmp(sortedLogFileArray[0].fileName, oldFileName) == 0);

        const char* newerFileName = "bar";
        const size_t newerFileSize = 512;
        const auto newFileLastWrite =
            static_cast<time_t>(time(nullptr) + 10); // Note: ensure the new file is newer than the old

        CHECK(FileInfoUtils_InsertFileInfoIntoArray(
            sortedLogFileArray, sortedLogFileArraySize, newerFileName, newerFileSize, newFileLastWrite));

        CHECK(strcmp(sortedLogFileArray[0].fileName, newerFileName) == 0);
        CHECK(sortedLogFileArray[0].lastWrite == newFileLastWrite);
        CHECK(sortedLogFileArray[0].fileSize == newerFileSize);
    }
}
