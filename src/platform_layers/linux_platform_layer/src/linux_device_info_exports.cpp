/**
 * @file linux_device_info_exports.cpp
 * @brief DeviceInfo implementation for Linux platform.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/device_info_exports.h"
#include "os_release_info.hpp"

#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include <cstring>
#include <sys/statvfs.h> // statvfs
#include <sys/sysinfo.h> // sysinfo
#include <sys/utsname.h> // uname

#include <aduc/config_utils.h>
#include <aduc/logging.h>
#include <aduc/string_c_utils.h>
#include <aduc/string_utils.hpp>
#include <aduc/system_utils.h>

#define ETC_OS_RELEASE_FILEPATH "/etc/os-release"
#define ETC_LSB_RELEASE_FILEPATH "/etc/lsb-release"

/**
 * @brief Get manufacturer
 * Company name of the device manufacturer.
 * This could be the same as the name of the original equipment manufacturer (OEM).
 * e.g. Contoso
 *
 * @return char* Value of property allocated with malloc, or nullptr on error or value not changed since last call.
 */
static char* DeviceInfo_GetManufacturer()
{
    // Value must be returned at least once, so initialize to true.
    static bool valueIsDirty = true;
    if (!valueIsDirty)
    {
        return nullptr;
    }

    char* result = nullptr;

    ADUC_ConfigInfo config = {};
    if (ADUC_ConfigInfo_Init(&config, ADUC_CONF_FILE_PATH) && config.manufacturer != nullptr)
    {
        result = strdup(config.manufacturer);
    }
    else
    {
        // If file doesn't exist, or value wasn't specified, use build default.
        result = strdup(ADUC_DEVICEINFO_MANUFACTURER);
    }

    valueIsDirty = false;
    ADUC_ConfigInfo_UnInit(&config);
    return result;
}

/**
 * @brief Get device model.
 * Device model name or ID.
 * e.g. Surface Book 2
 *
 * @return char* Value of property allocated with malloc, or nullptr on error or value not changed since last call.
 */
static char* DeviceInfo_GetModel()
{
    // Value must be returned at least once, so initialize to true.
    static bool valueIsDirty = true;
    if (!valueIsDirty)
    {
        return nullptr;
    }

    char* result = nullptr;
    ADUC_ConfigInfo config = {};
    if (ADUC_ConfigInfo_Init(&config, ADUC_CONF_FILE_PATH) && config.model != nullptr)
    {
        result = strdup(config.model);
    }
    else
    {
        // If file doesn't exist, or value wasn't specified, use build default.
        result = strdup(ADUC_DEVICEINFO_MODEL);
    }

    valueIsDirty = false;
    ADUC_ConfigInfo_UnInit(&config);
    return result;
}

/**
 * @brief Gets value for a property in an etc release file with one name-value pair separated by equal-sign delimiter per line.
 * @param path The path to the file of name-value pair lines
 * @param name_property_name The identifier for the property that represents the os name.
 * @param version_property_name The identifier for the property that represents the os version.
 * @return OsReleaseInfo* The new OS release infol, or nullptr on error.
 * @detail Caller will own OsReleaseInfo instance and must free with delete. Also, values will have surrounding double-quotes removed.
 */
static OsReleaseInfo* get_os_release_info(const char* path, const char* name_property_name, const char* version_property_name)
{
    std::unique_ptr<OsReleaseInfo> result;

    std::ifstream file{ path };
    if (!file.is_open())
    {
        Log_Error("File %s failed to open, error: %d", path, errno);
        return nullptr;
    }

    std::string os_name;
    std::string os_version;

    std::string line;
    while (std::getline(file, line))
    {
        std::string trimmed = ADUC::StringUtils::Trim(line);
        if (trimmed.length() == 0)
        {
            continue;
        }

        auto parts = ADUC::StringUtils::Split(trimmed, '=');
        if (parts[0] == name_property_name)
        {
            ADUC::StringUtils::RemoveSurrounding(parts[1], '"');
            os_name = std::move(parts[1]);
        }
        else if (parts[0] == version_property_name)
        {
            ADUC::StringUtils::RemoveSurrounding(parts[1], '"');
            os_version = std::move(parts[1]);
        }

        if (os_name.length() > 0 && os_version.length() > 0)
        {
            result = std::unique_ptr<OsReleaseInfo>{ new OsReleaseInfo{ os_name, os_version } };
            break;
        }
    }

    if (result)
    {
        return result.release();
    }

    Log_Debug("'%s' or '%s' property missing or '%s' does not exist", name_property_name, version_property_name, path);

    return nullptr;
}

/**
 * @brief Get operating system name.
 * Name of the operating system on the device.
 *
 * @return char* Value of property allocated with malloc, or nullptr on error or value not changed since last call.
 */
static char* DeviceInfo_GetOsName()
{
    // Value must be returned at least once, so initialize to true.
    static bool valueIsDirty = true;
    if (!valueIsDirty)
    {
        return nullptr;
    }

    valueIsDirty = false;

    // First, use /etc/os-release that is required by systemd-based systems.
    // As per os-release(5) manpage, read the announcement here: http://0pointer.de/blog/projects/os-release
    if (SystemUtils_IsFile(ETC_OS_RELEASE_FILEPATH, nullptr))
    {
        std::unique_ptr<OsReleaseInfo> releaseInfo { get_os_release_info(ETC_OS_RELEASE_FILEPATH, "NAME", "VERSION") };
        if (releaseInfo)
        {
            return strdup(releaseInfo->ExportOsName());
        }
    }

    // Next, try the Linux Standard Base file present on some systems
    if (SystemUtils_IsFile(ETC_LSB_RELEASE_FILEPATH, nullptr))
    {
        std::unique_ptr<OsReleaseInfo> releaseInfo { get_os_release_info(ETC_LSB_RELEASE_FILEPATH, "DISTRIB_ID", "DISTRIB_RELEASE") };
        if (releaseInfo)
        {
            return strdup(releaseInfo->ExportOsName());
        }
    }

    // Finally, fallback to uname strategy
    {
        utsname uts{};
        if (uname(&uts) < 0)
        {
            Log_Error("uname failed, error: %d", errno);
        }
        else
        {
            return strdup(uts.sysname);
        }
    }

    return nullptr;
}

/**
 * @brief Get OS version.
 * Version of the OS distro or linux kernel on the device.
 *
 * @return char* Value of property allocated with malloc, or nullptr on error or value not changed since last call.
 */
static char* DeviceInfo_GetOsVersion()
{
    static bool valueIsDirty = true;
    if (!valueIsDirty)
    {
        return nullptr;
    }

    valueIsDirty = false;

    // First, use /etc/os-release that is required by systemd-based systems.
    // As per os-release(5) manpage, read the announcement here: http://0pointer.de/blog/projects/os-release
    if (SystemUtils_IsFile(ETC_OS_RELEASE_FILEPATH, nullptr))
    {
        std::unique_ptr<OsReleaseInfo> releaseInfo { get_os_release_info(ETC_OS_RELEASE_FILEPATH, "NAME", "VERSION") };
        if (releaseInfo)
        {
            return strdup(releaseInfo->ExportOsVersion());
        }
    }

    // Next, try the Linux Standard Base file present on some systems
    if (SystemUtils_IsFile(ETC_LSB_RELEASE_FILEPATH, nullptr))
    {
        std::unique_ptr<OsReleaseInfo> releaseInfo { get_os_release_info(ETC_LSB_RELEASE_FILEPATH, "DISTRIB_ID", "DISTRIB_RELEASE") };
        if (releaseInfo)
        {
            return strdup(releaseInfo->ExportOsVersion());
        }
    }

    // Finally, fallback to uname strategy
    {
        // Note: In the V2 interface, we will implement swVersion in a more "standard" way
        // by querying the OS for version rather than reading the custom version file.
        //
        // It is "swVersion" on "the wire" in the device info interface, but has been repurposed for OS distro / kernel release version.
        utsname uts{};

        if (uname(&uts) < 0)
        {
            Log_Error("uname failed, error: %d", errno);
        }
        else
        {
            // This is typically just the kernel version.
            return strdup(uts.release /*Operating system release*/);
        }
    }

    return nullptr;
}

/**
 * @brief Get processor architecture.
 * Architecture of the processor on the device.
 * e.g. x64
 *
 * @return char* Value of property allocated with malloc, or nullptr on error or value not changed since last call.
 */
static char* DeviceInfo_GetProcessorArchitecture()
{
    // Value must be returned at least once, so initialize to true.
    static bool valueIsDirty = true;
    if (!valueIsDirty)
    {
        return nullptr;
    }

    utsname uts{};

    if (uname(&uts) < 0)
    {
        Log_Error("uname failed, error: %d", errno);
        return nullptr;
    }

    valueIsDirty = false;
    return strdup(ADUC_StringUtils_Trim(uts.machine /*Hardware identifier*/));
}

/**
 * @brief Get processor manufacturer.
 * Name of the manufacturer of the processor on the device.
 * e.g. Intel
 *
 * @return char* Value of property allocated with malloc, or nullptr on error or value not changed since last call.
 */
static char* DeviceInfo_GetProcessorManufacturer()
{
    // Value must be returned at least once, so initialize to true.
    static bool valueIsDirty = true;
    if (!valueIsDirty)
    {
        return nullptr;
    }

    std::string manufacturer;
    const unsigned int kBufferSize = 256;
    char buffer[kBufferSize];

    FILE* pipe{ popen("/usr/bin/lscpu", "r") }; // NOLINT(cert-env33-c)
    if (pipe != nullptr)
    {
        const char* prefix = "Vendor ID:           ";
        const unsigned int prefix_cch = strlen(prefix);
        while (fgets(buffer, kBufferSize, pipe) != nullptr)
        {
            if (strncmp(buffer, prefix, prefix_cch) == 0)
            {
                // Trim at newline.
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
                buffer[strcspn(buffer, "\r\n")] = '\0';

                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                manufacturer = buffer + prefix_cch;
            }
        }

        pclose(pipe);
    }

    if (manufacturer.empty())
    {
        return nullptr;
    }

    valueIsDirty = false;
    ADUC::StringUtils::Trim(manufacturer);
    return strdup(manufacturer.c_str());
}

/**
 * @brief Get total memory.
 * Total available memory on the device in kilobytes.
 * e.g. 256000
 *
 * @return char* Value of property allocated with malloc, or nullptr on error or value not changed since last call.
 */
static char* DeviceInfo_GetTotalMemory()
{
    // Value must be returned at least once, so initialize to true.
    static bool valueIsDirty = true;
    if (!valueIsDirty)
    {
        return nullptr;
    }

    struct sysinfo sys_info
    {
    };
    if (sysinfo(&sys_info) == -1)
    {
        Log_Error("sysinfo failed, error: %d", errno);
        return nullptr;
    }

    std::stringstream buffer;

    const unsigned int bytes_in_kilobyte = 1024;
    buffer << (sys_info.totalram * sys_info.mem_unit) / bytes_in_kilobyte;

    valueIsDirty = false;
    return strdup(buffer.str().c_str());
}

/**
 * @brief Get total storage.
 * Total available storage on the device in kilobytes.
 * e.g. 2048000
 *
 * @return char* Value of property allocated with malloc, or nullptr on error or value not changed since last call.
 */
static char* DeviceInfo_GetTotalStorage()
{
    // Value must be returned at least once, so initialize to true.
    static bool valueIsDirty = true;
    if (!valueIsDirty)
    {
        return nullptr;
    }

    struct statvfs buf
    {
    };

    if (statvfs("/", &buf) == -1)
    {
        Log_Error("statvfs failed, error: %d", errno);
        return nullptr;
    }

    std::stringstream buffer;

    const unsigned int bytes_in_kilobyte = 1024;
    buffer << (buf.f_blocks * buf.f_frsize) / bytes_in_kilobyte;

    valueIsDirty = false;
    return strdup(buffer.str().c_str());
}

//
// Exported methods
//

EXTERN_C_BEGIN

/**
 * @brief Return a specific device information value.
 *
 * @param property Property to retrieve
 * @return char* Value of property allocated with malloc, or nullptr on error or value not changed since last call.
 */
char* DI_GetDeviceInformationValue(DI_DeviceInfoProperty property)
{
    char* value = nullptr;

    try
    {
        const std::unordered_map<DI_DeviceInfoProperty, std::function<char*()>> simulationMap{
            { DIIP_Manufacturer, DeviceInfo_GetManufacturer },
            { DIIP_Model, DeviceInfo_GetModel },
            { DIIP_OsName, DeviceInfo_GetOsName },
            { DIIP_SoftwareVersion, DeviceInfo_GetOsVersion },
            { DIIP_ProcessorArchitecture, DeviceInfo_GetProcessorArchitecture },
            { DIIP_ProcessorManufacturer, DeviceInfo_GetProcessorManufacturer },
            { DIIP_TotalMemory, DeviceInfo_GetTotalMemory },
            { DIIP_TotalStorage, DeviceInfo_GetTotalStorage },
        };

        // Call the handler for the device info property to retrieve current value.
        value = simulationMap.at(property)();
    }
    catch (...)
    {
        value = nullptr;
    }

    return value;
}

EXTERN_C_END
