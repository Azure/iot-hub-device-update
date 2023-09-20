/**
 * @file adu_agent_info_management.h
 * @brief Header file for Device Update agent info management functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef __ADUC_AGENT_INFO_MANAGEMENT_H__
#define __ADUC_AGENT_INFO_MANAGEMENT_H__

#include "aduc/c_utils.h" // EXTERN_C_* macros
#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h> // mosquitto_property

EXTERN_C_BEGIN

/**
 * @brief Initialize the agent info management.
 */
bool ADUC_Agent_Info_Management_Initialize();

/**
 * @brief Deinitialize the agent info management.
 */
void ADUC_Agent_Info_Management_Deinitialize();

/**
 * @brief Ensure that the agent info is published to the Device Update service.
 */
bool ADUC_Agent_Info_Management_DoWork();

/**
 * @brief Callback should be called when the client receives an agent info response message from the Device Update service.
 */
void OnMessage_ainfo_resp(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props);

/**
 * @brief Callback that called when the client sent an agent info request message to the Device Update service.
*/
void OnPublished_ainfo_req(
    struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* props);

EXTERN_C_END

#endif // __ADUC_AGENT_INFO_MANAGEMENT_H__
