/**
 * @file adu_agent_info_management.c
 * @brief Implementation for Device Update agent info management functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_agent_info_management.h"
#include "aduc/adu_mosquitto_utils.h"
#include "aduc/adu_mqtt_protocol.h"
#include "aduc/adu_types.h"
#include "aduc/logging.h"
#include "du_agent_sdk/agent_module_interface.h"

#include <aducpal/time.h> // time_t
#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h> // mosquitto_property

/**
 * @brief This fuction handles the 'ainfo_resp' message from the broker. The function will create a copy of the
 * message and process in the net do_work cycle.
*/
void OnMessage_ainfo_resp(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    Log_Info("OnMessage_ainfo_resp: Received ainfo_resp message.");

    // TODO (nox-msft) - check the correlation ID and validate the message. Update the next attempt timestamp as needed.
}

void OnPublished_ainfo_req(
    struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* props)
{
    Log_Info("on_publish: Message with mid %d has been published.", mid);
}

bool ADUC_Agent_Info_Management_Initialize()
{
    return true;
}

void ADUC_Agent_Info_Management_Deinitialize()
{
}

/**
 * @brief This function waits for the client to verify that it is enrolled with the Device Update service.
 * Once the client is enrolled, it will publish an 'ainfo_req' message to the service.
 * The message will contains the following information:
 * - aiv: The agent information version. This is a timestamp in seconds.
 * - aduProtocolVersion: The ADU protocol version. This is a name and version number.
 * - compatProperties: A list of properties that the client supports.
 *
 * Once the message is published, the function will wait for the acknowledgement message ('ainfo_resp').
 * The acknowledgement message will contain the update information.
 *
 * This function will validate the update data and hand it off to the agent workflow util.
 *
 * @return true if the there's no error, false otherwise.
 *
 */
bool ADUC_Agent_Info_Management_DoWork()
{
    // TODO (nox-msft) - implement the update management workflow.

    return false;
}
