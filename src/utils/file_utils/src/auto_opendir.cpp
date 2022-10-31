/**
 * @file auto_opendir.hpp
 * @brief implementation of some AutoOpenDir methods.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/auto_opendir.hpp"
#include <stdexcept>

namespace aduc
{
AutoOpenDir::AutoOpenDir(const std::string& dirPath)
{
    auto d = opendir(dirPath.c_str());
    if (d != nullptr)
    {
        dirEntry = d;
    }
}

AutoOpenDir::~AutoOpenDir()
{
    if (dirEntry != nullptr)
    {
        closedir(dirEntry);
        dirEntry = nullptr;
    }
}

} // namespace aduc
