/**
 * @file http_url.h
 * @brief Header for HTTP URL utility
 *
 * @copyright Copyright (c) Microsoft Corp.
 * Licensed under the MIT License.
 */

#ifndef HTTP_URL_H
#define HTTP_URL_H

#ifdef __cplusplus
#    include <cstddef>
#else
#    include <stdbool.h>
#    include <stddef.h>
#endif

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct HTTP_URL_TAG* HTTP_URL_HANDLE;

    MOCKABLE_FUNCTION(, HTTP_URL_HANDLE, http_url_create, const char*, url);
    MOCKABLE_FUNCTION(, int, http_url_is_secure, HTTP_URL_HANDLE, url, bool*, is_secure);
    MOCKABLE_FUNCTION(, int, http_url_get_host, HTTP_URL_HANDLE, url, const char**, host, size_t*, length);
    MOCKABLE_FUNCTION(, int, http_url_get_port, HTTP_URL_HANDLE, url, size_t*, port);
    MOCKABLE_FUNCTION(, int, http_url_get_path, HTTP_URL_HANDLE, url, const char**, path, size_t*, length);
    MOCKABLE_FUNCTION(, int, http_url_get_query, HTTP_URL_HANDLE, url, const char**, query, size_t*, length);
    MOCKABLE_FUNCTION(, void, http_url_destroy, HTTP_URL_HANDLE, url);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // HTTP_URL_H
