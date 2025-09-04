#ifndef ADUC_VIEWSTATE_STORE_H
#define ADUC_VIEWSTATE_STORE_H

#include <aduc/aducsdk.h>
#include <aduc/types/update_content.h>

#include <stdint.h>
#include <unistd.h>

// /var/lib/adu/statestore/store.json 

// { 
//     "version": 1, 
//     "state": 0, // number of ADUCITF_WorkflowStep enum
//     "viewstate": 0, // number ADUC_ViewState_ServiceStatus enum
//     "lastRecvMsg": "<Escaped JSON>", // the C2D message from IoTHub 
// }   

// /var/lib/adu/statestore/reporting/lastreport.json
// { 
//     "version": 1, 
//     "report": "<escaped json>", 
//     "reportDate": 1756869087, // number: unix time 
// } 

// /var/lib/adu/statestore/reporting/pendingreport.json
// { 
//     "version": 1, 
//     "report": "<escaped json>", 
//     "numRetries": 0, // number 
//     "startTime": 1756869087, // number: unix time 
// } 

typedef ssize_t ADUC_StateStoreHandle;

typedef struct tagADUC_StateStoreData {
    int32_t version;
    ADUCITF_WorkflowStep state;
    ADUC_ServiceStatus viewState;
    char* lastReceivedMessage;
} ADUC_StateStoreData; 

typedef struct tagADUC_LastReportData {
    int32_t version;
    char* report;
    time_t reportDate;
} ADUC_LastReportData;

typedef struct tagADUC_PendingReportData {
    int32_t version;
    char* report;
    int32_t numRetries;
    time_t startTime;
} ADUC_PendingReportData; 

ADUC_StateStoreHandle ADUC_StateStore_Create(const char* baseDir);
void ADUC_StateStore_Destroy(ADUC_StateStoreHandle stateStore);

ADUC_Result ADUC_StateStore_ReadState(ADUC_StateStoreHandle stateStore, ADUC_StateStoreData* stateData);
ADUC_Result ADUC_StateStore_WriteState(ADUC_StateStoreHandle stateStore, const ADUC_StateStoreData* stateData);

ADUC_Result ADUC_StateStore_SetAgentState(ADUC_StateStoreHandle stateStore, ADUC_AgentState state);

ADUC_Result ADUC_StateStore_SetViewState(ADUC_StateStoreHandle stateStore, ADUC_ServiceStatus viewState);
ADUC_Result ADUC_StateStore_SetLastReceivedMessage(ADUC_StateStoreHandle stateStore, const char* message); 

ADUC_Result ADUC_StateStore_ReadLastReport(ADUC_StateStoreHandle stateStore, ADUC_LastReportData* reportData);
ADUC_Result ADUC_StateStore_WriteLastReport(ADUC_StateStoreHandle stateStore, const ADUC_LastReportData* reportData);

ADUC_Result ADUC_StateStore_ReadPendingReport(ADUC_StateStoreHandle stateStore, ADUC_PendingReportData* reportData);
ADUC_Result ADUC_StateStore_WritePendingReport(ADUC_StateStoreHandle stateStore, const ADUC_PendingReportData* reportData);

ADUC_Result ADUC_StateStore_ClearPendingReport(ADUC_StateStoreHandle stateStore);
ADUC_Result ADUC_StateStore_IncrementPendingReportRetryCount(ADUC_StateStoreHandle stateStore);

void ADUC_StateStoreData_Init(ADUC_StateStoreData* stateData);
void ADUC_StateStoreData_Free(ADUC_StateStoreData* stateData);

void ADUC_LastReportData_Init(ADUC_LastReportData* reportData);
void ADUC_LastReportData_Free(ADUC_LastReportData* reportData);

void ADUC_PendingReportData_Init(ADUC_PendingReportData* reportData);
void ADUC_PendingReportData_Free(ADUC_PendingReportData* reportData); 

#endif // ADUC_VIEWSTATE_STORE_H