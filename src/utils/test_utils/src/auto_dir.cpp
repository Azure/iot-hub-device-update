/**
 * @file audo_dir.hpp
 * @brief implementation of some AutoDir methods.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/auto_dir.hpp"
#include <aduc/system_utils.h> // [ADUC_]SystemUtils_*

namespace aduc
{
AutoDir::~AutoDir()
{
    if (SystemUtils_IsDir(dir.c_str(), nullptr))
    {
        ADUC_SystemUtils_RmDirRecursive(dir.c_str());
    }
}

bool AutoDir::RemoveDir() const
{
    if (SystemUtils_IsDir(dir.c_str(), nullptr))
    {
        return 0 == ADUC_SystemUtils_RmDirRecursive(dir.c_str());
    }

    return true;
}

bool AutoDir::CreateDir() const
{
    if (SystemUtils_IsDir(dir.c_str(), nullptr))
    {
        return false;
    }

    return 0 == ADUC_SystemUtils_MkDirRecursiveDefault(dir.c_str());
}

} // namespace aduc
