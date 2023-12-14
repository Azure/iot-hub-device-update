#include "aduc/adu_agentinfo_utils.h"
#include <aduc/adu_module_state.h> // ADUC_MQTT_CLIENT_MODULE_STATE
#include <aduc/adu_mqtt_protocol.h> // ADU_RESPONSE_MESSAGE_RESULT_CODE_*
#include <aduc/agent_state_store.h>
#include <aduc/logging.h>
#include <aduc/mqtt_broker_common.h> // ADUC_MQTT_Message_Context
#include <aduc/string_c_utils.h> // ADUC_Safe_StrCopyN
#include <string.h> // strlen

// keep this last to avoid interfering with system headers
#include <aduc/aduc_banned.h>

#define AGENT_INFO_PROTOCOL_NAME "adu"
#define AGENT_INFO_PROTOCOL_VERSION 1

/**
 * @brief Returns str representation of the agentinfo state enum
 * @param st The agentinfo state.
 * @return const char* State as a const string literal.
 */
const char* agentinfo_state_str(ADU_AGENTINFO_STATE st)
{
    switch (st)
    {
    case ADU_AGENTINFO_STATE_NOT_ACKNOWLEDGED:
        return "ADU_AGENTINFO_STATE_NOT_ACKNOWLEDGED";

    case ADU_AGENTINFO_STATE_UNKNOWN:
        return "ADU_AGENTINFO_STATE_UNKNOWN";

    case ADU_AGENTINFO_STATE_REQUESTING:
        return "ADU_AGENTINFO_STATE_REQUESTING";

    case ADU_AGENTINFO_STATE_ACKNOWLEDGED:
        return "ADU_AGENTINFO_STATE_ACKNOWLEDGED";

    default:
        return "???";
    }
}

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
 * @brief Gets the retriable operation context from the AgentInfo mqtt callback user object.
 * @param obj The callback's user object.
 * @return ADUC_Retriable_Operation_Context* The retriable operation context.
 */
ADUC_Retriable_Operation_Context* RetriableOperationContextFromAgentInfoMqttLibCallbackUserObj(void* obj)
{
    if (obj == NULL)
    {
        Log_Error("Null obj");
        return NULL;
    }

    ADUC_MQTT_CLIENT_MODULE_STATE* ownerModuleState = (ADUC_MQTT_CLIENT_MODULE_STATE*)obj;
    ADUC_AGENT_MODULE_INTERFACE* agentInfoModuleInterface =
        (ADUC_AGENT_MODULE_INTERFACE*)ownerModuleState->agentInfoModule;

    if (agentInfoModuleInterface == NULL)
    {
        Log_Error("Null obj");
        return NULL;
    }

    return (ADUC_Retriable_Operation_Context*)(agentInfoModuleInterface->moduleData);
}

/**
 * @brief Gets the AgentInfo Request Operation Data from RetriableOperatioContext.
 * @param retriableOperationContext The retriable operation context.
 * @return ADUC_Enrollment_Request_Operation_Data* The agent info request operation data.
 */
ADUC_AgentInfo_Request_Operation_Data*
AgentInfoDataFromRetriableOperationContext(ADUC_Retriable_Operation_Context* retriableOperationContext)
{
    if (retriableOperationContext == NULL)
    {
        return NULL;
    }

    return (ADUC_AgentInfo_Request_Operation_Data*)(retriableOperationContext->data);
}

/**
 * @brief Sets the correlation id for the agent info request message.
 * @param agentInfoData The agent info data object.
 * @param correlationId The correlation id to set.
 * @return true on success.
 */
bool AgentInfoData_SetCorrelationId(ADUC_AgentInfo_Request_Operation_Data* agentInfoData, const char* correlationId)
{
    if (agentInfoData == NULL || correlationId == NULL)
    {
        return false;
    }

    if (!ADUC_Safe_StrCopyN(
        agentInfoData->ainfoReqMessageContext.correlationId,
        ARRAY_SIZE(agentInfoData->ainfoReqMessageContext.correlationId),
        correlationId,
        strlen(correlationId)))
    {
        Log_Error("copy failed");
        return false;
    }

    return true;
}

/**
 * @brief Handles creating side-effects in response to incoming agentinfo response data from agent info response.
 *
 * @param responseSuccess Whether the agentinfo response had a success result code.
 * @return true on success.
 */

bool Handle_AgentInfo_Response(
    ADUC_AgentInfo_Request_Operation_Data* agentInfoData, ADUC_Retriable_Operation_Context* context)
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
            Log_Error("ainfo_resp - Bad Request(1), erc: 0x%08x", user_props->extendedresultcode);
            Log_Info("ainfo_resp bad request from agent. Cancelling...");
            context->cancelFunc(context);
            break;

        case ADU_RESPONSE_MESSAGE_RESULT_CODE_BUSY:
            Log_Error("ainfo_resp - Busy(2), erc: 0x%08x", user_props->extendedresultcode);
            break;

        case ADU_RESPONSE_MESSAGE_RESULT_CODE_CONFLICT:
            Log_Error("ainfo_resp - Conflict(3), erc: 0x%08x", user_props->extendedresultcode);
            break;

        case ADU_RESPONSE_MESSAGE_RESULT_CODE_SERVER_ERROR:
            Log_Error("ainfo_resp - Server Error(4), erc: 0x%08x", user_props->extendedresultcode);
            break;

        case ADU_RESPONSE_MESSAGE_RESULT_CODE_AGENT_NOT_ENROLLED:
            Log_Error("ainfo_resp - Agent Not Enrolled(5), erc: 0x%08x", user_props->extendedresultcode);
            break;

        default:
            Log_Error(
                "ainfo_resp - Unknown Error: %d, erc: 0x%08x", user_props->resultcode, user_props->extendedresultcode);
            break;
        }

        Log_Info("ainfo_resp error. Retrying...");
        Log_Info(
            "Transition from '%s' to '%s'",
            agentinfo_state_str(agentInfoData->agentInfoState),
            agentinfo_state_str(ADU_AGENTINFO_STATE_UNKNOWN));
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

    Log_Info(
        "Transition from '%s' to '%s'",
        agentinfo_state_str(agentInfoData->agentInfoState),
        agentinfo_state_str(ADU_AGENTINFO_STATE_ACKNOWLEDGED));
    agentInfoData->agentInfoState = ADU_AGENTINFO_STATE_ACKNOWLEDGED;
    ADUC_Retriable_Set_State(context, ADUC_Retriable_Operation_State_Completed);
    succeeded = true;
done:

    return succeeded;
}
