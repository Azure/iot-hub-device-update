/**
 * @file file_utils.c
 * @brief The implementation of file utilities.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/file_utils.hpp"
#include "aduc/auto_opendir.hpp"

#include <aduc/system_utils.h>
#include <dirent.h> // opendir, readdir, closedir
#include <sys/stat.h>

namespace aduc
{
void findFilesInDir(const std::string& dirPath, std::vector<std::string>* outPaths)
{
    aduc::AutoOpenDir dirEntry{ dirPath };

    while (auto file = dirEntry.NextDirEntry())
    {
        if (file->d_name[0] == '.')
        {
            continue; // Skip everything that starts with a dot
        }

        const std::string fileName{ file->d_name };
        const std::string path = dirPath + "/" + fileName;

        if (SystemUtils_IsDir(path.c_str(), nullptr))
        {
            findFilesInDir(path, outPaths);
        }
        else if (SystemUtils_IsFile(path.c_str(), nullptr))
        {
            outPaths->push_back(path);
        }
        else
        {
            // not a dir or regular file, e.g. fifo(named pipe), socket, symlink
            continue;
        }
    }
}

} // namespace aduc
