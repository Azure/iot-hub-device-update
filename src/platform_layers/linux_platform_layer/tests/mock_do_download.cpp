/**
 * @file mock_do_download.cpp
 * @brief Mock download service.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "mock_do_download.hpp"
#include "mock_do_exceptions.hpp"

#include <thread>

using microsoft::deliveryoptimization::download;
using microsoft::deliveryoptimization::download_state;
using microsoft::deliveryoptimization::download_status;
using microsoft::deliveryoptimization::mock_download_behavior;

// Mock download_status class implementation.
bool download_status::is_error() const
{
    return _errorCode != 0;
}

bool download_status::is_transient_error() const
{
    return (_errorCode == 0) && (_extendedErrorCode != 0);
}

bool download_status::is_complete() const
{
    return _state == download_state::Transferred;
}

// Mock DO exception helpers.
void microsoft::deliveryoptimization::ThrowException(int32_t errorCode)
{
    throw exception(errorCode);
}

void microsoft::deliveryoptimization::ThrowException(std::errc error)
{
    throw exception(static_cast<int32_t>(error));
}

// Mock download class implementation
download_status download::_mockStatus;
mock_download_behavior download::_mockBehavior = mock_download_behavior::Normal;

download::download(const std::string& /*uri*/, const std::string& /*downloadFilePath*/)
{
}

download::~download() = default;

std::error_code download::start()
{
    _mockStatus.set_mock_state(download_state::Created);
    return DO_OK;
}

std::error_code download::pause()
{
    if (_mockStatus.state() == download_state::Transferring)
    {
        _mockStatus.set_mock_state(download_state::Paused);
    }

    return DO_OK;
}

std::error_code download::resume()
{
    if (_mockStatus.state() == download_state::Paused)
    {
        _mockStatus.set_mock_state(download_state::Transferring);
    }

    return DO_OK;
}

std::error_code download::finalize()
{
    _mockStatus.set_mock_state(download_state::Finalized);

    return DO_OK;
}

std::error_code download::abort()
{
    _mockStatus.set_mock_state(download_state::Aborted);

    return DO_OK;
}

download_status download::get_status() const
{
    return _mockStatus;
}

const std::error_code download::download_url_to_path(
    const std::string& /*uri*/, const std::string& /*downloadFilePath*/, std::chrono::seconds /*timeoutSecs*/)
{
    // No-op.
    return DO_OK;
}

const std::error_code download::download_url_to_path(
    const std::string& /*uri*/,
    const std::string& /*downloadFilePath*/,
    const std::atomic_bool& /*isCancelled*/,
    std::chrono::seconds /*timeoutSecs*/)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    switch (_mockBehavior)
    {
    case mock_download_behavior::Normal:
        _mockStatus.set_mock_state(download_state::Finalized);
        break;

    case mock_download_behavior::Timeout:
        ThrowException(std::errc::timed_out);
        break;

    case mock_download_behavior::Aborted:
        _mockStatus.set_mock_state(download_state::Aborted);
        ThrowException(std::errc::operation_canceled);
        break;

    case mock_download_behavior::ConnectionRefused:
        ThrowException(std::errc::connection_refused);
        break;
    }

    return std::error_code();
}
