/**
 * @file excpetion_utils.hpp
 * @brief Contains utilities for handling C++ exceptions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_EXCEPTION_UTILS_HPP
#define ADUC_EXCEPTION_UTILS_HPP

#include <aduc/exceptions.hpp>
#include <aduc/logging.h>
#include <exception>

namespace ADUC
{
namespace ExceptionUtils
{
/**
 * @brief Wrapper function to call a method and handle standard exceptions so they dont "leak".
 *
 * @tparam TCallback Signature of callback method.
 * @param callback Method to call.
 * @return ADUC_Result Result.
 */
template<typename TCallback>
void CallVoidMethodAndHandleExceptions(const TCallback& callback)
{
    try
    {
        callback();
    }
    catch (const ADUC::Exception& e)
    {
        Log_Warn("Unhandled ADU Agent exception. code: %d, message: %s", e.Code(), e.Message().c_str());
    }
    catch (const std::exception& e)
    {
        Log_Warn("Unhandled std exception: %s", e.what());
    }
    catch (...)
    {
        Log_Warn("Unhandled exception");
    }
}

/**
 * @brief Wrapper function to call a method and handle standard exceptions so they dont "leak".
 *
 * @tparam TCallback Signature of callback method.
 * @param failureResultCode Error to return on failure.
 * @param callback Method to call.
 * @return ADUC_Result Result.
 */
template<typename TCallback>
ADUC_Result CallResultMethodAndHandleExceptions(ADUC_Result_t failureResultCode, const TCallback& callback)
{
    try
    {
        return callback();
    }
    catch (const ADUC::Exception& e)
    {
        const char* message = e.Message().c_str();
        const int code = e.Code();
        Log_Warn("Unhandled ADU Agent exception. code: %d, message: %s", code, message);
        return ADUC_Result{ failureResultCode, code };
    }
    catch (const std::exception& e)
    {
        const char* what = e.what();
        Log_Warn("Unhandled std exception: %s", what);
        return ADUC_Result{ failureResultCode, ADUC_ERC_NOTRECOVERABLE };
    }
    catch (...)
    {
        Log_Warn("Unhandled exception");
        return ADUC_Result{ failureResultCode, ADUC_ERC_NOTRECOVERABLE };
    }
}
} // namespace ExceptionUtils
} // namespace ADUC

#endif // ADUC_EXCEPTION_UTILS_HPP
