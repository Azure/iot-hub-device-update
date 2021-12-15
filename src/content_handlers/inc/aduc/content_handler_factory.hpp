/**
 * @file content_handler_factory.hpp
 * @brief Definition of the ContentHandlerFactory.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_CONTENT_HANDLER_FACTORY_HPP
#define ADUC_CONTENT_HANDLER_FACTORY_HPP

#include "aduc/result.h"
#include "aduc/logging.h"

#include <memory>
#include <string>
#include <unordered_map>

// Forward declaration.
class ContentHandler;

typedef ContentHandler* (*UPDATE_CONTENT_HANDLER_CREATE_PROC)(ADUC_LOG_SEVERITY logLevel);

class ContentHandlerFactory
{
public:
    static ADUC_Result LoadUpdateContentHandlerExtension(const std::string& updateType, ContentHandler** handler);
    static void UnloadAllUpdateContentHandlers();
    static void UnloadAllExtensions();
    static void Uninit();

private:
    static ADUC_Result LoadExtensionLibrary(const std::string& updateType, void** libHandle);
    static std::unordered_map<std::string, void*> _libs;
    static std::unordered_map<std::string, ContentHandler*> _contentHandlers;
    static pthread_mutex_t factoryMutex;
};

#endif // ADUC_CONTENT_HANDLER_FACTORY_HPP
