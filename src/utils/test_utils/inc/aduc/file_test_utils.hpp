/**
 * @file file_test_utils.hpp
 * @brief Function prototypes for file test utilities.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef FILE_TEST_UTILS_HPP
#define FILE_TEST_UTILS_HPP

#include <string>
#include <vector>

namespace aduc
{
std::string FileTestUtils_slurpFile(const std::string& path);

} // namespace aduc

#endif // FILE_TEST_UTILS_HPP
