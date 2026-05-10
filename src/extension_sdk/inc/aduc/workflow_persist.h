/**
 * @file workflow_persist.h
 * @brief Save/restore workflow state for resume after restart.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_WORKFLOW_PERSIST_H
#define ADUC_WORKFLOW_PERSIST_H

#include "aduc/extension_types.h"
#include "aduc/step_result.h"
#include "aduc/workflow_engine.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct ADUC_WorkflowState2 {
    char deploymentId[128];
    char workflowId[64];
    uint32_t currentStepIndex;
    ADUC_StepPhase currentPhase;
    ADUC_WorkflowState overallState;
    uint64_t lastUpdateTimeMs;
    char resumeToken[256];
} ADUC_WorkflowState2;

/**
 * @brief Save workflow state to file.
 */
ADUC_Result2 ADUC_WorkflowPersist_Save(const char* stateDir, const ADUC_WorkflowState2* state);

/**
 * @brief Load workflow state from file.
 * Returns success even if no state exists (outState zeroed).
 */
ADUC_Result2 ADUC_WorkflowPersist_Load(const char* stateDir, ADUC_WorkflowState2* outState);

/**
 * @brief Check if there's a persisted state to resume.
 */
bool ADUC_WorkflowPersist_HasState(const char* stateDir);

/**
 * @brief Clear persisted state (after successful completion).
 */
ADUC_Result2 ADUC_WorkflowPersist_Clear(const char* stateDir);

#ifdef __cplusplus
}
#endif

#endif /* ADUC_WORKFLOW_PERSIST_H */
