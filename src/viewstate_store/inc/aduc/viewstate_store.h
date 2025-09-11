#ifndef ADUC_VIEWSTATE_STORE_H
#define ADUC_VIEWSTATE_STORE_H

#include <aduc/aducsdk.h>
#include <aduc/types/update_content.h>

#include <stdint.h>
#include <unistd.h>

typedef ssize_t ADUC_StateStoreHandle;

typedef struct tagADUC_StateStoreData {
    int32_t version;
    ADUC_ServiceStatus viewState;
} ADUC_StateStoreData;

ADUC_StateStoreHandle ADUC_StateStore_Create();
void ADUC_StateStore_Destroy(ADUC_StateStoreHandle h);

ADUC_Result ADUC_StateStore_GetServiceStatus(ADUC_StateStoreHandle stateStore, ADUC_ServiceStatus* out_status);
ADUC_Result ADUC_StateStore_SetServiceStatus(ADUC_StateStoreHandle stateStore, ADUC_ServiceStatus status);

#endif // ADUC_VIEWSTATE_STORE_H
