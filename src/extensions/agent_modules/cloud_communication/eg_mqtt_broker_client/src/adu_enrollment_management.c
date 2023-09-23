/**
 * @file adu_enrollment_management.c
 * @brief Implementation for device enrollment status management.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_communication_channel.h"
#include "aduc/adu_mosquitto_utils.h"
#include "aduc/adu_mqtt_protocol.h"
#include "aduc/adu_types.h"
#include "aduc/config_utils.h"
#include "aduc/logging.h"
#include "aduc/retry_utils.h" // ADUC_GetTimeSinceEpochInSeconds
#include "aduc/string_c_utils.h" // IsNullOrEmpty
#include "aducpal/time.h" // time_t

#include "parson.h" // JSON_Value, JSON_Object

#include <mosquitto.h> // mosquitto related functions
#include <mqtt_protocol.h>
#include <string.h>

// Default timeout for enrollment request message - 1 hour.
// This can be overridden by the specifying value in du-config.json
// e.g.,
//   "agent.enrollmentSettings.enrollmentRequestTimeoutSeconds": 3600
#define DEFAULT_ENR_REQ_TIMEOUT_SECONDS (60 * 60)

// Default minimum retry delay for enrollment request message - 5 minutes.
#define DEFAULT_ENR_REQ_MIN_RETRY_DELAY_SECONDS (60 * 5)

// Enrollment request message content, which is always an empty JSON object.
static const char* s_enr_req_message_content = "{}";
static const size_t s_enr_req_message_content_len = 2;
static char* s_mqtt_topic_a2s_oto = NULL;

#define CONST_TIME_5_SECONDS 5
#define CONST_TIME_30_SECONDS 30

#if _ADU_DEBUG
time_t s_lastHeartBeatLogTime = 0;
#endif

/**
 * @struct ADU_ENROLLMENT_DATA_TAG
 * @brief Struct to represent enrollment status management data for an Azure Device Update (ADU) enrollment.
 */
typedef struct ADU_ENROLLMENT_DATA_TAG
{
    /// Current enrollment state.
    ADU_ENROLLMENT_STATE enrollmentState;

    /// Time of the last attempt to enroll.
    time_t enr_req_lastAttemptTime;

    /// Time of the last enrollment error.
    time_t enr_req_lastErrorTime;

    /// Time of the last successful enrollment.
    time_t enr_req_lastSuccessTime;

    /// Time of the next enrollment attempt.
    time_t enr_req_nextAttemptTime;

    /// Timeout duration in seconds for enrollment attempts.
    unsigned int enr_req_timeOutSeconds;

    /// Minimum delay in seconds before retrying enrollment.
    unsigned int enr_req_minRetryDelaySeconds;

    /// Correlation ID for the enrollment request.
    char* enr_req_correlationId;

    /// Time of the last received enrollment response.
    time_t enr_resp_lastReceivedTime;

    /// Device update instance associated with the enrollment response.
    char* enr_resp_duInstance;

    /// Result code indicating the enrollment response status.
    int enr_resp_resultCode;

    /// Extended result code providing additional enrollment response details.
    int enr_resp_extendedResultCode;

    /// Description of the enrollment response result.
    char* enr_resp_resultDescription;

    ADUC_AGENT_MODULE_HANDLE commModule; //!< Communication module handle.
} ADU_ENROLLMENT_DATA;

static ADU_ENROLLMENT_DATA s_enrMgrState = { 0 }; //!< Enrollment state

/**
 * @brief A helper function use for setting s_enrMgrState.enr_req_correlationId.
 */
static void SetCorrelationId(const char* correlationId)
{
    if (s_enrMgrState.enr_req_correlationId != NULL)
    {
        free(s_enrMgrState.enr_req_correlationId);
    }

    if (correlationId == NULL)
    {
        s_enrMgrState.enr_req_correlationId = NULL;
    }
    else
    {
        s_enrMgrState.enr_req_correlationId = strdup(correlationId);
    }
}

/**
 * @brief A helper function use for setting s_enrMgrState.enrollmentState.
 */
static void SetEnrollmentState(ADU_ENROLLMENT_STATE state, const char* reason)
{
    Log_Info(
        "Enrollment state changed from %d to %d (reason:%s)",
        s_enrMgrState.enrollmentState,
        state,
        IsNullOrEmpty(reason) ? "unknown" : reason);
    s_enrMgrState.enrollmentState = state;
}

/**
 * @brief Free memory allocated for the enrollment data.
 */
void Free_Enrollment_Data(ADU_ENROLLMENT_DATA* data)
{
    if (data == NULL)
    {
        return;
    }

    free(data->enr_resp_duInstance);
    free(data->enr_resp_resultDescription);
    free(data);
    memset(&data, 0, sizeof(data));
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
    //ADUC_MQTT_CLIENT_MODULE_STATE* state = (ADUC_MQTT_CLIENT_MODULE_STATE*)obj;
    char* correlationData = NULL;

    // Check if correlation data matches the latest enrollment message.
    const mosquitto_property* p =
        mosquitto_property_read_string(props, MQTT_PROP_CORRELATION_DATA, &correlationData, NULL);
    if (p == NULL)
    {
        Log_Error("Failed to read correlation data");
        goto done;
    }

    // Check if the correlation data matches the latest enrollment message.
    if (correlationData == NULL || strcmp(correlationData, s_enrMgrState.enr_req_correlationId) != 0)
    {
        Log_Warn(
            "IGNORING MSG: mismatch CIDs (expected:%s, actual:%s)",
            s_enrMgrState.enr_req_correlationId,
            correlationData);
        goto done;
    }

    if (msg == NULL || msg->payload == NULL || msg->payloadlen == 0)
    {
        Log_Error("Bad payload");
        goto done;
    }

    // Parse the JSON payload using Parson
    JSON_Value* root_value = json_parse_string(msg->payload);
    JSON_Object* root_object = json_value_get_object(root_value);

    if (root_value == NULL)
    {
        Log_Error("Failed to parse JSON");
        goto done;
    }

    int isEnrolled = json_object_dotget_boolean(root_object, "isEnrolled");
    const char* duInstance = json_object_dotget_string(root_object, "duinstance");
    int resultCode = (int)json_object_dotget_number(root_object, "resultcode");
    int extendedResultCode = (int)json_object_dotget_number(root_object, "extendedResultcode");
    const char* resultDescription = json_object_dotget_string(root_object, "resultDescription");

    // Validate received data
    if (isEnrolled == -1) // json_object_dotget_boolean returns -1 on failure
    {
        Log_Error("Failed to retrieve or validate isEnrolled from JSON payload");
        json_value_free(root_value);
        return;
    }

    // Store the enrollment response data in the state.
    time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();
    s_enrMgrState.enr_resp_lastReceivedTime = nowTime;

    free(s_enrMgrState.enr_resp_duInstance);
    s_enrMgrState.enr_resp_duInstance = IsNullOrEmpty(duInstance) ? NULL : strdup(duInstance);

    s_enrMgrState.enr_resp_resultCode = resultCode;
    s_enrMgrState.enr_resp_extendedResultCode = extendedResultCode;

    free(s_enrMgrState.enr_resp_resultDescription);
    s_enrMgrState.enr_resp_resultDescription = IsNullOrEmpty(resultDescription) ? NULL : strdup(resultDescription);
    SetEnrollmentState(isEnrolled ? ADU_ENROLLMENT_STATE_ENROLLED : ADU_ENROLLMENT_STATE_NOT_ENROLLED, "enr_resp");

    json_value_free(root_value);

done:
    free(correlationData);
}

/**
 * @brief Callback should be called when the client receives an enrollment status change notification message from the Device Update service.
 */
void OnMessage_enr_cn(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    // NOTE: NOT SUPPORTED IN V1 PROTOCOL - MVP
    Log_Info("OnMessage_enr_cn: NOT SUPPORTED IN V1 PROTOCOL - MVP");
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
 * @brief Initialize the enrollment management.
 * @return true if the enrollment management was successfully initialized; otherwise, false.
 */
bool ADUC_Enrollment_Management_Initialize(ADUC_AGENT_MODULE_HANDLE communicationModule)
{
    memset(&s_enrMgrState, 0, sizeof(s_enrMgrState));

    bool result = false;
    const ADUC_ConfigInfo* config = NULL;
    const ADUC_AgentInfo* agent_info = NULL;

    config = ADUC_ConfigInfo_GetInstance();
    if (config == NULL)
    {
        goto done;
    }

    agent_info = ADUC_ConfigInfo_GetAgent(config, 0);

    if (!ADUC_AgentInfo_GetUnsignedIntegerField(
            agent_info, "enrollmentSettings.enrollmentRequestTimeout", &s_enrMgrState.enr_req_timeOutSeconds)
        || s_enrMgrState.enr_req_timeOutSeconds == 0)
    {
        Log_Info("Using default enrollment request timeout: %d", DEFAULT_ENR_REQ_TIMEOUT_SECONDS);
        s_enrMgrState.enr_req_timeOutSeconds = DEFAULT_ENR_REQ_TIMEOUT_SECONDS;
    }

    if (!ADUC_AgentInfo_GetUnsignedIntegerField(
            agent_info, "enrollmentSettings.minimumRetryDelay", &s_enrMgrState.enr_req_minRetryDelaySeconds)
        || s_enrMgrState.enr_req_minRetryDelaySeconds == 0)
    {
        Log_Info("Using default minimum retry delay: %d", DEFAULT_ENR_REQ_MIN_RETRY_DELAY_SECONDS);
        s_enrMgrState.enr_req_minRetryDelaySeconds = DEFAULT_ENR_REQ_MIN_RETRY_DELAY_SECONDS;
    }

    s_enrMgrState.commModule = communicationModule;

    result = true;

done:
    ADUC_ConfigInfo_ReleaseInstance(config);
    return result;
}

/**
 * @brief Deinitialize the enrollment management.
 */
void ADUC_Enrollment_Management_Deinitialize()
{
    Free_Enrollment_Data(&s_enrMgrState);
}

/**
 * @brief Ensure that the device is enrolled with the Device Update service.
 */
bool ADUC_Enrollment_Management_DoWork()
{
    int result = false;
    int mqtt_res = MOSQ_ERR_UNKNOWN;

    time_t nowTime = ADUC_GetTimeSinceEpochInSeconds();

    if (s_enrMgrState.enrollmentState == ADU_ENROLLMENT_STATE_ENROLLED)
    {
#if _ADU_DEBUG
        if (s_lastHeartBeatLogTime + CONST_TIME_30_SECONDS < nowTime)
        {
            Log_Info("Enrollment state :%d", s_enrMgrState.enrollmentState);
            s_lastHeartBeatLogTime = nowTime;
        }
#endif
        return true;
    }

#if _ADU_DEBUG
    if (s_lastHeartBeatLogTime + CONST_TIME_5_SECONDS < nowTime)
    {
        Log_Info("Enrollment state :%d", s_enrMgrState.enrollmentState);
        s_lastHeartBeatLogTime = nowTime;
    }
#endif

    if (s_enrMgrState.enrollmentState == ADU_ENROLLMENT_STATE_ENROLLING)
    {
        if ((s_enrMgrState.enr_req_lastAttemptTime + s_enrMgrState.enr_req_timeOutSeconds) < nowTime)
        {
            Log_Warn("Enrollment request timeout");
            SetCorrelationId(NULL);
            SetEnrollmentState(ADU_ENROLLMENT_STATE_UNKNOWN, "enr_req timeout");
            s_enrMgrState.enr_req_lastErrorTime = nowTime;
            s_enrMgrState.enr_req_nextAttemptTime = nowTime + s_enrMgrState.enr_req_minRetryDelaySeconds;
        }
        return false;
    }

    if (s_enrMgrState.enrollmentState == ADU_ENROLLMENT_STATE_UNKNOWN)
    {
        if (s_enrMgrState.enr_req_nextAttemptTime < nowTime)
        {
            // Send enrollment request.
            mosquitto_property* props = NULL;
            time_t messageTimestamp = time(NULL);

            char* cid = GenerateCorrelationIdFromTime(messageTimestamp);

            // Discard older request by replacing the state's CID
            SetCorrelationId(cid);

            mosquitto_property_add_string_pair(
                &props, MQTT_PROP_USER_PROPERTY, ADU_MQTT_PROTOCOL_VERSION_PROPERTY_NAME, ADU_MQTT_PROTOCOL_VERSION);
            mosquitto_property_add_string_pair(
                &props,
                MQTT_PROP_USER_PROPERTY,
                ADU_MQTT_PROTOCOL_MESSAGE_TYPE_PROPERTY_NAME,
                ADU_MQTT_PROTOCOL_MESSAGE_TYPE_ENROLLMENT_REQUEST);
            mosquitto_property_add_string(&props, MQTT_PROP_CONTENT_TYPE, ADU_MQTT_PROTOCOL_MESSAGE_CONTENT_TYPE_JSON);
            mosquitto_property_add_string(&props, MQTT_PROP_CORRELATION_DATA, cid);

            int mid = 0;

            mqtt_res = ADUC_Communication_Channel_MQTT_Publish(
                s_enrMgrState.commModule /* handle */,
                s_mqtt_topic_a2s_oto /* topic */,
                &mid /* mid */,
                s_enr_req_message_content_len,
                s_enr_req_message_content, // "{}"
                1 /* qos */,
                false /* retain */,
                props);
            if (mqtt_res == MOSQ_ERR_SUCCESS)
            {
                SetEnrollmentState(ADU_ENROLLMENT_STATE_ENROLLING, "enr_req submitted");
                s_enrMgrState.enr_req_lastAttemptTime = nowTime;
                Log_Info(
                    "Submitting 'enr_req' (mid:%d, cid:%s, t:%ld, expired:%ld)",
                    mid,
                    cid,
                    nowTime,
                    nowTime + s_enrMgrState.enr_req_timeOutSeconds);

                result = true;
            }
            else
            {
                Log_Error(
                    "Failed to publish enrollment status request message. (mid:%d, cid:%s, err:%d - %s)",
                    mid,
                    cid,
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
                    // Don't retry for 1 hour.
                    s_enrMgrState.enr_req_nextAttemptTime = nowTime + (60 * 60);
                    break;

                case MOSQ_ERR_NO_CONN:
                    // Retry again after default retry delay.
                    // TODO (nox-msft) : compute next retry time using retry utils.
                    s_enrMgrState.enr_req_nextAttemptTime = nowTime + s_enrMgrState.enr_req_minRetryDelaySeconds;
                    Log_Error(
                        "Failed to publish (retry-able in %s seconds). Err:%d",
                        s_enrMgrState.enr_req_minRetryDelaySeconds,
                        mqtt_res);
                    break;
                default:
                    Log_Error("Failed to publish (unknown error). Err:%d", mqtt_res);
                    // Retry again after default retry delay.
                    // TODO (nox-msft) : compute next retry time using retry utils.
                    s_enrMgrState.enr_req_nextAttemptTime = nowTime + s_enrMgrState.enr_req_minRetryDelaySeconds;
                    break;
                }
            }

            free(cid);
            cid = NULL;

            return result;
        }
    }

    return false;
}

/**
 * @brief Retrieve the device enrollment state.
 * @return true if the device is enrolled with the Device Update service; otherwise, false.
 * @note This function will be replaced with the Inter-module communication (IMC) mechanism.
 */
bool ADUC_Enrollment_Management_IsEnrolled()
{
    return s_enrMgrState.enrollmentState == ADU_ENROLLMENT_STATE_ENROLLED;
}

/**
 * @brief Retrieve the Azure Device Update service instance.
 * @return The Azure Device Update service instance.
 */
const char* ADUC_Enrollment_Management_GetDUInstance()
{
    if (ADUC_Enrollment_Management_IsEnrolled())
    {
        return s_enrMgrState.enr_resp_duInstance;
    }
    return NULL;
}

/**
 * @brief Set the MQTT publishing topic for communication from the Agent to the Service.
 *
 * This function allows setting the MQTT publishing topic for messages sent from the Agent to the Service.
 * If the previous topic was set, it is freed to avoid memory leaks. If the provided 'topic' is not empty or NULL,
 * it is duplicated and assigned to the internal MQTT topic variable.
 *
 * @param topic A pointer to the MQTT publishing topic string to be set.
 *
 * @return true if the MQTT topic was successfully set or updated; false if 'topic' is empty or NULL.
 *
 * @note The memory allocated for the previous topic (if any) is freed to prevent memory leaks.
 * @note If 'topic' is empty or NULL, the function returns false.
 */
bool ADUC_Enrollment_Management_SetAgentToServiceTopic(const char* topic)
{
    if (s_mqtt_topic_a2s_oto != NULL)
    {
        free(s_mqtt_topic_a2s_oto);
        s_mqtt_topic_a2s_oto = NULL;
    }

    if (!IsNullOrEmpty(topic))
    {
        s_mqtt_topic_a2s_oto = strdup(topic);
        Log_Info("Changed publishing topic to '%s'", s_mqtt_topic_a2s_oto);
        return true;
    }

    return false;
}
