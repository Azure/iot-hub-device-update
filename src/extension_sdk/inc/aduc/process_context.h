#ifndef ADUC_PROCESS_CONTEXT_H
#define ADUC_PROCESS_CONTEXT_H

#include "aduc/extension_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ADUC_ProcessContextData {
    const char* workflowId;
    const char* deploymentId;
    const char* stepId;
    const char* handlerType;
    const char* workFolder;          // Working directory for the child
    const char* contentDir;          // Where downloaded content lives
    const char* ipcSocketPath;       // Local API socket for reporting back
    const char* installedCriteria;
    uint32_t stepIndex;
    uint32_t totalSteps;
    // Component targeting
    const char* componentId;
    const char* componentGroup;
} ADUC_ProcessContextData;

// Write context TOML file to the specified path
ADUC_Result2 ADUC_ProcessContext_Write(const char* filePath, const ADUC_ProcessContextData* data);
// Read context from TOML file (child-side)
ADUC_Result2 ADUC_ProcessContext_Read(const char* filePath, ADUC_ProcessContextData* outData);
// Free strings allocated by Read
void ADUC_ProcessContext_Free(ADUC_ProcessContextData* data);

#ifdef __cplusplus
}
#endif
#endif
