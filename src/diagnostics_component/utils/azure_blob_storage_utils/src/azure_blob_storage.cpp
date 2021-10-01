/**
 * @file azure_blob_storage.c
 * @brief Implements the interface for interacting with Azure Blob Storage
 *
 * @copyright Copyright (c) 2021, Microsoft Corp.
 */

#include "azure_blob_storage.h"
#include "blob_storage_helper.hpp"

#include <aduc/logging.h>
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
_Bool UploadFilesToContainer(
    const BlobStorageInfo* blobInfo, const int maxConcurrency, VECTOR_HANDLE fileNames, const char* directoryPath)
{
    if (blobInfo == nullptr || maxConcurrency == 0 || fileNames == nullptr || directoryPath == nullptr)
    {
        return false;
    }

    _Bool succeeded = false;

    try
    {
        AzureBlobStorageHelper storageHelper(*blobInfo, maxConcurrency);

        succeeded = storageHelper.UploadFilesToContainer(
            STRING_c_str(blobInfo->containerName),
            fileNames,
            directoryPath,
            STRING_c_str(blobInfo->virtualDirectoryPath));
    }
    catch (std::exception& e)
    {
        Log_Error("UploadFilesToContainer failed with error %s", e.what());
        return false;
    }
    catch (...)
    {
        Log_Error("UploadFilesToContainer with an unknown error");
        return false;
    }

    return succeeded;
}

/**
 * @brief Parses the container name from @p sasUrl
 * @details @p sasUrl must be a proper sas url doc: https://docs.microsoft.com/en-us/azure/storage/common/storage-sas-overview#sas-token
 * @param containerName destination for containerName
 * @param sasUrl url to be stripped of the container name
 * @returns true on success; false on failure
 */
bool ParseContainerNameFromSasUrl(STRING_HANDLE* containerName, const char* sasUrl)
{
    if (containerName == nullptr || sasUrl == nullptr)
    {
        return false;
    }

    try
    {
        return AzureBlobStorageHelper_ParseContainerNameFromSasUrl(containerName, sasUrl);
    }
    catch (std::exception& e)
    {
        Log_Error("ParseContainerNameFromSasUrl failed with error %s", e.what());
        return false;
    }
    catch (...)
    {
        Log_Error("ParseContainerNameFromSasUrl with an unknown error");
        return false;
    }
}

EXTERN_C_END
