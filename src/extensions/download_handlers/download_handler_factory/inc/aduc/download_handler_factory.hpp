/**
 * @file download_handler_factory.hpp
 * @brief Declaration of the DownloadHandlerFactory.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_DOWNLOAD_HANDLER_FACTORY_HPP
#define ADUC_DOWNLOAD_HANDLER_FACTORY_HPP

#include <memory>
#include <string>
#include <unordered_map>

// Forward declarations
class DownloadHandlerPlugin;

/**
 * @brief The download handler factory that creates and owns all download handler plugin instances.
 *
 */
class DownloadHandlerFactory
{
public:
    /**
     * @brief Get the singleton instance of the factory object.
     *
     * @return DownloadHandlerFactory* The factory singleton instance.
     */
    static DownloadHandlerFactory* GetInstance();

    /**
     * @brief Provides the plugin instance for the downloadHandlerId.
     *
     * @param downloadHandlerId The download handler id.
     * @return DownloadHandlerPlugin* The download handler plugin instance.
     */
    DownloadHandlerPlugin* LoadDownloadHandler(const std::string& downloadHandlerId) noexcept;

private:
    DownloadHandlerFactory() = default;

    /**
     * @brief The collection of download handler plugin instances, keyed by downloadHandleId string.
     * @details The plugin instances will be auto-freed upon destruction of the factory instance.
     *
     */
    std::unordered_map<std::string, std::unique_ptr<DownloadHandlerPlugin>> cachedPlugins;
};

#endif // ADUC_DOWNLOAD_HANDLER_FACTORY_HPP
