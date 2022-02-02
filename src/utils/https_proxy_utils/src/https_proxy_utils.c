/**
 * @file https_proxy_utils.c
 * @brief Functions for initialize and uninitialize HTTP_PROXY_OPTIONS struct.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/https_proxy_utils.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h"
#include <azure_c_shared_utility/crt_abstractions.h> // mallocAndStrcpy_s
#include <curl/curl.h> // curl_* functions
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/**
 * @brief Initializes @p proxyOptions object by reading and parsing environment variable 'https_proxy' or 'HTTPS_PROXY', in that order.
 * @param proxyOptions HTTP_PROXY_OPTIONS A pointer to HTTP_PROXY_OPTIONS struct to be initialized.
 *                     Caller must call UninitializeProxyOptions() to free memory allocated for the member variables.
 * @return _Bool True when the proxy options successfully initialized.
 */
_Bool InitializeProxyOptions(HTTP_PROXY_OPTIONS* proxyOptions)
{
    bool success = false;
    CURLU *curlHandle = curl_url();
    CURLcode uc;
    char* host = NULL;
    char* port = NULL;
    char* user = NULL;
    char* password = NULL;
    char* unescapedUrl = NULL;

    if (proxyOptions == NULL)
    {
        Log_Error("proxyOptions is NULL.");
        goto done;
    }

    proxyOptions->host_address = NULL;
    proxyOptions->username = NULL;
    proxyOptions->password = NULL;
    proxyOptions->port = 0;

    char* httpsProxyEnvvar = getenv("https_proxy");
    if (httpsProxyEnvvar == NULL)
    {
        httpsProxyEnvvar = getenv("HTTPS_PROXY");
    }
    if (httpsProxyEnvvar == NULL)
    {
        goto done;
    }

    int outLen = 0;
    unescapedUrl = curl_easy_unescape(NULL, httpsProxyEnvvar, strlen(httpsProxyEnvvar), &outLen);

    curlHandle = curl_url();
    if (curlHandle == NULL)
    {
        Log_Error("Cannot create curl handle.");
        goto done;
    }

    uc = curl_url_set(curlHandle, CURLUPART_URL, unescapedUrl, 0);
    if (uc != CURLE_OK)
    {
        Log_Error("Failed to parse url. uc:%d", uc);
        goto done;
    }

    // Get host address.
    uc = curl_url_get(curlHandle, CURLUPART_HOST, &host, 0);
    if (uc != CURLE_OK)
    {
        Log_Error("Bad hostname. uc:%d", uc);
        goto done;
    }

    if (mallocAndStrcpy_s((char**)&(proxyOptions->host_address), host) != 0)
    {
        Log_Error("Failed to copy host address.");
        goto done;
    }

    uc = curl_url_get(curlHandle, CURLUPART_PORT, &port, 0);
    if (uc != CURLE_OK)
    {
        Log_Info("No ports. uc:%d", uc);
    }
    else
    {
        char* endptr;
        errno = 0; /* To distinguish success/failure after call */
        proxyOptions->port = strtol(port, &endptr, 10);
        if (errno != 0 || endptr == port)
        {
            Log_Error("Invalid port number.");
            goto done;
        }
    }

    uc = curl_url_get(curlHandle, CURLUPART_USER, &user, 0);
    if (uc != CURLE_OK)
    {
        Log_Info("No username. uc:%d", uc);
    }
    else
    {
        if (mallocAndStrcpy_s((char**)&(proxyOptions->username), user) != 0)
        {
            goto done;
        }
    }

    uc = curl_url_get(curlHandle, CURLUPART_PASSWORD, &password, 0);
    if (uc != CURLE_OK)
    {
        Log_Info("No password. uc:%d", uc);
    }
    else
    {
        if (mallocAndStrcpy_s((char**)&(proxyOptions->password), password) != 0)
        {
            Log_Error("Failed to copy password");
            goto done;
        }
    }

    success = true;

done:

    if (password != NULL)
    {
        curl_free(password);
    }

    if (user != NULL)
    {
        curl_free(user);
    }

    if (port != NULL)
    {
        curl_free(port);
    }

    if (host != NULL)
    {
        curl_free(host);
    }

    if (curlHandle != NULL)
    {
        curl_url_cleanup(curlHandle);
    }

    if (!success)
    {
        UninitializeProxyOptions(proxyOptions);
    }

    return success;
}

/**
 * @brief Frees all memory allocated for @p proxyOptions member variables.
 *
 * @param proxyOptions IOTHUB_PROXY_OPTIONS An object to be uninitialized.
 */
void UninitializeProxyOptions(HTTP_PROXY_OPTIONS* proxyOptions)
{
    if (proxyOptions == NULL)
    {
        return;
    }

    free((char*)proxyOptions->username);
    proxyOptions->username = NULL;
    free((char*)proxyOptions->password);
    proxyOptions->password = NULL;
    free((char*)proxyOptions->host_address);
    proxyOptions->host_address = NULL;
}
