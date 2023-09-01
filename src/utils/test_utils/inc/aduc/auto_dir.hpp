/**
 * @file auto_dir.hpp
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
struct AutoDir //!< AutoDir wrapping struct
{
    /** 
     * @brief default constructor for AutoDir
     * @param dirPath value to set the member value dir too
    */
    AutoDir(const char* dirPath) : dir(dirPath)
    {
    }

    /**
    * @brief Default destructor
    */
    ~AutoDir();

    /**
     * @brief Getter function for the dir member value
     * @returns a const representation of dir
    */
    std::string GetDir() const
    {
        return dir;
    }

    /**
     * @brief Removes the directory set to @p dir
     * @returns true on success; false on failure
    */
    bool RemoveDir() const;

    /**
     * @brief Creates the directory set to @p dir
     * @returns true on success; false on failure
    */
    bool CreateDir() const;

private:
    std::string dir; //!< Member value that represents a path to a directory
};

} // namespace aduc

#endif // AUTO_DIR_HPP
