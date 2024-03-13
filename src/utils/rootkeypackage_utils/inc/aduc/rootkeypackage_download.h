
/**
 * @file rootkeypackage_download.h
 * @brief The root key package download header.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ROOTKEYPACKAGE_DOWNLOAD_H
#define ROOTKEYPACKAGE_DOWNLOAD_H

#include <aduc/result.h>

typedef ADUC_Result (*RootKeyPkgDownloadFunc)(const char* rootKeyPkgUrl, const char* targetFilePath);

/**
 * @brief Root key package downloader info.
 *
 */
typedef struct tagADUC_RootKeyPkgDownloaderInfo
{
    const char* name; /**< The name of the package downloader. */
    RootKeyPkgDownloadFunc downloadFn; /**< The downloader function. */
    const char*
        downloadBaseDir; /**< The base directory under which to create a download sandbox dir for the file download. */
} ADUC_RootKeyPkgDownloaderInfo;

#endif // ROOTKEYPACKAGE_DOWNLOAD_H
