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

ADUC_Result GetLastPathSegmentOfUrl(const char* url, STRING_HANDLE* outLastPathSegment)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    STRING_HANDLE tmp = NULL;
    int ret = -1;
    const char* url_path = NULL;
    size_t url_path_len = 0;
    int last_segment_index = -1;
    int i = -1;

    HTTP_URL_HANDLE url_handle = http_url_create(url);
    if (url_handle == NULL)
    {
        // http_url_create will fail if no path exists like in "http://a.b/"
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_URL_CREATE;
        goto done;
    }

    ret = http_url_get_path(url_handle, &url_path, &url_path_len);
    if (ret != 0 || url_path_len <= 0 || IsNullOrEmpty(url_path))
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_URL_GET_PATH;
        goto done;
    }

    for (i = url_path_len - 1; i >= -1; --i)
    {
        // will hit -1 when http_url_get_path() returns "x.y" for URLs with no path segments like "http://a.b/x.y"
        if (i == -1 || url_path[i] == '/')
        {
            last_segment_index = i + 1;
            break;
        }
    }

    tmp = STRING_construct_n(&url_path[last_segment_index], url_path_len - last_segment_index);
    if (tmp == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    *outLastPathSegment = tmp;
    tmp = NULL;
    result.ResultCode = ADUC_GeneralResult_Success;

done:

    http_url_destroy(url_handle);

    if (tmp != NULL)
    {
        STRING_delete(tmp);
    }

    return result;
}
