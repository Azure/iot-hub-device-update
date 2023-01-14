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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/**
 * @brief Implementation of the non-standard strdup
 * @param proxyOptions str string to duplicate
 * @return char* duplicated string, deallocate using free. NULL on failure.
 */
static char* strdupe(const char* str)
{
    char* dupe = malloc(strlen(str) + 1);
    if (dupe != NULL)
    {
        strcpy(dupe, str);
    }
    return dupe;
}

static int hex_to_int(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    else if (ch >= 'a' && ch <= 'f')
    {
        return ch - 'a' + 10;
    }
    else if (ch >= 'A' && ch <= 'F')
    {
        return ch - 'A' + 10;
    }

    return -1;
}

static bool unescape_data_string(char* input)
{
    char* src = input;
    char* dest = input;

    while (*src != '\0')
    {
        if (*src != '%')
        {
            *dest = *src;
            ++src;
            ++dest;
            continue;
        }

        ++src;
        const int ch1 = hex_to_int(*src);
        if (ch1 == -1)
        {
            return false;
        }

        ++src;
        const int ch2 = hex_to_int(*src);
        if (ch2 == -1)
        {
            return false;
        }

        *dest = (char)((ch1 << 4) + ch2);
        ++dest;
        *dest = '\0';

        ++src;
    }
    *dest = '\0';

    return true;
}

/**
 * @brief Initializes @p proxyOptions object by reading and parsing environment variable 'https_proxy' or 'HTTPS_PROXY', in that order.
 * @param proxyOptions HTTP_PROXY_OPTIONS A pointer to HTTP_PROXY_OPTIONS struct to be initialized.
 *                     Caller must call UninitializeProxyOptions() to free memory allocated for the member variables.
 * @return bool True when the proxy options successfully initialized.
 */
bool InitializeProxyOptions(HTTP_PROXY_OPTIONS* proxyOptions)
{
    bool success = false;
    char* proxy_copy = NULL;
    char* start = NULL;
    char* end = NULL;

    if (proxyOptions == NULL)
    {
        Log_Error("proxyOptions is NULL.");
        goto done;
    }

    proxyOptions->host_address = NULL;
    proxyOptions->username = NULL;
    proxyOptions->password = NULL;
    proxyOptions->port = 0;

    char* httpsProxyEnvVar = getenv("https_proxy");
    if (httpsProxyEnvVar == NULL)
    {
        httpsProxyEnvVar = getenv("HTTPS_PROXY");
    }
    if (httpsProxyEnvVar == NULL)
    {
        goto done;
    }

    // Create read-write copy of environment variable.
    proxy_copy = strdupe(httpsProxyEnvVar);
    if (proxy_copy == NULL)
    {
        goto done;
    }

    start = proxy_copy;

    end = strstr(start, "://");
    if (end == NULL)
    {
        // Can't find "://"
        goto done;
    }

    end += 3; // past ""://""

    start = end;

    end = strchr(start, '@');
    if (end != NULL)
    {
        // Left of '@' is username[:password]
        *end = '\0';
        char* after_at = end + 1; // after '@'

        end = strchr(start, ':');
        if (end != NULL)
        {
            *end = '\0';

            // Check for no username
            if (*start != '\0')
            {
                if (!unescape_data_string(start))
                {
                    goto done;
                }

                proxyOptions->username = strdupe(start);
                if (proxyOptions->username == NULL)
                {
                    goto done;
                }
            }

            start = end + 1;

            // Check for no password
            if (*start != '\0')
            {
                if (!unescape_data_string(start))
                {
                    goto done;
                }

                proxyOptions->password = strdupe(start);
                if (proxyOptions->password == NULL)
                {
                    goto done;
                }
            }
        }

        start = after_at;
    }

    end = strchr(start, ':');
    if (end != NULL)
    {
        *end = '\0';

        proxyOptions->port = atoi(end + 1);
    }

    if (!unescape_data_string(start))
    {
        goto done;
    }

    proxyOptions->host_address = strdupe(start);
    if (proxyOptions->host_address == NULL)
    {
        goto done;
    }

    success = true;

done:

    free(proxy_copy);

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
