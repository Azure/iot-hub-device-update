/**
 * @file download_handler_factory.hpp
 * @brief Declaration of the DownloadHandlerFactory.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_DOWNLOAD_HANDLER_FACTORY_HPP
#define ADUC_DOWNLOAD_HANDLER_FACTORY_HPP

#include <string>
#include <unordered_map>

// Forward declarations
class DownloadHandlerPlugin;

class DownloadHandlerFactory
{
public:
    static DownloadHandlerFactory* GetInstance();
    DownloadHandlerPlugin* LoadDownloadHandler(const std::string& downloadHandlerId);

private:
    DownloadHandlerFactory() = default;
    static std::unordered_map<std::string, std::string>* getIdToPathMapInstance();
};

#endif // ADUC_DOWNLOAD_HANDLER_FACTORY_HPP
