/**
 * @file adu_agent_info_management.c
 * @brief Implementation for Device Update agent info management functions.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_agent_info_management.h"

#include <aduc/adu_agent_info_management_internal.h> // ADUC_AGENT_INFO_MANAGEMENT_STATE
#include <aduc/adu_agentinfo.h> // ADUC_AgentInfo_Request_Operation_Data
#include <aduc/adu_agentinfo_utils.h>
#include <aduc/adu_mqtt_protocol.h> // SUBSCRIBE_TOPIC_TEMPLATE_ADU_OTO_WITH_DU_INSTANCE
#include <aduc/agent_state_store.h> // ADUC_StateStore_*
#include <aduc/config_utils.h> // ADUC_ConfigInfo
#include <aduc/logging.h>
#include <aduc/mqtt_broker_common.h> // ADUC_MQTT_Message_Context
#include <aduc/retry_utils.h> // ADUC_Retriable_Operation_Context
#include <aduc/string_c_utils.h> // IsNullOrEmpty
#include <aduc/types/update_content.h> // ADUCITF_FIELDNAME_DEVICEPROPERTIES_*
#include <du_agent_sdk/agent_module_interface.h> // ADUC_AGENT_MODULE_HANDLE
#include <parson.h> // JSON_Value

static ADUC_AGENT_INFO_MANAGEMENT_STATE s_agentInfoMgrState = { 0 };

/**
 * @brief This function ensures that the 'ainfo_req' topic template is initialized and the topic is subscribed.
 * @return bool true on success.
 */
static bool EnsureResponseTopicSubscribed()
{
    bool success = false;
    if (s_agentInfoMgrState.workflowState >= ADUC_AGENT_INFO_WORKFLOW_STATE_SUBSCRIBED)
    {
        success = true;
        goto done;
    }

    time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();
    if (nowTime < s_agentInfoMgrState.ainfo_req_NextAttemptTime)
    {
        // Wait for the next attempt time.
        goto done;
    }

    if (s_agentInfoMgrState.workflowState == ADUC_AGENT_INFO_WORKFLOW_STATE_INITIALIZED)
    {
        if (IsNullOrEmpty(s_agentInfoMgrState.subscribeTopic))
        {
            const char* duInstance = ADUC_StateStore_GetDeviceUpdateServiceInstance();
            if (IsNullOrEmpty(duInstance))
            {
                Log_Error("Invalid state. DU instance is NULL.");
                // UpdateNextAttemptTime(0 /* error code */);
                goto done;
            }

            const char* externalDeviceId = ADUC_StateStore_GetExternalDeviceId();
            s_agentInfoMgrState.subscribeTopic =
                ADUC_StringFormat(SUBSCRIBE_TOPIC_TEMPLATE_ADU_OTO_WITH_DU_INSTANCE, externalDeviceId, duInstance);

            if (IsNullOrEmpty(s_agentInfoMgrState.subscribeTopic))
            {
                Log_Error("Failed to format the subscribe topic.");
                // UpdateNextAttemptTime(0 /* error code */);
                goto done;
            }
        }

        ADUC_StateStore_GetCommunicationChannelHandle(
            &s_agentInfoMgrState.publishTopic, &s_agentInfoMgrState.publishTopic);

        if (ADUC_MQTT_Subscribe(s_agentInfoMgrState.subscribeTopic, OnMessage_ainfo_resp))

            // TODO: subscribe to response topic.
            // if (s_agentInfoMgrState.subscribeTopic, OnMessage_ainfo_resp))
            // {
            //     Log_Error("Failed to subscribe to the topic: %s", s_agentInfoMgrState.subscribeTopic);
            //     return false;
            // }

            s_agentInfoMgrState.workflowState = ADUC_AGENT_INFO_WORKFLOW_STATE_SUBSCRIBING;
            goto done;
    }

    if (s_agentInfoMgrState.workflowState == ADUC_AGENT_INFO_WORKFLOW_STATE_SUBSCRIBING)
    {
    }

done:
    return success;
}

/**
 * @brief Ensures that the agent info request is published.
 * @return bool true on success.
 */
static bool EnsureAgentInfoRequestPublished()
{

}

/**
 * @brief Gets the extension contract info.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 * @return ADUC_AGENT_CONTRACT_INFO The extension contract info.
 */
const ADUC_AGENT_CONTRACT_INFO* ADUC_AgentInfo_Management_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle)
{
    static ADUC_AGENT_CONTRACT_INFO s_moduleContractInfo = {
        "Microsoft", "Device Update Agent Info Module", 1, "Microsoft/DUAgentInfoModule:1"
    };

    IGNORED_PARAMETER(handle);

    return &s_moduleContractInfo;
}

/**
 * @brief Initialize the agent info management.
 * @param handle The agent module handle.
 * @param initData The initialization data.
 * @return 0 on successful initialization; -1, otherwise.
 */
int ADUC_AgentInfo_Management_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* initData)
{
    IGNORED_PARAMETER(initData);
    ADUC_Retriable_Operation_Context* context = OperationContextFromAgentModuleHandle(handle);
    if (context == NULL)
    {
        Log_Error("Failed to get operation context");
        return -1;
    }

    return 0;
}

/**
 * @brief Deinitializes the agent info management.
 * @param handle The module handle.
 * @return int 0 on success.
 */
void ADUC_AgentInfo_Management_Deinitialize(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_Retriable_Operation_Context* context = OperationContextFromAgentModuleHandle(handle);
    if (context == NULL)
    {
        Log_Error("Failed to get operation context");
        return false;
    }
    context->cancelFunc(context);
    return 0;
}

/**
 * @brief This function requests
 * @detail Once the client is enrolled, it will publish an 'ainfo_req' message to the service.
 * Once the message is published, the function will wait for the ACK response of agent info ('ainfo_resp').
 * The agent info response will have an empty message, but the resultcode of success (0) will allow agent
 * to proceed to do the update request-response.
 *
 * @return int -1 on error. 0 on success.
 *
 */
int ADUC_AgentInfo_Management_DoWork(ADUC_AGENT_MODULE_HANDLE moduleHandle)
{

    ADUC_Retriable_Operation_Context* context = OperationContextFromAgentModuleHandle(handle);
    if (context == NULL)
    {
        Log_Error("Failed to get operation context");
        return -1;
    }

    context->doWorkFunc(context);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// BEGIN - ADU_AGENT_INFO_MANAGEMENT.H Public Interface
//

/**
 * @brief Create the agent info management module instance.
 * @return ADUC_AGENT_MODULE_HANDLE The module handle.
 */
ADUC_AGENT_MODULE_HANDLE ADUC_AgentInfo_Management_Create()
{
    ADUC_AGENT_MODULE_INTERFACE* result = NULL;
    ADUC_AGENT_MODULE_INTERFACE* interface = NULL;

    ADUC_Retriable_Operation_Context* operationContext = CreateAndInitializeAgentInfoRequestOperation();
    if (operationContext == NULL)
    {
        Log_Error("Failed to create agent info request operation");
        goto done;
    }

    ADUC_ALLOC(interface);

    interface->getContractInfo = ADUC_AgentInfo_Management_GetContractInfo;
    interface->initializeModule = ADUC_AgentInfo_Management_Initialize;
    interface->deinitializeModule = ADUC_AgentInfo_Management_Deinitialize;
    interface->doWork = ADUC_AgentInfo_Management_DoWork;
    interface->destroy = ADUC_AgentInfo_Management_Destroy;
    interface->moduleData = operationContext;
    operationContext = NULL; // transfer ownership

    result = interface;
    interface = NULL; // transfer ownership

done:
    if (operationContext != NULL)
    {
        operationContext->dataDestroyFunc(operationContext);
        operationContext->operationDestroyFunc(operationContext);
        free(operationContext);
    }

    free(interface);

    return (ADUC_AGENT_MODULE_HANDLE) result;
}

/**
 * @brief Destroy the agent info management module instance.
 * @param handle The module handle.
 */
void ADUC_AgentInfo_Management_Destroy(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_Retriable_Operation_Context* context = OperationContextFromAgentModuleHandle(handle);
    if (context == NULL)
    {
        Log_Error("Failed to get operation context");
        return;
    }

    context->operationDestroyFunc(context);
    free(context);
}

/**
 * @brief On receive the "ainfo_resp" message type.
 *
 * @param mosq The mosquitto handle.
 * @param obj The context obj.
 * @param msg The mosquitto message.
 * @param props The mosquitto props.
 */
void OnMessage_ainfo_resp(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    int resultCode = 0;

    ADUC_Retriable_Operation_Context* context = (ADUC_Retriable_Operation_Context*)obj;
    ADUC_MQTT_Message_Context* messageContext = (ADUC_MQTT_Message_Context*)context->data;
    ADUC_AgentInfo_Request_Operation_Data* agentInfoData = AgentInfoData_FromOperationContext(context);
    char* msg_type = NULL;

    if (!ADU_are_correlation_ids_matching(props, messageContext->correlationId))
    {
        Log_Info("OnMessage_enr_resp: correlation data mismatch");
        goto done;
    }

    // Note: omit parsing message payload json since it's should be empty per design.

    if (!ParseCommonResponseUserProperties(props, &agentInfoData->respUserProps, agentInfoData))
    {
        Log_Error("Fail parse of common user props");
        goto done;
    }

    if (!Handle_AgentInfo_Response(context, true /* acknowledged */))
    {
        Log_Error("Failed handle agent info resp");
        goto done;
    }

done:
    free(msg_type);
}

//
// END - ADU_AGENT_INFO_MANAGEMENT.H Public Interface
//
/////////////////////////////////////////////////////////////////////////////
