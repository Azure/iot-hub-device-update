/**
 * @file file_info_utils.h
 * @brief Header file for utilities scanning, parsing, and interacting with the file system
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include <aduc/c_utils.h>
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <umock_c/umock_c_prod.h>

#ifndef FILE_INFO_UTILS_H
#    define FILE_INFO_UTILS_H

EXTERN_C_BEGIN

/**
 * @brief Struct that contains all the information for sorting files in order of newest file and processing according to the max allowed file size later on
 */
typedef struct tagFileInfo
{
    unsigned long fileSize; //!< size of the file in bytes
    char* fileName; //!< the name of the file
    time_t lastWrite; //!< the last time the file was modified
} FileInfo;

_Bool FileInfoUtils_GetNewestFilesInDirUnderSize(
    VECTOR_HANDLE* fileNameVector, const char* directoryPath, const unsigned int maxFileSize);

_Bool FileInfoUtils_InsertFileInfoIntoArray(
    FileInfo* sortedLogFiles,
    size_t sortedLogFileLength,
    const char* candidateFileName,
    unsigned long sizeOfCandidateFile,
    time_t candidateLastWrite);

EXTERN_C_END

#endif // FILE_INFO_UTILS_H
