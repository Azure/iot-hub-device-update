/**
 * @file file_test_utils.c
 * @brief The implementation of file test utilities.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/file_test_utils.hpp"

#include <fstream>
#include <sstream>

namespace aduc
{
std::string FileTestUtils_slurpFile(const std::string& path)
{
    std::ifstream file_stream{ path };
    std::stringstream buffer;
    buffer << file_stream.rdbuf();
    return buffer.str();
}

} // namespace aduc
