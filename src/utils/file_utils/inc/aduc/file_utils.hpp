/**
 * @file file_utils.hpp
 * @brief Function prototypes for file utilities.
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
void findFilesInDir(const std::string& dirPath, std::vector<std::string>* outPaths);

} // namespace aduc

#endif // FILE_UTILS_HPP
