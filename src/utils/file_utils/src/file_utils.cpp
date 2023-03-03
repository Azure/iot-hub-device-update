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
#include <dirent.h>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>

namespace aduc
{
void findFilesInDir(const std::string& dirPath, std::vector<std::string>* outPaths)
{
    if (!SystemUtils_IsDir(dirPath.c_str(), nullptr))
    {
        throw std::invalid_argument{ "not a dir" };
    }

    std::queue<std::string> dirQueue{ { dirPath } };

    while (!dirQueue.empty())
    {
        const std::string nextDir{ std::move(dirQueue.front()) };
        dirQueue.pop();

        aduc::AutoOpenDir dirEntry{ nextDir };

        struct dirent* file = nullptr;
        while ((file = dirEntry.NextDirEntry()) != nullptr)
        {
            if (file->d_name == nullptr || file->d_name[0] == '.')
            {
                continue;
            }

            std::stringstream ss;
            ss << dirPath << "/" << file->d_name;
            std::string path{ ss.str() };

            if (SystemUtils_IsDir(path.c_str(), nullptr))
            {
                dirQueue.push(path);
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
}

} // namespace aduc
