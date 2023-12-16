/**
 * @file adu_update_management.c
 * @brief Implementation for device update status management.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/adu_update_management.h"

#include <aduc/adu_communication_channel.h>
#include <aduc/adu_module_state.h> // ADUC_MQTT_CLIENT_MODULE_STATE
#include <aduc/adu_mosquitto_utils.h>
#include <aduc/adu_mqtt_common.h>
#include <aduc/adu_mqtt_protocol.h>
#include <aduc/adu_types.h>
#include <aduc/adu_upd.h>
#include <aduc/adu_upd_utils.h>
#include <aduc/agent_module_interface_internal.h>
#include <aduc/agent_state_store.h>
#include <aduc/config_utils.h>
#include <aduc/logging.h>
#include <aduc/retry_utils.h> // ADUC_GetTimeSinceEpochInSeconds
#include <aduc/string_c_utils.h> // IsNullOrEmpty
#include <aduc/topic_mgmt_lifecycle.h> // TopicMgmtLifecycle_UninitRetriableOperationContext
#include <aduc/upd_request_operation.h>
#include <aduc/workqueue.h> // WorkQueue_EnqueueWork
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
// BEGIN - ADU_UPDATE_MANAGEMENT.H Public Interface
//

/**
 * @brief Destroy the module handle.
 *
 * @param handle The module handle.
 */
void ADUC_Update_Management_Destroy(ADUC_AGENT_MODULE_HANDLE handle)
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
 * @brief Callback for when the client receives an update response message from the Device Update service.
 *
 * @param mosq The mosquitto instance making the callback.
 * @param obj The user object callback arg that was the module state passed to mosquitto_new.
 * @param msg The message data.
 * @param props The MQTT v5 properties returned with the message.
 */
void OnMessage_upd_resp(
    struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg, const mosquitto_property* props)
{
    char* correlationData = NULL;
    size_t correlationDataByteLen = 0;

    WorkQueueHandle updateWorkQueueHandle = WorkQueueHandleFromCallbackUserObj(obj);
    ADUC_Retriable_Operation_Context* retriableOperationContext = RetriableOperationContextFromCallbackUserObj(obj);
    ADUC_Update_Request_Operation_Data* updateData = UpdateDataFromRetriableOperationContext(retriableOperationContext);

    if (updateWorkQueueHandle == NULL || retriableOperationContext == NULL || updateData == NULL)
    {
        Log_Error("invalid callback user obj");
        goto done;
    }

    json_print_properties(props);

    if (!ADU_are_correlation_ids_matching(
            props, updateData->updReqMessageContext.correlationId, &correlationData, &correlationDataByteLen))
    {
        Log_Info(
            "correlation data mismatch. expected: '%s', actual: '%s' %u bytes",
            correlationData,
            correlationDataByteLen);
        goto done;
    }

    if (updateData->updState != ADU_UPD_STATE_REQUEST_ACK)
    {
        Log_Warn(
            "Unexpected state of %d ( '%s' ) instead of REQUEST_ACK state! Received '%s' without receiving acknowledgement of '%s' request!",
            updateData->updState,
            AduUpdState_str(updateData->updState),
            ADU_MQTT_PROTOCOL_MESSAGE_TYPE_UPDATE_SYNC_RESPONSE,
            ADU_MQTT_PROTOCOL_MESSAGE_TYPE_UPDATE_SYNC_REQUEST);
    }

    if (msg == NULL || msg->payload == NULL || msg->payloadlen == 0 || IsNullOrEmpty(msg->topic))
    {
        Log_Error(
            "Bad payload. msg:%p, payload:%p, payloadlen:%d, topic:%p",
            msg,
            msg->payload,
            msg->payloadlen,
            msg->topic);
        goto done;
    }

    if (!ADU_MosquittoUtils_ParseAndValidateCommonResponseUserProperties(
            props, "upd_resp" /* expectedMsgType */, &updateData->respUserProps))
    {
        Log_Error("Fail parse of common user props");
        goto done;
    }

    Log_Info(
        "'%s' msg resultcode: %d => '%s', erc: 0x%08X => '%s'",
        ADU_MQTT_PROTOCOL_MESSAGE_TYPE_UPDATE_SYNC_RESPONSE,
        updateData->respUserProps.resultcode,
        adu_mqtt_protocol_result_code_str(updateData->respUserProps.resultcode),
        updateData->respUserProps.extendedresultcode,
        adu_mqtt_protocol_erc_str(updateData->respUserProps.extendedresultcode));

    switch (updateData->respUserProps.resultcode)
    {
    case ADU_RESPONSE_MESSAGE_RESULT_CODE_SUCCESS:

        Log_Info("\nProcessing msgtype 'upd_resp', msgid: %d, topic: '%s'", msg->mid, msg->topic);
        Log_Debug("  payload_len: %d, payload: '%s'\n", msg->payloadlen, msg->payload);

        ADUC_StateStore_SetUpdateWorkQueueHandle(updateWorkQueueHandle);
        if (!WorkQueue_EnqueueWork(updateWorkQueueHandle, msg->payload))
        {
            Log_Error("Failed enqueuing work to update deployment work queue!");
            retriableOperationContext->retryFunc(retriableOperationContext, retriableOperationContext->retryParams);
        }
        else
        {
            Log_Info("Success queuing 'upd_resp' payload to work queue");
            AduUpdUtils_TransitionState(ADU_UPD_STATE_PROCESSING_UPDATE, updateData);
        }

        break;

    case ADU_RESPONSE_MESSAGE_RESULT_CODE_BAD_REQUEST:
        Log_Error(
            "Service considered previously published '%s' as BAD REQUEST",
            ADU_MQTT_PROTOCOL_MESSAGE_TYPE_UPDATE_SYNC_REQUEST);

        retriableOperationContext->nextExecutionTime = ADUC_GetTimeSinceEpochInSeconds() + 300;
        AduUpdUtils_TransitionState(ADU_UPD_STATE_IDLEWAIT, updateData);
        break;

    case ADU_RESPONSE_MESSAGE_RESULT_CODE_BUSY:
        Log_Debug("Service is Busy. Entering RETRY WAIT to try again later.");
        retriableOperationContext->nextExecutionTime = ADUC_GetTimeSinceEpochInSeconds() + 300;
        updateData->updState = ADU_UPD_STATE_RETRYWAIT;
        break;

    case ADU_RESPONSE_MESSAGE_RESULT_CODE_CONFLICT:
        Log_Debug("Service Conflict. Entering RETRY WAIT to try again later.");
        retriableOperationContext->nextExecutionTime = ADUC_GetTimeSinceEpochInSeconds() + 300;
        updateData->updState = ADU_UPD_STATE_RETRYWAIT;
        break;

    case ADU_RESPONSE_MESSAGE_RESULT_CODE_SERVER_ERROR:
        retriableOperationContext->nextExecutionTime =
            ADUC_GetTimeSinceEpochInSeconds() + 300; // 5 mins from now for now
        updateData->updState = ADU_UPD_STATE_RETRYWAIT;
        break;

    case ADU_RESPONSE_MESSAGE_RESULT_CODE_AGENT_NOT_ENROLLED:
        // Set the correlation id to NULL, so we can discard all inflight messages.
        UpdateData_SetCorrelationId(updateData, "");

        // Reset state store state so it will re-enroll and send agent info again.
        ADUC_StateStore_SetIsDeviceEnrolled(false /* isDeviceEnrolled */);
        ADUC_StateStore_SetIsAgentInfoReported(false /* isAgentInfoReported */);

        retriableOperationContext->lastFailureTime = ADUC_GetTimeSinceEpochInSeconds();
        retriableOperationContext->nextExecutionTime =
            ADUC_GetTimeSinceEpochInSeconds() + 300; // 5 mins from now for now

        updateData->updState = ADU_UPD_STATE_IDLEWAIT;

        break;

    default:
        Log_Warn("Unhandled result code: %d", updateData->respUserProps.resultcode);
        break;
    }

done:

    free(correlationData);
}

void OnPublish_upd_resp(struct mosquitto* mosq, void* obj, const mosquitto_property* props, int reason_code)
{
    UNREFERENCED_PARAMETER(mosq);
    UNREFERENCED_PARAMETER(props);

    ADUC_Retriable_Operation_Context* retriable_operation_context = RetriableOperationContextFromCallbackUserObj(obj);
    ADUC_Update_Request_Operation_Data* updData = UpdateDataFromRetriableOperationContext(retriable_operation_context);

    switch (reason_code)
    {
    case MQTT_RC_SUCCESS:
        // TODO: match on message id
        retriable_operation_context->lastExecutionTime = ADUC_GetTimeSinceEpochInSeconds();
        AduUpdUtils_TransitionState(ADU_UPD_STATE_REQUEST_ACK, updData);
        break;

    case MQTT_RC_NO_MATCHING_SUBSCRIBERS:
        // No Subscribers were subscribed to the topic we tried to publish to (as per mqtt 5 spec).
        // This is unexpected since at least the ADU service should be subscribed to receive the
        // agent topic's publish. Set timer and try again later in hopes that the service will be
        // subscribed, but fail and restart after max retries.
        if (retriable_operation_context != NULL)
        {
            retriable_operation_context->retryFunc(
                retriable_operation_context, retriable_operation_context->retryParams);
        }
        break;

    case MQTT_RC_UNSPECIFIED: // fall-through
    case MQTT_RC_IMPLEMENTATION_SPECIFIC: // fall-through
    case MQTT_RC_NOT_AUTHORIZED:
        // Not authorized at the moment but maybe it can auto-recover with retry if it is corrected.
        if (retriable_operation_context != NULL)
        {
            retriable_operation_context->retryFunc(
                retriable_operation_context, retriable_operation_context->retryParams);
        }
        break;

    case MQTT_RC_TOPIC_NAME_INVALID: // fall-through
    case MQTT_RC_PACKET_ID_IN_USE: // fall-through
    case MQTT_RC_PACKET_TOO_LARGE: // fall-through
    case MQTT_RC_QUOTA_EXCEEDED:
        if (retriable_operation_context != NULL)
        {
            retriable_operation_context->cancelFunc(retriable_operation_context);
        }
        break;
    }
}

//
// END - ADU_UPDATE_MANAGEMENT.H Public Interface
//
/////////////////////////////////////////////////////////////////////////////
