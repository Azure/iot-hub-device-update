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
 * @param agent the ADUC_AgentInfo that contains the agent info
 * @returns true on successful addition and false on failure
 */
_Bool DeviceProperties_AddManufacturerAndModel(JSON_Object* devicePropsObj, const ADUC_AgentInfo* agent);

/**
 * @brief Adds the customized additional device properties to the @p devicePropsObj
 * @param devicePropsObj the JSON_Object the additional device properties will be added to
 * @param agent the ADUC_AgentInfo that contains the agent info
 * @returns true on successful addition and false on failure
 */
_Bool DeviceProperties_AddAdditionalProperties(JSON_Object* devicePropsObj, const ADUC_AgentInfo* agent);

/**
 * @brief Clears the interfaceId property from the @p devicePropsObj
 * @param devicePropsObj the JSON_Object the interfaceId will be set to 'null' to remove any
 *      existing value set by DU Agents older than version 1.0.
 * @returns true on successful addition and false on failure
 */
_Bool DeviceProperties_ClearInterfaceId(JSON_Object* devicePropsObj);

/**
 * @brief Adds the contractModelId property into the @p devicePropsObj
 * @param devicePropsObj the JSON_Object the contractModelId will be added to
 * @returns true on successful addition and false on failure
 */
_Bool DeviceProperties_AddContractModelId(JSON_Object* devicePropsObj);

/**
 * @brief Adds the version properties into the @p devicePropsObj
 * @param devicePropsObj the JSON_Object the versions will be added to
 * @returns true on successful addition and false on failure
 */
_Bool DeviceProperties_AddVersions(JSON_Object* devicePropsObj);

#endif // __DEVICE_PROPERTIES_H__
