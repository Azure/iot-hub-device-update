/**
 * @file adu_agent_info_management.c
 * @brief Implementation for device agentinfo status management.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_agent_info_management.h"

#include <aduc/adu_agentinfo.h>
#include <aduc/adu_agentinfo_utils.h>
#include <aduc/adu_communication_channel.h>
#include <aduc/adu_module_state.h>
#include <aduc/adu_mosquitto_utils.h>
#include <aduc/adu_mqtt_common.h>
#include <aduc/adu_mqtt_protocol.h>
#include <aduc/adu_types.h>
#include <aduc/agent_module_interface_internal.h>
#include <aduc/agent_state_store.h>
#include <aduc/agentinfo_request_operation.h>
#include <aduc/config_utils.h>
#include <aduc/logging.h>
#include <aduc/retry_utils.h> // ADUC_GetTimeSinceEpochInSeconds
#include <aduc/string_c_utils.h> // IsNullOrEmpty
#include <aduc/topic_mgmt_lifecycle.h> // TopicMgmtLifecycle_UninitRetriableOperationContext
#include <aducpal/time.h> // time_t

#include <parson.h> // JSON_Value, JSON_Object
#include <parson_json_utils.h> // ADUC_JSON_Get*

#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h>
#include <string.h>

// keep this last to avoid interfering with system headers
#include <aduc/aduc_banned.h>

/////////////////////////////////////////////////////////////////////////////
//
// BEGIN - ADU_AGENT_INFO_MANAGEMENT.H Public Interface
//

/**
 * @brief Destroy the module handle.
 *
 * @param handle The module handle.
 */
void ADUC_AgentInfo_Management_Destroy(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_Retriable_Operation_Context* operationContext = OperationContextFromAgentModuleHandle(handle);

    if (operationContext == NULL)
    {
        Log_Error("Failed to get operation context");
        return;
    }

    TopicMgmtLifecycle_UninitRetriableOperationContext(operationContext);
    free(operationContext);
}

/**
 * @brief Callback should be called when the client receives an agentinfo status response message from the Device Update service.
 *  For 'ainfo_resp' messages, if the Correlation Data matches, then the client should parse the message and update the agentinfo state.
 *
 * @param mosq The mosquitto instance making the callback.
 * @param obj The user data provided in mosquitto_new. This is the module state
 * @param msg The message data.
 * @param props The MQTT v5 properties returned with the message.
 *
 * @remark This callback is called by the network thread. Usually by the same thread that calls mosquitto_loop function.
 * IMPORTANT - Do not use blocking functions in this callback.
 */
void OnMessage_ainfo_resp(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    char* correlationData = NULL;
    uint16_t correlationDataByteLen = 0;
    ADUC_Retriable_Operation_Context* retriableOperationContext =
        RetriableOperationContextFromAgentInfoMqttLibCallbackUserObj(obj);
    ADUC_AgentInfo_Request_Operation_Data* agentInfoData =
        AgentInfoDataFromRetriableOperationContext(retriableOperationContext);

    ADUC_MQTT_Message_Context* messageContext = agentInfoData == NULL ? NULL : &agentInfoData->ainfoReqMessageContext;

    if (retriableOperationContext == NULL || agentInfoData == NULL || messageContext == NULL)
    {
        Log_Error("Invalid callback obj");
        goto done;
    }

    json_print_properties(props);

    if (!ADU_are_correlation_ids_matching(
            props, messageContext->correlationId, &correlationData, &correlationDataByteLen))
    {
        Log_Info(
            "correlation data mismatch. expected: '%s', actual: '%s' %u bytes",
            correlationData,
            correlationDataByteLen);
        goto done;
    }

    if (msg == NULL || msg->payload == NULL || msg->payloadlen == 0)
    {
        Log_Error("Bad payload. msg:%p, payload:%p, payloadlen:%d", msg, msg->payload, msg->payloadlen);
        goto done;
    }

    if (!ParseAndValidateCommonResponseUserProperties(
            props, "ainfo_resp" /* expectedMsgType */, &agentInfoData->respUserProps))
    {
        Log_Error("Fail parse of common user props");
        goto done;
    }

    // NOTE: Do no parse ainfo_resp msg payload as it is empty as per client/server protocol design.

    if (!Handle_AgentInfo_Response(agentInfoData, retriableOperationContext))
    {
        Log_Error("Fail handling agentinfo response: correlationId '%s'", messageContext->correlationId);
        goto done;
    }

done:
    free(correlationData);
    return;
}

void OnPublish_ainfo_resp(struct mosquitto* mosq, void* obj, const mosquitto_property* props, int reason_code)
{
    ADUC_Retriable_Operation_Context* operationContext = RetriableOperationContextFromAgentInfoMqttLibCallbackUserObj(obj);
    ADUC_AgentInfo_Request_Operation_Data* agentInfoData = AgentInfoDataFromRetriableOperationContext(operationContext);
    ADUC_MQTT_Common_HandlePublishAck(mosq, obj, props, reason_code, operationContext, agentInfoData->ainfoReqMessageContext.correlationId);
}

//
// END - ADU_AGENT_INFO_MANAGEMENT.H Public Interface
//
/////////////////////////////////////////////////////////////////////////////
