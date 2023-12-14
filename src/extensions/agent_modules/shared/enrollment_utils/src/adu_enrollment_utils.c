#include "aduc/adu_enrollment_utils.h"
#include <aduc/adu_module_state.h> // ADUC_MQTT_CLIENT_MODULE_STATE
#include <aduc/adu_mqtt_protocol.h> // ADU_RESPONSE_MESSAGE_RESULT_CODE_*
#include <aduc/agent_state_store.h>
#include <aduc/logging.h>
#include <aduc/string_c_utils.h>
#include <parson_json_utils.h> // ADUC_JSON_*
#include <string.h> // strlen, strdup

// keep this last to avoid interfering with system headers
#include <aduc/aduc_banned.h>

/**
 * @brief Returns str representation of the enrollment state enum
 * @param st The enrollment state.
 * @return const char* State as a string.
 */
const char* enrollment_state_str(ADU_ENROLLMENT_STATE st)
{
    switch (st)
    {
    case ADU_ENROLLMENT_STATE_NOT_ENROLLED:
        return "ADU_ENROLLMENT_STATE_NOT_ENROLLED";

    case ADU_ENROLLMENT_STATE_UNKNOWN:
        return "ADU_ENROLLMENT_STATE_UNKNOWN";

    case ADU_ENROLLMENT_STATE_REQUESTING:
        return "ADU_ENROLLMENT_STATE_REQUESTING";

    case ADU_ENROLLMENT_STATE_ENROLLED:
        return "ADU_ENROLLMENT_STATE_ENROLLED";

    default:
        return "???";
    }
}

/**
 * @brief Gets the enrollment data object from the enrollment operation context.
 * @param context The enrollment operation context.
 * @return ADUC_Enrollment_Request_Operation_Data* The enrollment data object.
 */
ADUC_Enrollment_Request_Operation_Data* EnrollmentData_FromOperationContext(ADUC_Retriable_Operation_Context* context)
{
    if (context == NULL || context->data == NULL)
    {
        Log_Error("Null input (context:%p, data:%p)", context, context->data);
        return NULL;
    }

    return (ADUC_Enrollment_Request_Operation_Data*)(context->data);
}

/**
 * @brief Gets the ADUC_Retriable_Operation_Context from the enrollment mqtt library callback's user object.
 * @return ADUC_Retriable_Operation_Context* The retriable operation context.
 */
ADUC_Retriable_Operation_Context* RetriableOperationContextFromEnrollmentMqttLibCallbackUserObj(void* obj)
{
    ADUC_MQTT_CLIENT_MODULE_STATE* ownerModuleState = (ADUC_MQTT_CLIENT_MODULE_STATE*)obj;
    ADUC_AGENT_MODULE_INTERFACE* enrollmentModuleInterface =
        (ADUC_AGENT_MODULE_INTERFACE*)ownerModuleState->enrollmentModule;
    return (ADUC_Retriable_Operation_Context*)(enrollmentModuleInterface->moduleData);
}

/**
 * @brief Gets the Enrollment Request Operation Data from the mqtt library callback's user object.
 * @param retriableOperationContext The retriable operation context.
 * @return ADUC_Enrollment_Request_Operation_Data* The enrollment request operation data.
 */
ADUC_Enrollment_Request_Operation_Data*
EnrollmentDataFromRetriableOperationContext(ADUC_Retriable_Operation_Context* retriableOperationContext)
{
    if (retriableOperationContext == NULL)
    {
        return NULL;
    }

    return (ADUC_Enrollment_Request_Operation_Data*)(retriableOperationContext->data);
}

/**
 * @brief Sets the enrollment state and updates 'IsDeviceEnrolled' state in the DU agent state store.
 * @param data The enrollment data object.
 * @param state The enrollment state to set.
 * @return ADU_ENROLLMENT_STATE The old enrollment state.
 */
ADU_ENROLLMENT_STATE EnrollmentData_SetState(
    ADUC_Enrollment_Request_Operation_Data* enrollmentData, ADU_ENROLLMENT_STATE state, const char* reason)
{
    ADU_ENROLLMENT_STATE oldState = enrollmentData->enrollmentState;
    if (enrollmentData->enrollmentState != state)
    {
        if (ADUC_StateStore_SetIsDeviceEnrolled(state == ADU_ENROLLMENT_STATE_ENROLLED) != ADUC_STATE_STORE_RESULT_OK)
        {
            Log_Error("Failed to set enrollment state in state store");
        }
        else
        {
            Log_Info(
                "Enrollment state changed from %s(%d) to %s(%d) (reason:%s)",
                enrollment_state_str(enrollmentData->enrollmentState),
                enrollmentData->enrollmentState,
                enrollment_state_str(state),
                state,
                IsNullOrEmpty(reason) ? "unknown" : reason);

            enrollmentData->enrollmentState = state;
        }
    }
    return oldState;
}

/**
 * @brief Sets the correlation id for the enrollment request message.
 * @param enrollmentData The enrollment data object.
 * @param correlationId The correlation id to set.
 */
bool EnrollmentData_SetCorrelationId(ADUC_Enrollment_Request_Operation_Data* enrollmentData, const char* correlationId)
{
    if (enrollmentData == NULL || correlationId == NULL)
    {
        Log_Error("Null input (enrollmentData:%p, correlationId:%p)", enrollmentData, correlationId);
        return false;
    }

    if (!ADUC_Safe_StrCopyN(
        enrollmentData->enrReqMessageContext.correlationId,
        ARRAY_SIZE(enrollmentData->enrReqMessageContext.correlationId),
        correlationId,
        strlen(correlationId)))
    {
        Log_Error("copy failed");
        return false;
    }

    return true;
}

/**
 * @brief Handles creating side-effects in response to incoming enrollment data from enrollment response.
 *
 * @param enrollmentData The enrollment data.
 * @param isEnrolled The boolean value of enrolled in the message.
 * @param scopeId The du instance.
 * @param context The retriable operation context.
 * @return true on success.
 */
bool Handle_Enrollment_Response(
    ADUC_Enrollment_Request_Operation_Data* enrollmentData,
    bool isEnrolled,
    const char* scopeId,
    ADUC_Retriable_Operation_Context* context)
{
    bool succeeded = false;
    ADUC_STATE_STORE_RESULT stateStoreResult = ADUC_STATE_STORE_RESULT_ERROR;

    if (enrollmentData == NULL || scopeId == NULL || context == NULL)
    {
        return false;
    }

    const ADUC_Common_Response_User_Properties* user_props = &enrollmentData->respUserProps;

    // validate enrollmentData
    if (user_props->pid != 1)
    {
        Log_Error("Invalid enr_resp pid: %d", user_props->pid);
        return false;
    }

    if (user_props->resultcode != ADU_RESPONSE_MESSAGE_RESULT_CODE_SUCCESS)
    {
        switch (user_props->resultcode)
        {
        case ADU_RESPONSE_MESSAGE_RESULT_CODE_BAD_REQUEST:
            Log_Error("enr_resp - Bad Request(1), erc: 0x%08x", user_props->extendedresultcode);
            Log_Info("enr_resp bad request from agent. Cancelling...");
            context->cancelFunc(context);
            break;

        case ADU_RESPONSE_MESSAGE_RESULT_CODE_BUSY:
            Log_Error("enr_resp - Busy(2), erc: 0x%08x", user_props->extendedresultcode);
            break;

        case ADU_RESPONSE_MESSAGE_RESULT_CODE_CONFLICT:
            Log_Error("enr_resp - Conflict(3), erc: 0x%08x", user_props->extendedresultcode);
            break;

        case ADU_RESPONSE_MESSAGE_RESULT_CODE_SERVER_ERROR:
            Log_Error("enr_resp - Server Error(4), erc: 0x%08x", user_props->extendedresultcode);
            break;

        case ADU_RESPONSE_MESSAGE_RESULT_CODE_AGENT_NOT_ENROLLED:
            Log_Error("enr_resp - Agent Not Enrolled(5), erc: 0x%08x", user_props->extendedresultcode);
            break;

        default:
            Log_Error(
                "enr_resp - Unknown Error: %d, erc: 0x%08x", user_props->resultcode, user_props->extendedresultcode);
            break;
        }

        Log_Info("enr_resp error. Retrying...");
        EnrollmentData_SetState(enrollmentData, ADU_ENROLLMENT_STATE_UNKNOWN, "retry");
        goto done;
    }

    ADU_ENROLLMENT_STATE new_state = isEnrolled ? ADU_ENROLLMENT_STATE_ENROLLED : ADU_ENROLLMENT_STATE_NOT_ENROLLED;

    ADU_ENROLLMENT_STATE old_state = EnrollmentData_SetState(enrollmentData, new_state, NULL /* reason */);

    if (ADUC_StateStore_IsDeviceEnrolled() != isEnrolled)
    {
        Log_Error("Failed set enrollment state - '%d' to '%d'", old_state, new_state);
        goto done;
    }

    Log_Info("Enrollment state transitioned - '%d' to '%d'", old_state, new_state);

    if (isEnrolled)
    {
        Log_Info("Device is currently enrolled with scopeId '%s'", scopeId);

        context->completeFunc(context);

        stateStoreResult = ADUC_StateStore_SetScopeId(scopeId);
        if (stateStoreResult != ADUC_STATE_STORE_RESULT_OK)
        {
            Log_Error("Failed set scopeId in store");

            // Reset the enrollment state. so we can retry again.
            EnrollmentData_SetState(enrollmentData, ADU_ENROLLMENT_STATE_UNKNOWN, "retry");
            goto done;
        }
    }
    else
    {
        Log_Warn("Device is not currently enrolled with '%s'", scopeId);
    }

    succeeded = true;
done:
    return succeeded;
}

/**
 * @brief Parses the enrollment response message payload.
 *
 * @param payload The message payload.
 * @param outIsEnrolled Out parameter for whether enrolled as per the payload.
 * @param outScopeId Out parameter for the scope Id. Must be freed by caller.
 * @return true On successful parse and allocation of out parameter.
 */
bool ParseEnrollmentMessagePayload(const char* payload, bool* outIsEnrolled, char** outScopeId)
{
    bool succeeded = false;

    JSON_Value* root_value = NULL;
    bool isEnrolled = false;
    const char* scopeId = NULL; // does not need free()
    char* allocScopeId = NULL;

    if (payload == NULL || outIsEnrolled == NULL || outScopeId == NULL)
    {
        Log_Error("bad arg");
        goto done;
    }

    if (NULL == (root_value = json_parse_string(payload)))
    {
        Log_Error("Failed to parse JSON");
        goto done;
    }

    if (!ADUC_JSON_GetBooleanField(root_value, "IsEnrolled", &isEnrolled))
    {
        Log_Error("Failed to get 'isEnrolled' from payload");
        goto done;
    }

    if (NULL == (scopeId = ADUC_JSON_GetStringFieldPtr(root_value, "ScopeId")))
    {
        Log_Error("Failed to get 'ScopeId' from payload");
        goto done;
    }

    if (NULL == (allocScopeId = strdup(scopeId)))
    {
        goto done;
    }

    *outScopeId = allocScopeId;
    allocScopeId = NULL; // transfer ownership

    *outIsEnrolled = isEnrolled;

    succeeded = true;

done:
    json_value_free(root_value);

    return succeeded;
}
