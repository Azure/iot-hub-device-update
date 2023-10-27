#ifndef ADU_ENROLLMENT_UTILS_H
#define ADU_ENROLLMENT_UTILS_H

#include <aduc/adu_enrollment.h>

ADUC_Enrollment_Request_Operation_Data* EnrollmentData_FromOperationContext(ADUC_Retriable_Operation_Context* context);

ADU_ENROLLMENT_STATE EnrollmentData_SetState(
    ADUC_Enrollment_Request_Operation_Data* enrollmentData, ADU_ENROLLMENT_STATE state, const char* reason);

void EnrollmentData_SetCorrelationId(ADUC_Enrollment_Request_Operation_Data* enrollmentData, const char* correlationId);

bool handle_enrollment_side_effects(
    ADUC_Enrollment_Request_Operation_Data* enrollmentData,
    bool isEnrolled,
    const char* duInstance,
    ADUC_Retriable_Operation_Context* context);

#endif // ADU_ENROLLMENT_UTILS_H
