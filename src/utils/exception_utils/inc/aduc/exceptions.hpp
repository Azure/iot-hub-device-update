/**
 * @file exceptions.hpp
 * @brief Defines ADU Agent exceptions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_EXCEPTIONS_HPP
#define ADUC_EXCEPTIONS_HPP

#include <aduc/logging.h>
#include <aduc/result.h>
#include <errno.h>
#include <exception>
#include <system_error>

namespace ADUC
{
/**
     * @class Exception
     * @brief Exception class for ADU Agent exceptions. Inherits from std::exception.
     */
class Exception : public std::exception
{
public:
    /**
     * @brief Throw a new ADUC::Exception with an ADUC_Result_t
     *
     * Use the ADUC::Exception::Throw* factory functions instead of create an ADUC::Exception directly.
     *
     * @param code The extended result code associated with the exception.
     * @param message The message associated with the exception. Always pass a meaningful message.
     */
    static void ThrowAducResult(const ADUC_Result_t code, const std::string& message)
    {
        Log_Info("Throwing ADU Agent exception. code: %d (0x%x), message: %s", code, code, message.c_str());
        throw Exception(code, message);
    }

    /**
     * @brief Throw a new ADUC::Exception with a std::errc
     *
     * Use the ADUC::Exception::Throw* factory functions instead of create an ADUC::Exception directly.
     *
     * @param code The error code associated with the exception.
     * @param message The message associated with the exception. Always pass a meaningful message.
     */
    static void ThrowErrc(const std::errc code, const std::string& message)
    {
        ThrowErrno(static_cast<int>(code), message);
    }

    /**
     * @brief Throw a new ADUC::Exception with an errno
     *
     * Use the ADUC::Exception::Throw* factory functions instead of create an ADUC::Exception directly.
     *
     * @param code The error code associated with the exception.
     * @param message The message associated with the exception. Always pass a meaningful message.
     */
    static void ThrowErrno(int code, const std::string& message)
    {
        ThrowAducResult(MAKE_ADUC_ERRNO_EXTENDEDRESULTCODE(code), message);
    }

    /**
     * @brief Override of std::exception::what
     * @return String identifier of the exception.
     */
    const char* what() const noexcept override
    {
        return "ADU Agent Exception";
    }

    /**
     * @brief Get the result code associated with the exception.
     * @return The result code associated with the excpetion.
     */
    ADUC_Result_t Code() const noexcept
    {
        return _code;
    }

    /**
     * @brief Get the message associated with the exception.
     * @return The message associated with the exception.
     */
    std::string Message() const
    {
        return _message;
    }

private:
    Exception(const ADUC_Result_t code, const std::string& message) : _code{ code }, _message{ message }
    {
    }

    ADUC_Result_t _code{ ADUC_ERC_NOTRECOVERABLE };
    std::string _message;
};
} // namespace ADUC

#endif // ADUC_EXCEPTIONS_HPP
