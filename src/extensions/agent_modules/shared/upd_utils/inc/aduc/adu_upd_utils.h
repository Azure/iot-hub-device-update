#ifndef ADU_UPD_UTILS_H
#define ADU_UPD_UTILS_H

#include <aduc/adu_upd.h> // ADUC_Update_Request_Operation_Data
#include <aduc/retry_utils.h> // ADUC_Retriable_Operation_Context
#include <aduc/workqueue.h> // WorkQueueHandle

EXTERN_C_BEGIN

const char* upd_state_str(ADU_UPD_STATE st);
ADUC_Update_Request_Operation_Data* UpdateData_FromOperationContext(ADUC_Retriable_Operation_Context* context);
WorkQueueHandle WorkQueueHandleFromCallbackUserObj(void* obj);
WorkQueueHandle ReportingWorkQueueHandleFromCallbackUserObj(void* obj);
ADUC_Retriable_Operation_Context* RetriableOperationContextFromCallbackUserObj(void* obj);
ADUC_Update_Request_Operation_Data*
UpdateDataFromRetriableOperationContext(ADUC_Retriable_Operation_Context* retriableOperationContext);
bool UpdateData_SetCorrelationId(ADUC_Update_Request_Operation_Data* updateData, const char* correlationId);
void AduUpdUtils_TransitionState(
    ADU_UPD_STATE newState,
    ADUC_Update_Request_Operation_Data* updateData,
    ADUC_Retriable_Operation_Context* retriableOperationContext);
void AduUpdUtils_HandleProcessing(
    ADUC_Update_Request_Operation_Data* updateData, ADUC_Retriable_Operation_Context* retriableOperationContext);

EXTERN_C_END

#endif // ADU_UPD_UTILS_H
