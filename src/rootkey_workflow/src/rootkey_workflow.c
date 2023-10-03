/**
 * @file diagnostics_devicename.c
 * @brief Implements function necessary for getting and setting the devicename for the diagnostics_component
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/rootkey_workflow.h"

#include <aduc/logging.h>
#include <aduc/rootkeypackage_do_download.h>
#include <aduc/rootkeypackage_download.h>
#include <aduc/rootkeypackage_parse.h>
#include <aduc/rootkeypackage_utils.h>
#include <aduc/system_utils.h>
#include <aduc/types/adu_core.h> // ADUC_Result_RootKey_Continue
#include <azure_c_shared_utility/crt_abstractions.h>
#include <azure_c_shared_utility/strings.h>
#include <parson.h>
#include <root_key_util.h>

#include <stdlib.h>

/**
 * @brief The info for each rootkey package downloader.
 *
 */
static ADUC_RootKeyPkgDownloaderInfo s_default_rootkey_downloader = {
    .name = "DO", // DeliveryOptimization
    .downloadFn = DownloadRootKeyPkg_DO,
    .downloadBaseDir = ADUC_DOWNLOADS_FOLDER,
};

/**
 * @brief Downloads the root key package and updates local store with it if different from current.
 * @param[in] workflowId The workflow Id for use in loca dir path of the rootkey package download.
 * @param[in] rootKeyPkgUrl The URL of the rootkey package from the deployment metadata.
 * @returns ADUC_Result the result.
 * @details If local storage is not being used then the contents of outLocalStoreChanged will always be true.
 */
ADUC_Result RootKeyWorkflow_UpdateRootKeys(const char* workflowId, const char* rootKeyPkgUrl)
{
    ADUC_Result result = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };
    ADUC_Result tmpResult = { .ResultCode = ADUC_GeneralResult_Failure, .ExtendedResultCode = 0 };

    STRING_HANDLE downloadedFilePath = NULL;
    STRING_HANDLE fileDest = NULL;
    JSON_Value* rootKeyPackageJsonValue = NULL;
    char* rootKeyPackageJsonString = NULL;
    ADUC_RootKeyPackage rootKeyPackage;

    memset(&rootKeyPackage, 0, sizeof(ADUC_RootKeyPackage));

    if (workflowId == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_INVALIDARG;
        goto done;
    }

    RootKeyUtility_ClearReportingErc();

    tmpResult = ADUC_RootKeyPackageUtils_DownloadPackage(
        rootKeyPkgUrl, workflowId, &s_default_rootkey_downloader, &downloadedFilePath);

    if (IsAducResultCodeFailure(tmpResult.ResultCode))
    {
        result = tmpResult;
        goto done;
    }

    rootKeyPackageJsonValue = json_parse_file(STRING_c_str(downloadedFilePath));

    if (rootKeyPackageJsonValue == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_PARSE;
        goto done;
    }

    rootKeyPackageJsonString = json_serialize_to_string(rootKeyPackageJsonValue);

    if (rootKeyPackageJsonString == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_ROOTKEY_PKG_FAIL_JSON_SERIALIZE;
        goto done;
    }

    tmpResult = ADUC_RootKeyPackageUtils_Parse(rootKeyPackageJsonString, &rootKeyPackage);

    if (IsAducResultCodeFailure(tmpResult.ResultCode))
    {
        result = tmpResult;
        goto done;
    }

    tmpResult = RootKeyUtility_ValidateRootKeyPackageWithHardcodedKeys(&rootKeyPackage);

    if (IsAducResultCodeFailure(tmpResult.ResultCode))
    {
        result = tmpResult;
        goto done;
    }

#ifdef ADUC_E2E_TESTING_ENABLED
    if (!rootKeyPackage.protectedProperties.isTest)
    {
        result.ExtendedResultCode = ADUC_ERC_ROOTKEY_PROD_PKG_ON_TEST_AGENT;
        goto done;
    }
#else
    if (rootKeyPackage.protectedProperties.isTest)
    {
        result.ExtendedResultCode = ADUC_ERC_ROOTKEY_TEST_PKG_ON_PROD_AGENT;
        goto done;
    }
#endif

    if (!ADUC_SystemUtils_Exists(ADUC_ROOTKEY_STORE_PATH))
    {
        if (ADUC_SystemUtils_MkDirRecursiveDefault(ADUC_ROOTKEY_STORE_PATH) != 0)
        {
            result.ExtendedResultCode = ADUC_ERC_ROOTKEY_STORE_PATH_CREATE;
            goto done;
        }
    }

    fileDest = STRING_construct(ADUC_ROOTKEY_STORE_PACKAGE_PATH);

    if (fileDest == NULL)
    {
        result.ExtendedResultCode = ADUC_ERC_NOMEM;
        goto done;
    }

    if (!ADUC_RootKeyUtility_IsUpdateStoreNeeded(fileDest, rootKeyPackageJsonString))
    {
        // This is a success, but skips writing to local store and includes informational ERC.
        result.ResultCode = ADUC_Result_RootKey_Continue;
        result.ExtendedResultCode = ADUC_ERC_ROOTKEY_PKG_UNCHANGED;
        goto done;
    }

    tmpResult = RootKeyUtility_WriteRootKeyPackageToFileAtomically(&rootKeyPackage, fileDest);

    if (IsAducResultCodeFailure(tmpResult.ResultCode))
    {
        result = tmpResult;
        goto done;
    }

    tmpResult = RootKeyUtility_ReloadPackageFromDisk(STRING_c_str(fileDest), true /* validateSignatures */);

    if (IsAducResultCodeFailure(tmpResult.ResultCode))
    {
        result = tmpResult;
        goto done;
    }

    result.ResultCode = ADUC_GeneralResult_Success;
done:

    if (IsAducResultCodeFailure(result.ResultCode) || result.ResultCode == ADUC_Result_RootKey_Continue)
    {
        if (IsAducResultCodeFailure(result.ResultCode))
        {
            Log_Error("Fail update root keys, ERC 0x%08x", result.ExtendedResultCode);
        }
        else
        {
            Log_Debug("No root key change.");
        }

        RootKeyUtility_SetReportingErc(result.ExtendedResultCode);
    }
    else
    {
        Log_Info("Update RootKey, ResultCode %d, ERC 0x%08x", result.ResultCode, result.ExtendedResultCode);
    }

    STRING_delete(downloadedFilePath);
    STRING_delete(fileDest);
    json_value_free(rootKeyPackageJsonValue);
    free(rootKeyPackageJsonString);

    ADUC_RootKeyPackageUtils_Destroy(&rootKeyPackage);
    return result;
}
