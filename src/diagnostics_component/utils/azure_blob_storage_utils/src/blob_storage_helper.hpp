/**
 * @file azure_blob_storage.hpp
 * @brief Implements the interface for interacting with Azure Blob Storage
 *
 * @copyright Copyright (c) 2021, Microsoft Corp.
 */
#include "azure_blob_storage.h"

#include <azure_c_shared_utility/vector.h>
#include <blob/blob_client.h>
#include <storage_account.h>
#include <storage_credential.h>
#include <string.h>
#include <vector>

#ifndef BLOB_STORAGE_HELPER_HPP
#    define BLOB_STORAGE_HELPER_HPP
class AzureBlobStorageHelper
{
private:
    std::shared_ptr<azure::storage_lite::shared_access_signature_credential>
        credential; // !< credential for connecting to the Azure Blob Storage account
    std::shared_ptr<azure::storage_lite::storage_account> account; // !< Account descriptor for the client connection
    std::unique_ptr<azure::storage_lite::blob_client>
        client; // !< client connection object used for uploading files and creating containers

    bool ParseSasUrlForConnectionInfo(std::string& account, std::string& credential, const char* storageSasUrl);
    std::string CreatePathFromFileAndDirectory(const std::string& fileName, const std::string& directoryPath);

public:
    AzureBlobStorageHelper(const BlobStorageInfo& blobInfo, const unsigned int maxConcurrency);

    bool CreateContainerWithCredential(const std::string& containerName);

    bool UploadFilesToContainer(
        const std::string& containerName,
        const VECTOR_HANDLE fileNames,
        const std::string& directoryPath,
        const std::string& virtualDirectory);

    ~AzureBlobStorageHelper() = default;
};

bool AzureBlobStorageHelper_ParseContainerNameFromSasUrl(STRING_HANDLE* containerName, const char* sasUrl);

#endif // BLOB_STORAGE_HELPER_HPP
