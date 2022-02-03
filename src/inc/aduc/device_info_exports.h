/**
 * @file device_info_exports.h
 * @brief Describes methods to be exported from platform-specific ADUC agent code for the device information interface.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_DEVICE_INFO_EXPORTS_H
#define ADUC_DEVICE_INFO_EXPORTS_H

#include <aduc/c_utils.h>

EXTERN_C_BEGIN

/**
 * @brief Enumeration containing the device information fields that can be queried.
 */
typedef enum tagDI_DeviceInfoProperty
{
    DIIP_Manufacturer,
    DIIP_Model,
    DIIP_OsName,
    DIIP_ProcessorArchitecture,
    DIIP_ProcessorManufacturer,
    DIIP_SoftwareVersion,
    DIIP_TotalMemory,
    DIIP_TotalStorage,
} DI_DeviceInfoProperty;

//
// Exported methods.
//

/**
 * @brief Return a specific device information value.
 *
 * @param property Property to retrieve
 * @return char* Value of property allocated with malloc, or NULL on error or value not changed since last call.
 */
char* DI_GetDeviceInformationValue(DI_DeviceInfoProperty property);

EXTERN_C_END

#endif // ADUC_DEVICE_INFO_EXPORTS_H
