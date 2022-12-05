#include <aduc/logging.h>
#include <aduc/result.h>
#include <curl/curl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

// Default retry timeout for root key 24 hours.
#define CURL_ROOTKEY_PKG_RETRY_TIMEOUT (60 * 60 * 24)

EXTERN_C_BEGIN

/*
typedef struct tagADUC_curl_cb_info {
    FILE* fh;
    char *data;
    size_t curSize;
    size_t totalSize; // including null-terminator
} ADUC_curl_cb_info;

static size_t ADUC_curl_cb(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t chunkSize = size * nmemb;
    ADUC_curl_cb_info *info = (ADUC_curl_cb_info*)userp;

    memcpy(&(info->data[data->curSize]), contents, chunkSize);
    info->curSize += chunkSize;
    info->data[info->curSize] = 0;

    if (info->curSize == totalSize)
    {
        fwrite()
    }

    return chunkSize;
}
*/

struct MemoryStruct
{
    char* memory;
    size_t size;
};

static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct* mem = (struct MemoryStruct*)userp;

    char* ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr)
    {
        /* out of memory! */
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

ADUC_Result DownloadRootKeyPkg_curl(const char* url, const char* targetFilePath)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    FILE* file = NULL;
    CURL* curl_handle = NULL;
    CURLcode res;
    size_t bytes_written = 0;

    //    ADUC_rootkey_curl_cb_info cb_info;
    //    memset(&cb_info, 0, sizeof(cb_info));
    struct MemoryStruct chunk = {
        .memory = malloc(1), /* will be grown as needed by the realloc above */
        .size = 0, /* no data at this point */
    };

    Log_Info("Downloading File '%s' to '%s'", url, targetFilePath);

    curl_handle = curl_easy_init();
    if (curl_handle == NULL)
    {
        Log_Error("curl init failed.");
        result.ExtendedResultCode = ADUC_ERC_ROOTKEYPKG_DOWNLOADER_CURL_INIT;
        goto done;
    }

    errno = 0;
    file = fopen(targetFilePath, "wb");
    if (file == NULL)
    {
        Log_Error("fopen for write of '%s' failed. errno: %d", targetFilePath, errno);
        result.ExtendedResultCode = MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ERRNO(errno);
        goto done;
    }

    res = curl_easy_setopt(curl_handle, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
    if (res != CURLE_OK)
    {
        Log_Error("Failed to set curl PROTOCOLS, res: %d", res);
        result.ExtendedResultCode = ADUC_ERC_ROOTKEYPKG_DOWNLOADER_CURL_SETOPT_PROTOCOLS;
        goto done;
    }

    res = curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    if (res != CURLE_OK)
    {
        Log_Error("Failed to set curl URL, res: %d", res);
        result.ExtendedResultCode = ADUC_ERC_ROOTKEYPKG_DOWNLOADER_CURL_SETOPT_URL;
        goto done;
    }

    res = curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, CURL_ROOTKEY_PKG_RETRY_TIMEOUT);
    if (res != CURLE_OK)
    {
        Log_Error("Failed to set curl TIMEOUT, res: %d", res);
        result.ExtendedResultCode = ADUC_ERC_ROOTKEYPKG_DOWNLOADER_CURL_SETOPT_TIMEOUT;
        goto done;
    }

    res = curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    if (res != CURLE_OK)
    {
        Log_Error("Failed to set curl WRITEFUNCTION, res: %d", res);
        result.ExtendedResultCode = ADUC_ERC_ROOTKEYPKG_DOWNLOADER_CURL_SETOPT_WRITEFUNCTION;
        goto done;
    }

    res = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&chunk);
    if (res != CURLE_OK)
    {
        Log_Error("Failed to set curl WRITEDATA, res: %d", res);
        result.ExtendedResultCode = ADUC_ERC_ROOTKEYPKG_DOWNLOADER_CURL_SETOPT_WRITEDATA;
        goto done;
    }

    res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK)
    {
        Log_Error("Failed to perform download, res: %d", res);
        result.ExtendedResultCode = ADUC_ERC_ROOTKEYPKG_DOWNLOADER_CURL_PERFORM_DOWNLOAD;
        goto done;
    }

    bytes_written = fwrite(chunk.memory, sizeof(char), chunk.size, file);
    if (bytes_written != chunk.size)
    {
        result.ExtendedResultCode = ADUC_ERC_ROOTKEYPKG_DOWNLOADER_CURL_FILEWRITE;
        goto done;
    }

    Log_Info("Success writing %d bytes of '%s' data to file '%s'", chunk.size, url, targetFilePath);

    Log_Debug("Data: ->%s<-", chunk.memory);

    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;

done:
    if (file != NULL)
    {
        fclose(file);
    }

    if (curl_handle != NULL)
    {
        curl_easy_cleanup(curl_handle);
    }

    Log_Info("Download rc: %d, erc: 0x%08x", result.ResultCode, result.ExtendedResultCode);
    return result;
}

EXTERN_C_END
