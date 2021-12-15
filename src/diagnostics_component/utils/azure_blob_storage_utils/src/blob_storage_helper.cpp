/**
 * @file azure_blob_storage.cpp
 * @brief Implements the interface for interacting with Azure Blob Storage
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "blob_storage_helper.hpp"

#include <aduc/http_url.h>
#include <aduc/logging.h>
#include <aduc/string_handle_wrapper.hpp>
#include <aduc/string_utils.hpp>
#include <azure_c_shared_utility/string_token.h>
#include <azure_c_shared_utility/urlencode.h>
#include <cstring>
#include <fstream>

//
// Sentinel Classes for underlying C structs
//
class HTTPUrlHandleWrapper
{
public:
    HTTP_URL_HANDLE urlHandle = nullptr;

    explicit HTTPUrlHandleWrapper(const char* url)
    {
        urlHandle = http_url_create(url);

        if (urlHandle == nullptr)
        {
            throw std::runtime_error("url could not be parsed");
        }
    }
    HTTPUrlHandleWrapper(const HTTPUrlHandleWrapper&) = delete;
    HTTPUrlHandleWrapper(HTTPUrlHandleWrapper&&) = delete;
    HTTPUrlHandleWrapper& operator=(const HTTPUrlHandleWrapper&) = delete;
    HTTPUrlHandleWrapper& operator=(HTTPUrlHandleWrapper&&) = delete;

    ~HTTPUrlHandleWrapper()
    {
        http_url_destroy(urlHandle);
    }
};

/**
 * @brief Parses the container name from the @p sasUrl
 * @details @p containerName should be freed by caller using STRING_delete()
 * @param[out] containerName value that will be loaded with the container name found in sasUrl
 * @param[in] sasUrl Properly formatted sasUrl of the form: <hostname-url>/<container-name>
 * Refer to
 * @returns true if a properly formatted; false if no container name is found
 */
bool AzureBlobStorageHelper_ParseContainerNameFromSasUrl(STRING_HANDLE* containerName, const char* sasUrl)
{
    if (containerName == nullptr || sasUrl == nullptr)
    {
        return false;
    }

    STRING_HANDLE container = nullptr;
    try
    {
        HTTPUrlHandleWrapper httpUrl(sasUrl);

        const char* path = nullptr;
        size_t pathLen = 0;

        if (http_url_get_path(httpUrl.urlHandle, &path, &pathLen) != 0)
        {
            return false;
        }

        if (pathLen == 0)
        {
            return false;
        }

        STRING_HANDLE container = STRING_construct_n(path, pathLen);
        if (container == nullptr)
        {
            return false;
        }

        //
        // Path is a pointer to the beginning of the path and is managed by pathLen
        const auto slashPos = static_cast<const char*>(strchr(STRING_c_str(container), '/'));

        if (slashPos != nullptr)
        {
            STRING_delete(container);
            return false;
        }

        *containerName = container;
        return true;
    }
    catch (...)
    {
        if (container != nullptr)
        {
            STRING_delete(container);
        }

        return false;
    }
}

/**
 * @brief Creates the blob storage client using the information in @p blobInfo and then constructs the object
 * @param blobInfo information related to the blob storage account
 * @param maxConcurrency the maximum number of threads that can be working at one time for the client connection
 */
AzureBlobStorageHelper::AzureBlobStorageHelper(const BlobStorageInfo& blobInfo, const unsigned int maxConcurrency)
{
    if (blobInfo.containerName == nullptr)
    {
        throw std::invalid_argument("Container name invalid");
    }

    std::string accountName;
    std::string sasCredential;

    if (!ParseSasUrlForConnectionInfo(accountName, sasCredential, STRING_c_str(blobInfo.storageSasCredential)))
    {
        throw std::invalid_argument("Could not parse storageSasCredential for account and sas token");
    }

    credential = std::make_shared<azure::storage_lite::shared_access_signature_credential>(sasCredential);

    account = std::make_shared<azure::storage_lite::storage_account>(accountName, credential, /*use https*/ true);

    // NOLINTNEXTLINE (modernize-make-unique): There is no std::make_unique in c++11
    client = std::unique_ptr<azure::storage_lite::blob_client>(
        new azure::storage_lite::blob_client(account, maxConcurrency));
}

/**
 * @brief Parses the @p blobInfo->storageSasCredential for the account and credential
 * @param[out] account value to assign the account name to
 * @param[out] credential value to assign the credential to
 * @param[in] storageSasUrl storage sas url to be parsed for the account and credential
 * @returns true on success; false on failure
 */
bool AzureBlobStorageHelper::ParseSasUrlForConnectionInfo(
    std::string& account, std::string& credential, const char* storageSasUrl)
{
    HTTPUrlHandleWrapper httpUrlSentinel(storageSasUrl);

    // Account name is the first value in host
    const char* hostValue = nullptr;
    size_t hostSize = 0;

    if (http_url_get_host(httpUrlSentinel.urlHandle, &hostValue, &hostSize) != 0)
    {
        return false;
    }

    // Note: String handle wrapper is used to construct a usable string without using pointer arithmetic
    ADUC::StringUtils::STRING_HANDLE_wrapper hostWrapper(STRING_construct_n(hostValue, hostSize));

    if (hostWrapper.get() == nullptr)
    {
        return false;
    }

    std::string host{ hostWrapper.c_str() };

    if (host.empty())
    {
        return false;
    }

    std::string hostName{ host.substr(0, host.find('.')) };

    const char* query = nullptr;
    size_t queryLen = 0;

    if (http_url_get_query(httpUrlSentinel.urlHandle, &query, &queryLen) != 0)
    {
        return false;
    }

    account = hostName;
    credential = std::string(query);

    return true;
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
        throw std::runtime_error("Invalid Arguments to CreatePathFromFileAndDirectory");
    }

    _Bool needsSeperator = false;
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
 * @param containerName the name of the container to upload these files to
 * @param fileNames vector of file names to upload
 * @param directoryPath path to the directory where @p fileNames can be found
 * @param virtualDirectory a properly formatted virtual directory (ending in '/') to be used when uploading the files
 * @returns true on success; false on any failure
 */
bool AzureBlobStorageHelper::UploadFilesToContainer(
    const std::string& containerName,
    VECTOR_HANDLE fileNames,
    const std::string& directoryPath,
    const std::string& virtualDirectory)
{
    if (containerName.empty() || VECTOR_size(fileNames) == 0)
    {
        throw std::runtime_error("Invalid Arguments to CreatePathFromFileAndDirectory");
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

        std::string filePath = nullptr;

        try
        {
            filePath = CreatePathFromFileAndDirectory(fileName, directoryPath);
        }
        catch (std::exception& e)
        {
            continue;
        }

        std::ifstream ifStream(filePath, std::ios_base::in | std::ios_base::binary);

        if (!ifStream.good())
        {
            continue;
        }

        std::string blobName = virtualDirectoryPath + fileName;

        if (client->get_blob_properties(containerName, blobName).get().response().valid())
        {
            if (!client->delete_blob(containerName, blobName).get().success())
            {
                continue;
            }
        }

        std::vector<std::pair<std::string, std::string>> metadata;

        azure::storage_lite::storage_outcome<void> ret =
            client->upload_block_blob_from_stream(containerName, blobName, ifStream, metadata).get();

        if (!ret.success())
        {
            Log_Info("File Upload failed for: %s with error %s", fileName, ret.error().code.c_str());
        }
    }

    return true;
}
