/**
 * @file handler_create.cpp
 * @brief Create update content handler extension.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/swupdate_handler_v2.hpp"
#include "aduc/logging.h"
#include <exception>

extern "C" {

/**
 * @brief Instantiates an Update Content Handler for 'microsoft/swupdate:2' update type.
 */
ContentHandler* CreateUpdateContentHandlerExtension(ADUC_LOG_SEVERITY logLevel)
{
    ADUC_Logging_Init(logLevel, "swupdate-handler-v2");
    Log_Info("Instantiating an Update Content Handler for 'microsoft/swupdate:2'");
    try
    {
        return SWUpdateHandlerImpl::CreateContentHandler();
    }
    catch (const std::exception& e)
    {
        const char* what = e.what();
        Log_Error("Unhandled std exception: %s", what);
    }
    catch (...)
    {
        Log_Error("Unhandled exception");
    }

    return nullptr;
}

}
