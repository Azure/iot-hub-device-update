/**
 * @file audo_dir.hpp
 * @brief header for AutoOpenDir. On scope exit, deletes directory if exists and is a directory.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef AUTO_OPENDIR_HPP
#define AUTO_OPENDIR_HPP

#include <dirent.h>
#include <string>

namespace aduc
{
class AutoOpenDir
{
public:
    AutoOpenDir(const std::string& dirPath);
    ~AutoOpenDir();

    DIR* GetDirectoryStreamHandle()
    {
        return dirEntry;
    }

    struct dirent* NextDirEntry()
    {
        return readdir(dirEntry);
    }

private:
    DIR* dirEntry;
};

} // namespace aduc

#endif // AUTO_OPENDIR_HPP
