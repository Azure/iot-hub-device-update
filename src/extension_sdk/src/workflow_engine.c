/**
 * @file workflow_engine.c
 * @brief Workflow engine implementation — orchestrates deployment step execution.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/workflow_engine.h"
#include "aduc/dag_engine.h"
#include "aduc/extension_descriptor.h"

#ifdef _MSC_VER
/* MSVC doesn't support C11 <stdatomic.h>; use volatile + Interlocked */
#include <windows.h>
typedef volatile long adu_atomic_bool;
#define adu_atomic_init(ptr, val) (*(ptr) = (val))
#define adu_atomic_load(ptr) (InterlockedCompareExchange((ptr), 0, 0) != 0)
#define adu_atomic_store(ptr, val) InterlockedExchange((ptr), (long)(val))
#else
#include <stdatomic.h>
typedef atomic_bool adu_atomic_bool;
#define adu_atomic_init(ptr, val) atomic_init(ptr, val)
#define adu_atomic_load(ptr) atomic_load(ptr)
#define adu_atomic_store(ptr, val) atomic_store(ptr, val)
#endif
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/*  Internal types                                                            */
/* -------------------------------------------------------------------------- */

typedef struct WorkflowInstance
{
    const ADUC_Deployment* deployment;
    ADUC_ExtensionRegistryHandle registry;
    ADUC_WorkflowProgressFn progressFn;
    ADUC_WorkflowCompleteFn completeFn;
    void* callbackCtx;

    ADUC_WorkflowState state;
    adu_atomic_bool cancelRequested;

    /* Current step handler (for cancel forwarding) */
    ADUC_StepHandle currentStepHandle;
    const ADUC_StepHandlerVtable* currentVtable;

    /* Collected results */
    ADUC_StepResult* stepResults;
    size_t stepResultCount;
} WorkflowInstance;

/* -------------------------------------------------------------------------- */
/*  Helpers                                                                    */
/* -------------------------------------------------------------------------- */

static void report_progress(
    WorkflowInstance* wf,
    const char* stepId,
    const char* phase,
    uint32_t percent)
{
    if (wf->progressFn != NULL)
    {
        wf->progressFn(wf->deployment->workflowId, stepId, phase, percent, wf->callbackCtx);
    }
}

static bool is_cancel_requested(WorkflowInstance* wf)
{
    return adu_atomic_load(&wf->cancelRequested);
}

/**
 * @brief Build a StepContext from a DeploymentStep.
 */
static ADUC_StepContext make_step_context(
    const ADUC_Deployment* deployment,
    const ADUC_DeploymentStep* step,
    uint32_t attempt)
{
    ADUC_StepContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.workflowId = deployment->workflowId;
    ctx.stepId = step->stepId;
    ctx.attempt = attempt;
    ctx.files = step->files;
    ctx.fileCount = step->fileCount;
    ctx.handlerConfigJson = step->handlerConfigJson;
    ctx.componentId = step->componentId;
    ctx.componentGroup = step->componentGroup;
    ctx.componentProperties = step->componentProperties;
    ctx.installedCriteria = step->installedCriteria;
    return ctx;
}

/**
 * @brief Find the step handler vtable for a given handler type from the registry.
 */
static const ADUC_StepHandlerVtable* find_handler_vtable(
    ADUC_ExtensionRegistryHandle registry,
    const char* handlerType)
{
    const ADUC_ExtensionDescriptor* desc =
        ADUC_ExtensionRegistry_FindByCapability(registry, handlerType);
    if (desc == NULL)
    {
        return NULL;
    }
    if (desc->type != ADUC_EXT_TYPE_STEP_HANDLER)
    {
        return NULL;
    }
    return (const ADUC_StepHandlerVtable*)desc->vtable;
}

/**
 * @brief Find a deployment step by its stepId.
 */
static const ADUC_DeploymentStep* find_step_by_id(const ADUC_Deployment* deployment, const char* stepId)
{
    if (deployment == NULL || stepId == NULL)
    {
        return NULL;
    }
    for (size_t i = 0; i < deployment->stepCount; i++)
    {
        if (deployment->steps[i].stepId != NULL && strcmp(deployment->steps[i].stepId, stepId) == 0)
        {
            return &deployment->steps[i];
        }
    }
    return NULL;
}

/**
 * @brief Execute one deployment step through its full lifecycle.
 *
 * Lifecycle: Evaluate → Acquire → Preprocess → Execute → Validate → Postprocess
 * On failure: calls Postprocess(rollback=true).
 *
 * @return The step result.
 */
static ADUC_StepResult execute_step(
    WorkflowInstance* wf,
    const ADUC_DeploymentStep* step,
    const ADUC_StepHandlerVtable* vtable)
{
    ADUC_StepResult result;
    memset(&result, 0, sizeof(result));

    ADUC_StepContext ctx = make_step_context(wf->deployment, step, 1);
    ADUC_StepHandle handle = NULL;
    ADUC_Result2 phaseResult;
    bool needRollback = false;

    /* Track current handler for cancel */
    wf->currentVtable = vtable;
    wf->currentStepHandle = NULL;

    /* --- Evaluate --- */
    report_progress(wf, step->stepId, "Evaluate", 0);
    phaseResult = vtable->Evaluate(&ctx, &handle);
    wf->currentStepHandle = handle;

    if (ADUC_RESULT2_IS_FAILURE(phaseResult))
    {
        /* Evaluate failure means skip this step (not necessarily deployment failure) */
        result.result = phaseResult;
        result.resultDetails = "Evaluate failed";
        result.signal = ADUC_SIGNAL_CONTINUE;
        goto done;
    }

    if (is_cancel_requested(wf))
    {
        goto cancelled;
    }

    /* --- Acquire --- */
    report_progress(wf, step->stepId, "Acquire", 10);
    phaseResult = vtable->Acquire(handle);
    if (ADUC_RESULT2_IS_FAILURE(phaseResult))
    {
        needRollback = true;
        goto failed;
    }

    if (is_cancel_requested(wf))
    {
        needRollback = true;
        goto cancelled;
    }

    /* --- Preprocess --- */
    report_progress(wf, step->stepId, "Preprocess", 20);
    phaseResult = vtable->Preprocess(handle);
    if (ADUC_RESULT2_IS_FAILURE(phaseResult))
    {
        needRollback = true;
        goto failed;
    }

    if (is_cancel_requested(wf))
    {
        needRollback = true;
        goto cancelled;
    }

    /* --- Execute --- */
    report_progress(wf, step->stepId, "Execute", 30);
    phaseResult = vtable->Execute(handle, NULL, NULL);
    if (ADUC_RESULT2_IS_FAILURE(phaseResult))
    {
        needRollback = true;
        goto failed;
    }

    if (is_cancel_requested(wf))
    {
        needRollback = true;
        goto cancelled;
    }

    /* --- Validate --- */
    report_progress(wf, step->stepId, "Validate", 80);
    phaseResult = vtable->Validate(handle);
    if (ADUC_RESULT2_IS_FAILURE(phaseResult))
    {
        needRollback = true;
        goto failed;
    }

    if (is_cancel_requested(wf))
    {
        needRollback = true;
        goto cancelled;
    }

    /* --- Postprocess (success) --- */
    report_progress(wf, step->stepId, "Postprocess", 90);
    vtable->Postprocess(handle, false);

    report_progress(wf, step->stepId, "Complete", 100);
    result = vtable->GetResult(handle);
    goto done;

failed:
    /* Run Postprocess with rollback */
    report_progress(wf, step->stepId, "Postprocess(rollback)", 90);
    vtable->Postprocess(handle, true);

    result = vtable->GetResult(handle);
    result.result = phaseResult;
    result.rollbackPerformed = true;
    goto done;

cancelled:
    if (handle != NULL)
    {
        vtable->Cancel(handle);
        if (needRollback)
        {
            vtable->Postprocess(handle, true);
        }
    }
    result.result = ADUC_RESULT2_MAKE(0, 1, 1); /* generic cancel code */
    result.resultDetails = "Cancelled";
    result.signal = ADUC_SIGNAL_ABORT_DEPLOYMENT;
    result.rollbackPerformed = needRollback;
    goto done;

done:
    if (handle != NULL)
    {
        vtable->Release(handle);
    }
    wf->currentStepHandle = NULL;
    wf->currentVtable = NULL;
    return result;
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                 */
/* -------------------------------------------------------------------------- */

ADUC_Result2 ADUC_Workflow_Execute(
    const ADUC_Deployment* deployment,
    ADUC_ExtensionRegistryHandle registry,
    ADUC_WorkflowProgressFn progressFn,
    ADUC_WorkflowCompleteFn completeFn,
    void* callbackCtx,
    ADUC_WorkflowEngineHandle* outHandle)
{
    if (deployment == NULL || registry == NULL || outHandle == NULL)
    {
        return ADUC_RESULT2_MAKE(0, 1, 2); /* invalid argument */
    }

    WorkflowInstance* wf = (WorkflowInstance*)calloc(1, sizeof(WorkflowInstance));
    if (wf == NULL)
    {
        return ADUC_RESULT2_MAKE(0, 1, 3); /* out of memory */
    }

    wf->deployment = deployment;
    wf->registry = registry;
    wf->progressFn = progressFn;
    wf->completeFn = completeFn;
    wf->callbackCtx = callbackCtx;
    wf->state = ADUC_WF_STATE_RUNNING;
    adu_atomic_init(&wf->cancelRequested, false);

    /* Allocate result array */
    wf->stepResults = (ADUC_StepResult*)calloc(deployment->stepCount, sizeof(ADUC_StepResult));
    if (wf->stepResults == NULL && deployment->stepCount > 0)
    {
        free(wf);
        return ADUC_RESULT2_MAKE(0, 1, 3);
    }
    wf->stepResultCount = 0;

    /* Build DAG node definitions from deployment steps */
    ADUC_DagNodeDef* dagDefs = NULL;
    if (deployment->stepCount > 0)
    {
        dagDefs = (ADUC_DagNodeDef*)calloc(deployment->stepCount, sizeof(ADUC_DagNodeDef));
        if (dagDefs == NULL)
        {
            free(wf->stepResults);
            free(wf);
            return ADUC_RESULT2_MAKE(0, 1, 3);
        }

        for (size_t i = 0; i < deployment->stepCount; i++)
        {
            dagDefs[i].stepId = deployment->steps[i].stepId;
            dagDefs[i].dependsOn = deployment->steps[i].requires;
            dagDefs[i].dependsOnCount = deployment->steps[i].requiresCount;
            dagDefs[i].skipOnFailed = deployment->steps[i].skipOnFailed;
            dagDefs[i].skipOnFailedCount = deployment->steps[i].skipOnFailedCount;
            dagDefs[i].runOnFailed = deployment->steps[i].runOnFailed;
            dagDefs[i].runOnFailedCount = deployment->steps[i].runOnFailedCount;
        }
    }

    ADUC_DagEngineHandle dag = NULL;
    ADUC_Result2 dagResult = ADUC_DagEngine_Create(dagDefs, deployment->stepCount, &dag);
    free(dagDefs);

    if (ADUC_RESULT2_IS_FAILURE(dagResult))
    {
        free(wf->stepResults);
        free(wf);
        return dagResult;
    }

    if (ADUC_DagEngine_HasCycle(dag))
    {
        ADUC_DagEngine_Destroy(dag);
        free(wf->stepResults);
        free(wf);
        return ADUC_RESULT2_MAKE(0, 1, 4); /* cycle detected */
    }

    *outHandle = (ADUC_WorkflowEngineHandle)wf;

    /* Execute steps via DAG scheduling */
    ADUC_Result2 overallResult = ADUC_RESULT2_SUCCESS;
    bool aborted = false;

    while (!ADUC_DagEngine_IsComplete(dag) && !aborted)
    {
        if (is_cancel_requested(wf))
        {
            wf->state = ADUC_WF_STATE_CANCELLED;
            overallResult = ADUC_RESULT2_MAKE(0, 1, 1);
            break;
        }

        const char* readyIds[16];
        size_t readyCount = ADUC_DagEngine_GetReady(dag, readyIds, 16);

        if (readyCount == 0)
        {
            /* No ready nodes but not complete — all remaining are blocked/failed */
            break;
        }

        for (size_t r = 0; r < readyCount && !aborted; r++)
        {
            const char* stepId = readyIds[r];
            const ADUC_DeploymentStep* step = find_step_by_id(deployment, stepId);
            if (step == NULL)
            {
                ADUC_DagEngine_MarkFailed(dag, stepId);
                overallResult = ADUC_RESULT2_MAKE(0, 2, 2);
                aborted = true;
                continue;
            }

            /* Find handler */
            const ADUC_StepHandlerVtable* vtable = find_handler_vtable(registry, step->handlerType);
            if (vtable == NULL)
            {
                ADUC_StepResult failResult;
                memset(&failResult, 0, sizeof(failResult));
                failResult.result = ADUC_RESULT2_MAKE(0, 2, 1);
                failResult.resultDetails = "Handler not found for handlerType";
                failResult.signal = ADUC_SIGNAL_ABORT_DEPLOYMENT;
                wf->stepResults[wf->stepResultCount++] = failResult;
                ADUC_DagEngine_MarkFailed(dag, stepId);
                overallResult = failResult.result;
                aborted = true;
                continue;
            }

            /* Execute the step */
            ADUC_StepResult stepResult = execute_step(wf, step, vtable);
            wf->stepResults[wf->stepResultCount++] = stepResult;

            /* Check signals */
            switch (stepResult.signal)
            {
                case ADUC_SIGNAL_ABORT_DEPLOYMENT:
                    ADUC_DagEngine_MarkFailed(dag, stepId);
                    overallResult = stepResult.result;
                    aborted = true;
                    break;

                case ADUC_SIGNAL_IMMEDIATE_REBOOT:
                    ADUC_DagEngine_MarkDone(dag, stepId);
                    overallResult = ADUC_RESULT2_SUCCESS;
                    aborted = true;
                    break;

                case ADUC_SIGNAL_SKIP_REMAINING:
                    ADUC_DagEngine_MarkDone(dag, stepId);
                    aborted = true;
                    break;

                case ADUC_SIGNAL_CONTINUE:
                case ADUC_SIGNAL_DEFER_REBOOT:
                default:
                    if (ADUC_RESULT2_IS_FAILURE(stepResult.result))
                    {
                        ADUC_DagEngine_MarkFailed(dag, stepId);
                        overallResult = stepResult.result;
                        aborted = true;
                    }
                    else
                    {
                        ADUC_DagEngine_MarkDone(dag, stepId);
                    }
                    break;
            }
        }
    }

    ADUC_DagEngine_Destroy(dag);

    /* Set final state */
    if (is_cancel_requested(wf))
    {
        wf->state = ADUC_WF_STATE_CANCELLED;
    }
    else if (ADUC_RESULT2_IS_FAILURE(overallResult))
    {
        wf->state = ADUC_WF_STATE_FAILED;
    }
    else
    {
        wf->state = ADUC_WF_STATE_COMPLETE;
    }

    /* Invoke completion callback */
    if (wf->completeFn != NULL)
    {
        wf->completeFn(
            deployment->workflowId,
            overallResult,
            wf->stepResults,
            wf->stepResultCount,
            wf->callbackCtx);
    }

    return overallResult;
}

void ADUC_Workflow_Cancel(ADUC_WorkflowEngineHandle handle)
{
    if (handle == NULL)
    {
        return;
    }

    WorkflowInstance* wf = (WorkflowInstance*)handle;
    adu_atomic_store(&wf->cancelRequested, true);
    wf->state = ADUC_WF_STATE_CANCELLING;

    /* Forward cancel to the active handler if one is running */
    if (wf->currentVtable != NULL && wf->currentStepHandle != NULL)
    {
        wf->currentVtable->Cancel(wf->currentStepHandle);
    }
}

ADUC_WorkflowState ADUC_Workflow_GetState(ADUC_WorkflowEngineHandle handle)
{
    if (handle == NULL)
    {
        return ADUC_WF_STATE_IDLE;
    }

    WorkflowInstance* wf = (WorkflowInstance*)handle;
    return wf->state;
}

void ADUC_Workflow_Destroy(ADUC_WorkflowEngineHandle handle)
{
    if (handle == NULL)
    {
        return;
    }

    WorkflowInstance* wf = (WorkflowInstance*)handle;
    free(wf->stepResults);
    free(wf);
}
