/**
 * @file url_utils.c
 * @brief Implements the url utils.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/url_utils.h"
#include <aduc/logging.h>
#include <aduc/result.h>
#include <aduc/string_c_utils.h>
#include <curl/curl.h>
#include <string.h>

ADUC_Result ADUC_UrlUtils_GetLastPathSegmentOfUrl(const char* url, STRING_HANDLE* outLastPathSegment)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    STRING_HANDLE tmp = NULL;
    char* url_path = NULL;
    char* p = NULL;

    CURLcode uc;
    CURLU* curl_handle = NULL;

    if (url == NULL || outLastPathSegment == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_INVAL;
        return result;
    }

    curl_handle = curl_url();
    if (curl_handle == NULL)
    {
        Log_Error("Cannot create curl handle.");
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_URL_CREATE;
        goto done;
    }

    uc = curl_url_set(curl_handle, CURLUPART_URL, url, 0);
    if (uc != CURLE_OK)
    {
        Log_Error("Failed to parse url '%s', uc:%d", url, uc);
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_URL_SET;
        goto done;
    }

    uc = curl_url_get(curl_handle, CURLUPART_PATH, &url_path, 0);
    if (uc != CURLE_OK)
    {
        Log_Error("Fail to get path. uc:%d", uc);
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_URL_GET_PATH;
        goto done;
    }

    p = strrchr(url_path, '/');
    if (p == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_URL_BAD_PATH;
        goto done;
    }
    // otherwise, move to next char in string, which might even be '\0'.
    p++;

    tmp = STRING_construct(p);
    if (tmp == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    *outLastPathSegment = tmp;
    tmp = NULL;
    result.ResultCode = ADUC_GeneralResult_Success;

done:

    if (url_path != NULL)
    {
        curl_free(url_path);
    }

    if (curl_handle != NULL)
    {
        curl_url_cleanup(curl_handle);
    }

    if (tmp != NULL)
    {
        STRING_delete(tmp);
    }

    return result;
}
