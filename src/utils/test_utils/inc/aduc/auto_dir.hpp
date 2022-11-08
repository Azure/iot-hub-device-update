/**
 * @file audo_dir.hpp
 * @brief header for AutoDir. On scope exit, deletes directory if exists and is a directory.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef AUTO_DIR_HPP
#define AUTO_DIR_HPP

#include <string>

namespace aduc
{
struct AutoDir
{
    AutoDir(const char* dirPath) : dir(dirPath)
    {
    }

    ~AutoDir();

    std::string GetDir() const
    {
        return dir;
    }

    bool RemoveDir() const;
    bool CreateDir() const;

private:
    std::string dir;
};

} // namespace aduc

#endif // AUTO_DIR_HPP
