/**
 * @file blob_storage_helper.hpp
 * @brief Implements the interface for interacting with Azure Blob Storage
 *
 * @copyright Copyright (c) Microsoft Corp.
 */
#include "file_upload_utility.h"

// Note: This is just the top level portion
#include <azure/storage/blobs.hpp>
#include <azure_c_shared_utility/vector.h>
#include <string>
#include <vector>

#ifndef BLOB_STORAGE_HELPER_HPP
#    define BLOB_STORAGE_HELPER_HPP
class AzureBlobStorageHelper
{
private:
    std::unique_ptr<Azure::Storage::Blobs::BlobContainerClient>
        client; // !< client connection object used for uploading files and creating containers

    std::string CreatePathFromFileAndDirectory(const std::string& fileName, const std::string& directoryPath);

public:
    AzureBlobStorageHelper(const BlobStorageInfo& blobInfo);

    bool UploadFilesToContainer(
        const VECTOR_HANDLE fileNames, const std::string& directoryPath, const std::string& virtualDirectory);

    ~AzureBlobStorageHelper() = default;
};

#endif // BLOB_STORAGE_HELPER_HPP
