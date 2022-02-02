/**
 * @file string_handle_wrapper.h
 * @brief Defines STRING_HANDLE_wrapper class
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_STRING_HANDLE_WRAPPER_HPP
#define ADUC_STRING_HANDLE_WRAPPER_HPP

#include <azure_c_shared_utility/strings.h>

namespace ADUC
{
namespace StringUtils
{
/**
 * @brief RAII class for STRING_HANDLE.
 */
struct STRING_HANDLE_wrapper
{
    /**
     * @brief Construct a new string handle wrapper object
     *
     * @param _h Existing STRING_HANDLE. This class will take ownership.
     */
    explicit STRING_HANDLE_wrapper(const STRING_HANDLE& _h) : h(_h)
    {
    }

    ~STRING_HANDLE_wrapper()
    {
        delete_handle();
    }

    STRING_HANDLE_wrapper(STRING_HANDLE_wrapper const&) = delete;
    STRING_HANDLE_wrapper& operator=(STRING_HANDLE_wrapper const&) = delete;

    /**
     * @brief Gets non-const handle to underlying STRING_HANDLE.
     *
     * @return STRING_HANDLE Underlying handle.
     */
    STRING_HANDLE get()
    {
        return h;
    }

    /**
     * @brief Get the address of the wrapped object.
     * Useful for passing to methods with the signature "f([out] STRING_HANDLE* pp)"
     *
     * @return STRING_HANDLE* Address of wrapped object.
     */
    STRING_HANDLE* address_of()
    {
        delete_handle();
        return &h;
    }

    /**
     * @brief Determines if underlying handle is valid.
     *
     * @note Does not determine if object is valid.
     *
     * @return true Valid.
     * @return false Invalid (e.g. out of memory).
     */
    bool is_null() const
    {
        return h == nullptr;
    }

    /**
     * @brief Returns const C string representation of underlying string.
     *
     * @return const char*
     */
    const char* c_str()
    {
        return STRING_c_str(h);
    }

private:
    void delete_handle()
    {
        STRING_delete(h);
        h = nullptr;
    }

    STRING_HANDLE h;
};
} // namespace StringUtils
} // namespace ADUC

#endif // ADUC_STRING_HANDLE_WRAPPER_HPP
