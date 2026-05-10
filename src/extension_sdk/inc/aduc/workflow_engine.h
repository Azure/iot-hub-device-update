/**
 * @file workflow_engine.h
 * @brief Workflow engine — orchestrates deployment step execution.
 *
 * The workflow engine receives a parsed deployment manifest and iterates
 * through steps in order, running the lifecycle for each:
 *   Evaluate → Acquire → Preprocess → Execute → Validate → Postprocess
 *
 * Supports cancellation, progress reporting, and rollback on failure.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_WORKFLOW_ENGINE_H
#define ADUC_WORKFLOW_ENGINE_H

#include "aduc/extension_types.h"
#include "aduc/manifest_parser.h"
#include "aduc/step_handler_vtable.h"
#include "aduc/extension_loader.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Workflow execution states.
 */
typedef enum ADUC_WorkflowState
{
    ADUC_WF_STATE_IDLE = 0,
    ADUC_WF_STATE_RUNNING,
    ADUC_WF_STATE_CANCELLING,
    ADUC_WF_STATE_COMPLETE,
    ADUC_WF_STATE_FAILED,
    ADUC_WF_STATE_CANCELLED,
} ADUC_WorkflowState;

/**
 * @brief A complete deployment (parsed from deployment manifest).
 */
typedef struct ADUC_Deployment
{
    const char* workflowId;
    const char* updateId;              /**< provider.name.version */
    ADUC_DeploymentStep* steps;
    size_t stepCount;
    uint32_t retryLimit;
} ADUC_Deployment;

/**
 * @brief Opaque workflow instance handle.
 */
typedef void* ADUC_WorkflowEngineHandle;

/**
 * @brief Workflow progress callback.
 *
 * Called as each step phase begins/completes.
 */
typedef void (*ADUC_WorkflowProgressFn)(
    const char* workflowId,
    const char* stepId,
    const char* phase,
    uint32_t percentComplete,
    void* ctx);

/**
 * @brief Workflow completion callback.
 *
 * Called when the entire workflow finishes (success, failure, or cancel).
 */
typedef void (*ADUC_WorkflowCompleteFn)(
    const char* workflowId,
    ADUC_Result2 overallResult,
    const ADUC_StepResult* stepResults,
    size_t stepCount,
    void* ctx);

/**
 * @brief Execute a deployment workflow.
 *
 * Creates a workflow instance and iterates through steps synchronously.
 * Calls completeFn when done.
 *
 * @param deployment   Parsed deployment manifest.
 * @param registry     Extension registry to find step handlers.
 * @param progressFn   Progress callback (may be NULL).
 * @param completeFn   Completion callback (may be NULL).
 * @param callbackCtx  Opaque context for callbacks.
 * @param outHandle    Receives workflow handle (for cancel/query).
 * @return ADUC_Result2 Success if workflow started, failure otherwise.
 */
ADUC_Result2 ADUC_Workflow_Execute(
    const ADUC_Deployment* deployment,
    ADUC_ExtensionRegistryHandle registry,
    ADUC_WorkflowProgressFn progressFn,
    ADUC_WorkflowCompleteFn completeFn,
    void* callbackCtx,
    ADUC_WorkflowEngineHandle* outHandle);

/**
 * @brief Request cancellation of a running workflow.
 *
 * Sets the cancel flag and calls the current handler's Cancel().
 * The workflow will transition to CANCELLED state.
 */
void ADUC_Workflow_Cancel(ADUC_WorkflowEngineHandle handle);

/**
 * @brief Get the current state of a workflow.
 */
ADUC_WorkflowState ADUC_Workflow_GetState(ADUC_WorkflowEngineHandle handle);

/**
 * @brief Destroy a workflow instance and free resources.
 *
 * Must be called after the workflow has completed/failed/cancelled.
 */
void ADUC_Workflow_Destroy(ADUC_WorkflowEngineHandle handle);

#ifdef __cplusplus
}
#endif

#endif /* ADUC_WORKFLOW_ENGINE_H */
