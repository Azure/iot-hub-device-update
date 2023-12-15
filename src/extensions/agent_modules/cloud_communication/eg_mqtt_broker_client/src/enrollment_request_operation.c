#include "aduc/enrollment_request_operation.h"

#include <aduc/adu_communication_channel.h> // ADUC_DU_SERVICE_COMMUNICATION_CHANNEL_ID
#include <aduc/adu_enrollment.h> // ADUC_Enrollment_Request_Operation_Data
#include <aduc/adu_enrollment_utils.h> // EnrollmentData_FromOperationContext
#include <aduc/adu_mosquitto_utils.h> // ADU_mosquitto_add_user_property
#include <aduc/adu_mqtt_common.h>
#include <aduc/agent_state_store.h> // ADUC_StateStore_IsDeviceEnrolled
#include <aduc/config_utils.h> // ADUC_ConfigInfo, ADUC_AgentInfo
#include <aduc/logging.h>
#include <du_agent_sdk/agent_module_interface.h> // ADUC_AGENT_MODULE_HANDLE

/**
 * @brief Free memory allocated for the enrollment data.
 */
static void EnrollmentData_Deinit(ADUC_Enrollment_Request_Operation_Data* data)
{
    free(data);
}

/**
 * @brief Cancel operation.
 *
 * @param context The retriable context
 * @return true on success.
 */
bool EnrollmentRequestOperation_CancelOperation(ADUC_Retriable_Operation_Context* context)
{
    ADUC_Enrollment_Request_Operation_Data* enrollmentData = EnrollmentData_FromOperationContext(context);
    if (enrollmentData == NULL)
    {
        return false;
    }

    // Set the correlation id to NULL, so we can discard all inflight messages.
    if (!EnrollmentData_SetCorrelationId(enrollmentData, ""))
    {
        return false;
    }

    EnrollmentData_SetState(enrollmentData, ADU_ENROLLMENT_STATE_UNKNOWN, "Cancel requested");
    return true;
}

/**
 * @brief Request Operation Complete handler.
 *
 * @param context The retriable operation context.
 * @return true on success.
 */
bool EnrollmentRequestOperation_Complete(ADUC_Retriable_Operation_Context* context)
{
    if (context == NULL)
    {
        Log_Error("context is NULL");
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
 * For 'enr_req' operation, we'll set the enrollment state to 'unknown', and clear the message's correlation id,
 * which will any incoming 'enr_resp' message to be ignored.
 *
 * At next 'DoWork' call, we'll generate a new correlation id and send the new 'enr_req' message.
 *
 * @param context The context for the operation.
 *
 * @return true if the operation is successfully retried. false otherwise.
 */
bool EnrollmentRequestOperation_DoRetry(
    ADUC_Retriable_Operation_Context* context, const ADUC_Retry_Params* retryParams)
{
    Log_Info("Will retry the enrollment request operation.");
    ADUC_Enrollment_Request_Operation_Data* data = EnrollmentData_FromOperationContext(context);

    if (data == NULL)
    {
        return false;
    }

    EnrollmentData_SetState(data, ADU_ENROLLMENT_STATE_UNKNOWN, "retrying");
    if (!EnrollmentData_SetCorrelationId(data, ""))
    {
        return false;
    }

    return true;
}

void EnrollmentRequestOperation_DataDestroy(ADUC_Retriable_Operation_Context* context)
{
    Log_Info("Destroying context data");
    if (context != NULL && context->data != NULL)
    {
        // Deinitialize and destroy the associated
        EnrollmentData_Deinit(EnrollmentData_FromOperationContext(context));
        context->data = NULL;

        free(context->operationName);
        context->operationName = NULL;
    }
}

void EnrollmentRequestOperation_OperationDestroy(ADUC_Retriable_Operation_Context* context)
{
    Log_Info("Destroying operation context");
    EnrollmentRequestOperation_DataDestroy(context);
}

/**
 * @brief The callback function to be called when the enrollment request operation is expired before it is completed.
 * i.e., the operation is not completed within the timeout period. For MQTT operations, this could mean that the agent
 * does not receive the response message from the service.
 *
 * @param data The context for the operation.
*/
void EnrollmentRequestOperation_OnExpired(ADUC_Retriable_Operation_Context* data)
{
    ADUC_Retriable_Set_State(data, ADUC_Retriable_Operation_State_Expired);
    ADUC_Retriable_Set_State(data, ADUC_Retriable_Operation_State_Cancelling);
}

/**
 * @brief Request operation success handler.
 *
 * @param data The operation context.
 */
void EnrollmentRequestOperation_OnSuccess(ADUC_Retriable_Operation_Context* data)
{
    ADUC_Retriable_Set_State(data, ADUC_Retriable_Operation_State_Completed);
}

/**
 * @brief Request operation failure handler.
 *
 * @param data The operation context.
 */
void EnrollmentRequestOperation_OnFailure(ADUC_Retriable_Operation_Context* data)
{
    ADUC_Retriable_Set_State(data, ADUC_Retriable_Operation_State_Failure);
}

/**
 * @brief Request Operation Retry handler.
 *
 * @param data The operation context.
 */
void EnrollmentRequestOperation_OnRetry(ADUC_Retriable_Operation_Context* data)
{
    // TODO: Compute next attempt time.
}

/**
 * @brief check if enrolled and update last heart beat log time and signal to skip to next frame.
 *
 * @param context  The operation context.
 * @param nowTime The current time.
 * @return true on success.
 */
bool IsEnrollAlreadyHandled(ADUC_Retriable_Operation_Context* context, time_t nowTime)
{
    ADUC_Enrollment_Request_Operation_Data* enrollmentData = EnrollmentData_FromOperationContext(context);

    if (enrollmentData->enrollmentState == ADU_ENROLLMENT_STATE_ENROLLED || ADUC_StateStore_IsDeviceEnrolled())
    {
        // enrollment is completed.
        ADUC_Retriable_Set_State(context, ADUC_Retriable_Operation_State_Completed);

        return true;
    }

    return false;
}

/**
 * @brief If in requesting enrollment state, transitions to InProgress retriable state.
 *
 * @param context The retriable operation context.
 * @param nowTime The current time.
 * @return true if was in enrollment requesting state.
 */
bool HandlingRequestEnrollment(ADUC_Retriable_Operation_Context* context, time_t nowTime)
{
    ADUC_Enrollment_Request_Operation_Data* enrollmentData = EnrollmentData_FromOperationContext(context);

    // are we querying the enrollment status?
    if (enrollmentData->enrollmentState == ADU_ENROLLMENT_STATE_REQUESTING)
    {
        ADUC_Retriable_Set_State(context, ADUC_Retriable_Operation_State_InProgress);

        // is the current request timed-out?
        //
        // TODO: figure out why context->operationTimeoutSecs is 0
        // if (context->lastExecutionTime + context->operationTimeoutSecs < nowTime)
        if (context->lastExecutionTime + 180 < nowTime) // timeout after 180 seconds for now
        {
            Log_Debug(
                "timeout 'enr_req' timeout:%d optimeout:%d t:%d",
                context->lastExecutionTime,
                context->operationTimeoutSecs,
                nowTime);

            context->cancelFunc(context);
        }

        // exit and wait for next 'do work' call.
        return true;
    }

    return false;
}

static bool SendEnrollmentStatusRequest(ADUC_Retriable_Operation_Context* context, time_t nowTime)
{
    bool opSucceeded = false;
    int mqtt_res = 0;
    mosquitto_property* user_prop_list = NULL;
    ADUC_MQTT_Message_Context* messageContext = NULL;
    ADUC_Enrollment_Request_Operation_Data* enrollmentData = EnrollmentData_FromOperationContext(context);

    if (enrollmentData == NULL)
    {
        Log_Error("NULL enrollmentData");
        goto done;
    }

    if (!ADU_mosquitto_add_user_property(&user_prop_list, "mt", "enr_req")
        || !ADU_mosquitto_add_user_property(&user_prop_list, "pid", "1"))
    {
        Log_Error("fail add user props");
        goto done;
    }

    if (strlen(enrollmentData->enrReqMessageContext.correlationId) == 0)
    {
        // The DU service can handle with or without hyphens, but reducing data transferred by omitting them.
        if (!ADUC_generate_correlation_id(
            false /* with_hyphens */,
            &(enrollmentData->enrReqMessageContext.correlationId)[0],
            ARRAY_SIZE(enrollmentData->enrReqMessageContext.correlationId)))
        {
            Log_Error("Fail to generate correlationid");
            goto done;
        }
    }

    if (!ADU_mosquitto_set_correlation_data_property(&user_prop_list, &(enrollmentData->enrReqMessageContext.correlationId)[0]))
    {
        Log_Error("fail set corr id");
        goto done;
    }

    if (!ADU_mosquitto_set_content_type_property(&user_prop_list, "json"))
    {
        Log_Error("fail set content type user property");
        goto done;
    }

    messageContext = &enrollmentData->enrReqMessageContext;

    mqtt_res = ADUC_Communication_Channel_MQTT_Publish(
        (ADUC_AGENT_MODULE_HANDLE)context->commChannelHandle,
        messageContext->publishTopic,
        &messageContext->messageId,
        2,
        "{}", // enrollment request message content, which is always an empty json object.
        1, // qos 1 is required for adu gen2 protocol v1
        false,
        user_prop_list);

    if (mqtt_res != MOSQ_ERR_SUCCESS)
    {
        opSucceeded = false;
        Log_Error(
            "fail PUBLISH 'enr_req' msgid: %d, correlationid: %s, err: %d, errmsg: '%s')",
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

    EnrollmentData_SetState(enrollmentData, ADU_ENROLLMENT_STATE_REQUESTING, "enr_req submit");

    context->lastExecutionTime = nowTime;

    Log_Info(
        "--> PUBLISH 'enr_req' msgid: %d correlationid: '%s' lastExecTime: %ld timeoutSecs: %ld)",
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
 * @brief The main workflow for managing device enrollment status request operation.
 * @details The workflow is as follows:
 *      1. subscribe to the enrollment status response topic.
 *      2. send enrollment status request message.
 *      3. wait for enrollment status response message.
 *        3.1 if the response message correlation-data doesn't match the latest request, ignore the message.
 *        3.2 if the response message correlation-data matches the latest request, parse the message and update the enrollment state.
 *          3.2.1 if the enrollment state is enrolled, stop the the operation.
 *          3.2.2 if the enrollment state is not enrolled, retry the operation after the retry delay.
 */
bool EnrollmentStatusRequestOperation_doWork(ADUC_Retriable_Operation_Context* context)
{
    bool opSucceeded = false;

    ADUC_Enrollment_Request_Operation_Data* enrollmentData = NULL;

    if (context == NULL || context->data == NULL)
    {
        return false;
    }

    const time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();

    if (IsEnrollAlreadyHandled(context, nowTime))
    {
        goto done;
    }

    if (HandlingRequestEnrollment(context, nowTime))
    {
        goto done;
    }

    enrollmentData = EnrollmentData_FromOperationContext(context);
    if (enrollmentData == NULL)
    {
        goto done;
    }

    if (SettingUpAduMqttRequestPrerequisites(context, &enrollmentData->enrReqMessageContext, false /* isScoped */))
    {
        goto done;
    }

    if (!SendEnrollmentStatusRequest(context, nowTime))
    {
        goto done;
    }

    opSucceeded = true;
done:

    return opSucceeded;
}

/**
 * @brief create a and initialize enrollment request operation object
 *
 * @return ADUC_Retriable_Operation_Context* the operation context, or NULL on failure.
 */
ADUC_Retriable_Operation_Context* CreateAndInitializeEnrollmentRequestOperation()
{
    ADUC_Retriable_Operation_Context* context = NULL;

    ADUC_Retriable_Operation_Context* tmp = NULL;
    ADUC_Enrollment_Request_Operation_Data* operationDataContext = NULL;

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
    if (!ADUC_generate_correlation_id(
            false /* with_hyphens */,
            &(operationDataContext->enrReqMessageContext.correlationId)[0],
            ARRAY_SIZE(operationDataContext->enrReqMessageContext.correlationId)))
    {
        Log_Error("Fail to generate correlation id");
        goto done;
    }

    tmp->data = operationDataContext;
    operationDataContext = NULL; // transfer ownership

    // Initialize lifecycle functions
    tmp->dataDestroyFunc = EnrollmentRequestOperation_DataDestroy;
    tmp->operationDestroyFunc = EnrollmentRequestOperation_OperationDestroy;
    tmp->doWorkFunc = EnrollmentStatusRequestOperation_doWork;
    tmp->cancelFunc = EnrollmentRequestOperation_CancelOperation;
    tmp->retryFunc = EnrollmentRequestOperation_DoRetry;
    tmp->completeFunc = EnrollmentRequestOperation_Complete;
    tmp->retryParamsCount = RetryUtils_GetRetryParamsMapSize();

    // initialize callbacks
    tmp->onExpired = EnrollmentRequestOperation_OnExpired;
    tmp->onSuccess = EnrollmentRequestOperation_OnSuccess;
    tmp->onFailure = EnrollmentRequestOperation_OnFailure;
    tmp->onRetry =
        EnrollmentRequestOperation_OnRetry; // read or retry strategy parameters from the agent's configuration file.

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
        log_warn("failed to get enrollment status request interval setting");
        value = DEFAULT_ENR_REQ_OP_INTERVAL_SECONDS;
    }

    tmp->operationIntervalSecs = value;

    value = 0;
    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(
            agent_info, SETTING_KEY_ENR_REQ_OP_TIMEOUT_SECONDS, &value))
    {
        log_warn("failed to get enrollment status request timeout setting");
        value = DEFAULT_ENR_REQ_OP_TIMEOUT_SECONDS;
    }

    tmp->operationTimeoutSecs = value;

    // get retry parameters for transient client error.
    JSON_Value* retrySettings = json_object_dotget_value(
        json_value_get_object(agent_info->agentJsonValue), SETTING_KEY_ENR_REQ_OP_RETRY_PARAMS);
    if (retrySettings == NULL)
    {
        Log_Error("failed to get retry settings");
        goto done;
    }

    // Initialize the retry parameters
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