/**
 * @file mock_do_exceptions.hpp
 * @brief Implements a mock exception class used in unit tests.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#pragma once

#include <exception>
#include <stdint.h>
#include <system_error>

namespace microsoft
{
namespace deliveryoptimization
{
const int32_t S_OK = 0x00000000;
const int32_t E_UNEXPECTED = 0x8000FFFF;
const int32_t E_INVALIDARG = 0x80070057L;
const int32_t E_TIMEOUT = 0x800705B4;
const int32_t DO_E_NO_SERVICE = 0x80D01001L;
const int32_t DO_E_DOWNLOAD_NO_PROGRESS = 0x80d02002;
const int32_t HTTP_E_STATUS_NOT_FOUND = 0x80190194;
const int32_t E_NOT_FOUND = 0x80070490;

class exception : public std::exception
{
public:
    explicit exception(int32_t code = -1) : _errorCode(code)
    {
    }

    const char* what() const noexcept override
    {
        return "Unspecified DO Exception";
    }

    int32_t error_code() const
    {
        return _errorCode;
    }

private:
    int32_t _errorCode;
};

void ThrowException(int32_t errorCode);
void ThrowException(std::errc error);

} // namespace deliveryoptimization
} // namespace microsoft
