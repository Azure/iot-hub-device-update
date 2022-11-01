/**
 * @file component_enumerator_extension.hpp
 * @brief The header for component enumerator extension
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef _COMPONENT_ENUMERATOR_EXTENSION_HPP_
#define _COMPONENT_ENUMERATOR_EXTENSION_HPP_

#include <aduc/c_utils.h>
#include <string>
#include <vector>

EXTERN_C_BEGIN

/**
 * @brief Select component(s) that contain property or properties matching specified in @p selectorJson string.
 *
 * Example input json:
 *      - Select all components belong to a 'Motors' group
 *              "{\"group\":\"Motors\"}"
 *
 *      - Select a component with name equals 'left-motor'
 *              "{\"name\":\"left-motor\"}"
 *
 *      - Select components matching specified class (manufature/model)
 *              "{\"manufacturer\":\"Contoso\",\"model\":\"USB-Motor-0001\"}"
 *
 * @param selectorJson A stringifed json containing one or more properties use for components selection.
 * @return Returns a serialized json data containing components information.
 * Caller must call FreeString function when done with the returned string.
 *
 * Example output:
 *
 * {
 *   "components": [
 *       {
 *           "id": "contoso-motor-serial-00001",
 *           "name": "left-motor",
 *           "group": "motors",
 *           "manufacturer": "contoso",
 *           "model": "virtual-motor",
 *           "properties": {
 *               "path": "\/tmp\/contoso-devices\/vacuum-1\/motors\/contoso-motor-serial-00001",
 *               "firmwareDataFile": "firmware.json",
 *               "status": "unknown"
 *           }
 *       },
 *       {
 *           "id": "contoso-motor-serial-00002",
 *           "name": "right-motor",
 *           "group": "motors",
 *           "manufacturer": "contoso",
 *           "model": "virtual-motor",
 *           "properties": {
 *               "path": "\/tmp\/contoso-devices\/vacuum-1\/motors\/contoso-motor-serial-00002",
 *               "firmwareDataFile": "firmware.json",
 *               "status": "unknown"
 *           }
 *       },
 *       {
 *           "id": "contoso-motor-serial-00003",
 *           "name": "vacuum-motor",
 *           "group": "motors",
 *           "manufacturer": "contoso",
 *           "model": "virtual-motor",
 *           "properties": {
 *               "path": "\/tmp\/contoso-devices\/vacuum-1\/motors\/contoso-motor-serial-00003",
 *               "firmwareDataFile": "firmware.json",
 *               "status": "unknown"
 *           }
 *       }
 *   ]
 * }
 *
 */
typedef char* (*SelectComponentsProc)(const char* selector);

/**
 * @brief Returns all components information in JSON format.
 * @return Returns a serialized json data containing components information.
 * Caller must call FreeString function when done with the returned string.
 *
 * Example output:
 *
 * {
 *   "components": [
 *       {
 *           "id": "contoso-motor-serial-00001",
 *           "name": "left-motor",
 *           "group": "motors",
 *           "manufacturer": "contoso",
 *           "model": "virtual-motor",
 *           "properties": {
 *               "path": "\/tmp\/contoso-devices\/vacuum-1\/motors\/contoso-motor-serial-00001",
 *               "firmwareDataFile": "firmware.json",
 *               "status": "unknown"
 *           }
 *       },
 *       {
 *           "id": "contoso-motor-serial-00002",
 *           "name": "right-motor",
 *           "group": "motors",
 *           "manufacturer": "contoso",
 *           "model": "virtual-motor",
 *           "properties": {
 *               "path": "\/tmp\/contoso-devices\/vacuum-1\/motors\/contoso-motor-serial-00002",
 *               "firmwareDataFile": "firmware.json",
 *               "status": "unknown"
 *           }
 *       },
 *       {
 *           "id": "contoso-motor-serial-00003",
 *           "name": "vacuum-motor",
 *           "group": "motors",
 *           "manufacturer": "contoso",
 *           "model": "virtual-motor",
 *           "properties": {
 *               "path": "\/tmp\/contoso-devices\/vacuum-1\/motors\/contoso-motor-serial-00003",
 *               "firmwareDataFile": "firmware.json",
 *               "status": "unknown"
 *           }
 *       }
 *   ]
 * }
 *
 */
typedef char* (*GetAllComponentsProc)();

/**
 * @brief Free string buffer previously returned by Component Enumerator APIs.
 * @param string A pointer to string to be freed.
 */
typedef void (*FreeComponentsDataStringProc)(char* string);

EXTERN_C_END

#endif // _COMPONENT_ENUMERATOR_EXTENSION_HPP_
