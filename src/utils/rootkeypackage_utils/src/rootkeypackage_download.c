/**
 * @file rootkeypackage_download.c
 * @brief Implements ADUC_RootKeyPackageUtils_DownloadPackage of rootkeypackage_utils interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/rootkeypackage_download.h"
#include "aduc/rootkeypackage_do_download.h"
#include "aduc/rootkeypackage_utils.h"
#include <aduc/logging.h>
#include <aduc/string_c_utils.h>
#include <aduc/system_utils.h>
#include <aduc/url_utils.h>
#include <aducpal/unistd.h> // unlink

EXTERN_C_BEGIN

/**
 * @brief Forces a download of the given URL to the rootkey pkg.
 *
 * @param rootKeyPkgUrl The URL of the root key package.
 * @param workflowId The workflow Id for the associated deployment.
 * @param downloaderInfo The downloader info used to download the rootkey package.
 * @param[out] outRootKeyPackageDownloadedFile The resultant path to the root key package file.
 *
 * @result ADUC_Result On success, the outPathToRootKey with contain the path to the downloaded file.
 */
ADUC_Result ADUC_RootKeyPackageUtils_DownloadPackage(
    const char* rootKeyPkgUrl,
    const char* workflowId,
    ADUC_RootKeyPkgDownloaderInfo* downloaderInfo,
    STRING_HANDLE* outRootKeyPackageDownloadedFile)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    int err = 0;

    STRING_HANDLE targetDir = NULL;
    STRING_HANDLE targetFileName = NULL;
    STRING_HANDLE targetFilePath = NULL;
    STRING_HANDLE targetUrl = NULL;

    if (IsNullOrEmpty(rootKeyPkgUrl) || IsNullOrEmpty(workflowId) || downloaderInfo == NULL
        || IsNullOrEmpty(downloaderInfo->name) || IsNullOrEmpty(downloaderInfo->downloadBaseDir)
        || downloaderInfo->downloadFn == NULL || outRootKeyPackageDownloadedFile == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_DOWNLOAD_BADARG;
        goto done;
    }

    if ((targetDir = STRING_construct_sprintf("%s/%s", downloaderInfo->downloadBaseDir, workflowId)) == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
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

    result = ADUC_UrlUtils_GetPathFileName(STRING_c_str(targetUrl), &targetFileName);
    if (IsAducResultCodeFailure(result.ResultCode) || IsNullOrEmpty(STRING_c_str(targetFileName)))
    {
        result.ResultCode = ADUC_GeneralResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_UTILITIES_ROOTKEYPKG_DOWNLOAD_URL_BAD_PATH;
        goto done;
    }

    targetFilePath = STRING_construct_sprintf("%s/%s", STRING_c_str(targetDir), STRING_c_str(targetFileName));
    if (targetFilePath == NULL)
    {
        result.ResultCode = ADUC_GeneralResult_Failure;
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    if (SystemUtils_IsFile(STRING_c_str(targetFilePath), &err))
    {
        Log_Warn("rootkey package '%s' in sandbox. Try deletion...", STRING_c_str(targetFileName));
        if (unlink(STRING_c_str(targetFilePath)) != 0)
        {
            Log_Warn("Fail unlink '%s': %d", STRING_c_str(targetFilePath), errno);
            // continue below and try to download anyways.
        }
    }

    // There is no hash to check so it is a forced download without lookup of
    // existing file. Also, the package has self-referential integrity with
    // signatures json array of signatures of the protected properties in the
    // same json file, so download the file without hash verification.
    Log_Debug("Attempting download of '%s' using '%s'", STRING_c_str(targetUrl), downloaderInfo->name);

    result = (*(downloaderInfo->downloadFn))(STRING_c_str(targetUrl), STRING_c_str(targetFilePath));

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        Log_Error("Unable to download root key pkg.");
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
