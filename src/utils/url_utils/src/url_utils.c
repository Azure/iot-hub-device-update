/**
 * @file url_utils.c
 * @brief Implements the url utils.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/url_utils.h"
#include "aduc/http_url.h"
#include "aduc/string_c_utils.h"
#include <string.h>

/**
 * @brief Gets the filename at the end of URL path, or NULL if no such filename.
 * @param[in] url The URL.
 * @param[out] outFileName The out parameter to hold the filename string, or NULL.
 *
 * @returns ADUC_Result The result. On success, contents of outFileName will be
 * the allocated STRING_HANDLE for the filename, or NULL if no trailing path segment.
 *
 * @details If no filename at the end, then result will be a failure and
 * ExtendedResultCode will be ADUC_ERC_UTILITIES_URL_BAD_PATH for e.g.
 * http://example.com/ or http://example.com
 *
 * If there is a query string, it will not include it. e.g.
 * http://example.com/foo/bar?a=b will result in outFileName of bar.
 */
ADUC_Result ADUC_UrlUtils_GetPathFileName(const char* url, STRING_HANDLE* outFileName)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    const char *p = NULL, *q = NULL;
    char* filename = NULL;
    size_t filename_len = 0;

    if (IsNullOrEmpty(url) || outFileName == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_URL_BAD_ARG;
        return result;
    }

    p = strstr(url, "://");
    if (p == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_URL_BAD_URL;
        goto done;
    }

    p += 2; // Advance to the last fwd slash

    q = strrchr(url, '/');
    if (p == q)
    {
        // No slash after scheme separator so there's no file path
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_URL_BAD_PATH;
        goto done;
    }

    p = q + 1; // p is one past the right-most path separator

    while (*q != '\0' && *q != '?' && *q != '#')
    {
        q++;
    }

    filename_len = (size_t)(q - p);

    if (filename_len == 0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_URL_BAD_PATH;
        goto done;
    }

    filename = malloc(filename_len + 1);
    if (filename == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    memcpy(filename, p, filename_len);
    filename[filename_len] = '\0';

    // Transfer ownership of memory to the STRING_HANDLE
    *outFileName = STRING_new_with_memory(filename);
    filename = NULL;

    result.ResultCode = ADUC_GeneralResult_Success;

done:

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        *outFileName = NULL;
    }

    return result;
}
