/**
 * @file file_upload_utility.h
 * @brief Defines the interface for interacting with Azure Blob Storage and uploading files
 *
 * @copyright Copyright (c) Microsoft Corp.
 */
#ifndef FILE_UPLOAD_UTILITY_H
#define FILE_UPLOAD_UTILITY_H

#include <aduc/c_utils.h>
#include <azure_c_shared_utility/strings.h>
#include <azure_c_shared_utility/vector.h>
#include <stdbool.h>
#include <stdlib.h>

EXTERN_C_BEGIN

/**
 * @brief Struct that contains the information for uploading a set of blobs to Azure Blob Storage
 */
typedef struct tagBlobStorageInfo
{
    STRING_HANDLE
    virtualDirectoryPath; //!< Virtual hierarchy for the blobs
    STRING_HANDLE storageSasCredential; //!< Combined SAS URI and SAS Token for connecting to storage
} BlobStorageInfo;

bool FileUploadUtility_UploadFilesToContainer(
    const BlobStorageInfo* blobInfo, VECTOR_HANDLE fileNames, const char* directoryPath);

EXTERN_C_END

#endif // FILE_UPLOAD_UTILITY_H
