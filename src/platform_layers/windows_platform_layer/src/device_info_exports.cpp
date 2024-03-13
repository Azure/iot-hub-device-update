/**
 * @file device_info_exports.cpp
 * @brief DeviceInfo implementation for Windows platform.
 *
 * Exports "DI_GetDeviceInformationValue" method.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/device_info_exports.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <aduc/config_utils.h>
#include <aduc/logging.h>

static std::string RegQueryCVStringValue(const char* subkey)
{
    std::string value;

    HKEY hkey;
    if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", &hkey) == ERROR_SUCCESS)
    {
        char buffer[128];
        DWORD buffer_size = ARRAYSIZE(buffer);
        DWORD type;
        if (RegQueryValueExA(hkey, subkey, NULL, &type, (BYTE*)buffer, &buffer_size) == 0)
        {
            if (type == REG_SZ)
            {
                value = buffer;
            }
        }

        RegCloseKey(hkey);
    }

    return value;
}

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

    std::string osName{ RegQueryCVStringValue("ProductName") };
    if (osName.empty())
    {
        return nullptr;
    }

    valueIsDirty = false;

    return strdup(osName.c_str());
}

/**
 * @brief Get OS version.
 * Version of the OS distro on the device.
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
    std::string osVersion{ RegQueryCVStringValue("CurrentVersion") };
    if (osVersion.empty())
    {
        return nullptr;
    }

    valueIsDirty = false;

    return strdup(osVersion.c_str());
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

    std::string processorArchitecture{ RegQueryCVStringValue("CurrentType") };
    if (processorArchitecture.empty())
    {
        return nullptr;
    }

    valueIsDirty = false;
    return strdup(processorArchitecture.c_str());
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

    std::string processorManufacturer{ "n/a" };
    if (processorManufacturer.empty())
    {
        return nullptr;
    }

    valueIsDirty = false;
    return strdup(processorManufacturer.c_str());
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

    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms))
    {
        Log_Error("GlobalMemoryStatusEx failed, error: %d", GetLastError());
        return nullptr;
    }

    const unsigned int bytes_in_kilobyte = 1024;
    const std::string buffer{ std::to_string(ms.ullTotalPhys / bytes_in_kilobyte) };

    valueIsDirty = false;
    return strdup(buffer.c_str());
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

    ULARGE_INTEGER totalNumberOfBytes;
    if (!GetDiskFreeSpaceExA(NULL /*current drive*/, NULL, &totalNumberOfBytes, NULL))
    {
        Log_Error("GetDiskFreeSpaceEx failed, error: %d", GetLastError());
        return nullptr;
    }

    const unsigned int bytes_in_kilobyte = 1024;
    const std::string buffer{ std::to_string(totalNumberOfBytes.QuadPart / bytes_in_kilobyte) };

    valueIsDirty = false;
    return strdup(buffer.c_str());
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
        const std::unordered_map<DI_DeviceInfoProperty, std::function<char*()>> functionMap{
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
        value = functionMap.at(property)();
    }
    catch (...)
    {
        value = nullptr;
    }

    return value;
}

EXTERN_C_END
