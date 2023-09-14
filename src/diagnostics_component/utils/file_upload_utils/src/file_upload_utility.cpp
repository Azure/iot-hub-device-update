/**
 * @file file_upload_utility.cpp
 * @brief Implements the interface for interacting with Azure Blob Storage
 *
 * @copyright Copyright (c) Microsoft Corp.
 */

#include "file_upload_utility.h"
#include "blob_storage_helper.hpp"

#include <aduc/exception_utils.hpp>
#include <azure_c_shared_utility/crt_abstractions.h>
#include <azure_c_shared_utility/strings.h>
#include <exception>
#include <string.h>

EXTERN_C_BEGIN

/**
 * @brief Uploads all the files listed in @p files using the storage information in @p blobInfo
 * @param blobInfo struct describing the connection information
 * @param maxConcurrency the max amount of concurrent threads for storage operations
 * @param fileNames vector of STRING_HANDLEs listing the names of the files to be uploaded
 * @param directoryPath path to the directory which holds the files listed in @p fileNames
 * @returns true on successful upload of all files; false on any failure
 */
bool FileUploadUtility_UploadFilesToContainer(
    const BlobStorageInfo* blobInfo, VECTOR_HANDLE fileNames, const char* directoryPath)
{
    if (blobInfo == nullptr || fileNames == nullptr || directoryPath == nullptr)
    {
        return false;
    }

    bool succeeded = false;

    ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions(
        [blobInfo, &fileNames, directoryPath, &succeeded]() -> void {
            AzureBlobStorageHelper storageHelper(*blobInfo);
            succeeded = storageHelper.UploadFilesToContainer(
                fileNames, directoryPath, STRING_c_str(blobInfo->virtualDirectoryPath));
        });

    return succeeded;
}

EXTERN_C_END
