
/**
 * @file rootkeypackage_download.c
 * @brief Implements ADUC_RootKeyPackageUtil_DownloadPackage of rootkeypackage_utils interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/rootkeypackage_utils.h"
#include <aduc/logging.h>
#include <aduc/string_c_utils.h>
#include <aduc/system_utils.h>
#include <aduc/url_utils.h>

typedef ADUC_Result (*RootKeyPkgDownloadFunc)(const char* rootKeyPkgUrl, const char* targetFilePath);

/**
 * @brief Root key package downloader info.
 *
 */
typedef struct tagADUC_RootKeyPkgDownloaderInfo
{
    const char* name; /**< The name of the package downloader. */
    RootKeyPkgDownloadFunc downloadFn; /**< The downloader function. */
} ADUC_RootKeyPkgDownloaderInfo;

/* Forward declarations for the s_pkgDownloadersInfo. These are implemented in
   rootkeypackage_{name})download.c(pp) */
ADUC_Result DownloadRootKeyPkg_DO(const char* rootKeyPkgUrl, const char* targetFilePath);
ADUC_Result DownloadRootKeyPkg_curl(const char* rootKeyPkgUrl, const char* targetFilePath);

/**
 * @brief The info for each rootkey package downloader.
 *
 */
static ADUC_RootKeyPkgDownloaderInfo s_pkgDownloadersInfo[] = {
    {
        .name = "DO", // DeliveryOptimization
        .downloadFn = DownloadRootKeyPkg_DO,
    },
    {
        .name = "curl", // libcurl
        .downloadFn = DownloadRootKeyPkg_curl,
    },
};

EXTERN_C_BEGIN

/**
 * @brief Forces a download of the given URL to the rootkey pkg.
 *
 * @param rootKeyPkgUrl The URL of the root key package.
 * @param workflowId The workflow Id for the associated deployment.
 * @param outRootKeyPackageDownloadedFile The resultant path to the root key package file.
 *
 * @result ADUC_Result On success, the outPathToRootKey with contain the path to the downloaded file.
 */
ADUC_Result ADUC_RootKeyPackageUtil_DownloadPackage(
    const char* rootKeyPkgUrl, const char* workflowId, STRING_HANDLE* outRootKeyPackageDownloadedFile)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    ADUC_Result tmpResult = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    STRING_HANDLE targetDir = NULL;
    STRING_HANDLE targetFileName = NULL;
    STRING_HANDLE targetFilePath = NULL;
    STRING_HANDLE targetUrl = NULL;

    if (IsNullOrEmpty(rootKeyPkgUrl) || IsNullOrEmpty(workflowId) || outRootKeyPackageDownloadedFile == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_DOWNLOAD_BADARG;
        goto done;
    }

    if ((targetDir = STRING_construct_sprintf("%s/%s", ADUC_DOWNLOADS_FOLDER, workflowId)) == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_DOWNLOAD_URL_BAD_PATH;
        goto done;
    }

    if (ADUC_SystemUtils_MkDirRecursiveDefault(STRING_c_str(targetDir)) != 0)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_DOWNLOAD_MKDIR_DWNLD_FOLDER;
        goto done;
    }

    if (IsNullOrEmpty(ADUC_ROOTKEY_PKG_URL_OVERRIDE))
    {
        targetUrl = STRING_construct(rootKeyPkgUrl);
    }
    else
    {
        targetUrl = STRING_construct(ADUC_ROOTKEY_PKG_URL_OVERRIDE);
    }

    if (targetUrl == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    // The filename should be the last path segment of the URL path.
    tmpResult = ADUC_UrlUtils_GetLastPathSegmentOfUrl(STRING_c_str(targetUrl), &targetFileName);
    if (IsAducResultCodeFailure(tmpResult.ResultCode) || IsNullOrEmpty(STRING_c_str(targetFileName)))
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_DOWNLOAD_URL_BAD_PATH;
        goto done;
    }

    targetFilePath = STRING_construct_sprintf("%s/%s", STRING_c_str(targetDir), targetFileName);
    if (targetFilePath == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    // There is no hash to check so it is a forced download without lookup of
    // existing file. Also, the package has self-referential integrity with
    // signatures json array of signatures of the protected properties in the
    // same json file, so download the file without hash verification.
    for (size_t i = 0; i < ARRAY_SIZE(s_pkgDownloadersInfo); ++i)
    {
        Log_Debug("Attempting download of '%s' using '%s'", STRING_c_str(targetUrl), s_pkgDownloadersInfo[i].name);

        tmpResult = (s_pkgDownloadersInfo[i].downloadFn)(STRING_c_str(targetUrl), STRING_c_str(targetFilePath));

        if (IsAducResultCodeSuccess(result.ResultCode))
        {
            Log_Debug("Success using '%s'", s_pkgDownloadersInfo[i].name);
            break;
        }

        Log_Error(
            "Failed download using '%s', erc 0x%08x", s_pkgDownloadersInfo[i].name, tmpResult.ExtendedResultCode);
    }

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("Unable to download root key pkg.");

        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_DOWNLOAD_FAILED;
        goto done;
    }

    *outRootKeyPackageDownloadedFile = targetFilePath;
    targetFilePath = NULL;

    result.ResultCode = ADUC_GeneralResult_Success;
    result.ExtendedResultCode = 0;

done:
    STRING_delete(targetDir);
    STRING_delete(targetFileName);
    STRING_delete(targetFilePath);
    STRING_delete(targetUrl);

    return result;
}

EXTERN_C_END
