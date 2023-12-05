#ifndef UPD_REQUEST_OPERATION_H
#define UPD_REQUEST_OPERATION_H

#include <aduc/adu_upd.h> // ADUC_Update_Request_Operation_Data
#include <aduc/mqtt_broker_common.h> // ADUC_MQTT_Message_Context
#include <aduc/retry_utils.h> // ADUC_Retriable_Operation_Context, ADUC_Retry_Params
#include <aducpal/sys_time.h> // time_t
#include <du_agent_sdk/agent_module_interface.h> // ADUC_AGENT_MODULE_HANDLE

EXTERN_C_BEGIN

bool UpdateRequestOperation_CancelOperation(ADUC_Retriable_Operation_Context* context);
bool UpdateRequestOperation_Complete(ADUC_Retriable_Operation_Context* context);
bool UpdateRequestOperation_DoRetry(ADUC_Retriable_Operation_Context* context, const ADUC_Retry_Params* retryParams);

void UpdateRequestOperation_DataDestroy(ADUC_Retriable_Operation_Context* context);
void UpdateRequestOperation_OperationDestroy(ADUC_Retriable_Operation_Context* context);

void UpdateRequestOperation_OnExpired(ADUC_Retriable_Operation_Context* data);
void UpdateRequestOperation_OnSuccess(ADUC_Retriable_Operation_Context* data);
void UpdateRequestOperation_OnFailure(ADUC_Retriable_Operation_Context* data);
void UpdateRequestOperation_OnRetry(ADUC_Retriable_Operation_Context* data);

bool UpdateRequestOperation_DoWork(ADUC_Retriable_Operation_Context* context);
ADUC_Retriable_Operation_Context* CreateAndInitializeUpdateRequestOperation();
void Adu_ProcessUpdate(ADUC_Update_Request_Operation_Data* updateData, ADUC_Retriable_Operation_Context* retriableOperationContext);

EXTERN_C_END

#endif // UPD_REQUEST_OPERATION_H
