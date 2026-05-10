/**
 * @file swupdate_adapter.c
 * @brief SWUpdate backward-compatibility adapter (stub).
 *
 * Implements ADUC_StepHandlerVtable as a placeholder for the Gen1
 * SWUpdate handler integration.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "swupdate_adapter.h"

#include "aduc/extension_context.h"
#include "aduc/extension_descriptor.h"
#include "aduc/extension_types.h"
#include "aduc/step_handler_vtable.h"
#include "aduc/step_result.h"

#include <stdlib.h>
#include <string.h>

#define SWU_COMP "swupdate-adapter"

static const ADUC_ExtensionContext* s_extCtx = NULL;

/* ─── Per-step handle state ───────────────────────────────────────────────── */

typedef struct SwuStepState
{
    ADUC_StepContext stepCtx;
    ADUC_StepResult collectedResult;
} SwuStepState;

/* ─── Vtable implementations ─────────────────────────────────────────────── */

static ADUC_Result2 Swu_Evaluate(const ADUC_StepContext* ctx, ADUC_StepHandle* handle)
{
    ADUC_EXT_LOG_INFO(s_extCtx, SWU_COMP, 3001, "Evaluate (stub)");

    SwuStepState* state = (SwuStepState*)calloc(1, sizeof(SwuStepState));
    if (state == NULL)
    {
        return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_RESOURCE, 1);
    }

    state->stepCtx = *ctx;
    memset(&state->collectedResult, 0, sizeof(state->collectedResult));
    *handle = (ADUC_StepHandle)state;

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 Swu_Acquire(ADUC_StepHandle handle)
{
    (void)handle;
    ADUC_EXT_LOG_INFO(s_extCtx, SWU_COMP, 3002, "Acquire (stub)");
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 Swu_Preprocess(ADUC_StepHandle handle)
{
    (void)handle;
    ADUC_EXT_LOG_INFO(s_extCtx, SWU_COMP, 3003, "Preprocess (stub)");
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 Swu_Execute(ADUC_StepHandle handle, ADUC_StepProgressFn progressFn, void* progressCtx)
{
    SwuStepState* state = (SwuStepState*)handle;
    const char* filename = "(unknown)";

    if (state->stepCtx.files != NULL && state->stepCtx.fileCount > 0 && state->stepCtx.files[0].name != NULL)
    {
        filename = state->stepCtx.files[0].name;
    }

    ADUC_EXT_LOG_INFO(s_extCtx, SWU_COMP, 3004, "SWUpdate adapter: would apply %s", filename);

    if (progressFn != NULL)
    {
        progressFn(state->stepCtx.stepId, 100, "Execute", "SWUpdate stub complete", progressCtx);
    }

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 Swu_Validate(ADUC_StepHandle handle)
{
    (void)handle;
    ADUC_EXT_LOG_INFO(s_extCtx, SWU_COMP, 3005, "Validate (stub)");
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 Swu_Postprocess(ADUC_StepHandle handle, bool rollback)
{
    (void)handle;
    ADUC_EXT_LOG_INFO(s_extCtx, SWU_COMP, 3006, "Postprocess (stub, rollback=%s)", rollback ? "true" : "false");
    return ADUC_RESULT2_SUCCESS;
}

static ADUC_StepResult Swu_GetResult(ADUC_StepHandle handle)
{
    SwuStepState* state = (SwuStepState*)handle;
    state->collectedResult.result = ADUC_RESULT2_SUCCESS;
    state->collectedResult.resultDetails = "SWUpdate adapter stub completed successfully";
    state->collectedResult.signal = ADUC_SIGNAL_CONTINUE;
    state->collectedResult.rollbackPerformed = false;
    state->collectedResult.outputStateJson = NULL;
    return state->collectedResult;
}

static ADUC_Result2 Swu_Cancel(ADUC_StepHandle handle)
{
    (void)handle;
    ADUC_EXT_LOG_INFO(s_extCtx, SWU_COMP, 3007, "Cancel (stub)");
    return ADUC_RESULT2_SUCCESS;
}

static void Swu_Release(ADUC_StepHandle handle)
{
    if (handle != NULL)
    {
        free(handle);
    }
}

static ADUC_StepResultDetail Swu_Report(ADUC_StepHandle handle)
{
    (void)handle;
    return ADUC_StepResult_Success(ADUC_STEP_PHASE_REPORT);
}

static ADUC_StepResultDetail Swu_Signal(ADUC_StepHandle handle)
{
    (void)handle;
    return ADUC_StepResult_Success(ADUC_STEP_PHASE_SIGNAL);
}

static ADUC_Result2 Swu_IsInstalled(const ADUC_StepContext* ctx, bool* outIsInstalled)
{
    (void)ctx;
    if (outIsInstalled != NULL)
    {
        *outIsInstalled = false;
    }
    return ADUC_RESULT2_SUCCESS;
}

static const char* s_capabilities[] = { "microsoft/swupdate:2", NULL };

static const char** Swu_GetCapabilities(void)
{
    return s_capabilities;
}

/* ─── Vtable ──────────────────────────────────────────────────────────────── */

static const ADUC_StepHandlerVtable s_vtable = {
    .structVersion = 1,
    .Evaluate = Swu_Evaluate,
    .Acquire = Swu_Acquire,
    .Preprocess = Swu_Preprocess,
    .Execute = Swu_Execute,
    .Validate = Swu_Validate,
    .Postprocess = Swu_Postprocess,
    .GetResult = Swu_GetResult,
    .Cancel = Swu_Cancel,
    .Release = Swu_Release,
    .Report = Swu_Report,
    .Signal = Swu_Signal,
    .IsInstalled = Swu_IsInstalled,
    .GetCapabilities = Swu_GetCapabilities,
};

/* ─── Extension lifecycle ─────────────────────────────────────────────────── */

ADUC_Result2 SwUpdateAdapter_Initialize(const ADUC_ExtensionContext* ctx)
{
    s_extCtx = ctx;
    ADUC_EXT_LOG_INFO(ctx, SWU_COMP, 3000, "SWUpdate adapter initialized (stub)");
    return ADUC_RESULT2_SUCCESS;
}

void SwUpdateAdapter_Uninitialize(void)
{
    if (s_extCtx != NULL)
    {
        ADUC_EXT_LOG_INFO(s_extCtx, SWU_COMP, 3099, "SWUpdate adapter uninitialized");
    }
    s_extCtx = NULL;
}

/* ─── Extension descriptor ────────────────────────────────────────────────── */

static const ADUC_ExtensionDescriptor s_descriptor = {
    .structVersion = ADUC_EXTENSION_DESCRIPTOR_VERSION,
    .id = "swupdate-adapter",
    .name = "SWUpdate Backward-Compat Adapter",
    .version = "1.0.0",
    .type = ADUC_EXT_TYPE_STEP_HANDLER,
    .minHostApiVersion = 1,
    .Initialize = SwUpdateAdapter_Initialize,
    .Uninitialize = SwUpdateAdapter_Uninitialize,
    .vtable = &s_vtable,
    .capabilities = s_capabilities,
};

ADUC_DECLARE_EXTENSION(s_descriptor)
