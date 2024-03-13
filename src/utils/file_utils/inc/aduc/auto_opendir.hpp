/**
 * @file auto_opendir.hpp
 * @brief header for AutoOpenDir. On scope exit, deletes directory if exists and is a directory.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef AUTO_OPENDIR_HPP
#define AUTO_OPENDIR_HPP

#include <string>

#include <aducpal/dirent.h>

namespace aduc
{
/**
 * @brief AutoOpenDir class wraps handling the openning / closing of a directory
 */
class AutoOpenDir
{
public:
    /**
     * @brief Default constructor for AutoOpenDir class
     * @param dirPath the path to the directory to set the member value dirEntry to
    */
    AutoOpenDir(const std::string& dirPath);
    /** @brief default destructor for AutoOpenDir
     */
    ~AutoOpenDir();

    /**
    * @brief Returns the directory in stream handle form
    * @returns a value of dirEntry in the form of a DIR*
    */
    DIR* GetDirectoryStreamHandle();
    /**
     * @brief returns the next directory entry under dirEntry
     * @returns a value of struct dirent*
    */
    struct dirent* NextDirEntry();

private:
    DIR* dirEntry; //!< Local member value pointing to the first dirEntry under the path
};

} // namespace aduc

#endif // AUTO_OPENDIR_HPP
