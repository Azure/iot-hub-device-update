/**
 * @file adu_enrollment_management.c
 * @brief Implementation for device enrollment status management.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_communication_channel.h"
#include "aduc/adu_enrollment_utils.h"
#include "aduc/adu_enrollment_management.h"
#include "aduc/adu_mosquitto_utils.h"
#include "aduc/adu_mqtt_protocol.h"
#include "aduc/adu_types.h"
#include "aduc/agent_state_store.h"
#include "aduc/config_utils.h"
#include "aduc/adu_enrollment.h"
#include "aduc/logging.h"
#include "aduc/retry_utils.h" // ADUC_GetTimeSinceEpochInSeconds
#include "aduc/string_c_utils.h" // IsNullOrEmpty
#include "aducpal/time.h" // time_t

#include "parson.h" // JSON_Value, JSON_Object
#include "parson_json_utils.h" // ADUC_JSON_Get*

#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h>
#include <stdint.h> // INT32_MAX
#include <string.h>

/// @brief The interval in seconds between enrollment status request operations. Default is 1 hour.
#define DEFAULT_ENR_REQ_OP_INTERVAL_SECONDS (60 * 60)
#define SETTING_KEY_ENR_REQ_OP_INTERVAL_SECONDS "operations.enrollment.statusRequest.intervalSeconds"
#define DEFAULT_ENR_REQ_OP_TIMEOUT_SECONDS (60 * 60 * 24)
#define SETTING_KEY_ENR_REQ_OP_TIMEOUT_SECONDS "operations.enrollment.statusRequest.timeoutSeconds"

#define SETTING_KEY_ENR_REQ_OP_RETRY_PARAMS "operations.enrollment.statusRequest.retrySettings"

#define SETTING_KEY_ENR_REQ_OP_TRACE_LEVEL "operations.enrollment.traceLevel"

#define DEFAULT_ENR_REQ_OP_MAX_RETRIES (INT32_MAX)
#define SETTING_KEY_ENR_REQ_OP_MAX_RETRIES "maxRetries"

#define SETTING_KEY_ENR_REQ_OP_MAX_WAIT_SECONDS "maxDelaySeconds"

#define DEFAULT_ENR_REQ_OP_FALLBACK_WAITTIME_SECONDS (60)
#define SETTING_KEY_ENR_REQ_OP_FALLBACK_WAITTIME_SECONDS "fallbackWaitTimeSeconds"

#define DEFAULT_ENR_REQ_OP_INITIAL_DELAY_MILLISECONDS (0)
#define SETTING_KEY_ENR_REQ_OP_INITIAL_DELAY_MILLISECONDS "initialDelayUnitMS"

#define DEFAULT_ENR_REQ_OP_MAX_JITTER_PERCENT (60)
#define SETTING_KEY_ENR_REQ_OP_MAX_JITTER_PERCENT "maxJitterPercent"

// Enrollment request message content, which is always an empty JSON object.
static const char* s_enr_req_message_content = "{}";
static const int s_enr_req_message_content_len = 2;

// Forward declarations
const ADUC_AGENT_CONTRACT_INFO* ADUC_Enrollment_Management_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle);
int ADUC_Enrollment_Management_DoWork(ADUC_AGENT_MODULE_HANDLE handle);
int ADUC_Enrollment_Management_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* initData);
int ADUC_Enrollment_Management_Deinitialize(ADUC_AGENT_MODULE_HANDLE handle);

#define CONST_TIME_5_SECONDS 5
#define CONST_TIME_30_SECONDS 30

time_t s_lastHeartBeatLogTime = 0;

enum ADUC_RETRY_PARAMS_INDEX
{
    ADUC_RETRY_PARAMS_INDEX_DEFAULT = 0,
    ADUC_RETRY_PARAMS_INDEX_CLIENT_TRANSIENT = 1,
    ADUC_RETRY_PARAMS_INDEX_CLIENT_UNRECOVERABLE = 2,
    ADUC_RETRY_PARAMS_INDEX_SERVICE_TRANSIENT = 3,
    ADUC_RETRY_PARAMS_INDEX_SERVICE_UNRECOVERABLE = 4
};

typedef struct tagADUC_RETRY_PARAMS_MAP
{
    const char* name;
    int index;
} ADUC_RETRY_PARAMS_MAP;

static const ADUC_RETRY_PARAMS_MAP s_retryParamsMap[] = {
    { "default", ADUC_RETRY_PARAMS_INDEX_DEFAULT },
    { "clientTransient", ADUC_RETRY_PARAMS_INDEX_CLIENT_TRANSIENT },
    { "clientUnrecoverable", ADUC_RETRY_PARAMS_INDEX_CLIENT_UNRECOVERABLE },
    { "serviceTransient", ADUC_RETRY_PARAMS_INDEX_SERVICE_TRANSIENT },
    { "serviceUnrecoverable", ADUC_RETRY_PARAMS_INDEX_SERVICE_UNRECOVERABLE }
};

static const int s_retryParamsMapSize = sizeof(s_retryParamsMap) / sizeof(s_retryParamsMap[0]);

/**
 * @brief Free memory allocated for the enrollment data.
 */
void EnrollmentData_Deinit(ADUC_Enrollment_Request_Operation_Data* data)
{
    // Currently nothing to be freed. Implement this for completeness and future improvement.
    IGNORED_PARAMETER(data);
}

static int json_print_properties(const mosquitto_property* properties)
{
    int identifier;
    uint8_t i8value = 0;
    uint16_t i16value = 0;
    uint32_t i32value = 0;
    char *strname = NULL, *strvalue = NULL;
    char* binvalue = NULL;
    const mosquitto_property* prop = NULL;

    for (prop = properties; prop != NULL; prop = mosquitto_property_next(prop))
    {
        identifier = mosquitto_property_identifier(prop);
        switch (identifier)
        {
        case MQTT_PROP_PAYLOAD_FORMAT_INDICATOR:
            mosquitto_property_read_byte(prop, MQTT_PROP_PAYLOAD_FORMAT_INDICATOR, &i8value, false);
            break;

        case MQTT_PROP_MESSAGE_EXPIRY_INTERVAL:
            mosquitto_property_read_int32(prop, MQTT_PROP_MESSAGE_EXPIRY_INTERVAL, &i32value, false);
            break;

        case MQTT_PROP_CONTENT_TYPE:
        case MQTT_PROP_RESPONSE_TOPIC:
            mosquitto_property_read_string(prop, identifier, &strvalue, false);
            if (strvalue == NULL)
                return MOSQ_ERR_NOMEM;
            free(strvalue);
            strvalue = NULL;
            break;

        case MQTT_PROP_CORRELATION_DATA:
            mosquitto_property_read_binary(prop, MQTT_PROP_CORRELATION_DATA, (void**)&binvalue, &i16value, false);
            if (binvalue == NULL)
                return MOSQ_ERR_NOMEM;
            free(binvalue);
            binvalue = NULL;
            break;

        case MQTT_PROP_SUBSCRIPTION_IDENTIFIER:
            mosquitto_property_read_varint(prop, MQTT_PROP_SUBSCRIPTION_IDENTIFIER, &i32value, false);
            break;

        case MQTT_PROP_TOPIC_ALIAS:
            mosquitto_property_read_int16(prop, MQTT_PROP_TOPIC_ALIAS, &i16value, false);
            break;

        case MQTT_PROP_USER_PROPERTY:
            mosquitto_property_read_string_pair(prop, MQTT_PROP_USER_PROPERTY, &strname, &strvalue, false);
            if (strname == NULL || strvalue == NULL)
                return MOSQ_ERR_NOMEM;

            free(strvalue);

            free(strname);
            strname = NULL;
            strvalue = NULL;
            break;
        }
    }
    return MOSQ_ERR_SUCCESS;
}

/**
 * @brief The contract info for the module.
 */
static ADUC_AGENT_CONTRACT_INFO s_moduleContractInfo = {
    "Microsoft", "Device Update Enrollment Module", 1, "Microsoft/DUEnrollmentModule:1"
};

/**
 * @brief Gets the extension contract info.
 * @param handle The handle to the module. This is the same handle that was returned by the Create function.
 * @return ADUC_AGENT_CONTRACT_INFO The extension contract info.
 */
const ADUC_AGENT_CONTRACT_INFO* ADUC_Enrollment_Management_GetContractInfo(ADUC_AGENT_MODULE_HANDLE handle)
{
    IGNORED_PARAMETER(handle);
    return &s_moduleContractInfo;
}

/**
 * @brief Callback should be called when the client receives an enrollment status response message from the Device Update service.
 *  For 'enr_resp' messages, if the Correlation Data matches the 'en,the client should parse the message and update the enrollment state.
 *
 * @param mosq The mosquitto instance making the callback.
 * @param obj The user data provided in mosquitto_new. This is the module state
 * @param msg The message data.
 * @param props The MQTT v5 properties returned with the message.
 *
 * @remark This callback is called by the network thread. Usually by the same thread that calls mosquitto_loop function.
 * IMPORTANT - Do not use blocking functions in this callback.
 */
void OnMessage_enr_resp(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    bool isEnrolled = false;
    const char* scopeId = NULL;
    JSON_Value* root_value = NULL;

    ADUC_Retriable_Operation_Context* context = (ADUC_Retriable_Operation_Context*)obj;
    ADUC_MQTT_Message_Context* messageContext = (ADUC_MQTT_Message_Context*)context->data;
    ADUC_Enrollment_Request_Operation_Data* enrollmentData = EnrollmentData_FromOperationContext(context);

    json_print_properties(props);

    // BUG: Can't get correlation id from the message props`.
    if (!ADU_are_correlation_ids_matching(props, messageContext->correlationId))
    {
        Log_Info("OnMessage_enr_resp: correlation data mismatch");
        goto done;
    }

    if (msg == NULL || msg->payload == NULL || msg->payloadlen == 0)
    {
        Log_Error("Bad payload. msg:%p, payload:%p, payloadlen:%d", msg, msg->payload, msg->payloadlen);
        goto done;
    }

    // Parse the JSON payload using Parson
    root_value = json_parse_string(msg->payload);

    if (root_value == NULL)
    {
        Log_Error("Failed to parse JSON");
        goto done;
    }

    if (!ADUC_JSON_GetBooleanField(root_value, "IsEnrolled", &isEnrolled))
    {
        Log_Error("Failed to get 'isEnrolled' from payload");
        goto done;
    }

    scopeId = ADUC_JSON_GetStringFieldPtr(root_value, "ScopeId");
    if (scopeId == NULL)
    {
        Log_Error("Failed to get 'ScopeId' from payload");
        goto done;
    }

    // TODO: Read 'resultcode', 'extendedresultcode' and 'resultdescription' from the response message properties.

    if (!handle_enrollment(
        enrollmentData,
        isEnrolled,
        scopeId,
        context))
    {
        Log_Error("Failed handling enrollment side effects.");
        goto done;
    }

done:
    json_value_free(root_value);
    if (!isEnrolled)
    {
        // Retry again after default retry delay.
    }
}

/**
 * @brief Callback should be called when the client receives an enrollment status change notification message from the Device Update service.
 */
void OnMessage_enr_cn(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    // NOTE: NOT SUPPORTED IN V1 PROTOCOL - MVP
    Log_Error("OnMessage_enr_cn: NOT SUPPORTED IN V1 PROTOCOL - MVP");
}

/*
 * @brief This callback is called when the client successfully sent to the broker.
 *
 * @param mosq The mosquitto instance making the callback.
 * @param obj The user data provided in mosquitto_new. This is the module state
 * @param mid The message id of the published message.
 * @param reason_code The reason code of the publish.
 * @param props The MQTT v5 properties returned with the message.
 *
 * @remark This callback is called by the network thread. Usually by the same thread that calls mosquitto_loop function.
 * IMPORTANT - Do not use blocking functions in this callback.
 *
 * @remark This callback is called when the message has been successfully sent to the broker.
 * It does not indicate the message has been successfully received by the broker.
 *
 * @ref https://mosquitto.org/api/files/mosquitto-h.html#mosquitto_publish_v5_callback
 */
void OnPublished_enr_req(struct mosquitto* mosq, void* obj, int mid, int reason_code, const mosquitto_property* props)
{
    Log_Info("'enr_req' sent. mid:%d", mid);
}

/**
 *  @brief The main workflow for managing device enrollment status request operation.
 *      The workflow is as follows:
 *      1. Subscribe to the enrollment status response topic.
 *      2. Send enrollment status request message.
 *      3. Wait for enrollment status response message.
 *        3.1 If the response message correlation-data doesn't match the latest request, ignore the message.
 *        3.2 If the response message correlation-data matches the latest request, parse the message and update the enrollment state.
 *          3.2.1 If the enrollment state is enrolled, stop the the operation.
 *          3.2.2 If the enrollment state is not enrolled, retry the operation after the retry delay.
 */
bool EnrollmentStatusRequestOperation_DoWork(ADUC_Retriable_Operation_Context* context)
{
    bool opSucceeded = false;
    if (context == NULL || context->data == NULL)
    {
        Log_Error("Null input (context:%p, data:%p)", context, context->data);
        return opSucceeded;
    }

    time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();

    // Get enrollment operation data from the context.
    ADUC_Enrollment_Request_Operation_Data* enrollmentData = EnrollmentData_FromOperationContext(context);
    ADUC_MQTT_Message_Context* messageContext = &enrollmentData->enrReqMessageContext;
    const char* externalDeviceId = NULL;
    int messageId = 0;
    int mqtt_res = 0;

    // If the enrollment state received, stop the operation.
    if (enrollmentData->enrollmentState == ADU_ENROLLMENT_STATE_ENROLLED || ADUC_StateStore_GetIsDeviceEnrolled())
    {
        // Enrollment is completed.
        Log_Info("Enrollment completed");
        ADUC_Retriable_Set_State(context, ADUC_Retriable_Operation_State_Completed);

        if (s_lastHeartBeatLogTime + CONST_TIME_30_SECONDS < nowTime)
        {
            Log_Debug("enr_state:%d", enrollmentData->enrollmentState);
            s_lastHeartBeatLogTime = nowTime;
        }

        opSucceeded = true;
        goto done;
    }

    // Heart beat while waiting for enrollment completion.
    if (s_lastHeartBeatLogTime + CONST_TIME_5_SECONDS < nowTime)
    {
        Log_Info("enr_state :%d", enrollmentData->enrollmentState);
        s_lastHeartBeatLogTime = nowTime;
    }

    // Are we querying the enrollment status?
    if (enrollmentData->enrollmentState == ADU_ENROLLMENT_STATE_REQUESTING)
    {
        ADUC_Retriable_Set_State(context, ADUC_Retriable_Operation_State_InProgress);

        // Is the current request timed-out?
        if (context->lastExecutionTime + context->operationTimeoutSecs < nowTime)
        {
            context->cancelFunc(context);

            // REVIEW: do we need this? CancelFunc should already took care of this.
            Log_Info("Enrollment request timed-out");
            EnrollmentData_SetCorrelationId(enrollmentData, "");
            EnrollmentData_SetState(enrollmentData, ADU_ENROLLMENT_STATE_UNKNOWN, "timed out");
            context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_SERVICE_TRANSIENT]);
        }

        // Exit and wait for next 'do work' call.
        goto done;
    }

    // At this point, we should send 'enr_req' message.

    // Ensure that the device is registered with the IoT cloud service (e.g., DPS, Azure Device Registry)
    if (!ADUC_StateStore_GetIsDeviceRegistered())
    {
        Log_Info("Device is not registered. Will retry");
        context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_DEFAULT]);
        goto done;
    }

    // This operation is depending on the "DUServiceCommunicationChannel".
    // Note: by default, the DU Service communication already subscribed to the common service-to-device messaging topic.
    if (context->commChannelHandle == NULL)
    {
        context->commChannelHandle =
            ADUC_StateStore_GetCommunicationChannelHandle(ADUC_DU_SERVICE_COMMUNICATION_CHANNEL_ID);
    }

    if (context->commChannelHandle == NULL)
    {
        Log_Info("Communication channel is not ready. Will retry");
        context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_DEFAULT]);
        goto done;
    }

    // And ensure that we have a valid external device id. This should usually provided by a DPS.
    externalDeviceId = ADUC_StateStore_GetExternalDeviceId();
    if (IsNullOrEmpty(externalDeviceId))
    {
        Log_Info("An external device id is not available. Will retry");
        context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_DEFAULT]);
        goto done;
    }

    // Prepare a topic for the enrollment status response.
    if (IsNullOrEmpty(messageContext->publishTopic))
    {
        // Initialize the publish topic.
        messageContext->publishTopic = ADUC_StringFormat(PUBLISH_TOPIC_TEMPLATE_ADU_OTO, externalDeviceId);
        if (messageContext->publishTopic == NULL)
        {
            Log_Error("Failed to allocate memory for publish topic. Cancelling the operation.");
            context->cancelFunc(context);
            goto done;
        }
    }

    // Prepare a topic for the response subscription.
    if (IsNullOrEmpty(messageContext->responseTopic))
    {
        // Initialize the response topic.
        messageContext->responseTopic = ADUC_StringFormat(SUBSCRIBE_TOPIC_TEMPLATE_ADU_OTO, externalDeviceId);
        if (messageContext->responseTopic == NULL)
        {
            Log_Error("Failed to allocate memory for response topic. Cancelling the operation.");
            context->cancelFunc(context);
            goto done;
        }
    }

    // Ensure that we have subscribed for the response.
    if (!ADUC_Communication_Channel_MQTT_IsSubscribed(
            (ADUC_AGENT_MODULE_HANDLE)context->commChannelHandle, messageContext->responseTopic))
    {
        // Subscribe to the response topic.
        int subscribeMessageId = 0;
        int mqtt_res = ADUC_Communication_Channel_MQTT_Subscribe(
            (ADUC_AGENT_MODULE_HANDLE)context->commChannelHandle,
            messageContext->responseTopic,
            &subscribeMessageId,
            messageContext->qos,
            0 /*options*/,
            NULL /*props*/,
            NULL /*userData*/,
            NULL /*callback*/);
        if (mqtt_res != MOSQ_ERR_SUCCESS)
        {
            Log_Error("Failed to subscribe to response topic. Cancelling the operation.");
            context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_CLIENT_TRANSIENT]);
            goto done;
        }

        Log_Info(
            "Subscribing to response topic '%s', messageId:%d", messageContext->responseTopic, subscribeMessageId);
    }

    // Send enrollment status request message.
    messageId = 0;
    mqtt_res = ADUC_Communication_Channel_MQTT_Publish(
        (ADUC_AGENT_MODULE_HANDLE)context->commChannelHandle,
        messageContext->publishTopic,
        &messageContext->messageId,
        s_enr_req_message_content_len,
        s_enr_req_message_content,
        messageContext->qos,
        false,
        messageContext->properties);

    if (mqtt_res != MOSQ_ERR_SUCCESS)
    {
        opSucceeded = false;
        Log_Error(
            "Failed to publish enrollment status request message. (mid:%d, cid:%s, err:%d - %s)",
            messageId,
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
            // Following error is non-recoverable, so we'll bail out.
            Log_Error("Failed to publish (non-recoverable). Err:%d", mqtt_res);
            context->cancelFunc(context);
            break;

        case MOSQ_ERR_NO_CONN:

            // Compute and apply the next execution time, based on the specified retry parameters.
            context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_CLIENT_TRANSIENT]);

            Log_Error(
                "Failed to publish (retry-able after t:%ld). Err:%d", context->operationIntervalSecs, mqtt_res);
            break;

        default:
            Log_Error("Failed to publish (unknown error). Err:%d ", mqtt_res);
            // Retry again after default retry delay.
            context->retryFunc(context, &context->retryParams[ADUC_RETRY_PARAMS_INDEX_DEFAULT]);
            break;
        }

        goto done;
    }

    EnrollmentData_SetState(
        EnrollmentData_FromOperationContext(context), ADU_ENROLLMENT_STATE_REQUESTING, "enr_req submitted");
    context->lastExecutionTime = nowTime;
    Log_Info(
        "Submitting 'enr_req' (mid:%d, cid:%s, t:%ld, timeout in:%ld)",
        messageId,
        messageContext->correlationId,
        context->lastExecutionTime,
        context->operationTimeoutSecs);
    opSucceeded = true;
done:

    return opSucceeded;
}

bool EnrollmentRequestOperation_CancelOperation(ADUC_Retriable_Operation_Context* context)
{
    ADUC_Enrollment_Request_Operation_Data* enrollmentData = EnrollmentData_FromOperationContext(context);
    if (enrollmentData == NULL)
    {
        Log_Error("Null input (context:%p, data:%p)", context, enrollmentData);
        return false;
    }

    // Set the correlation id to NULL, so we can discard all inflight messages.
    EnrollmentData_SetCorrelationId(enrollmentData, "");
    EnrollmentData_SetState(enrollmentData, ADU_ENROLLMENT_STATE_UNKNOWN, "Cancel requested");
    return true;
}

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
        Log_Error("Null input (context:%p, data:%p)", context, data);
        return false;
    }

    EnrollmentData_SetState(data, ADU_ENROLLMENT_STATE_UNKNOWN, "retrying");
    EnrollmentData_SetCorrelationId(data, "");
    return true;
}

const ADUC_Retry_Params* GetRetryParamsForLastErrorClass(ADUC_Retriable_Operation_Context* context)
{
    if (context == NULL)
    {
        Log_Error("Null context");
        return NULL;
    }

    // TODO: return retry params based on the error class.

    // use default retry params.
    return &context->retryParams[ADUC_RETRY_PARAMS_INDEX_DEFAULT];
}

void EnrollmentRequestOperation_UpdateNextAttemptTime(ADUC_Retriable_Operation_Context* context)
{
    if (context == NULL)
    {
        Log_Error("Null context");
        return;
    }

    const ADUC_Retry_Params* retryParams = GetRetryParamsForLastErrorClass(context);
    if (retryParams == NULL)
    {
        Log_Error("Failed to get retry params for last error class");
        return;
    }

    if (context->attemptCount >= retryParams->maxRetries)
    {
        Log_Info("Max retry count reached. Cancelling the operation.");
        context->cancelFunc(context);
        return;
    }

    time_t nextAttemptTime = ADUC_Retry_Delay_Calculator(
        0 /* no additional delay */,
        context->attemptCount,
        retryParams->initialDelayUnitMilliSecs,
        retryParams->maxDelaySecs,
        retryParams->maxJitterPercent);

    context->nextExecutionTime = nextAttemptTime;
    context->attemptCount++;
}

void EnrollmentRequestOperation_DataDestroy(ADUC_Retriable_Operation_Context* context)
{
    Log_Info("Destroying context data. (context:%p, data:%p)", context, context->data);
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
    Log_Info("Destroying operation context. (context:%p)", context);
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

void EnrollmentRequestOperation_OnSuccess(ADUC_Retriable_Operation_Context* data)
{
    ADUC_Retriable_Set_State(data, ADUC_Retriable_Operation_State_Completed);
}

void EnrollmentRequestOperation_OnFailure(ADUC_Retriable_Operation_Context* data)
{
    ADUC_Retriable_Set_State(data, ADUC_Retriable_Operation_State_Failure);
}

void EnrollmentRequestOperation_OnRetry(ADUC_Retriable_Operation_Context* data)
{
    // Compute next attempt time.
}

static ADUC_Retriable_Operation_Context* OperationContextFromAgentModuleHandle(ADUC_AGENT_MODULE_HANDLE handle)
{
    ADUC_AGENT_MODULE_INTERFACE* moduleInterface = (ADUC_AGENT_MODULE_INTERFACE*)handle;
    if (moduleInterface == NULL || moduleInterface->moduleData == NULL)
    {
        return NULL;
    }

    return (ADUC_Retriable_Operation_Context*)moduleInterface->moduleData;
}

void ReadRetryParamsArrayFromAgentConfigJson(ADUC_Retriable_Operation_Context* context, JSON_Value* agentJsonValue)
{
    const char* infoFormatString = "Failed to read '%s.%s' from agent config. Using default value (%d)";
    for (int i = 0; i < s_retryParamsMapSize; i++)
    {
        ADUC_Retry_Params* params = &context->retryParams[i];
        JSON_Value* retryParams = json_object_dotget_value(json_object(agentJsonValue), s_retryParamsMap[i].name);
        if (retryParams == NULL)
        {
            Log_Info("Retry params for '%s' is not specified. Using default values.", s_retryParamsMap[i].name);
            params->maxRetries = DEFAULT_ENR_REQ_OP_MAX_RETRIES;
            params->maxDelaySecs = context->operationIntervalSecs;
            params->fallbackWaitTimeSec = context->operationIntervalSecs;
            params->initialDelayUnitMilliSecs = DEFAULT_ENR_REQ_OP_INITIAL_DELAY_MILLISECONDS;
            params->maxJitterPercent = DEFAULT_ENR_REQ_OP_MAX_JITTER_PERCENT;
            continue;
        }

        if (!ADUC_JSON_GetUnsignedIntegerField(retryParams, SETTING_KEY_ENR_REQ_OP_MAX_RETRIES, &params->maxRetries))
        {
            Log_Info(
                infoFormatString,
                s_retryParamsMap[i].name,
                SETTING_KEY_ENR_REQ_OP_MAX_RETRIES,
                DEFAULT_ENR_REQ_OP_MAX_RETRIES);
            params->maxRetries = DEFAULT_ENR_REQ_OP_MAX_RETRIES;
        }

        if (!ADUC_JSON_GetUnsignedIntegerField(
                retryParams, SETTING_KEY_ENR_REQ_OP_MAX_WAIT_SECONDS, &params->maxDelaySecs))
        {
            Log_Info(
                infoFormatString,
                s_retryParamsMap[i].name,
                SETTING_KEY_ENR_REQ_OP_MAX_WAIT_SECONDS,
                context->operationIntervalSecs);
            params->maxDelaySecs = context->operationIntervalSecs;
        }

        if (!ADUC_JSON_GetUnsignedIntegerField(
                retryParams, SETTING_KEY_ENR_REQ_OP_FALLBACK_WAITTIME_SECONDS, &params->fallbackWaitTimeSec))
        {
            Log_Info(
                infoFormatString,
                s_retryParamsMap[i].name,
                SETTING_KEY_ENR_REQ_OP_FALLBACK_WAITTIME_SECONDS,
                context->operationIntervalSecs);
            params->fallbackWaitTimeSec = context->operationIntervalSecs;
        }

        if (!ADUC_JSON_GetUnsignedIntegerField(
                retryParams, SETTING_KEY_ENR_REQ_OP_INITIAL_DELAY_MILLISECONDS, &params->initialDelayUnitMilliSecs))
        {
            Log_Info(
                infoFormatString,
                s_retryParamsMap[i].name,
                SETTING_KEY_ENR_REQ_OP_INITIAL_DELAY_MILLISECONDS,
                DEFAULT_ENR_REQ_OP_INITIAL_DELAY_MILLISECONDS);
            params->initialDelayUnitMilliSecs = DEFAULT_ENR_REQ_OP_INITIAL_DELAY_MILLISECONDS;
        }

        unsigned int value = 0;
        if (!ADUC_JSON_GetUnsignedIntegerField(retryParams, SETTING_KEY_ENR_REQ_OP_MAX_JITTER_PERCENT, &value))
        {
            Log_Info(
                infoFormatString,
                s_retryParamsMap[i].name,
                SETTING_KEY_ENR_REQ_OP_MAX_JITTER_PERCENT,
                DEFAULT_ENR_REQ_OP_MAX_JITTER_PERCENT);
            value = DEFAULT_ENR_REQ_OP_MAX_JITTER_PERCENT;
        }
        params->maxJitterPercent = (double)value;
    }
}

ADUC_Retriable_Operation_Context* CreateAndInitializeEnrollmentRequestOperation()
{
    ADUC_Retriable_Operation_Context* ret = NULL;
    ADUC_Retriable_Operation_Context* context = NULL;
    ADUC_MQTT_Message_Context* messageContext = NULL;

    const ADUC_ConfigInfo* config = NULL;
    const ADUC_AgentInfo* agent_info = NULL;
    unsigned int value = 0;

    context = malloc(sizeof(ADUC_Retriable_Operation_Context));
    if (context == NULL)
    {
        goto done;
    }
    memset(context, 0, sizeof(ADUC_Retriable_Operation_Context));

    messageContext = malloc(sizeof(*messageContext));
    if (messageContext == NULL)
    {
        goto done;
    }
    memset(messageContext, 0, sizeof(*messageContext));

    context->data = messageContext;

    ADUC_Retriable_Operation_Init(context, false);

    // Initialize custom functions
    context->doWorkFunc = EnrollmentStatusRequestOperation_DoWork;
    context->cancelFunc = EnrollmentRequestOperation_CancelOperation;
    context->dataDestroyFunc = EnrollmentRequestOperation_DataDestroy;
    context->operationDestroyFunc = EnrollmentRequestOperation_OperationDestroy;
    context->retryFunc = EnrollmentRequestOperation_DoRetry;
    context->completeFunc = EnrollmentRequestOperation_Complete;
    context->retryParamsCount = s_retryParamsMapSize;
    context->retryParams = malloc(sizeof(*context->retryParams) * (size_t)context->retryParamsCount);
    if (context->retryParams == NULL)
    {
        goto done;
    }

    // Initialize callbacks
    context->onExpired = EnrollmentRequestOperation_OnExpired;
    context->onSuccess = EnrollmentRequestOperation_OnSuccess;
    context->onFailure = EnrollmentRequestOperation_OnFailure;
    context->onRetry =
        EnrollmentRequestOperation_OnRetry; // Read or retry strategy parameters from the agent's configuration file.
    config = ADUC_ConfigInfo_GetInstance();
    if (config == NULL)
    {
        Log_Error("Failed to get config instance");
        goto done;
    }

    agent_info = ADUC_ConfigInfo_GetAgent(config, 0);
    if (agent_info == NULL)
    {
        Log_Error("Failed to get agent info");
        goto done;
    }

    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(agent_info, SETTING_KEY_ENR_REQ_OP_INTERVAL_SECONDS, &value))
    {
        Log_Warn("Failed to get enrollment status request interval setting");
        value = DEFAULT_ENR_REQ_OP_INTERVAL_SECONDS;
    }
    context->operationIntervalSecs = value;

    value = 0;
    if (!ADUC_AgentInfo_ConnectionData_GetUnsignedIntegerField(agent_info, SETTING_KEY_ENR_REQ_OP_TIMEOUT_SECONDS, &value))
    {
        Log_Warn("Failed to get enrollment status request timeout setting");
        value = DEFAULT_ENR_REQ_OP_TIMEOUT_SECONDS;
    }
    context->operationTimeoutSecs = value;

    // Get retry parameters for transient client error.
    JSON_Value* retrySettings = json_object_dotget_value(
        json_value_get_object(agent_info->agentJsonValue), "operations.enrollment.statusRequest.retrySettings");
    if (retrySettings == NULL)
    {
        Log_Error("Failed to get retry settings");
        goto done;
    }

    ReadRetryParamsArrayFromAgentConfigJson(context, retrySettings);

    ret = context;

done:

    if (ret == NULL)
    {
        ADUC_ConfigInfo_ReleaseInstance(config);
        free(context);
    }

    return ret;
}

void ADUC_Enrollment_Management_Destroy(ADUC_AGENT_MODULE_HANDLE handle)
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

ADUC_AGENT_MODULE_HANDLE ADUC_Enrollment_Management_Create()
{
    ADUC_AGENT_MODULE_INTERFACE* interface = NULL;
    ADUC_Retriable_Operation_Context* operationContext = CreateAndInitializeEnrollmentRequestOperation();
    bool success = false;

    if (operationContext == NULL)
    {
        Log_Error("Failed to create enrollment request operation");
        goto done;
    }

    operationContext->data = malloc(sizeof(ADUC_Enrollment_Request_Operation_Data));
    if (operationContext->data == NULL)
    {
        goto done;
    }
    memset(operationContext->data, 0, sizeof(ADUC_Enrollment_Request_Operation_Data));

    interface = malloc(sizeof(ADUC_AGENT_MODULE_INTERFACE));
    if (interface == NULL)
    {
        goto done;
    }

    if (memset(interface, 0, sizeof(ADUC_AGENT_MODULE_INTERFACE)) == NULL)
    {
        goto done;
    }

    interface->getContractInfo = ADUC_Enrollment_Management_GetContractInfo;
    interface->initializeModule = ADUC_Enrollment_Management_Initialize;
    interface->deinitializeModule = ADUC_Enrollment_Management_Deinitialize;
    interface->doWork = ADUC_Enrollment_Management_DoWork;
    interface->destroy = ADUC_Enrollment_Management_Destroy;
    interface->moduleData = operationContext;

    success = true;
done:

    if (!success)
    {
        // Failed to initialize.
        free(interface);
        interface = NULL;
        if (operationContext != NULL)
        {
            operationContext->dataDestroyFunc(operationContext);
            operationContext->operationDestroyFunc(operationContext);
        }
    }

    return (ADUC_AGENT_MODULE_HANDLE)(interface);
}

/**
 * @brief Initialize the enrollment management.
 * @return true if the enrollment management was successfully initialized; otherwise, false.
 */
int ADUC_Enrollment_Management_Initialize(ADUC_AGENT_MODULE_HANDLE handle, void* initData)
{
    ADUC_Retriable_Operation_Context* context = OperationContextFromAgentModuleHandle(handle);
    if (context == NULL)
    {
        Log_Error("Failed to get operation context");
        return -1;
    }

    return 0;
}

/**
 * @brief Deinitialize the enrollment management.
 */
int ADUC_Enrollment_Management_Deinitialize(ADUC_AGENT_MODULE_HANDLE handle)
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
 * @brief Ensure that the device is enrolled with the Device Update service.
 */
int ADUC_Enrollment_Management_DoWork(ADUC_AGENT_MODULE_HANDLE handle)
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
