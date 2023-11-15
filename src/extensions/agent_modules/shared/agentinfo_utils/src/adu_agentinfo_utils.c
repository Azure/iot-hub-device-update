#include "aduc/adu_agentinfo_utils.h"
#include <aduc/adu_mqtt_protocol.h> // ADU_RESPONSE_MESSAGE_RESULT_CODE_*
#include <aduc/agent_state_store.h>
#include <aduc/logging.h>
#include <aduc/mqtt_broker_common.h> // ADUC_MQTT_Message_Context
#include <aduc/string_c_utils.h> // ADUC_Safe_StrCopyN
#include <string.h> // strlen

// clang-format off
#include <aduc/aduc_banned.h> // make sure this is last to avoid affecting system includes
// clang-format on

#define AGENT_INFO_FIELD_NAME_SEQUENCE_NUMBER "sn"
#define AGENT_INFO_FIELD_NAME_COMPAT_PROPERTIES "compatProperties"
#define AGENT_INFO_PROTOCOL_NAME "adu"
#define AGENT_INFO_PROTOCOL_VERSION 1

/**
 * @brief Gets the agent info data object from the agent info operation context.
 * @param context The agent info operation context.
 * @return ADUC_AgentInfo_Request_Operation_Data* The agent info data object.
 */
ADUC_AgentInfo_Request_Operation_Data* AgentInfoData_FromOperationContext(ADUC_Retriable_Operation_Context* context)
{
    if (context == NULL || context->data == NULL)
    {
        Log_Error("Null input (context:%p, data:%p)", context, context->data);
        return NULL;
    }

    return (ADUC_AgentInfo_Request_Operation_Data*)(context->data);
}

/**
 * @brief Sets the correlation id for the agent info request message.
 * @param agentInfoData The agent info data object.
 * @param correlationId The correlation id to set.
 */
void AgentInfoData_SetCorrelationId(ADUC_AgentInfo_Request_Operation_Data* agentInfoData, const char* correlationId)
{
    if (agentInfoData == NULL || correlationId == NULL)
    {
        return;
    }

    ADUC_Safe_StrCopyN(
        agentInfoData->ainfoReqMessageContext.correlationId,
        correlationId,
        sizeof(agentInfoData->ainfoReqMessageContext.correlationId) - 1,
        strlen(correlationId));
}

/**
 * @brief Handles creating side-effects in response to incoming agentinfo response data from agent info response.
 *
 * @param responseSuccess Whether the agentinfo response had a success result code.
 * @return true on success.
 */

bool Handle_AgentInfo_Response(ADUC_AgentInfo_Request_Operation_Data* agentInfoData, ADUC_Retriable_Operation_Context* context)
{
    bool succeeded = false;
    const char Fail_Save_StateStore_StrFmt[] = "Fail saving isAgentInfoReported '%s' in state store";

    if (agentInfoData == NULL || context == NULL)
    {
        return false;
    }

    const ADUC_Common_Response_User_Properties* user_props = &agentInfoData->respUserProps;
    if (user_props->resultcode != ADU_RESPONSE_MESSAGE_RESULT_CODE_SUCCESS)
    {
        switch (user_props->resultcode)
        {
        case ADU_RESPONSE_MESSAGE_RESULT_CODE_BAD_REQUEST:
            Log_Error("enr_resp - Bad Request(1), erc: 0x%08x", user_props->extendedresultcode);
            Log_Info("enr_resp bad request from agent. Cancelling...");
            context->cancelFunc(context);
            break;

        case ADU_RESPONSE_MESSAGE_RESULT_CODE_BUSY:
            Log_Error("enr_resp - Busy(2), erc: 0x%08x", user_props->extendedresultcode);
            break;

        case ADU_RESPONSE_MESSAGE_RESULT_CODE_CONFLICT:
            Log_Error("enr_resp - Conflict(3), erc: 0x%08x", user_props->extendedresultcode);
            break;

        case ADU_RESPONSE_MESSAGE_RESULT_CODE_SERVER_ERROR:
            Log_Error("enr_resp - Server Error(4), erc: 0x%08x", user_props->extendedresultcode);
            break;

        case ADU_RESPONSE_MESSAGE_RESULT_CODE_AGENT_NOT_ENROLLED:
            Log_Error("enr_resp - Agent Not Enrolled(5), erc: 0x%08x", user_props->extendedresultcode);
            break;

        default:
            Log_Error("enr_resp - Unknown Error: %d, erc: 0x%08x", user_props->resultcode, user_props->extendedresultcode);
            break;
        }

        Log_Info("enr_resp error. Retrying...");
        agentInfoData->agentInfoState = ADU_AGENTINFO_STATE_UNKNOWN;

        if (ADUC_STATE_STORE_RESULT_OK != ADUC_StateStore_SetIsAgentInfoReported(false /* isAgentInfoReported */))
        {
            Log_Error(Fail_Save_StateStore_StrFmt, "false");
        }

        goto done;
    }

    if (ADUC_STATE_STORE_RESULT_OK != ADUC_StateStore_SetIsAgentInfoReported(true /* isAgentInfoReported */))
    {
        Log_Error(Fail_Save_StateStore_StrFmt, "true");
        goto done;
    }

    agentInfoData->agentInfoState = ADU_AGENTINFO_STATE_ACKNOWLEDGED;
    succeeded = true;
done:

    return succeeded;
}
