/**
 * @file blob_storage_helper.cpp
 * @brief Implements the interface for interacting with Azure Blob Storage
 *
 * @copyright Copyright (c) Microsoft Corp.
 */
#include "blob_storage_helper.hpp"

#include <aduc/exception_utils.hpp>
#include <azure_c_shared_utility/string_token.h>
#include <azure_c_shared_utility/urlencode.h>
#include <cstring>
#include <fstream>

/**
 * @brief Creates the blob storage client using the information in @p blobInfo and then constructs the object
 * @param blobInfo information related to the blob storage account
 * @param maxConcurrency the maximum number of threads that can be working at one time for the client connection
 */
AzureBlobStorageHelper::AzureBlobStorageHelper(const BlobStorageInfo& blobInfo)
{
    if (blobInfo.storageSasCredential == nullptr)
    {
        throw std::invalid_argument("Container name invalid");
    }

    client = std::make_unique<Azure::Storage::Blobs::BlobContainerClient>(STRING_c_str(blobInfo.storageSasCredential));
}

/**
 * @brief strips the file name from the @p filePath
 * @param fileName a properly formed name for the file who's path is being constructed- must include extension if the file has one
 * @param directoryPath a properly formed path to the directory where the file exists
 * @returns the filePath, or ""
 */
std::string
AzureBlobStorageHelper::CreatePathFromFileAndDirectory(const std::string& fileName, const std::string& directoryPath)
{
    if (fileName.empty() || directoryPath.empty())
    {
        throw std::invalid_argument(__FUNCTION__);
    }

    bool needsSeperator = false;
    if (directoryPath.find_last_of('/') != directoryPath.length() - 1)
    {
        needsSeperator = true;
    }

    if (needsSeperator)
    {
        return directoryPath + "/" + fileName;
    }
    return directoryPath + fileName;
}

/**
 * @brief Uploads all the files listed in @p files using the storage account associated with this object
 * @param fileNames vector of file names to upload
 * @param directoryPath path to the directory where @p fileNames can be found
 * @param virtualDirectory a properly formatted virtual directory (ending in '/') to be used when uploading the files
 * @returns true on success; false on any failure
 */
bool AzureBlobStorageHelper::UploadFilesToContainer(
    VECTOR_HANDLE fileNames, const std::string& directoryPath, const std::string& virtualDirectory)
{
    if (VECTOR_size(fileNames) == 0)
    {
        throw std::invalid_argument("blobInfo.storageSasCredential");
    }

    std::string virtualDirectoryPath = virtualDirectory;
    if (!virtualDirectoryPath.empty() && virtualDirectoryPath[virtualDirectoryPath.length() - 1] != '/')
    {
        virtualDirectoryPath += "/";
    }

    size_t fileNameSize = VECTOR_size(fileNames);
    for (unsigned int i = 0; i < fileNameSize; ++i)
    {
        auto fileNameHandle = static_cast<const STRING_HANDLE*>(VECTOR_element(fileNames, i));
        const char* fileName = STRING_c_str(*fileNameHandle);

        ADUC::ExceptionUtils::CallVoidMethodAndHandleExceptions(
            [fileName, directoryPath, virtualDirectoryPath, this]() -> void {
                std::string filePath = CreatePathFromFileAndDirectory(fileName, directoryPath);

                Azure::Core::IO::FileBodyStream fileStream(filePath);

                std::string blobName = virtualDirectoryPath + fileName;

                std::vector<std::pair<std::string, std::string>> metadata;

                this->client->UploadBlob(blobName, fileStream);
            });
    }

    return true;
}
