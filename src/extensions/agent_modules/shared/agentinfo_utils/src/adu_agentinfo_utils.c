#include "aduc/adu_agentinfo_utils.h"
#include <aduc/agent_state_store.h>
#include <aduc/mqtt_broker_common.h> // ADUC_MQTT_Message_Context

// clang-format:off
#include <aduc/aduc_banned.h> // make sure this is last to avoid affecting system includes
// clang-format:on

#define AGENT_INFO_FIELD_NAME_SEQUENCE_NUMBER "sn"
#define AGENT_INFO_FIELD_NAME_COMPAT_PROPERTIES "compatProperties"
#define AGENT_INFO_PROTOCOL_NAME "adu"
#define AGENT_INFO_PROTOCOL_VERSION 1

/**
 * @brief Handles creating side-effects in response to incoming agentinfo response data from agent info response.
 *
 * @param responseSuccess Whether the agentinfo response had a success result code.
 * @return true on success.
 */

bool Handle_AgentInfo_Response(ADUC_MQTT_Message_Context* context, int resultCode)
{

    return ADUC_StateStore_SetIsAgentInfoReported(200 == resultCode);
}
