/**
 * @file free_deleter.hpp
 * @brief A std::unique_ptr deleter to use with malloc allocated pointers.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef FREE_DELETER_HPP
#define FREE_DELETER_HPP

#include <stdlib.h> // free

namespace aduc
{
template<typename T>
struct FreeDeleter
{
    T& operator()(T* ptr)
    {
        free(ptr);
        return *ptr;
    }
};

} // namespace aduc

#endif // FREE_DELETER_HPP
