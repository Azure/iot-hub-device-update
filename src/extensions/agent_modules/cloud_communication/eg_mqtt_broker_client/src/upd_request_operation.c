#include "aduc/upd_request_operation.h"

#include <aduc/adu_communication_channel.h> // ADUC_DU_SERVICE_COMMUNICATION_CHANNEL_ID
#include <aduc/adu_mosquitto_utils.h> // ADU_mosquitto_add_user_property
#include <aduc/adu_mqtt_common.h>
#include <aduc/adu_upd.h> // ADUC_Update_Request_Operation_Data
#include <aduc/adu_upd_utils.h> // UpdateData_FromOperationContext
#include <aduc/agent_state_store.h> // ADUC_StateStore_IsDeviceEnrolled
#include <aduc/config_utils.h> // ADUC_ConfigInfo, ADUC_AgentInfo
#include <aduc/logging.h>
#include <du_agent_sdk/agent_module_interface.h> // ADUC_AGENT_MODULE_HANDLE

// Forward declarations
void Adu_ProcessUpdate(ADUC_Update_Request_Operation_Data* updateData, ADUC_Retriable_Operation_Context* retriableOperationContext);
time_t ADUC_Retry_Delay_Calculator(
    int additionalDelaySecs,
    unsigned int retries,
    unsigned long initialDelayUnitMilliSecs,
    unsigned long maxDelaySecs,
    double maxJitterPercent);

/**
 * @brief Free memory allocated for the upd data.
 */
static void UpdateData_Deinit(ADUC_Update_Request_Operation_Data* data)
{
    free(data);
}

/**
 * @brief Cancel operation to set execution time to future and enter Idle Wait state.
 *
 * @param context The retriable context
 * @return true on success.
 */
bool UpdateRequestOperation_CancelOperation(ADUC_Retriable_Operation_Context* context)
{
    time_t now = ADUC_GetTimeSinceEpochInSeconds();
    ADUC_Update_Request_Operation_Data* updData = UpdateData_FromOperationContext(context);
    if (updData == NULL)
    {
        return false;
    }

    context->lastFailureTime = now;
    context->nextExecutionTime = now + 180; // TODO: use retryParams.
    AduUpdUtils_TransitionState(ADU_UPD_STATE_IDLEWAIT, updData);
    return true;
}

/**
 * @brief Request Operation Complete handler.
 *
 * @param context The retriable operation context.
 * @return true on success.
 */
bool UpdateRequestOperation_Complete(ADUC_Retriable_Operation_Context* context)
{
    if (context == NULL)
    {
        Log_Error("context is NULL");
        return false;
    }

    ADUC_Update_Request_Operation_Data* updData = UpdateDataFromRetriableOperationContext(context);

    Log_Debug("Completed");
    ADUC_Retriable_Set_State(context, ADUC_Retriable_Operation_State_Completed);

    context->nextExecutionTime = ADUC_GetTimeSinceEpochInSeconds() + 30;

    AduUpdUtils_TransitionState(ADU_UPD_STATE_IDLEWAIT, updData);
    return true;
}

/**
 * @brief This fuction is call when the caller wants to retry the operation. This function should update the
 * internal states of the operation so that the next call to 'DoWork' will retry the operation.
 *
 * For 'upd_req' operation, we'll set the upd state to 'unknown', and clear the message's correlation id,
 * which will any incoming 'upd_resp' message to be ignored.
 *
 * At next 'DoWork' call, we'll generate a new correlation id and send the new 'upd_req' message.
 *
 * @param context The context for the operation.
 *
 * @return true if the operation is successfully retried. false otherwise.
 */
bool UpdateRequestOperation_DoRetry(ADUC_Retriable_Operation_Context* context, const ADUC_Retry_Params* retryParams)
{
    time_t now = 0;
    time_t newTime = 0;
    ADUC_Retry_Params* retryCfg = &context->retryParams[ADUC_RETRY_PARAMS_INDEX_DEFAULT];

    Log_Info("Will retry the upd request operation.");
    ADUC_Update_Request_Operation_Data* data = UpdateData_FromOperationContext(context);

    if (data == NULL)
    {
        return false;
    }

    // reset so it's regenerated with new request.
    UpdateData_SetCorrelationId(data, "");

    now = ADUC_GetTimeSinceEpochInSeconds();

    context->attemptCount++;

    if (context->attemptCount > retryCfg->maxRetries)
    {
        Log_Debug("Max retry of %d exceeded. Failing.", retryCfg->maxRetries);

        context->lastFailureTime = now;
        AduUpdUtils_TransitionState(ADU_UPD_STATE_IDLEWAIT, data);
    }
    else
    {
        newTime = ADUC_Retry_Delay_Calculator(
            30, // additionalDelaySecs
            context->attemptCount,
            retryCfg->initialDelayUnitMilliSecs,
            retryCfg->maxDelaySecs,
            retryCfg->maxJitterPercent);

        Log_Debug(
            "Will resend the message in %d second(s) (t:%d, r:%d)", newTime - now, newTime, context->attemptCount);

        context->nextExecutionTime = newTime;

        AduUpdUtils_TransitionState(ADU_UPD_STATE_RETRYWAIT, data);
    }

    return true;
}

void UpdateRequestOperation_DataDestroy(ADUC_Retriable_Operation_Context* context)
{
    Log_Info("Destroying context data");
    if (context != NULL && context->data != NULL)
    {
        // Deinitialize and destroy the associated
        UpdateData_Deinit(UpdateData_FromOperationContext(context));
        context->data = NULL;

        free(context->operationName);
        context->operationName = NULL;
    }
}

void UpdateRequestOperation_OperationDestroy(ADUC_Retriable_Operation_Context* context)
{
    Log_Info("Destroying operation context");
    UpdateRequestOperation_DataDestroy(context);
}

/**
 * @brief The callback function to be called when the upd request operation is expired before it is completed.
 * i.e., the operation is not completed within the timeout period. For MQTT operations, this could mean that the agent
 * does not receive the response message from the service.
 *
 * @param data The context for the operation.
*/
void UpdateRequestOperation_OnExpired(ADUC_Retriable_Operation_Context* data)
{
    ADUC_Retriable_Set_State(data, ADUC_Retriable_Operation_State_Expired);
}

/**
 * @brief Request operation success handler.
 *
 * @param data The operation context.
 */
void UpdateRequestOperation_OnSuccess(ADUC_Retriable_Operation_Context* data)
{
    ADUC_Retriable_Set_State(data, ADUC_Retriable_Operation_State_Completed);
}

/**
 * @brief Request operation failure handler.
 *
 * @param data The operation context.
 */
void UpdateRequestOperation_OnFailure(ADUC_Retriable_Operation_Context* data)
{
    ADUC_Retriable_Set_State(data, ADUC_Retriable_Operation_State_Failure);
}

/**
 * @brief Request operation retry handler.
 *
 * @param data The operation context.
 */
void UpdateRequestOperation_OnRetry(ADUC_Retriable_Operation_Context* data)
{
    ADUC_Retriable_Set_State(data, ADUC_Retriable_Operation_State_Failure_Retriable);
}

static bool SendUpdateRequest(ADUC_Retriable_Operation_Context* context, time_t nowTime)
{
    bool opSucceeded = false;
    int mqtt_res = 0;
    mosquitto_property* user_prop_list = NULL;
    ADUC_MQTT_Message_Context* messageContext = NULL;
    ADUC_Update_Request_Operation_Data* updData = UpdateData_FromOperationContext(context);

    if (updData == NULL)
    {
        Log_Error("NULL updData");
        goto done;
    }

    if (!ADU_mosquitto_add_user_property(&user_prop_list, "mt", "upd_req")
        || !ADU_mosquitto_add_user_property(&user_prop_list, "pid", "1"))
    {
        Log_Error("fail add user props");
        goto done;
    }

    // Set the Correlation Data MQTT property
    if (strlen(updData->updReqMessageContext.correlationId) == 0)
    {
        // The DU service can handle with or without hyphens, but reducing data transferred by omitting them.
        if (!ADUC_generate_correlation_id(
                false /* with_hyphens */,
                &(updData->updReqMessageContext.correlationId)[0],
                ARRAY_SIZE(updData->updReqMessageContext.correlationId)))
        {
            Log_Error("Fail to generate correlationid");
            goto done;
        }
    }

    if (!ADU_mosquitto_set_correlation_data_property(
            &user_prop_list, &(updData->updReqMessageContext.correlationId)[0]))
    {
        Log_Error("set correlationId");
        goto done;
    }

    messageContext = &updData->updReqMessageContext;

    mqtt_res = ADUC_Communication_Channel_MQTT_Publish(
        (ADUC_AGENT_MODULE_HANDLE)context->commChannelHandle,
        messageContext->publishTopic,
        &messageContext->messageId,
        2,
        "{}", // upd request message content, which is always an empty json object.
        1, // qos 1 is required for adu gen2 protocol v1
        false,
        user_prop_list);

    if (mqtt_res != MOSQ_ERR_SUCCESS)
    {
        opSucceeded = false;
        Log_Error(
            "fail PUBLISH 'upd_req' msgid: %d, correlationid: %s, err: %d, errmsg: '%s')",
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

            Log_Error("retry-able after t:%ld, err:%d", context->operationIntervalSecs, mqtt_res);
            break;

        default:
            Log_Error("unhandled err:%d", mqtt_res);
            // retry again after default retry delay.
            context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_DEFAULT]);
            break;
        }

        goto done;
    }

    AduUpdUtils_TransitionState(ADU_UPD_STATE_REQUESTING, updData);
    ADUC_Retriable_Set_State(context, ADUC_Retriable_Operation_State_InProgress);

    context->lastExecutionTime = nowTime;

    Log_Info(
        "--> PUBLISH 'upd_req' msgid: %d correlationid: '%s' lastExecTime: %ld timeoutSecs: %ld)",
        messageContext->messageId,
        messageContext->correlationId,
        context->lastExecutionTime,
        context->operationTimeoutSecs);

    opSucceeded = true;
done:
    if (user_prop_list != NULL)
    {
        ADU_mosquitto_free_properties(&user_prop_list);
    }

    return opSucceeded;
}


/**
 * @brief The main workflow for managing device upd status request operation.
 * @return true on operation succeeded.
 */
bool UpdateRequestOperation_DoWork(ADUC_Retriable_Operation_Context* context)
{
    bool opSucceeded = false;

    ADUC_Update_Request_Operation_Data* updData = NULL;

    if (context == NULL || context->data == NULL)
    {
        return false;
    }

    const time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();

    updData = UpdateData_FromOperationContext(context);
    switch (updData->updState)
    {
    case ADU_UPD_STATE_READY:
        // Do the normal pre-requisite checks and send the update request to ask the service if there
        // are is a new update available.
        updData = UpdateData_FromOperationContext(context);
        if (updData == NULL)
        {
            goto done;
        }

        if (!ADUC_StateStore_IsAgentInfoReported())
        {
            goto done;
        }

        if (SettingUpAduMqttRequestPrerequisites(context, &updData->updReqMessageContext, true /* isScoped */))
        {
            goto done;
        }

        if (!SendUpdateRequest(context, nowTime))
        {
            goto done;
        }
        break;

    case ADU_UPD_STATE_IDLEWAIT:
        if (nowTime > context->nextExecutionTime)
        {
            Log_Debug("exiting IDLEWAIT state: %lu > %lu", nowTime, context->nextExecutionTime);

            AduUpdUtils_TransitionState(ADU_UPD_STATE_READY, updData);
        }
        break;

    case ADU_UPD_STATE_REQUESTING:
        // TODO: determine why context->operationTimeoutSecs is 0
        if (nowTime > context->lastExecutionTime + 30) // timeout after 30 seconds for now
        {
            Log_Debug(
                "timeout 'upd_req' timeout:%d optimeout:%d t:%d",
                context->lastExecutionTime,
                context->operationTimeoutSecs,
                nowTime);

            context->cancelFunc(context);
        }
        break;

    case ADU_UPD_STATE_RETRYWAIT:
        // TODO: Use configurable retry wait value.
        if (nowTime > context->lastExecutionTime + 60)
        {
            Log_Debug(
                "timeout 'upd_req' timeout:%d optimeout:%d t:%d",
                context->lastExecutionTime,
                context->operationTimeoutSecs,
                nowTime);

            context->cancelFunc(context);
        }
        break;

    case ADU_UPD_STATE_REQUEST_ACK:
        // TODO: Use configurable retry wait value.
        if (nowTime > context->lastExecutionTime + 120)
        {
            Log_Debug("timeout waiting for 'upd_Resp' response message");
            context->cancelFunc(context);
        }

        break;

    case ADU_UPD_STATE_PROCESSING_UPDATE:
        Adu_ProcessUpdate(updData, context);
        break;

    default:
        break;
    }

    opSucceeded = true;
done:

    return opSucceeded;
}

/**
 * @brief create a and initialize upd request operation object
 *
 * @return ADUC_Retriable_Operation_Context* the operation context, or NULL on failure.
 */
ADUC_Retriable_Operation_Context* CreateAndInitializeUpdateRequestOperation()
{
    ADUC_Retriable_Operation_Context* context = NULL;

    ADUC_Retriable_Operation_Context* tmp = NULL;
    ADUC_Update_Request_Operation_Data* operationDataContext = NULL;

    const ADUC_ConfigInfo* config = NULL;
    const ADUC_AgentInfo* agent_info = NULL;
    unsigned int value = 0;
    JSON_Value* retrySettings = NULL;

    tmp = (ADUC_Retriable_Operation_Context*)calloc(1, sizeof(*tmp));
    if (tmp == NULL)
    {
        goto done;
    }

    operationDataContext = (ADUC_Update_Request_Operation_Data*)calloc(1, sizeof(*operationDataContext));
    if (operationDataContext == NULL)
    {
        goto done;
    }

    // For this request, generate correlation Id sent as CorrelationData property
    // and used to match with response.
    if (!ADUC_generate_correlation_id(
            false /* with_hyphens */,
            &(operationDataContext->updReqMessageContext.correlationId)[0],
            ARRAY_SIZE(operationDataContext->updReqMessageContext.correlationId)))
    {
        Log_Error("Fail to generate correlation id");
        goto done;
    }

    tmp->data = operationDataContext;
    operationDataContext = NULL; // transfer ownership

    // Initialize lifecycle functions
    tmp->dataDestroyFunc = UpdateRequestOperation_DataDestroy;
    tmp->operationDestroyFunc = UpdateRequestOperation_OperationDestroy;
    tmp->doWorkFunc = UpdateRequestOperation_DoWork;
    tmp->cancelFunc = UpdateRequestOperation_CancelOperation;
    tmp->retryFunc = UpdateRequestOperation_DoRetry;
    tmp->completeFunc = UpdateRequestOperation_Complete;
    tmp->retryParamsCount = RetryUtils_GetRetryParamsMapSize();

    // initialize callbacks
    tmp->onExpired = UpdateRequestOperation_OnExpired;
    tmp->onSuccess = UpdateRequestOperation_OnSuccess;
    tmp->onFailure = UpdateRequestOperation_OnFailure;
    tmp->onRetry = UpdateRequestOperation_OnRetry;

    config = ADUC_ConfigInfo_GetInstance();
    if (config == NULL)
    {
        Log_Error("failed to get config instance");
        goto done;
    }

    agent_info = ADUC_ConfigInfo_GetAgent(config, 0);
    if (agent_info == NULL)
    {
        Log_Error("failed to get agent info");
        goto done;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(
            agent_info, SETTING_KEY_ENR_REQ_OP_INTERVAL_SECONDS, &value))
    {
        log_warn("failed to get upd status request interval setting");
        value = DEFAULT_ENR_REQ_OP_INTERVAL_SECONDS;
    }

    tmp->operationIntervalSecs = value;

    value = 0;
    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(
            agent_info, SETTING_KEY_ENR_REQ_OP_TIMEOUT_SECONDS, &value))
    {
        log_warn("failed to get upd status request timeout setting");
        value = DEFAULT_ENR_REQ_OP_TIMEOUT_SECONDS;
    }

    tmp->operationTimeoutSecs = value;

    // get retry parameters for transient client error.
    retrySettings = json_object_dotget_value(
        json_value_get_object(agent_info->agentJsonValue), SETTING_KEY_ENR_REQ_OP_RETRY_PARAMS);
    if (retrySettings == NULL)
    {
        Log_Error("failed to get retry settings");
        goto done;
    }

    // Initialize the retry parameters
    tmp->retryParams = (ADUC_Retry_Params*)calloc((size_t)tmp->retryParamsCount, sizeof(*tmp->retryParams));
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
