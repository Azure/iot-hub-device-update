/**
 * @file auto_opendir.hpp
 * @brief implementation of some AutoOpenDir methods.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/auto_opendir.hpp"
#include <stdexcept>

#include <aducpal/dirent.h>

namespace aduc
{
AutoOpenDir::AutoOpenDir(const std::string& dirPath)
{
    DIR* d = ADUCPAL_opendir(dirPath.c_str());
    if (d == nullptr)
    {
        throw std::invalid_argument("opendir failed");
    }

    dirEntry = d;
}

AutoOpenDir::~AutoOpenDir()
{
    if (dirEntry != nullptr)
    {
        ADUCPAL_closedir(dirEntry);
        dirEntry = nullptr;
    }
}

DIR* AutoOpenDir::GetDirectoryStreamHandle()
{
    return dirEntry;
}

struct dirent* AutoOpenDir::NextDirEntry()
{
    return ADUCPAL_readdir(dirEntry);
}

} // namespace aduc
