/**
 * @file vector_test_utils.cpp
 * @brief Function implementations for vector test utilities for converting to std::vector.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/vector_test_utils.hpp"
#include <azure_c_shared_utility/strings.h>

namespace aduc {
namespace testutils {

/**
 * @brief Creates a std::vector<std::string> and populates it from a VECTOR_HANDLE of STRING_HANDLE.
 * @tparam TSourceElement The type of the elements in the source VECTOR_HANDLE.
 * @tparam TTargetElement The type of the elements in the target std::vector.
 * @param vector_handle The VECTOR_HANDLE of @p TSourceElement elements to convert.
 * @return std::vector<std::string> The resultant std::vector of @p TTargetElement.
 */
template <typename TSourceElement, typename TTargetElement>
std::vector<TTargetElement> Convert_VECTOR_HANDLE_to_std_vector(const VECTOR_HANDLE& vector_handle, const std::function<TTargetElement(const TSourceElement* const&)> elementConverterFn)
{
    std::vector<TTargetElement> result_vec;
    size_t vec_len = VECTOR_size(vector_handle);
    if (vec_len == 0)
    {
        return result_vec;
    }

    for (size_t i = 0; i < vec_len; ++i)
    {
        const TSourceElement* const source_element =
            static_cast<TSourceElement*>(VECTOR_element(vector_handle, i));

        TTargetElement converted_element = elementConverterFn(source_element);
        result_vec.push_back(converted_element);
    }

    return result_vec;
}

} // namespace testutils
} // namespace aduc
