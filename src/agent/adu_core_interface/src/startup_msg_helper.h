/**
 * @file startup_msg_helper.h
 * @brief Implements helper functions for building the startup message
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef STARTUP_MSG_HELPER_H
#define STARTUP_MSG_HELPER_H
#include <aduc/c_utils.h>
#include <parson.h>
#include <stdbool.h>

EXTERN_C_BEGIN

/**
 * @brief Adds the deviceProperties to the @p startupObj
 * @param startupObj the JSON Object which will have the device properties added to it
 * @returns true on successful addition, false on failure
 */
_Bool StartupMsg_AddDeviceProperties(JSON_Object* startupObj);

/**
 * @brief Adds the compatPropertyNames to the @p startupObj
 * @param startupObj the JSON Object which will have the compatPropertyNames from config added to it
 * @returns true on successful addition, false on failure
 */
_Bool StartupMsg_AddCompatPropertyNames(JSON_Object* startupObj);

EXTERN_C_END
#endif // STARTUP_MSG_HELPER_H
