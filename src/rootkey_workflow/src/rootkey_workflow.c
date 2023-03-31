/**
 * @file diagnostics_devicename.c
 * @brief Implements function necessary for getting and setting the devicename for the diagnostics_component
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/rootkey_workflow.h"

#include <aduc/rootkeypackage_do_download.h>
#include <aduc/rootkeypackage_download.h>
#include <aduc/rootkeypackage_parse.h>
#include <aduc/rootkeypackage_utils.h>
#include <aduc/system_utils.h>
#include <azure_c_shared_utility/crt_abstractions.h>
#include <azure_c_shared_utility/strings.h>
#include <parson.h>
#include <root_key_util.h>

#include <stdlib.h>
#include <time.h>

static time_t lastUpdateTime = 0;

#define ROOTKEY_UPDATE_PERIOD 24 /* hours in a day*/ * 60 /* minutes in an hour*/ * 60 /* seconds in an minute*/

/**
 * @brief The info for each rootkey package downloader.
 *
 */
static ADUC_RootKeyPkgDownloaderInfo s_default_rootkey_downloader = {
    .name = "DO", // DeliveryOptimization
    .downloadFn = DownloadRootKeyPkg_DO,
    .downloadBaseDir = ADUC_DOWNLOADS_FOLDER,
};

bool RootKeyWorkflow_NeedToUpdateRootKeys(void)
{
    if (lastUpdateTime == 0)
    {
        return true;
    }
    else if (difftime(lastUpdateTime, time(0)) >= ROOTKEY_UPDATE_PERIOD)
    {
        return true;
    }

    return false;
}

ADUC_Result RootKeyWorkflow_UpdateRootKeys(const char* workflowId, const char* rootKeyPkgUrl)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    STRING_HANDLE downloadedFilePath = NULL;
    STRING_HANDLE fileDest = NULL;
    JSON_Value* rootKeyPackageJsonValue = NULL;
    char* rootKeyPackageJsonString = NULL;
    ADUC_RootKeyPackage rootKeyPackage;

    memset(&rootKeyPackage, 0, sizeof(ADUC_RootKeyPackage));

    if (workflowId == NULL)
    {
        goto done;
    }

    result = ADUC_RootKeyPackageUtils_DownloadPackage(
        rootKeyPkgUrl, workflowId, &s_default_rootkey_downloader, &downloadedFilePath);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    rootKeyPackageJsonValue = json_parse_file(STRING_c_str(downloadedFilePath));

    if (rootKeyPackageJsonValue == NULL)
    {
        // TODO: Error Code
        goto done;
    }

    rootKeyPackageJsonString = json_serialize_to_string(rootKeyPackageJsonValue);

    if (rootKeyPackageJsonString == NULL)
    {
        // TODO: Error Code
        goto done;
    }

    result = ADUC_RootKeyPackageUtils_Parse(rootKeyPackageJsonString, &rootKeyPackage);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(&rootKeyPackage);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    if ( ! ADUC_SystemUtils_Exists(ADUC_ROOTKEY_STORE_PATH) )
    {
        if ( ADUC_SystemUtils_MkDirRecursiveDefault(ADUC_ROOTKEY_STORE_PATH) != 0)
        {
            goto done;
        }
    }

    fileDest = STRING_construct(ADUC_ROOTKEY_STORE_PACKAGE_PATH);

    if (fileDest == NULL)
    {
        //TODO: Error Code
        goto done;
    }

    result = RootKeyUtility_WriteRootKeyPackageToFileAtomically(&rootKeyPackage, fileDest);

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    result = RootKeyUtility_ReloadPackageFromDisk();

    if (IsAducResultCodeFailure(result.ResultCode))
    {
        goto done;
    }

    lastUpdateTime = time(0);

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    STRING_delete(downloadedFilePath);
    STRING_delete(fileDest);
    json_value_free(rootKeyPackageJsonValue);
    free(rootKeyPackageJsonString);

    ADUC_RootKeyPackageUtils_Destroy(&rootKeyPackage);
    return result;
}
