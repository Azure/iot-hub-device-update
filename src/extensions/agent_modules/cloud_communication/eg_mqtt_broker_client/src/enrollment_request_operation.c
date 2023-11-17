#include "aduc/enrollment_request_operation.h"

#include <aduc/adu_mqtt_common.h>
#include <aduc/adu_enrollment.h> // ADUC_Enrollment_Request_Operation_Data
#include <aduc/adu_enrollment_utils.h> // EnrollmentData_FromOperationContext
#include <aduc/adu_communication_channel.h> // ADUC_DU_SERVICE_COMMUNICATION_CHANNEL_ID
#include <aduc/adu_mosquitto_utils.h> // ADU_mosquitto_add_user_property
#include <aduc/agent_state_store.h> // ADUC_StateStore_GetIsDeviceEnrolled
#include <aduc/config_utils.h> // ADUC_ConfigInfo, ADUC_AgentInfo
#include <aduc/logging.h>
#include <du_agent_sdk/agent_module_interface.h>  // ADUC_AGENT_MODULE_HANDLE

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
    EnrollmentData_SetCorrelationId(enrollmentData, "");
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
    EnrollmentData_SetCorrelationId(data, "");
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

    if (enrollmentData->enrollmentState == ADU_ENROLLMENT_STATE_ENROLLED || ADUC_StateStore_GetIsDeviceEnrolled())
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
        if (context->lastExecutionTime + 30 < nowTime) // timeout after 30 seconds for now
        {
            Log_Debug("Timing out request enrollment: lastExecTime: %d, operationTimeoutSecs: %d, nowTime: %d", context->lastExecutionTime, context->operationTimeoutSecs, nowTime);

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

    ADUC_Enrollment_Request_Operation_Data* enrollmentData = EnrollmentData_FromOperationContext(context);
    ADUC_MQTT_Message_Context* messageContext = &enrollmentData->enrReqMessageContext;

    // Set MQTT 5 user propertie as per ainfo req-res
    if (!ADU_mosquitto_add_user_property(&user_prop_list, "mt", "enr_req") ||
        !ADU_mosquitto_add_user_property(&user_prop_list, "pid", "1"))
    {
        goto done;
    }

    mqtt_res = ADUC_Communication_Channel_MQTT_Publish(
        (ADUC_AGENT_MODULE_HANDLE)context->commChannelHandle,
        messageContext->publishTopic,
        &messageContext->messageId,
        2,
        "{}", // enrollment request message content, which is always an empty json object.
        1, // qos 1 is required for adu gen2 protocol v1
        false,
        user_prop_list);

    Log_Info("PUBLISH 'enr_req' to topic '%s', msgid: %d", messageContext->publishTopic, messageContext->messageId);

    if (mqtt_res != MOSQ_ERR_SUCCESS)
    {
        opSucceeded = false;
        Log_Error(
            "failed to publish enrollment status request message. (mid:%d, cid:%s, err:%d - %s)",
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
            Log_Error("failed to publish (non-recoverable). err:%d", mqtt_res);
            context->cancelFunc(context);
            break;

        case MOSQ_ERR_NO_CONN:

            // compute and apply the next execution time, based on the specified retry parameters.
            context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_CLIENT_TRANSIENT]);

            Log_Error(
                "failed publish (retry-able after t:%ld). err:%d", context->operationIntervalSecs, mqtt_res);
            break;

        default:
            Log_Error("failed publish (unhandled error). err:%d ", mqtt_res);
            // retry again after default retry delay.
            context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_DEFAULT]);
            break;
        }

        goto done;
    }

    EnrollmentData_SetState(enrollmentData, ADU_ENROLLMENT_STATE_REQUESTING, "enr_req submitted");

    context->lastExecutionTime = nowTime;

    Log_Info(
        "PUBLISHING 'enr_req' request (mid:%d, cid:%s, t:%ld, timeout in:%ld)",
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
    ADUC_MQTT_Message_Context* messageContext = NULL;

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

    // at this point, we should send 'enr_req' message.
    enrollmentData = EnrollmentData_FromOperationContext(context);
    messageContext = &enrollmentData->enrReqMessageContext;

    if (SettingUpAduMqttRequestPrerequisites(context, messageContext, false /* isScoped */))
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
    ADUC_Retriable_Operation_Context* ret = NULL;
    ADUC_Retriable_Operation_Context* context = NULL;
    ADUC_MQTT_Message_Context* messageContext = NULL;

    const ADUC_ConfigInfo* config = NULL;
    const ADUC_AgentInfo* agent_info = NULL;
    unsigned int value = 0;
    bool incremented_config_refcount = false;

    ADUC_ALLOC(context);
    ADUC_ALLOC(messageContext);

    context->data = messageContext;
    messageContext = NULL; // transfer ownership

    ADUC_Retriable_Operation_Init(context, false);

    // initialize custom functions
    context->dataDestroyFunc = EnrollmentRequestOperation_DataDestroy;
    context->operationDestroyFunc = EnrollmentRequestOperation_OperationDestroy;
    context->doWorkFunc = EnrollmentStatusRequestOperation_doWork;
    context->cancelFunc = EnrollmentRequestOperation_CancelOperation;
    context->retryFunc = EnrollmentRequestOperation_DoRetry;
    context->completeFunc = EnrollmentRequestOperation_Complete;
    context->retryParamsCount = RetryUtils_GetRetryParamsMapSize();

    ADUC_ALLOC_BLOCK(context->retryParams, 1, sizeof(*context->retryParams) * (size_t)context->retryParamsCount);

    // initialize callbacks
    context->onExpired = EnrollmentRequestOperation_OnExpired;
    context->onSuccess = EnrollmentRequestOperation_OnSuccess;
    context->onFailure = EnrollmentRequestOperation_OnFailure;
    context->onRetry =
        EnrollmentRequestOperation_OnRetry; // read or retry strategy parameters from the agent's configuration file.

    config = ADUC_ConfigInfo_GetInstance();
    if (config == NULL)
    {
        Log_Error("failed to get config instance");
        goto done;
    }
    incremented_config_refcount = true;

    agent_info = ADUC_ConfigInfo_GetAgent(config, 0);
    if (agent_info == NULL)
    {
        Log_Error("failed to get agent info");
        goto done;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(agent_info, SETTING_KEY_ENR_REQ_OP_INTERVAL_SECONDS, &value))
    {
        log_warn("failed to get enrollment status request interval setting");
        value = DEFAULT_ENR_REQ_OP_INTERVAL_SECONDS;
    }
    context->operationIntervalSecs = value;

    value = 0;
    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(agent_info, SETTING_KEY_ENR_REQ_OP_TIMEOUT_SECONDS, &value))
    {
        log_warn("failed to get enrollment status request timeout setting");
        value = DEFAULT_ENR_REQ_OP_TIMEOUT_SECONDS;
    }
    context->operationTimeoutSecs = value;

    // get retry parameters for transient client error.
    JSON_Value* retrySettings = json_object_dotget_value(
        json_value_get_object(agent_info->agentJsonValue), SETTING_KEY_ENR_REQ_OP_RETRY_PARAMS);
    if (retrySettings == NULL)
    {
        Log_Error("failed to get retry settings");
        goto done;
    }

    ReadRetryParamsArrayFromAgentConfigJson(context, retrySettings, RetryUtils_GetRetryParamsMapSize());

    // ADUC_ALLOC(context->data);

    ret = context;
    context = NULL; // transfer ownership

done:

    free(messageContext);
    free(context);

    if (config != NULL && incremented_config_refcount)
    {
        ADUC_ConfigInfo_ReleaseInstance(config);
    }

    return ret;
}
