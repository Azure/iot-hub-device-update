/**
 * @file simulator_device_info.cpp
 * @brief Methods to set and return simulated device info values.
 *
 * These methods are not thread safe.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "simulator_device_info.h"

#include <string>

/**
 * @brief Simulated manufacturer.
 */
static std::string g_simulatedManufacturer{ ADUC_DEVICEINFO_MANUFACTURER }; // NOLINT(cert-err58-cpp)

/**
 * @brief Simulated model.
 */
static std::string g_simulatedModel{ ADUC_DEVICEINFO_MODEL }; // NOLINT(cert-err58-cpp)

/**
 * @brief Simulated software version.
 */
static std::string g_simulatedSwVersion{ "0.0.0.0" }; // NOLINT(cert-err58-cpp)

/**
 * @brief Set the simulated manufacturer name.
 *
 * @param manufacturer Manufacturer name.
 */
void Simulator_DeviceInfo_SetManufacturer(const char* manufacturer)
{
    try
    {
        g_simulatedManufacturer = manufacturer;
    }
    catch (...)
    {
    }
}

/**
 * @brief Get the simulated manufacturer name.
 *
 * @return const char* Manufacturer name.
 */
const char* Simulator_DeviceInfo_GetManufacturer()
{
    return g_simulatedManufacturer.c_str();
}

/**
 * @brief Set the simulated model name.
 *
 * @param model Model name.
 */
void Simulator_DeviceInfo_SetModel(const char* model)
{
    try
    {
        g_simulatedModel = model;
    }
    catch (...)
    {
    }
}

/**
 * @brief Get the simulated model name.
 *
 * @return const char* Model name.
 */
const char* Simulator_DeviceInfo_GetModel()
{
    return g_simulatedModel.c_str();
}

/**
 * @brief Set the simulated software version.
 *
 * @param version Software version.
 */
void Simulator_DeviceInfo_SetSwVersion(const char* version)
{
    try
    {
        g_simulatedSwVersion = version;
    }
    catch (...)
    {
    }
}

/**
 * @brief Get the simulated software version.
 *
 * @return const char* Software version.
 */
const char* Simulator_DeviceInfo_GetSwVersion()
{
    return g_simulatedSwVersion.c_str();
}
