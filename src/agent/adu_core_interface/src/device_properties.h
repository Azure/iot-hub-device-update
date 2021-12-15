/**
 * @file device_properties.h
 * @brief The header for device properties functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef __DEVICE_PROPERTIES_H__
#define __DEVICE_PROPERTIES_H__

#include <aduc/config_utils.h>

/**
 * @brief Adds the aduc_manufacturer and aduc_model to the @p devicePropsObj
 * @param devicePropsObj the JSON_Object the manufacturer and model will be added to
 * @returns true on successful addition and false on failure
 */
_Bool DeviceProperties_AddManufacturerAndModel(JSON_Object* devicePropsObj);

/**
 * @brief Adds the interfaceId property into the @p devicePropsObj
 * @param devicePropsObj the JSON_Object the interfaceId will be added to
 * @returns true on successful addition and false on failure
 */
_Bool DeviceProperties_AddInterfaceId(JSON_Object* devicePropsObj);

/**
 * @brief Adds the version properties into the @p devicePropsObj
 * @param devicePropsObj the JSON_Object the versions will be added to
 * @returns true on successful addition and false on failure
 */
_Bool DeviceProperties_AddVersions(JSON_Object* devicePropsObj);

#endif // __DEVICE_PROPERTIES_H__
