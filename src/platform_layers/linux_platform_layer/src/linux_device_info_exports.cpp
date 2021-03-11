/**
 * @file linux_device_info_exports.cpp
 * @brief DeviceInfo implementation for Linux platform.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/device_info_exports.h"

#include <functional>
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
 * @brief Get operating system name.
 * Name of the operating system on the device.
 * e.g. Windows 10 IoT Core
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

    utsname uts{};

    if (uname(&uts) < 0)
    {
        Log_Error("uname failed, error: %d", errno);
        return nullptr;
    }

    valueIsDirty = false;
    return strdup(ADUC_StringUtils_Trim(uts.sysname /*Operating system name*/));
}

/**
 * @brief Get software version.
 * Version of the software on your device.
 * This could be the version of your firmware.
 * e.g. 1.3.45
 *
 * @return char* Value of property allocated with malloc, or nullptr on error or value not changed since last call.
 */
static char* DeviceInfo_GetSwVersion()
{
    static bool valueIsDirty = true;
    if (!valueIsDirty)
    {
        return nullptr;
    }

    // Note: In the V2 interface, we will implement swVersion in a more "standard" way
    // by querying the OS for version rather than reading the custom version file.
    utsname uts{};

    if (uname(&uts) < 0)
    {
        Log_Error("uname failed, error: %d", errno);
        return nullptr;
    }

    valueIsDirty = false;
    return strdup(uts.release /*Operating system release*/);
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
            { DIIP_SoftwareVersion, DeviceInfo_GetSwVersion },
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
