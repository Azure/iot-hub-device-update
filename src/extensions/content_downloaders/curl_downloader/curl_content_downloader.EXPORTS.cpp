/**
 * @file curl_content_downloader.EXPORTS.cpp
 * @brief The exports for Content Downloader Extension.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "curl_content_downloader.h" // for Download_curl
#include <aduc/c_utils.h> // for EXTERN_C_BEGIN, EXTERN_C_END
#include <aduc/contract_utils.h> // for ADUC_ExtensionContractInfo
#include <aduc/types/download.h> // for ADUC_DownloadProgressCallback
#include <aduc/types/update_content.h> // for ADUC_FileEntity

EXTERN_C_BEGIN

/////////////////////////////////////////////////////////////////////////////
// BEGIN Shared Library Export Functions
//
// These are the function symbols that the device update agent will
// lookup and call.
//

ADUC_Result Download(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int retryTimeout,
    ADUC_DownloadProgressCallback downloadProgressCallback)
{
    return Download_curl(entity, workflowId, workFolder, retryTimeout, downloadProgressCallback);
}

ADUC_Result Initialize(const char* initializeData)
{
    UNREFERENCED_PARAMETER(initializeData);
    return { ADUC_GeneralResult_Success };
}

/**
 * @brief Gets the extension contract info.
 *
 * @param[out] contractInfo The extension contract info.
 * @return ADUC_Result The result.
 */
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo)
{
    contractInfo->majorVer = ADUC_V1_CONTRACT_MAJOR_VER;
    contractInfo->minorVer = ADUC_V1_CONTRACT_MINOR_VER;
    return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
}

EXTERN_C_END
