/**
 * @file handler_create.cpp
 * @brief Create update content handler extension.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include <aduc/c_utils.h>
#include <aduc/logging.h>
#include <aduc/steps_handler.hpp>
#include <exception>

EXTERN_C_BEGIN

/////////////////////////////////////////////////////////////////////////////
// BEGIN Shared Library Export Functions
//
// These are the function symbols that the device update agent will
// lookup and call.
//

/**
 * @brief Instantiates a special handler that performs multi-steps ordered execution.
 * @return ContentHandler* The created instance.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel)
{
    ADUC_Logging_Init(logLevel, "steps-handler");
    Log_Info("Instantiating an Update Content Handler for MSOE");
    try
    {
        return StepsHandlerImpl::CreateContentHandler();
    }
    catch (const std::exception& e)
    {
        Log_Error("Unhandled std exception: %s", e.what());
    }
    catch (...)
    {
        Log_Error("Unhandled exception");
    }

    return nullptr;
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

//
// END Shared Library Export Functions
/////////////////////////////////////////////////////////////////////////////

EXTERN_C_END
