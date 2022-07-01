/**
 * @file handler_create.cpp
 * @brief Create update content handler extension.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/steps_handler.hpp"
#include "aduc/logging.h"
#include <exception>

extern "C" {

/**
 * @brief Instantiates a special handler that performs multi-steps ordered execution.
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

}
