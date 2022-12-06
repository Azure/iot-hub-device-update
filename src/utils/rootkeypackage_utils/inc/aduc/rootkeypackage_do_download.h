
/**
 * @file rootkeypackage_do_download.h
 * @brief The root key package delivery optimization download header.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ROOTKEYPACKAGE_DO_DOWNLOAD_H
#define ROOTKEYPACKAGE_DO_DOWNLOAD_H

#include <aduc/result.h>

ADUC_Result DownloadRootKeyPkg_DO(const char* url, const char* targetFilePath);

#endif // ROOTKEYPACKAGE_DO_DOWNLOAD_H
