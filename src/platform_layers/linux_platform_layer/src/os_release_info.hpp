/**
 * @file os_release_info.hpp
 * @brief Declaration of os release info.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef OS_RELEASE_INFO_HPP
#define OS_RELEASE_INFO_HPP

#include <string.h>
#include <string>

/**
 * @brief A class for holding OS release information.
 */
class OsReleaseInfo
{
    std::string os_name;
    std::string os_version;

public:
    OsReleaseInfo(const std::string& name, const std::string& ver) : os_name{ name }, os_version{ ver }
    {
    }

    const char* ExportOsName()
    {
        return os_name.c_str();
    }

    const char* ExportOsVersion()
    {
        return os_version.c_str();
    }
};

#endif // #define OS_RELEASE_INFO_HPP
