/**
 * @file uhttp_downloader.cpp
 * @brief Simple HTTP downloader based on uHTTP
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 * Note that uHTTP is a rudimentary HTTP implementation and may not support production-level requirements.
 */
#include "uhttp_downloader.h"

#include <azure_c_shared_utility/azure_base64.h>
#include <azure_c_shared_utility/buffer_.h>
#include <azure_c_shared_utility/platform.h>
#include <azure_c_shared_utility/sha.h>
#include <azure_c_shared_utility/socketio.h>
#include <azure_c_shared_utility/tlsio.h>
#include <azure_uhttp_c/uhttp.h>

#include <fstream>
#include <string>

#include <aduc/logging.h>

class UHttpDownloader
{
public:
    UHttpDownloader() = default;
    ~UHttpDownloader() = default;

    UHttpDownloader(const UHttpDownloader&) = delete;
    UHttpDownloader& operator=(const UHttpDownloader&) = delete;

    UHttpDownloader(UHttpDownloader&&) = delete;
    UHttpDownloader& operator=(UHttpDownloader&&) = delete;

    UHttpDownloaderResult
    Download(const char* url, const char* base64Sha256Hash, const char* outputFile, unsigned int timeoutSecs);

    static UHttpDownloaderResult ResultFromHttpClientResult(HTTP_CLIENT_RESULT result);

    static UHttpDownloaderResult ResultFromHttpCallbackReason(HTTP_CALLBACK_REASON result);

private:
    bool HashMatches(const unsigned char* content, size_t content_len);

    void OnRequestCallback(
        HTTP_CALLBACK_REASON reason,
        const unsigned char* content,
        size_t content_len,
        unsigned int statusCode,
        HTTP_HEADERS_HANDLE /*responseHeadersHandle*/);

    static UHttpDownloaderResult
    ParseUrl(const char* url, unsigned* port, std::string* hostName, std::string* relativePath)
    {
        std::string temp{ url };
        size_t scheme_cch;

        if (temp.substr(0, 7) == "http://")
        {
            *port = 80;

            scheme_cch = 7;
        }
        else if (temp.substr(0, 8) == "https://")
        {
            *port = 443;

            scheme_cch = 8;
        }
        else
        {
            return DR_INVALID_ARG;
        }

        // NOTE: Assumes port isn't specified, e.g. "example.com:80"
        const std::size_t start = temp.find('/', scheme_cch);
        if (start == std::string::npos)
        {
            return DR_INVALID_ARG;
        }

        const std::size_t end = temp.find('/', start);
        if (end == std::string::npos)
        {
            return DR_INVALID_ARG;
        }

        *hostName = temp.substr(scheme_cch, start - scheme_cch);
        *relativePath = temp.substr(end);

        return DR_OK;
    }

    std::string m_base64Sha256Hash;
    std::string m_outputFile;

    bool m_keepRunning = false;
    UHttpDownloaderResult m_reason = DR_INVALID_STATE;
    unsigned int m_statusCode = 500;
};

bool UHttpDownloader::HashMatches(const unsigned char* content, size_t content_len)
{
    bool hashMatches = false;

    const SHAversion algorithm = SHAversion::SHA256;
    USHAContext context;
    if (USHAReset(&context, algorithm) == 0)
    {
        if (USHAInput(&context, content, content_len) == 0)
        {
            // "USHAHashSize(algorithm)" is more precise, but requires a variable length array, or heap allocation.
            unsigned char buffer_hash[USHAMaxHashSize];

            if (USHAResult(&context, buffer_hash) == 0)
            {
                STRING_HANDLE encoded_file_hash = Azure_Base64_Encode_Bytes(buffer_hash, USHAHashSize(algorithm));
                if (encoded_file_hash != nullptr)
                {
                    hashMatches = (strcmp(m_base64Sha256Hash.c_str(), STRING_c_str(encoded_file_hash)) == 0);
                    STRING_delete(encoded_file_hash);
                }
            }
        }
    }

    return hashMatches;
}

void UHttpDownloader::OnRequestCallback(
    HTTP_CALLBACK_REASON reason,
    const unsigned char* content,
    size_t content_len,
    unsigned int statusCode,
    HTTP_HEADERS_HANDLE /*responseHeadersHandle*/)
{
    // No more work to do after we get the callback.
    m_keepRunning = false;

    m_reason = ResultFromHttpCallbackReason(reason);
    if (reason != HTTP_CALLBACK_REASON_OK)
    {
        Log_Warn("onrequestcallback failed, error %u", reason);
        return;
    }

    if (statusCode != 200)
    {
        Log_Warn("onrequestcallback failed, statuscode %u", statusCode);
        m_reason = DR_CALLBACK_ERROR;
        m_statusCode = statusCode;
        return;
    }

    //
    // We've got data!
    //

    // check the hash.

    if (!HashMatches(content, content_len))
    {
        Log_Warn("Invalid content hash");
        m_reason = DR_CALLBACK_ERROR;
        return;
    }

    // Hash checks out, so Write it to a file.

    std::ofstream ofile(m_outputFile, std::ios::binary);
    if (ofile.fail())
    {
        Log_Warn("unable to open %s", m_outputFile.c_str());
        m_reason = DR_FILE_ERROR;
        return;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    ofile.write(reinterpret_cast<const char*>(content), sizeof(*content) * content_len);
    if (ofile.fail())
    {
        Log_Warn("unable to write %s", m_outputFile.c_str());
        m_reason = DR_FILE_ERROR;
        return;
    }
}

UHttpDownloaderResult UHttpDownloader::Download(
    const char* url, const char* base64Sha256Hash, const char* outputFile, unsigned int timeoutSecs)
{
    // RAII wrapper for HTTP_CLIENT_HANDLE
    class HttpClientHandle
    {
    public:
        explicit HttpClientHandle(HTTP_CLIENT_HANDLE handle) : _handle(handle)
        {
        }
        ~HttpClientHandle()
        {
            if (_handle != nullptr)
            {
                uhttp_client_close(
                    _handle,
                    [](void* /*context*/) {
                        // auto instance = static_cast<UHttpDownloader*>(context);
                    },
                    this);

                uhttp_client_destroy(_handle);
            }
        }

        HttpClientHandle(const HttpClientHandle&) = delete;
        HttpClientHandle& operator=(const HttpClientHandle&) = delete;

        HttpClientHandle(HttpClientHandle&&) = delete;
        HttpClientHandle& operator=(HttpClientHandle&&) = delete;

        HTTP_CLIENT_HANDLE Get() const
        {
            return _handle;
        }

    private:
        HTTP_CLIENT_HANDLE _handle;
    };

    unsigned int port;
    std::string hostname;
    std::string relativePath;

    m_reason = ParseUrl(url, &port, &hostname, &relativePath);
    if (m_reason != DR_OK)
    {
        Log_Warn("ParseUrl failed, error %u", m_reason);
        return m_reason;
    }

    m_base64Sha256Hash = base64Sha256Hash;
    m_outputFile = outputFile;

    m_keepRunning = true;
    m_statusCode = 200;

    //
    // Create HTTP or HTTPS uHttp client.
    //

    const IO_INTERFACE_DESCRIPTION* io_interface_desc;
    TLSIO_CONFIG tlsIoConfig = {}; // HTTPS
    SOCKETIO_CONFIG socketIoConfig = {}; // HTTP
    const void* xioParam; // points to either tlsIoConfig or socketIoConfig.

    if (port != 80)
    {
        // HTTPS
        tlsIoConfig.hostname = hostname.c_str();
        tlsIoConfig.port = port;
        xioParam = &tlsIoConfig;
        // Get the TLS definition
        io_interface_desc = platform_get_default_tlsio();
    }
    else
    {
        // HTTP
        socketIoConfig.hostname = hostname.c_str();
        socketIoConfig.port = port;
        xioParam = &socketIoConfig;
        // Get the socket definition
        io_interface_desc = socketio_get_interface_description();
    }

    HttpClientHandle handle{ uhttp_client_create(
        io_interface_desc,
        xioParam,
        [](void* context, HTTP_CALLBACK_REASON reason) -> void {
            auto instance = static_cast<UHttpDownloader*>(context);

            instance->m_reason = ResultFromHttpCallbackReason(reason);
            if (reason != HTTP_CALLBACK_REASON_OK)
            {
                instance->m_keepRunning = false;
                return;
            }
        },
        this) };
    if (handle.Get() == nullptr)
    {
        Log_Warn("client_create failed");
        return DR_ERROR;
    }

    //
    // Open uHttp connection.
    //

    HTTP_CLIENT_RESULT result;

    result = uhttp_client_open(
        handle.Get(),
        hostname.c_str(),
        port,
        [](void* context, HTTP_CALLBACK_REASON reason) -> void {
            auto instance = static_cast<UHttpDownloader*>(context);

            instance->m_reason = ResultFromHttpCallbackReason(reason);
            if (reason != HTTP_CALLBACK_REASON_OK)
            {
                Log_Warn("client_open callback failed, error %u", reason);
                instance->m_keepRunning = false;
                return;
            }
        },
        this);
    if (result != HTTP_CLIENT_OK)
    {
        Log_Warn("client_create failed, error %u", result);
        return ResultFromHttpClientResult(result);
    }

    //
    // Execute the GET request.
    //

    result = uhttp_client_execute_request(
        handle.Get(),
        HTTP_CLIENT_REQUEST_GET,
        relativePath.c_str(),
        nullptr /*HTTP_HEADERS_HANDLE*/,
        nullptr /*content*/,
        0 /*content_len*/,
        [](void* context,
           HTTP_CALLBACK_REASON reason,
           const unsigned char* content,
           size_t content_len,
           unsigned int statusCode,
           HTTP_HEADERS_HANDLE responseHeadersHandle) -> void {
            auto instance = static_cast<UHttpDownloader*>(context);

            instance->OnRequestCallback(reason, content, content_len, statusCode, responseHeadersHandle);
        },
        this);
    if (result != HTTP_CLIENT_OK)
    {
        Log_Warn("client_execute failed, error %u", result);
        return ResultFromHttpClientResult(result);
    }

    //
    // Start worker loop. Run until complete or timeout.
    //

    const time_t start_request_time = get_time(nullptr);
    bool timeout = false;

    do
    {
        uhttp_client_dowork(handle.Get());
        timeout = difftime(get_time(nullptr), start_request_time) > timeoutSecs;
    } while (m_keepRunning && !timeout);

    if (timeout)
    {
        Log_Warn("dowork timed out");
        return DR_TIMEOUT;
    }

    return m_reason;
}

/* static */
UHttpDownloaderResult UHttpDownloader::ResultFromHttpClientResult(HTTP_CLIENT_RESULT result)
{
    using ResultMap = struct tagResultMap
    {
        HTTP_CLIENT_RESULT From;
        UHttpDownloaderResult To;
    };

    const ResultMap resultMap[] = {
        { HTTP_CLIENT_OK, DR_OK },
        { HTTP_CLIENT_INVALID_ARG, DR_INVALID_ARG },
        { HTTP_CLIENT_ERROR, DR_ERROR },
        { HTTP_CLIENT_OPEN_FAILED, DR_OPEN_FAILED },
        { HTTP_CLIENT_SEND_FAILED, DR_SEND_FAILED },
        { HTTP_CLIENT_ALREADY_INIT, DR_ALREADY_INIT },
        { HTTP_CLIENT_HTTP_HEADERS_FAILED, DR_HTTP_HEADERS_FAILED },
        { HTTP_CLIENT_INVALID_STATE, DR_INVALID_STATE },
    };

    for (const ResultMap& entry : resultMap)
    {
        if (result == entry.From)
        {
            return entry.To;
        }
    }

    return DR_ERROR;
}

/* static */
UHttpDownloaderResult UHttpDownloader::ResultFromHttpCallbackReason(HTTP_CALLBACK_REASON result)
{
    using ResultMap = struct tagResultMap
    {
        HTTP_CALLBACK_REASON From;
        UHttpDownloaderResult To;
    };

    const ResultMap resultMap[] = {
        // #define HTTP_CALLBACK_REASON_VALUES
        { HTTP_CALLBACK_REASON_OK, DR_OK },
        { HTTP_CALLBACK_REASON_OPEN_FAILED, DR_CALLBACK_OPEN_FAILED },
        { HTTP_CALLBACK_REASON_SEND_FAILED, DR_CALLBACK_SEND_FAILED },
        { HTTP_CALLBACK_REASON_ERROR, DR_CALLBACK_ERROR },
        { HTTP_CALLBACK_REASON_PARSING_ERROR, DR_CALLBACK_PARSING_ERROR },
        { HTTP_CALLBACK_REASON_DESTROY, DR_CALLBACK_DESTROY },
        { HTTP_CALLBACK_REASON_DISCONNECTED, DR_CALLBACK_DISCONNECTED },
    };

    for (const ResultMap& entry : resultMap)
    {
        if (result == entry.From)
        {
            return entry.To;
        }
    }

    return DR_CALLBACK_ERROR;
}

UHttpDownloaderResult
DownloadFile(const char* url, const char* base64Sha256Hash, const char* outputFile, unsigned int timeoutSecs)
{
    UHttpDownloader downloader;

    return downloader.Download(url, base64Sha256Hash, outputFile, timeoutSecs);
}
