/**
 * @file url_utils.h
 * @brief Utilities for working with urls.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_URL_UTILS_H
#define ADUC_URL_UTILS_H

#include "aduc/c_utils.h"
#include "aduc/result.h"
#include "azure_c_shared_utility/strings.h"

EXTERN_C_BEGIN

ADUC_Result GetLastPathSegmentOfUrl(const char* url, STRING_HANDLE* outLastPathSegment);

EXTERN_C_END

#endif // ADUC_URL_UTILS_H
