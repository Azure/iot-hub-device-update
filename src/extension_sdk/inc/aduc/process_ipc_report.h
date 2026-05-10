#ifndef ADUC_PROCESS_IPC_REPORT_H
#define ADUC_PROCESS_IPC_REPORT_H

#include "aduc/extension_types.h"
#include "aduc/step_result.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// IPC message types for child->parent communication
typedef enum ADUC_IpcMessageType {
    ADUC_IPC_MSG_PROGRESS = 1,       // Report progress update
    ADUC_IPC_MSG_RESULT = 2,         // Report step completion
    ADUC_IPC_MSG_LOG = 3,            // Forward a log message
    ADUC_IPC_MSG_CANCEL_ACK = 4,     // Acknowledge cancel request
} ADUC_IpcMessageType;

typedef struct ADUC_IpcProgressMsg {
    char stepId[64];
    uint32_t percentComplete;
    uint64_t bytesTransferred;
    uint64_t bytesTotal;
    char statusMessage[256];
} ADUC_IpcProgressMsg;

typedef struct ADUC_IpcResultMsg {
    char stepId[64];
    ADUC_StepResultDetail result;
} ADUC_IpcResultMsg;

// Client handle for child process IPC
typedef struct ADUC_IpcClient* ADUC_IpcClientHandle;

// Connect to parent's IPC socket
ADUC_Result2 ADUC_IpcClient_Connect(const char* socketPath, ADUC_IpcClientHandle* outHandle);
// Send progress report
ADUC_Result2 ADUC_IpcClient_ReportProgress(ADUC_IpcClientHandle handle, const ADUC_IpcProgressMsg* msg);
// Send final result
ADUC_Result2 ADUC_IpcClient_ReportResult(ADUC_IpcClientHandle handle, const ADUC_IpcResultMsg* msg);
// Check if parent sent cancel
bool ADUC_IpcClient_IsCancelRequested(ADUC_IpcClientHandle handle);
// Acknowledge cancel
ADUC_Result2 ADUC_IpcClient_AckCancel(ADUC_IpcClientHandle handle);
// Disconnect
void ADUC_IpcClient_Disconnect(ADUC_IpcClientHandle handle);

#ifdef __cplusplus
}
#endif
#endif
