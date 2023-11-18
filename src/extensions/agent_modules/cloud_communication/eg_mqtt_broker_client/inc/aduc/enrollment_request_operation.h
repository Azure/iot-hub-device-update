#ifndef ENROLLMENT_REQUEST_OPERATION_H
#define ENROLLMENT_REQUEST_OPERATION_H

#include <aduc/retry_utils.h> // ADUC_Retriable_Operation_Context, ADUC_Retry_Params
#include <aduc/mqtt_broker_common.h> // ADUC_MQTT_Message_Context
#include <aducpal/sys_time.h> // time_t
#include <du_agent_sdk/agent_module_interface.h> // ADUC_AGENT_MODULE_HANDLE
#include <aduc/adu_enrollment.h> // ADUC_Enrollment_Request_Operation_Data

bool EnrollmentRequestOperation_CancelOperation(ADUC_Retriable_Operation_Context* context);
bool EnrollmentRequestOperation_Complete(ADUC_Retriable_Operation_Context* context);
bool EnrollmentRequestOperation_DoRetry(ADUC_Retriable_Operation_Context* context, const ADUC_Retry_Params* retryParams);

void EnrollmentRequestOperation_DataDestroy(ADUC_Retriable_Operation_Context* context);
void EnrollmentRequestOperation_OperationDestroy(ADUC_Retriable_Operation_Context* context);

void EnrollmentRequestOperation_OnExpired(ADUC_Retriable_Operation_Context* data);
void EnrollmentRequestOperation_OnSuccess(ADUC_Retriable_Operation_Context* data);
void EnrollmentRequestOperation_OnFailure(ADUC_Retriable_Operation_Context* data);
void EnrollmentRequestOperation_OnRetry(ADUC_Retriable_Operation_Context* data);

bool IsEnrolled(const ADUC_Enrollment_Request_Operation_Data* enrollmentData);
bool IsEnrollAlreadyHandled(ADUC_Retriable_Operation_Context* context, time_t nowTime);
bool HandlingRequestEnrollment(ADUC_Retriable_Operation_Context* context, time_t nowTime);

bool EnrollmentStatusRequestOperation_DoWork(ADUC_Retriable_Operation_Context* context);
ADUC_Retriable_Operation_Context* CreateAndInitializeEnrollmentRequestOperation();

#endif // ENROLLMENT_REQUEST_OPERATION_H
