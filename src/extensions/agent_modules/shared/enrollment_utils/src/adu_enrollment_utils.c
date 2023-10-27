#include "aduc/adu_enrollment_utils.h"
#include <aduc/agent_state_store.h>
#include <aduc/logging.h>
#include <aduc/string_c_utils.h>
#include <string.h> // strncpy

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
        Log_Info(
            "Enrollment state changed from %d to %d (reason:%s)",
            enrollmentData->enrollmentState,
            state,
            IsNullOrEmpty(reason) ? "unknown" : reason);
        enrollmentData->enrollmentState = state;

        if (ADUC_StateStore_SetIsDeviceEnrolled(state == ADU_ENROLLMENT_STATE_ENROLLED) != ADUC_STATE_STORE_RESULT_OK)
        {
            Log_Error("Failed to set enrollment state in state store");
        }
    }
    return oldState;
}

/**
 * @brief Sets the correlation id for the enrollment request message.
 * @param enrollmentData The enrollment data object.
 * @param correlationId The correlation id to set.
 */
void EnrollmentData_SetCorrelationId(ADUC_Enrollment_Request_Operation_Data* enrollmentData, const char* correlationId)
{
    if (enrollmentData == NULL || correlationId == NULL)
    {
        Log_Error("Null input (enrollmentData:%p, correlationId:%p)", enrollmentData, correlationId);
        return;
    }

    strncpy(
        enrollmentData->enrReqMessageContext.correlationId,
        correlationId,
        sizeof(enrollmentData->enrReqMessageContext.correlationId) - 1);

    enrollmentData->enrReqMessageContext
        .correlationId[sizeof(enrollmentData->enrReqMessageContext.correlationId) - 1] = '\0';
}

/**
 * @brief Handles creating side-effects in response to incoming enrollment data from enrollment response.
 *
 * @param enrollmentData The enrollment data.
 * @param isEnrolled The boolean value of enrolled in the message.
 * @param duInstance The du instance.
 * @param context The retriable operation context.
 * @return true on success.
 */
bool handle_enrollment_side_effects(
    ADUC_Enrollment_Request_Operation_Data* enrollmentData,
    bool isEnrolled,
    const char* duInstance,
    ADUC_Retriable_Operation_Context* context)
{
    bool succeeded = false;
    ADUC_STATE_STORE_RESULT stateStoreResult = ADUC_STATE_STORE_RESULT_ERROR;

    EnrollmentData_SetState(
        enrollmentData,
        isEnrolled
            ? ADU_ENROLLMENT_STATE_ENROLLED
            : ADU_ENROLLMENT_STATE_NOT_ENROLLED,
        isEnrolled ? "enrolled" : "not enrolled");

    if (ADUC_StateStore_GetIsDeviceEnrolled() != isEnrolled)
    {
        Log_Error("Failed to set enrollment state to '%s'", isEnrolled ? "true" : "false");
        goto done;
    }

    if (isEnrolled)
    {
        Log_Info("Device is currently enrolled with '%s'", duInstance);

        context->completeFunc(context);

        stateStoreResult = ADUC_StateStore_SetDeviceUpdateServiceInstance(duInstance);
        if (stateStoreResult != ADUC_STATE_STORE_RESULT_OK)
        {
            Log_Error("Failed set duinstance in store");

            // Reset the enrollment state. so we can retry again.
            EnrollmentData_SetState(enrollmentData, ADU_ENROLLMENT_STATE_UNKNOWN, "reset unknown enrollment state");
            goto done;
        }
    }
    else
    {
        Log_Warn("Device is not currently enrolled with '%s'", duInstance);
    }

done:
    return succeeded;
}
