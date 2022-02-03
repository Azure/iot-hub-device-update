/**
 * @file calloc_wrapper.h
 * @brief Defines calloc_wrapper class, which performs RAII for malloc'd pointers.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef ADUC_CALLOC_WRAPPER_HPP
#define ADUC_CALLOC_WRAPPER_HPP

#include <cstdlib> // for free

namespace ADUC
{
namespace StringUtils
{
template<class T>
class calloc_wrapper
{
public:
    /**
     * @brief Construct a new calloc wrapper object
     *
     * @param ptr Initial pointer value, or nullptr for empty.
     */
    explicit calloc_wrapper(T* ptr = nullptr) : m_ptr(ptr)
    {
    }

    // Delete copy ctor, copy assignment, move ctor and move assignment operators.
    calloc_wrapper(const calloc_wrapper&) = delete;
    calloc_wrapper& operator=(const calloc_wrapper&) = delete;
    calloc_wrapper(calloc_wrapper&&) = delete;
    calloc_wrapper& operator=(calloc_wrapper&&) = delete;

    /**
     * @brief Destroy the calloc wrapper object
     */
    ~calloc_wrapper()
    {
        free();
    }

    /**
     * @brief Get the wrapped object.
     *
     * @return const T* Pointer to wrapped object.
     */
    const T* get() const
    {
        return m_ptr;
    }

    /**
     * @brief Get the address of the wrapped object.
     * Useful for passing to methods with the signature "f([out] T** pp)"
     *
     * @return T** Address of wrapped object.
     */
    T** address_of()
    {
        free();
        return &m_ptr;
    }

    /**
     * @brief Free and null the wrapped object, using free()
     */
    void free()
    {
        if (m_ptr != nullptr)
        {
            // NOLINTNEXTLINE(hicpp-no-malloc,cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
            ::free(m_ptr);
            m_ptr = nullptr;
        }
    }

private:
    T* m_ptr;
};

/**
 * @brief Specialization for char* buffers allocated with malloc.
 */
using cstr_wrapper = calloc_wrapper<char>;

} // namespace StringUtils
} // namespace ADUC

#endif // ADUC_CALLOC_WRAPPER_HPP
