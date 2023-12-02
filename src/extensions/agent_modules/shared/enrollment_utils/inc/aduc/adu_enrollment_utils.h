#ifndef ADU_ENROLLMENT_UTILS_H
#define ADU_ENROLLMENT_UTILS_H

#include <aduc/adu_enrollment.h>
#include <aduc/retry_utils.h> // ADUC_Retriable_Operation_Context

EXTERN_C_BEGIN

const char* enrollment_state_str(ADU_ENROLLMENT_STATE st);

ADUC_Enrollment_Request_Operation_Data* EnrollmentData_FromOperationContext(ADUC_Retriable_Operation_Context* context);

ADUC_Retriable_Operation_Context* RetriableOperationContextFromEnrollmentMqttLibCallbackUserObj(void* obj);
ADUC_Enrollment_Request_Operation_Data* EnrollmentDataFromRetriableOperationContext(ADUC_Retriable_Operation_Context* retriableOperationContext);

ADU_ENROLLMENT_STATE EnrollmentData_SetState(
    ADUC_Enrollment_Request_Operation_Data* enrollmentData, ADU_ENROLLMENT_STATE state, const char* reason);

void EnrollmentData_SetCorrelationId(ADUC_Enrollment_Request_Operation_Data* enrollmentData, const char* correlationId);

bool Handle_Enrollment_Response(
    ADUC_Enrollment_Request_Operation_Data* enrollmentData,
    bool isEnrolled,
    const char* duInstance,
    ADUC_Retriable_Operation_Context* context);

bool ParseEnrollmentMessagePayload(const char* payload, bool* outIsEnrolled, char** outScopeId);

EXTERN_C_END

#endif // ADU_ENROLLMENT_UTILS_H
