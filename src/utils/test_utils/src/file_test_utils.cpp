/**
 * @file file_test_utils.cpp
 * @brief The implementation of file test utilities.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/file_test_utils.hpp"

#include <fstream>
#include <regex>
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

std::string
FileTestUtils_applyTemplateParam(const std::string& templateStr, const char* parameterName, const char* parameterValue)
{
    std::stringstream ss;
    ss << "\\{\\{" << parameterName << "\\}\\}";
    std::string placeholder{ ss.str() };
    return std::regex_replace(templateStr, std::regex(placeholder.c_str()), parameterValue);
}

} // namespace aduc
