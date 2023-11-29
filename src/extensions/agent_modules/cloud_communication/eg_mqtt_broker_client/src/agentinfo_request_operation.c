
#include "aduc/agentinfo_request_operation.h"

#include <aduc/adu_agentinfo.h> // ADUC_AgentInfo_Request_Operation_Data
#include <aduc/adu_agentinfo_utils.h> // AgentInfoData_FromOperationContext, AgentInfoData_SetCorrelationId
#include <aduc/adu_mosquitto_utils.h> // ADU_mosquitto_add_user_property
#include <aduc/adu_communication_channel.h> // ADUC_DU_SERVICE_COMMUNICATION_CHANNEL_ID
#include <aduc/adu_mqtt_common.h>
#include <aduc/agent_state_store.h> // ADUC_StateStore_GetIsAgentInfoReported
#include <aduc/config_utils.h> // ADUC_ConfigInfo, ADUC_AgentInfo
#include <aduc/logging.h>
#include <du_agent_sdk/agent_module_interface.h>  // ADUC_AGENT_MODULE_HANDLE

/**
 * @brief Free memory allocated for the agentinfo data.
 */
static void AgentInfoData_Deinit(ADUC_AgentInfo_Request_Operation_Data* data)
{
    // Currently nothing to be freed. Implement this for completeness and future improvement.
    IGNORED_PARAMETER(data);
}

static void Set_Agent_State(ADUC_AgentInfo_Request_Operation_Data* agentInfoData, ADU_AGENTINFO_STATE newState)
{
    if (agentInfoData->agentInfoState != newState)
    {
        Log_Info("Transition '%s' -> '%s'", agentinfo_state_str(agentInfoData->agentInfoState), agentinfo_state_str(newState));
        agentInfoData->agentInfoState = newState;
    }
    else
    {
        Log_Warn("'%s' -> '%s'", agentinfo_state_str(agentInfoData->agentInfoState), agentinfo_state_str(agentInfoData->agentInfoState));
    }
}

/**
 * @brief Cancel operation.
 *
 * @param context The retriable context
 * @return true on success.
 */
bool AgentInfoRequestOperation_CancelOperation(ADUC_Retriable_Operation_Context* context)
{
    ADUC_AgentInfo_Request_Operation_Data* agentInfoData = AgentInfoData_FromOperationContext(context);
    if (agentInfoData == NULL)
    {
        return false;
    }

    // Set the correlation id to NULL, so we can discard all inflight messages.
    AgentInfoData_SetCorrelationId(agentInfoData, "");

    Set_Agent_State(agentInfoData, ADU_AGENTINFO_STATE_UNKNOWN);

    return true;
}

/**
 * @brief Request Operation Complete handler.
 *
 * @param context The retriable operation context.
 * @return true on success.
 */
bool AgentInfoRequestOperation_Complete(ADUC_Retriable_Operation_Context* context)
{
    if (context == NULL)
    {
        Log_Error("context NULL");
        return false;
    }

    // TODO: cleanup resources.
    ADUC_Retriable_Set_State(context, ADUC_Retriable_Operation_State_Completed);
    return true;
}

/**
 * @brief This fuction is call when the caller wants to retry the operation. This function should update the
 * internal states of the operation so that the next call to 'DoWork' will retry the operation.
 *
 * For 'ainfo_req' operation, we'll set the agentinfo state to 'unknown', and clear the message's correlation id,
 * which will any incoming 'ainfo_resp' message to be ignored.
 *
 * At next 'DoWork' call, we'll generate a new correlation id and send the new 'ainfo_req' message.
 *
 * @param context The context for the operation.
 *
 * @return true if the operation is successfully retried. false otherwise.
 */
bool AgentInfoRequestOperation_DoRetry(
    ADUC_Retriable_Operation_Context* context, const ADUC_Retry_Params* retryParams)
{
    Log_Debug("retry");
    ADUC_AgentInfo_Request_Operation_Data* agentInfoData = AgentInfoData_FromOperationContext(context);

    if (agentInfoData == NULL)
    {
        return false;
    }

    Set_Agent_State(agentInfoData, ADU_AGENTINFO_STATE_UNKNOWN);

    AgentInfoData_SetCorrelationId(agentInfoData, "");
    return true;
}

void AgentInfoRequestOperation_DataDestroy(ADUC_Retriable_Operation_Context* context)
{
    Log_Debug("Destroy context data");
    if (context != NULL && context->data != NULL)
    {
        // Deinitialize and destroy the associated
        AgentInfoData_Deinit(AgentInfoData_FromOperationContext(context));
        context->data = NULL;

        free(context->operationName);
        context->operationName = NULL;
    }
}

void AgentInfoRequestOperation_OperationDestroy(ADUC_Retriable_Operation_Context* context)
{
    Log_Debug("Destroy operation context");
    AgentInfoRequestOperation_DataDestroy(context);
}

/**
 * @brief The callback function to be called when the agentinfo request operation is expired before it is completed.
 * i.e., the operation is not completed within the timeout period. For MQTT operations, this could mean that the agent
 * does not receive the response message from the service.
 *
 * @param data The context for the operation.
*/
void AgentInfoRequestOperation_OnExpired(ADUC_Retriable_Operation_Context* data)
{
    ADUC_Retriable_Set_State(data, ADUC_Retriable_Operation_State_Expired);
    ADUC_Retriable_Set_State(data, ADUC_Retriable_Operation_State_Cancelling);
}

/**
 * @brief Request operation success handler.
 *
 * @param data The operation context.
 */
void AgentInfoRequestOperation_OnSuccess(ADUC_Retriable_Operation_Context* data)
{
    ADUC_Retriable_Set_State(data, ADUC_Retriable_Operation_State_Completed);
}

/**
 * @brief Request operation failure handler.
 *
 * @param data The operation context.
 */
void AgentInfoRequestOperation_OnFailure(ADUC_Retriable_Operation_Context* data)
{
    ADUC_Retriable_Set_State(data, ADUC_Retriable_Operation_State_Failure);
}

/**
 * @brief Request Operation Retry handler.
 *
 * @param data The operation context.
 */
void AgentInfoRequestOperation_OnRetry(ADUC_Retriable_Operation_Context* data)
{
    // TODO: Compute next attempt time.
}

/**
 * @brief check if acknowledged and update last heart beat log time and signal to skip to next frame.
 *
 * @param context  The operation context.
 * @param nowTime The current time.
 * @return true on success.
 */
bool IsAgentInfoAlreadyHandled(ADUC_Retriable_Operation_Context* context, time_t nowTime)
{
    return ADUC_StateStore_GetIsAgentInfoReported();
}

/**
 * @brief If in requesting agentinfo state, transitions to InProgress retriable state.
 *
 * @param context The retriable operation context.
 * @param nowTime The current time.
 * @return true if was in agentinfo requesting state.
 */
static bool HandlingRequestAgentInfo(ADUC_Retriable_Operation_Context* context, time_t nowTime)
{
    ADUC_AgentInfo_Request_Operation_Data* agentInfoData = AgentInfoData_FromOperationContext(context);

    if (agentInfoData->agentInfoState == ADU_AGENTINFO_STATE_REQUESTING)
    {
        ADUC_Retriable_Set_State(context, ADUC_Retriable_Operation_State_InProgress);

        // is the current request timed-out?
        if (context->lastExecutionTime + context->operationTimeoutSecs < nowTime)
        {
            context->cancelFunc(context);

            // review: do we need this? cancelfunc should already took care of this.
            Log_Info("agentinfo request timed-out");
            AgentInfoData_SetCorrelationId(agentInfoData, "");

            Set_Agent_State(agentInfoData, ADU_AGENTINFO_STATE_UNKNOWN);

            context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_SERVICE_TRANSIENT]);
        }

        // exit and wait for next 'do work' call.
        return true;
    }

    return false;
}

/**
 * @brief Builds the compatProperties JSON value object
 *
 * @return JSON_Value* The object with compatProperties, or NULL on failure.
 */
static JSON_Value* Build_Compat_Properties_JSON_Value()
{
    JSON_Value* tmp = NULL;
    JSON_Value* compatProps_value = NULL;

    const ADUC_AgentInfo* agent_cfg = NULL;

    if (JSONSuccess != (tmp = json_value_init_object()))
    {
        goto done;
    }

    const ADUC_ConfigInfo* config = ADUC_ConfigInfo_GetInstance();
    if (config == NULL)
    {
        goto done;
    }

    agent_cfg = ADUC_ConfigInfo_GetAgent(config, 0);

    // TODO: model compat properties differently in duconfig.json perhaps with a "compatProperties" JSON object.
    // TOOD: remove top-level manufacturer and model and remove agents array.
    //
    //   {
    //     "compatProp1": "compatPropValue1",
    //     "compatProp2": "compatPropValue2"
    //   }

    // TODO: get manufacturer, model, and additional properties properly.
    // For now, ignore compatPropertyNames and just add "model" and "manufacturer" from agents[0]

    if (JSONSuccess != json_object_set_string(json_object(tmp), "manufacturer", agent_cfg->manufacturer))
    {
        goto done;
    }

    if (JSONSuccess != json_object_set_string(json_object(tmp), "model", agent_cfg->model))
    {
        goto done;
    }

    compatProps_value = tmp;
    tmp = NULL; // transfer ownership

done:
    json_value_free(tmp);

    if (config != NULL)
    {
        ADUC_ConfigInfo_ReleaseInstance(config);
    }

    return compatProps_value;
}

/**
 * @brief Builds the ainfo_req agent info message payload.
 *
 * @param nowTime The current time.
 * @return char* The JSON string payload.
 */
static char* Build_AgentInfo_Request_Msg_Payload(time_t nowTime)
{
    char* payload = NULL;
    char* tmp = NULL;

    JSON_Value* root = NULL;
    JSON_Value* compatProps_value = NULL;

    // Format:
    // {
    //   "sn": "<timestamp>",
    //   "compatProperties": {
    //     "compatProp1": "compatPropValue1",
    //     "compatProp2": "compatPropValue2"
    //   }
    // }

    char long_time[21]; // max positive value 9 223 372 036 854 775 807 + 1 (sign) + (term)
    memset(&long_time, 0, sizeof(long_time));
    sprintf(&long_time[0], "%ld", nowTime);

    root = json_value_init_object();
    if (root == NULL)
    {
        goto done;
    }

    compatProps_value = json_value_init_object();
    if (compatProps_value == NULL)
    {
        goto done;
    }

    if (JSONSuccess != json_object_set_string(json_object(root), "sn", &long_time[0]))
    {
        goto done;
    }

    compatProps_value = Build_Compat_Properties_JSON_Value();
    if (compatProps_value == NULL)
    {
        goto done;
    }

    if (JSONSuccess != json_object_set_value(json_object(root), "compatProperties", compatProps_value))
    {
        goto done;
    }

    // TODO: use non-pretty variant for compactness before shipping
    // tmp = json_serialize_to_string(root);
    tmp = json_serialize_to_string_pretty(root);
    if (tmp == NULL)
    {
        goto done;
    }

    payload = tmp;
    tmp = NULL; // transfer ownership

done:
    free(tmp);
    json_value_free(compatProps_value);
    json_value_free(root);

    return payload;
}

/*
 * The agent info response message 'ainfo_resp' will contain the following:
 *
 * Message payload:
 * - sn: The sequence number.
 * - compatProperties: A list of properties that the client supports.
 *   example:
 *   {
 *     "sn": "<timestamp>",
 *     "compatProperties": {
 *       "compatProp1": "compatPropValue1",
 *       "compatProp2": "compatPropValue2"
 *     }
 *   }
 *
 * User Properties:
 *  "mt" -> "ainfo_req"
 * "pid" -> 1
 *
 * Content Type: json
 * Correlation Data: UUID
 */
static bool SendAgentInfoStatusRequest(ADUC_Retriable_Operation_Context* context, time_t nowTime)
{
    bool opSucceeded = false;
    int mqtt_res = 0;

    ADUC_AgentInfo_Request_Operation_Data* agentInfoData = AgentInfoData_FromOperationContext(context);
    ADUC_MQTT_Message_Context* messageContext = &agentInfoData->ainfoReqMessageContext;
    mosquitto_property* user_prop_list = NULL;

    char* msg_payload = Build_AgentInfo_Request_Msg_Payload(nowTime);
    if (msg_payload == NULL)
    {
        Log_Error("msg_payload");
        goto done;
    }

    // Set MQTT 5 user propertie as per ainfo req-res
    if (!ADU_mosquitto_add_user_property(&user_prop_list, "mt", "ainfo_req") ||
        !ADU_mosquitto_add_user_property(&user_prop_list, "pid", "1"))
    {
        goto done;
    }

    if (!ADU_mosquitto_set_correlation_data_property(&user_prop_list, &(agentInfoData->ainfoReqMessageContext.correlationId)[0]))
    {
        Log_Error("set correlationId");
        goto done;
    }

    mqtt_res = ADUC_Communication_Channel_MQTT_Publish(
        (ADUC_AGENT_MODULE_HANDLE)context->commChannelHandle,
        messageContext->publishTopic,
        &messageContext->messageId, // generated message id to save
        (int)strlen(msg_payload),
        msg_payload,
        1, // qos 1 is required for adu gen2 protocol v1
        false /* retain */,
        user_prop_list);

    if (mqtt_res != MOSQ_ERR_SUCCESS)
    {
        opSucceeded = false;
        Log_Error(
            "pub 'ainfo_req' mid:%d cid:%s err:%d, %s)",
            messageContext->messageId,
            messageContext->correlationId,
            mqtt_res,
            mosquitto_strerror(mqtt_res));

        switch (mqtt_res)
        {
        case MOSQ_ERR_INVAL:
        case MOSQ_ERR_NOMEM:
        case MOSQ_ERR_PROTOCOL:
        case MOSQ_ERR_PAYLOAD_SIZE:
        case MOSQ_ERR_MALFORMED_UTF8:
        case MOSQ_ERR_DUPLICATE_PROPERTY:
        case MOSQ_ERR_QOS_NOT_SUPPORTED:
        case MOSQ_ERR_OVERSIZE_PACKET:
            // following error is non-recoverable, so we'll bail out.
            Log_Error("non-recoverable:%d", mqtt_res);
            context->cancelFunc(context);
            break;

        case MOSQ_ERR_NO_CONN:

            // compute and apply the next execution time, based on the specified retry parameters.
            context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_CLIENT_TRANSIENT]);

            Log_Error(
                "retry-able after t[%ld]: %d", context->operationIntervalSecs, mqtt_res);
            break;

        default:
            Log_Error("unhandled:%d ", mqtt_res);
            // retry again after default retry delay.
            context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_DEFAULT]);
            break;
        }

        goto done;
    }

    Set_Agent_State(agentInfoData, ADU_AGENTINFO_STATE_REQUESTING);

    context->lastExecutionTime = nowTime;

    Log_Debug(
        "--> 'ainfo_req' (mid:%d, cid:%s, t:%ld, timeout in:%ld)",
        messageContext->messageId,
        messageContext->correlationId,
        context->lastExecutionTime,
        context->operationTimeoutSecs);

    opSucceeded = true;
done:
    if (msg_payload != NULL)
    {
        json_free_serialized_string(msg_payload);
    }

    if (user_prop_list != NULL)
    {
        ADU_mosquitto_free_properties(&user_prop_list);
    }

    return opSucceeded;
}

/**
 * @brief The main workflow for managing device agentinfo status request operation.
 * @details The workflow is as follows:
 *      1. subscribe to the agentinfo status response topic.
 *      2. send agentinfo status request message.
 *      3. wait for agentinfo status response message.
 *        3.1 if the response message correlation-data doesn't match the latest request, ignore the message.
 *        3.2 if the response message correlation-data matches the latest request, parse the message and update the agentinfo state.
 *          3.2.1 if the agentinfo state is acknowledged, stop the the operation.
 *          3.2.2 if the agentinfo state is not acknowledged, retry the operation after the retry delay.
 */
bool AgentInfoStatusRequestOperation_doWork(ADUC_Retriable_Operation_Context* context)
{
    bool opSucceeded = false;

    ADUC_AgentInfo_Request_Operation_Data* agentInfoData = NULL;

    if (context == NULL || context->data == NULL)
    {
        return opSucceeded;
    }

    const time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();

    if (IsAgentInfoAlreadyHandled(context, nowTime))
    {
        goto done;
    }

    if (!ADUC_StateStore_GetIsDeviceEnrolled())
    {
        goto done;
    }

    if (HandlingRequestAgentInfo(context, nowTime))
    {
        goto done;
    }

    // at this point, we should send 'ainfo_req' message.
    agentInfoData = AgentInfoData_FromOperationContext(context);

    if (SettingUpAduMqttRequestPrerequisites(context, &agentInfoData->ainfoReqMessageContext, true /* isScoped */))
    {
        goto done;
    }

    if (!SendAgentInfoStatusRequest(context, nowTime))
    {
        goto done;
    }

    opSucceeded = true;
done:

    return opSucceeded;
}

/**
 * @brief create a and initialize agentinfo request operation object
 *
 * @return ADUC_Retriable_Operation_Context* the operation context, or NULL on failure.
 */
ADUC_Retriable_Operation_Context* CreateAndInitializeAgentInfoRequestOperation()
{
    ADUC_Retriable_Operation_Context* context = NULL;

    ADUC_Retriable_Operation_Context* tmp = NULL;
    ADUC_AgentInfo_Request_Operation_Data* operationDataContext = NULL;

    const ADUC_ConfigInfo* config = NULL;
    const ADUC_AgentInfo* agent_info = NULL;
    unsigned int value = 0;

    tmp = calloc(1, sizeof(*tmp));
    if (tmp == NULL)
    {
        goto done;
    }

    operationDataContext = calloc(1, sizeof(*operationDataContext));
    if (operationDataContext == NULL)
    {
        goto done;
    }

    // For this request, generate correlation Id sent as CorrelationData property
    // and used to match with response.
    if (!ADUC_generate_correlation_id(false /* with_hyphens */, &(operationDataContext->ainfoReqMessageContext.correlationId)[0], ARRAY_SIZE(operationDataContext->ainfoReqMessageContext.correlationId)))
    {
        Log_Error("correlationid");
        goto done;
    }

    tmp->data = operationDataContext;
    operationDataContext = NULL;

    // initialize custom functions
    tmp->dataDestroyFunc = AgentInfoRequestOperation_DataDestroy;
    tmp->operationDestroyFunc = AgentInfoRequestOperation_OperationDestroy;
    tmp->doWorkFunc = AgentInfoStatusRequestOperation_doWork;
    tmp->cancelFunc = AgentInfoRequestOperation_CancelOperation;
    tmp->retryFunc = AgentInfoRequestOperation_DoRetry;
    tmp->completeFunc = AgentInfoRequestOperation_Complete;
    tmp->retryParamsCount = RetryUtils_GetRetryParamsMapSize();

    // initialize callbacks
    tmp->onExpired = AgentInfoRequestOperation_OnExpired;
    tmp->onSuccess = AgentInfoRequestOperation_OnSuccess;
    tmp->onFailure = AgentInfoRequestOperation_OnFailure;
    tmp->onRetry =
        AgentInfoRequestOperation_OnRetry; // read or retry strategy parameters from the agent's configuration file.

    // initialize operation and retry settings
    config = ADUC_ConfigInfo_GetInstance();
    if (config == NULL)
    {
        Log_Error("fail get config instance");
        goto done;
    }

    agent_info = ADUC_ConfigInfo_GetAgent(config, 0);
    if (agent_info == NULL)
    {
        Log_Error("fail get agentinfo");
        goto done;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(agent_info, SETTING_KEY_ENR_REQ_OP_INTERVAL_SECONDS, &value))
    {
        log_warn("using default request interval: %d", DEFAULT_ENR_REQ_OP_INTERVAL_SECONDS);
        value = DEFAULT_ENR_REQ_OP_INTERVAL_SECONDS;
    }
    tmp->operationIntervalSecs = value;

    value = 0;
    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(agent_info, SETTING_KEY_ENR_REQ_OP_TIMEOUT_SECONDS, &value))
    {
        log_warn("using default request timeout: %d", DEFAULT_ENR_REQ_OP_TIMEOUT_SECONDS);
        value = DEFAULT_ENR_REQ_OP_TIMEOUT_SECONDS;
    }
    tmp->operationTimeoutSecs = value;

    // get retry parameters for transient client error.
    JSON_Value* retrySettings = json_object_dotget_value(
        json_value_get_object(agent_info->agentJsonValue), SETTING_KEY_ENR_REQ_OP_RETRY_PARAMS);
    if (retrySettings == NULL)
    {
        Log_Error("retry param");
        goto done;
    }

    // Initialize retry parameters.
    tmp->retryParams = calloc((size_t)tmp->retryParamsCount, sizeof(*tmp->retryParams));
    if (tmp->retryParams == NULL)
    {
        goto done;
    }

    ReadRetryParamsArrayFromAgentConfigJson(tmp, retrySettings, RetryUtils_GetRetryParamsMapSize());

    context = tmp;
    tmp = NULL;

done:

    free(operationDataContext);
    free(tmp);

    if (config != NULL)
    {
        ADUC_ConfigInfo_ReleaseInstance(config);
    }

    return context;
}
