#ifndef ADUC_CHILD_REPORTER_H
#define ADUC_CHILD_REPORTER_H

#include "aduc/extension_types.h"
#include "aduc/process_context.h"
#include "aduc/process_ipc_report.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ADUC_ChildReporter* ADUC_ChildReporterHandle;

// Initialize from context file (reads context, connects to IPC)
ADUC_Result2 ADUC_ChildReporter_Init(const char* contextFilePath, ADUC_ChildReporterHandle* outHandle);
// Get the context data
const ADUC_ProcessContextData* ADUC_ChildReporter_GetContext(ADUC_ChildReporterHandle handle);
// Report progress (percentage)
ADUC_Result2 ADUC_ChildReporter_ReportProgress(ADUC_ChildReporterHandle handle, uint32_t percent, const char* message);
// Report success
ADUC_Result2 ADUC_ChildReporter_ReportSuccess(ADUC_ChildReporterHandle handle);
// Report failure
ADUC_Result2 ADUC_ChildReporter_ReportFailure(ADUC_ChildReporterHandle handle, ADUC_Result2 errorCode, const char* message);
// Check for cancel
bool ADUC_ChildReporter_IsCancelled(ADUC_ChildReporterHandle handle);
// Cleanup
void ADUC_ChildReporter_Destroy(ADUC_ChildReporterHandle handle);

#ifdef __cplusplus
}
#endif
#endif
