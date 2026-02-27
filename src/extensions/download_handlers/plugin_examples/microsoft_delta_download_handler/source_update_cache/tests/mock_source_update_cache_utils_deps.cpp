/**
 * @file mock_source_update_cache_utils_deps.cpp
 * @brief C++ mock implementations for source_update_cache_utils.cpp dependencies.
 *
 * This provides the C++ mock for aduc::findFilesInDir which can't be in a .c file.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

extern "C"
{
#include "mock_source_update_cache_utils_deps.h"
}

#include <stdexcept>
#include <string>
#include <vector>

namespace aduc
{

void findFilesInDir(const std::string& /*dirPath*/, std::vector<std::string>* outFiles)
{
    if (mock_findFilesInDir_throw_std_exception)
    {
        throw std::runtime_error("mock findFilesInDir exception");
    }
    if (mock_findFilesInDir_throw_unknown)
    {
        throw 42;
    }

    if (outFiles == nullptr)
    {
        return;
    }

    outFiles->clear();
    for (size_t i = 0; i < mock_files_in_dir_count; ++i)
    {
        if (mock_files_in_dir[i] != nullptr)
        {
            outFiles->push_back(mock_files_in_dir[i]);
        }
    }
}

} // namespace aduc
