/**
 * @file simulator_device_info.h
 * @brief Methods to set and return simulated device info values.
 *
 * These methods are not thread safe.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef SIMULATOR_DEVICE_INFO_H
#define SIMULATOR_DEVICE_INFO_H

void Simulator_DeviceInfo_SetManufacturer(const char* manufacturer);
const char* Simulator_DeviceInfo_GetManufacturer();

void Simulator_DeviceInfo_SetModel(const char* model);
const char* Simulator_DeviceInfo_GetModel();

void Simulator_DeviceInfo_SetSwVersion(const char* version);
const char* Simulator_DeviceInfo_GetSwVersion();

#endif // SIMULATOR_DEVICE_INFO_H
