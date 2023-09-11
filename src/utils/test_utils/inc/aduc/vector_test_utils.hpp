/**
 * @file vector_test_utils.hpp
 * @brief Function prototypes for vector test utilities for converting to std::vector.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef VECTOR_TEST_UTILS_HPP
#define VECTOR_TEST_UTILS_HPP

namespace aduc {
namespace testutils {

#include <functional>
#include <string>
#include <vector>
#include <azure_c_shared_utility/vector.h>

using ElementConverterFunction = std::function<TTargetElement(const TSourceElement&)>;

template <typename TSourceElement, typename TTargetElement>
std::vector<TTargetElement> Convert_VECTOR_HANDLE_to_std_vector(const VECTOR_HANDLE& vector_handle, const std::function<TTargetElement(const TSourceElement* const&)> elementConverterFn);

} // namespace testutils
} // namespace aduc
//
#endif // VECTOR_TEST_UTILS_HPP
