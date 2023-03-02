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
#include <aduc/config_utils.h>
#include <parson.h>
#include <stdbool.h>

EXTERN_C_BEGIN

/**
 * @brief Adds the deviceProperties to the @p startupObj
 * @param startupObj the JSON Object which will have the device properties added to it
 * @param agent the ADUC_AgentInfo that contains the agent info
 * @returns true on successful addition, false on failure
 */
bool StartupMsg_AddDeviceProperties(JSON_Object* startupObj, const ADUC_AgentInfo* agent);

/**
 * @brief Adds the compatPropertyNames to the @p startupObj
 * @param startupObj the JSON Object which will have the compatPropertyNames from config added to it
 * @param config the ADUC_ConfigInfo that contains the config info
 * @returns true on successful addition, false on failure
 */
bool StartupMsg_AddCompatPropertyNames(JSON_Object* startupObj, ADUC_ConfigInfo* config);

EXTERN_C_END
#endif // STARTUP_MSG_HELPER_H
