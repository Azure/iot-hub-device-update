/**
 * @file mock_do_download.hpp
 * @brief Implements a mock download class used in unit tests.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 *
 */
#pragma once

#include "mock_do_download_status.hpp"
#include <atomic>
#include <chrono>
#include <memory>
#include <string>

namespace microsoft
{
namespace deliveryoptimization
{
namespace details
{
class IDownload;
} // namespace details

enum class mock_download_behavior
{
    Normal,
    Timeout,
    Aborted,
    ConnectionRefused
};

class download
{
public:
    download(const std::string& uri, const std::string& downloadFilePath);
    ~download();

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    download(const download&) = delete;
    download& operator=(const download&) = delete;
    download(download&&) = delete;
    download& operator=(download&&) = delete;

    void start();
    void pause();
    void resume();
    void finalize();
    void abort();

    download_status get_status() const;

    static void download_url_to_path(
        const std::string& uri,
        const std::string& downloadFilePath,
        std::chrono::seconds timeoutSecs = std::chrono::hours(24));

    static void download_url_to_path(
        const std::string& uri,
        const std::string& downloadFilePath,
        const std::atomic_bool& isCancelled,
        std::chrono::seconds timeoutSecs = std::chrono::hours(24));

    // Test helpers.
    static void set_mock_download_behavior(mock_download_behavior behavior)
    {
        _mockBehavior = behavior;
    }

private:
    static mock_download_behavior _mockBehavior;
    static download_status _mockStatus;
};

} // namespace deliveryoptimization
} // namespace microsoft
