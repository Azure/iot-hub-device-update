/**
 * @file adu_enrollment_management.c
 * @brief Implementation for device enrollment status management.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_enrollment_management.h"

#include <aduc/adu_communication_channel.h>
#include <aduc/adu_enrollment.h>
#include <aduc/adu_enrollment_utils.h>
#include <aduc/adu_module_state.h> // ADUC_MQTT_CLIENT_MODULE_STATE
#include <aduc/adu_mosquitto_utils.h>
#include <aduc/adu_mqtt_common.h>
#include <aduc/adu_mqtt_protocol.h>
#include <aduc/adu_types.h>
#include <aduc/agent_module_interface_internal.h>
#include <aduc/agent_state_store.h>
#include <aduc/config_utils.h>
#include <aduc/enrollment_request_operation.h>
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
// BEGIN - ADU_ENROLLMENT_MANAGEMENT.H Public Interface
//

/**
 * @brief Destroy the module handle.
 *
 * @param handle The module handle.
 */
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

/**
 * @brief Callback should be called when the client receives an enrollment status response message from the Device Update service.
 *  For 'enr_resp' messages, if the Correlation Data matches, then the client should parse the message and update the enrollment state.
 *
 * @param mosq The mosquitto instance making the callback.
 * @param obj The user data provided in mosquitto_new. This is the module state
 * @param msg The message data.
 * @param props The MQTT v5 properties returned with the message.
 */
void OnMessage_enr_resp(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    bool isEnrolled = false;
    char* scopeId = NULL;
    char* correlationData = NULL;
    size_t correlationDataByteLen = 0;

    ADUC_Retriable_Operation_Context* retriableOperationContext =
        RetriableOperationContextFromEnrollmentMqttLibCallbackUserObj(obj);
    ADUC_Enrollment_Request_Operation_Data* enrollmentData = retriableOperationContext == NULL
        ? NULL
        : EnrollmentDataFromRetriableOperationContext(retriableOperationContext);

    if (retriableOperationContext == NULL || enrollmentData == NULL)
    {
        Log_Error("invalid user obj from mqtt lib callback");
        goto done;
    }

    json_print_properties(props);

    if (!ADU_are_correlation_ids_matching(
            props, enrollmentData->enrReqMessageContext.correlationId, &correlationData, &correlationDataByteLen))
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

    if (!ADU_MosquittoUtils_ParseAndValidateCommonResponseUserProperties(
            props, "enr_resp" /* expectedMsgType */, &enrollmentData->respUserProps))
    {
        Log_Error("Fail parse of common user props");
        goto done;
    }

    if (!ParseEnrollmentMessagePayload(msg->payload, &isEnrolled, &scopeId))
    {
        Log_Error("Fail parse of msg payload: %s", (char*)msg->payload);
        goto done;
    }

    if (!Handle_Enrollment_Response(enrollmentData, isEnrolled, scopeId, retriableOperationContext))
    {
        Log_Error("Fail handling enrollment response.");
        goto done;
    }

done:
    free(scopeId);
    free(correlationData);
}

void OnPublish_enr_resp(struct mosquitto* mosq, void* obj, const mosquitto_property* props, int reason_code)
{
    ADUC_Retriable_Operation_Context* operationContext =
        RetriableOperationContextFromEnrollmentMqttLibCallbackUserObj(obj);
    ADUC_Enrollment_Request_Operation_Data* enrollmentData =
        EnrollmentDataFromRetriableOperationContext(operationContext);
    ADUC_MQTT_Common_HandlePublishAck(
        mosq, obj, props, reason_code, operationContext, enrollmentData->enrReqMessageContext.correlationId);
}

//
// END - ADU_ENROLLMENT_MANAGEMENT.H Public Interface
//
/////////////////////////////////////////////////////////////////////////////
