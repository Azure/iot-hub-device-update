
/**
 * @file rootkeypackage_curl_download.h
 * @brief The root key package curl download header.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ROOTKEYPACKAGE_CURL_DOWNLOAD_H
#define ROOTKEYPACKAGE_CURL_DOWNLOAD_H

#include <aduc/c_utils.h>
#include <aduc/result.h>

EXTERN_C_BEGIN

ADUC_Result DownloadRootKeyPkg_Curl(const char* url, const char* targetFilePath);

EXTERN_C_END

#endif // ROOTKEYPACKAGE_CURL_DOWNLOAD_H
