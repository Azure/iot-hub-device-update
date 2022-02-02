/**
 * @file deliveryoptimization_content_downloader.cpp
 * @brief Content Downloader Extension using Microsoft Delivery Optimization Agent.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "deliveryoptimization_content_downloader.h" // for do_download
#include <aduc/c_utils.h> // for EXTERN_C_BEGIN, EXTERN_C_END
#include <aduc/connection_string_utils.h>
#include <aduc/contract_utils.h>
#include <aduc/hash_utils.h>
#include <aduc/logging.h>
#include <aduc/types/adu_core.h>

#include <sys/stat.h> // for stat

#include <do_config.h>
#include <do_download.h>

EXTERN_C_BEGIN

/////////////////////////////////////////////////////////////////////////////
// BEGIN Shared Library Export Functions
//
// These are the function symbols that the device update agent will
// lookup and call.
//

/**
 * @brief Gets the extension contract info.
 *
 * @param[out] contractInfo The extension contract info.
 * @return ADUC_Result The result.
 */
ADUC_Result GetContractInfo(ADUC_ExtensionContractInfo* contractInfo)
{
    contractInfo->majorVer = ADUC_V1_CONTRACT_MAJOR_VER;
    contractInfo->majorVer = ADUC_V1_CONTRACT_MINOR_VER;
    return ADUC_Result{ ADUC_GeneralResult_Success, 0 };
}

/**
 * @brief Initializes the content downloader.
 *
 * @param initializeData The initialization data.
 * @return ADUC_Result The result.
 */
ADUC_Result Initialize(const char* initializeData)
{
    ADUC_Result result{ ADUC_GeneralResult_Success };

    if (initializeData == nullptr)
    {
        Log_Info("Skipping downloader initialization. NULL input.");
        goto done;
    }

    // The connection string is valid (IoT hub connection successful) and we are ready for further processing.
    // Send connection string to DO SDK for it to discover the Edge gateway if present.
    if (ConnectionStringUtils_IsNestedEdge(initializeData))
    {
        const int ret = deliveryoptimization_set_iot_connection_string(initializeData);
        if (ret != 0)
        {
            // Since it is nested edge and if DO fails to accept the connection string, then we go ahead and
            // fail the startup.
            Log_Error("Failed to set DO connection string in Nested Edge scenario, result: %d", ret);
            result.ResultCode = ADUC_Result_Failure;
            result.ExtendedResultCode = ADUC_ERROR_DELIVERY_OPTIMIZATION_DOWNLOADER_EXTERNAL_FAILURE(ret);
            goto done;
        }
    }

done:
    return result;
}

/**
 * @brief The download export.
 *
 * @param entity The file entity.
 * @param workflowId The workflow id.
 * @param workFolder The work folder for the update payloads.
 * @param retryTimeout The retry timeout.
 * @param downloadProgressCallback The download progress callback function.
 * @return ADUC_Result The result.
 */
ADUC_Result Download(
    const ADUC_FileEntity* entity,
    const char* workflowId,
    const char* workFolder,
    unsigned int retryTimeout,
    ADUC_DownloadProgressCallback downloadProgressCallback)
{
    return do_download(entity, workflowId, workFolder, retryTimeout, downloadProgressCallback);
}

//
// END Shared Library Export Functions
/////////////////////////////////////////////////////////////////////////////

EXTERN_C_END
