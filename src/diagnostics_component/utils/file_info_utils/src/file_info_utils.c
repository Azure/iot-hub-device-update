/**
 * @file file_info_utils.h
 * @brief Implementation file for utilities scanning, parsing, and interacting with the file system
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "file_info_utils.h"

#include <aduc/logging.h>
#include <aduc/string_c_utils.h>
#include <azure_c_shared_utility/crt_abstractions.h>
#include <azure_c_shared_utility/strings.h>
#include <dirent.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

/**
 * @brief Maximum amount of files to scan before we quit
 * @details this is to prevent a denial of service attack by some malicious attacker filling the directory with garbage to prevent the diagnostics component from running.
 */
#define MAX_FILES_TO_SCAN 100

/**
 * @brief this is the absolute max amount of files we will upload per component non-dependent on size
 */
#define MAX_FILES_TO_REPORT 20

/**
 * @brief Creates a new entry within the @p sortedLogFiles array when the new file is newer than any current entry up to @p sortedLogFiles
 * @details The array @p sortedLogFiles is kept in order of newest to oldest, expected to be zeroed out before the first call
 * @param sortedLogFiles the array that will contain the sorted files retrieved from the system
 * @param sortedLogFileLength the array of FileInfo structs for the candidate to be inserted into if it is newer than the current values
 * @param candidateFileName the name of the candidate file to be inserted
 * @param sizeOfCandidateFile the size of the candidate file to be inserted
 * @param candidateLastWrite the lastWrite time of the candidate file to be inserted
 */
bool FileInfoUtils_InsertFileInfoIntoArray(
    FileInfo* sortedLogFiles,
    size_t sortedLogFileLength,
    const char* candidateFileName,
    unsigned long sizeOfCandidateFile,
    time_t candidateLastWrite)
{
    if (sortedLogFiles == NULL || sortedLogFileLength < 1 || candidateFileName == NULL || sizeOfCandidateFile == 0)
    {
        return false;
    }

    if (sortedLogFiles[sortedLogFileLength - 1].lastWrite >= candidateLastWrite)
    {
        return false;
    }

    for (size_t index = 0; index < sortedLogFileLength; ++index)
    {
        if (candidateLastWrite > sortedLogFiles[index].lastWrite)
        {
            free(sortedLogFiles[sortedLogFileLength - 1].fileName);
            sortedLogFiles[sortedLogFileLength - 1].fileName = NULL;

            const size_t destIndex = index + 1;

            size_t sizeToMove = (sortedLogFileLength - destIndex);

            memmove((sortedLogFiles + destIndex), (sortedLogFiles + index), sizeToMove * sizeof(FileInfo));

            char* fileName = NULL;

            if (mallocAndStrcpy_s(&fileName, candidateFileName) != 0)
            {
                return false;
            }

            sortedLogFiles[index].fileName = fileName;
            sortedLogFiles[index].fileSize = sizeOfCandidateFile;
            sortedLogFiles[index].lastWrite = candidateLastWrite;
            return true;
        }
    }
    return false;
}

/**
 * @brief Fills @p logFiles with up to @p logFileSize newest files found in the directory at @p directoryPath
 * @details No spelunking, not recursively searching
 * @param[out] logFiles the array of FileInfo structs that will hold the newest files
 * @param[in] logFileSize the size of @p logFiles
 * @param[in] directoryPath the directory to scan for the newest files
 * @returns true if we're able to find new files to add to @p logFiles, false if unsuccessful
 */
_Bool FileInfoUtils_FillFileInfoWithNewestFilesInDir(FileInfo* logFiles, size_t logFileSize, const char* directoryPath)
{
    _Bool succeeded = false;

    if (logFiles == NULL || logFileSize == 0 || directoryPath == NULL)
    {
        return false;
    }

    memset(logFiles, 0, sizeof(FileInfo) * logFileSize);

    unsigned int totalFilesRead = 0;
    DIR* dp = opendir(directoryPath);

    if (dp == NULL)
    {
        goto done;
    }

    // Walk through each file and find each top level file
    do
    {
        struct dirent* entry = readdir(dp); //Note: No need to free according to man readdir is static
        struct stat statbuf;

        STRING_HANDLE filePath = STRING_new();

        if (filePath == NULL)
        {
            goto done;
        }

        if (entry == NULL)
        {
            break;
        }

        if (STRING_sprintf(filePath, "%s/%s", directoryPath, entry->d_name) != 0)
        {
            continue;
        }

        if (stat(STRING_c_str(filePath), &statbuf) == -1)
        {
            STRING_delete(filePath);
            continue;
        }

        // Note: Only care about the first level files that are not symbolic
        if (S_ISDIR(statbuf.st_mode) || S_ISLNK(statbuf.st_mode) || statbuf.st_size == 0)
        {
            STRING_delete(filePath);
            continue;
        }

        FileInfoUtils_InsertFileInfoIntoArray(logFiles, logFileSize, entry->d_name, statbuf.st_size, statbuf.st_mtime);

        STRING_delete(filePath);
        ++totalFilesRead;

    } while (totalFilesRead < MAX_FILES_TO_SCAN);

    if (logFiles[0].fileName == NULL)
    {
        goto done;
    }

    succeeded = true;

done:

    if (!succeeded)
    {
        for (int i = 0; i < logFileSize; ++i)
        {
            FileInfo logFileEntry = logFiles[i];

            free(logFileEntry.fileName);

            memset(&logFileEntry, 0, sizeof(FileInfo));
        }
    }

    if (dp != NULL)
    {
        closedir(dp);
    }

    return succeeded;
}

/**
 * @brief Fills adds up to MAX_FILES_TO_REPORT of the newest files in a directory who's total size is less than @p maxFileSize
 * @param[out] fileNameVector a pointer to a VECTOR_HANDLE of STRING_HANDLEs to be populated with the newest file names who's total size is less than @p maxFileSize - up to the caller to free with Vector_destroy()
 * @param[in] directoryPath path to the directory to scan
 * @param[in] maxFileSize the maximum size of all the files that can be uploaded in bytes
 * @returns true on successful scanning and populating of filePathVectorHandle; false on failure
 */
_Bool FileInfoUtils_GetNewestFilesInDirUnderSize(
    VECTOR_HANDLE* fileNameVector, const char* directoryPath, const unsigned int maxFileSize)
{
    _Bool succeeded = false;

    VECTOR_HANDLE fileVector = NULL;

    // Note: Total amount of files set to MAX_FILES_TO_REPORT to ease diagnostics and scanning efforts.
    FileInfo discoveredFiles[MAX_FILES_TO_REPORT] = {};

    const size_t discoveredFilesSize = ARRAY_SIZE(discoveredFiles);

    if (directoryPath == NULL || fileNameVector == NULL || maxFileSize == 0)
    {
        goto done;
    }

    fileVector = VECTOR_create(sizeof(STRING_HANDLE));

    if (fileVector == NULL)
    {
        goto done;
    }

    if (!FileInfoUtils_FillFileInfoWithNewestFilesInDir(discoveredFiles, discoveredFilesSize, directoryPath))
    {
        goto done;
    }

    int fileIndex = 0;
    unsigned long currentFileMaxCount = 0;
    while (currentFileMaxCount < maxFileSize && fileIndex < discoveredFilesSize)
    {
        if (discoveredFiles[fileIndex].fileName == NULL)
        {
            // No more files to add
            break;
        }
        currentFileMaxCount += discoveredFiles[fileIndex].fileSize;
        ++fileIndex;
    }

    // Only log file found is larger than our maxFileSize
    if (fileIndex == 1 && currentFileMaxCount > maxFileSize)
    {
        goto done;
    }

    for (int i = 0; i < fileIndex; ++i)
    {
        STRING_HANDLE fileName = STRING_construct(discoveredFiles[i].fileName);
        if (fileName == NULL)
        {
            goto done;
        }

        if (VECTOR_push_back(fileVector, &fileName, 1) != 0)
        {
            STRING_delete(fileName);
            goto done;
        }
    }
    succeeded = true;

done:

    for (int i = 0; i < discoveredFilesSize; ++i)
    {
        free(discoveredFiles[i].fileName);
        memset(&discoveredFiles[i], 0, sizeof(FileInfo));
    }

    if (!succeeded)
    {
        const size_t fileVectorSize = VECTOR_size(fileVector);

        for (size_t i = 0; i < fileVectorSize; ++i)
        {
            STRING_HANDLE* elem = (STRING_HANDLE*)VECTOR_element(fileVector, i);
            STRING_delete(*elem);
        }

        VECTOR_destroy(fileVector);
        fileVector = NULL;
    }

    *fileNameVector = fileVector;

    return succeeded;
}
