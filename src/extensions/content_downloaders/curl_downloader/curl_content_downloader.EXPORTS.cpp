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
#include <aduc/logging.h> // ADUC_Logging_*, Log_*

EXTERN_C_BEGIN

/////////////////////////////////////////////////////////////////////////////
// BEGIN Shared Library Export Functions
//
// These are the function symbols that the device update agent will
// lookup and call.
//

EXPORTED_METHOD ADUC_Result Download(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int timeoutInSeconds,
    ADUC_DownloadProgressCallback downloadProgressCallback)
{
    return Download_curl(entity, workflowId, workFolder, timeoutInSeconds, downloadProgressCallback);
}

/**
 * @brief One-time initialization for the content downloader.
 *
 * @param initializeData The initialization data.
 * @param logLevel The desired loglevel if logging is used.
 */
EXPORTED_METHOD ADUC_Result Initialize(const char* initializeData, ADUC_LOG_SEVERITY logLevel)
{
    UNREFERENCED_PARAMETER(initializeData);
    ADUC_Logging_Init(logLevel, "curl-content-downloader");
    return { ADUC_GeneralResult_Success };
}

/**
 * @brief Cleanup logic before library is unloaded.
 */
EXPORTED_METHOD void Cleanup()
{
    ADUC_Logging_Uninit();
}

/**
 * @brief Gets the extension contract info.
 *
 * @param[out] contractInfo The extension contract info.
 * @return ADUC_Result The result.
 */
EXPORTED_METHOD ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo)
{
    contractInfo->majorVer = ADUC_V2_CONTRACT_MAJOR_VER;
    contractInfo->minorVer = ADUC_V2_CONTRACT_MINOR_VER;
    return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
}

EXTERN_C_END
