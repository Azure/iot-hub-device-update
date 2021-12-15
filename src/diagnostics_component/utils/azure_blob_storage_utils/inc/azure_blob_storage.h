/**
 * @file azure_blob_storage.h
 * @brief Defines the interface for interacting with Azure Blob Storage
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef AZURE_BLOB_STORAGE_H
#define AZURE_BLOB_STORAGE_H

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
    STRING_HANDLE containerName; //!< Name of the container to upload the blobs to
    STRING_HANDLE storageSasCredential; //!< Combined SAS URI and SAS Token for connecting to storage
} BlobStorageInfo;

_Bool UploadFilesToContainer(
    const BlobStorageInfo* blobInfo, const int maxConcurrency, VECTOR_HANDLE fileNames, const char* directoryPath);

_Bool ParseContainerNameFromSasUrl(STRING_HANDLE* containerName, const char* sasUrl);

EXTERN_C_END

#endif // AZURE_BLOB_STORAGE_H
