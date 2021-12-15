/**
 * @file simulator_device_info_exports.cpp
 * @brief Implements exported methods for platform-specific device information code.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "simulator_device_info.h"

#include <functional>
#include <unordered_map>

#include <cstring>

#include <aduc/device_info_exports.h>

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
    static std::string lastReportedValue;
    if (lastReportedValue == Simulator_DeviceInfo_GetManufacturer())
    {
        // Value hasn't changed since last report.
        return nullptr;
    }

    try
    {
        lastReportedValue = Simulator_DeviceInfo_GetManufacturer();
    }
    catch (...)
    {
    }

    return strdup(lastReportedValue.c_str());
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
    static std::string lastReportedValue;
    if (lastReportedValue == Simulator_DeviceInfo_GetModel())
    {
        // Value hasn't changed since last report.
        return nullptr;
    }

    try
    {
        lastReportedValue = Simulator_DeviceInfo_GetModel();
    }
    catch (...)
    {
    }

    return strdup(lastReportedValue.c_str());
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
    if (valueIsDirty)
    {
        valueIsDirty = false;
        return strdup("Linux");
    }

    // Value not expected to change again, so return nullptr;
    return nullptr;
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
    // Value must be returned at least once, so initialize to true.
    static std::string lastReportedValue;
    if (lastReportedValue == Simulator_DeviceInfo_GetSwVersion())
    {
        // Value hasn't changed since last report.
        return nullptr;
    }

    try
    {
        lastReportedValue = Simulator_DeviceInfo_GetSwVersion();
    }
    catch (...)
    {
    }

    return strdup(lastReportedValue.c_str());
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
    if (valueIsDirty)
    {
        valueIsDirty = false;
        return strdup("x86_64");
    }

    // Value not expected to change again, so return nullptr;
    return nullptr;
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
    if (valueIsDirty)
    {
        valueIsDirty = false;
        return strdup("GenuineIntel");
    }

    // Value not expected to change again, so return nullptr;
    return nullptr;
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
    if (valueIsDirty)
    {
        valueIsDirty = false;
        return strdup("256000");
    }

    // Value not expected to change again, so return nullptr;
    return nullptr;
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
    if (valueIsDirty)
    {
        valueIsDirty = false;
        return strdup("2048000");
    }

    // Value not expected to change again, so return nullptr;
    return nullptr;
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
