/**
 * @file simulator_handler.c
 * @brief Gen2 Simulator Step Handler implementation.
 *
 * Demonstrates the ADUC_StepHandlerVtable pattern with configurable delays
 * and failure injection for workflow engine testing.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "simulator_handler.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define sim_sleep_ms(ms) Sleep(ms)
#else
#include <time.h>
static void sim_sleep_ms(uint32_t ms)
{
    struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}
#endif

#define SIM_COMP "gen2-simulator"

/* ─── Simple JSON helpers (strstr-based, no dependencies) ─────────────────── */

static bool json_get_bool(const char* json, const char* key, bool defaultVal)
{
    if (json == NULL)
    {
        return defaultVal;
    }
    const char* pos = strstr(json, key);
    if (pos == NULL)
    {
        return defaultVal;
    }
    pos += strlen(key);
    /* Skip past ": */
    pos = strchr(pos, ':');
    if (pos == NULL)
    {
        return defaultVal;
    }
    pos++;
    while (*pos == ' ' || *pos == '\t')
    {
        pos++;
    }
    if (strncmp(pos, "true", 4) == 0)
    {
        return true;
    }
    return false;
}

static uint32_t json_get_uint(const char* json, const char* key, uint32_t defaultVal)
{
    if (json == NULL)
    {
        return defaultVal;
    }
    const char* pos = strstr(json, key);
    if (pos == NULL)
    {
        return defaultVal;
    }
    pos += strlen(key);
    pos = strchr(pos, ':');
    if (pos == NULL)
    {
        return defaultVal;
    }
    pos++;
    while (*pos == ' ' || *pos == '\t')
    {
        pos++;
    }
    uint32_t val = 0;
    while (*pos >= '0' && *pos <= '9')
    {
        val = val * 10 + (uint32_t)(*pos - '0');
        pos++;
    }
    return (val > 0) ? val : defaultVal;
}

static void json_get_string(const char* json, const char* key, char* out, size_t outLen, const char* defaultVal)
{
    if (out == NULL || outLen == 0)
    {
        return;
    }
    strncpy(out, defaultVal, outLen - 1);
    out[outLen - 1] = '\0';

    if (json == NULL)
    {
        return;
    }
    const char* pos = strstr(json, key);
    if (pos == NULL)
    {
        return;
    }
    pos += strlen(key);
    pos = strchr(pos, ':');
    if (pos == NULL)
    {
        return;
    }
    pos++;
    while (*pos == ' ' || *pos == '\t')
    {
        pos++;
    }
    if (*pos != '"')
    {
        return;
    }
    pos++;
    size_t i = 0;
    while (*pos != '\0' && *pos != '"' && i < outLen - 1)
    {
        out[i++] = *pos++;
    }
    out[i] = '\0';
}

static SimHandlerConfig parse_config(const char* json)
{
    SimHandlerConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.simulateFailure = json_get_bool(json, "simulateFailure", false);
    cfg.simulateReboot = json_get_bool(json, "simulateReboot", false);
    cfg.delayMs = json_get_uint(json, "delayMs", 100);
    json_get_string(json, "failInPhase", cfg.failInPhase, sizeof(cfg.failInPhase), "Execute");
    return cfg;
}

/* ─── Extension-level state ───────────────────────────────────────────────── */

static const ADUC_ExtensionContext* s_extCtx = NULL;

static ADUC_Result2 make_failure(void)
{
    return ADUC_RESULT2_MAKE(ADUC_FACILITY_EXTENSION, ADUC_CATEGORY_STATE, 1);
}

static bool should_fail(const ADUC_StepHandleImpl* h, const char* phase)
{
    return h->config.simulateFailure && strcmp(h->config.failInPhase, phase) == 0;
}

/* ─── Vtable implementations ─────────────────────────────────────────────── */

static ADUC_Result2 SimHandler_Evaluate(const ADUC_StepContext* ctx, ADUC_StepHandle* handle)
{
    ADUC_EXT_LOG_INFO(s_extCtx, SIM_COMP, 1001, "Evaluating step %s", ctx->stepId ? ctx->stepId : "(null)");

    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)calloc(1, sizeof(ADUC_StepHandleImpl));
    if (h == NULL)
    {
        return make_failure();
    }

    h->stepCtx = *ctx;
    h->config = parse_config(ctx->handlerConfigJson);
    h->extCtx = s_extCtx;
    h->cancelled = false;
    memset(&h->collectedResult, 0, sizeof(h->collectedResult));

    *handle = h;

    if (should_fail(h, "Evaluate"))
    {
        ADUC_EXT_LOG_ERROR(s_extCtx, SIM_COMP, 1002, "Simulated failure in Evaluate");
        h->collectedResult.result = make_failure();
        h->collectedResult.resultDetails = "Simulated failure in Evaluate";
        return make_failure();
    }

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SimHandler_Acquire(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;
    ADUC_EXT_LOG_INFO(h->extCtx, SIM_COMP, 1010, "Acquiring");

    sim_sleep_ms(h->config.delayMs);

    if (h->cancelled)
    {
        return make_failure();
    }

    if (should_fail(h, "Acquire"))
    {
        ADUC_EXT_LOG_ERROR(h->extCtx, SIM_COMP, 1011, "Simulated failure in Acquire");
        h->collectedResult.result = make_failure();
        h->collectedResult.resultDetails = "Simulated failure in Acquire";
        return make_failure();
    }

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SimHandler_Preprocess(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;
    ADUC_EXT_LOG_INFO(h->extCtx, SIM_COMP, 1020, "Preprocessing (backup)");

    sim_sleep_ms(h->config.delayMs);

    if (h->cancelled)
    {
        return make_failure();
    }

    if (should_fail(h, "Preprocess"))
    {
        ADUC_EXT_LOG_ERROR(h->extCtx, SIM_COMP, 1021, "Simulated failure in Preprocess");
        h->collectedResult.result = make_failure();
        h->collectedResult.resultDetails = "Simulated failure in Preprocess";
        return make_failure();
    }

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SimHandler_Execute(ADUC_StepHandle handle, ADUC_StepProgressFn progressFn, void* progressCtx)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;
    ADUC_EXT_LOG_INFO(h->extCtx, SIM_COMP, 1030, "Executing");

    static const uint32_t progressSteps[] = { 0, 25, 50, 75, 100 };
    static const size_t numSteps = sizeof(progressSteps) / sizeof(progressSteps[0]);

    for (size_t i = 0; i < numSteps; i++)
    {
        if (h->cancelled)
        {
            return make_failure();
        }

        if (progressFn != NULL)
        {
            progressFn(h->stepCtx.stepId, progressSteps[i], "Execute", "Simulating work", progressCtx);
        }

        if (i < numSteps - 1)
        {
            sim_sleep_ms(h->config.delayMs);
        }
    }

    if (should_fail(h, "Execute"))
    {
        ADUC_EXT_LOG_ERROR(h->extCtx, SIM_COMP, 1031, "Simulated failure in Execute");
        h->collectedResult.result = make_failure();
        h->collectedResult.resultDetails = "Simulated failure in Execute";
        return make_failure();
    }

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SimHandler_Validate(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;
    ADUC_EXT_LOG_INFO(h->extCtx, SIM_COMP, 1040, "Validating");

    if (should_fail(h, "Validate"))
    {
        ADUC_EXT_LOG_ERROR(h->extCtx, SIM_COMP, 1041, "Simulated failure in Validate");
        h->collectedResult.result = make_failure();
        h->collectedResult.resultDetails = "Simulated failure in Validate";
        return make_failure();
    }

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_Result2 SimHandler_Postprocess(ADUC_StepHandle handle, bool rollback)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;
    ADUC_EXT_LOG_INFO(h->extCtx, SIM_COMP, 1050, "Postprocessing");

    if (rollback)
    {
        ADUC_EXT_LOG_WARN(h->extCtx, SIM_COMP, 1051, "Rolling back");
        h->collectedResult.rollbackPerformed = true;
    }

    if (should_fail(h, "Postprocess"))
    {
        ADUC_EXT_LOG_ERROR(h->extCtx, SIM_COMP, 1052, "Simulated failure in Postprocess");
        h->collectedResult.result = make_failure();
        h->collectedResult.resultDetails = "Simulated failure in Postprocess";
        return make_failure();
    }

    return ADUC_RESULT2_SUCCESS;
}

static ADUC_StepResult SimHandler_GetResult(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;

    /* If no failure was collected, report overall success. */
    if (ADUC_RESULT2_IS_SUCCESS(h->collectedResult.result))
    {
        h->collectedResult.result = ADUC_RESULT2_SUCCESS;
        h->collectedResult.resultDetails = "Simulation completed successfully";
    }

    if (h->config.simulateReboot)
    {
        h->collectedResult.signal = ADUC_SIGNAL_DEFER_REBOOT;
    }
    else
    {
        h->collectedResult.signal = ADUC_SIGNAL_CONTINUE;
    }

    return h->collectedResult;
}

static ADUC_Result2 SimHandler_Cancel(ADUC_StepHandle handle)
{
    ADUC_StepHandleImpl* h = (ADUC_StepHandleImpl*)handle;
    h->cancelled = true;
    ADUC_EXT_LOG_WARN(h->extCtx, SIM_COMP, 1060, "Cancelled");
    return ADUC_RESULT2_SUCCESS;
}

static void SimHandler_Release(ADUC_StepHandle handle)
{
    if (handle != NULL)
    {
        free(handle);
    }
}

static ADUC_Result2 SimHandler_IsInstalled(const ADUC_StepContext* ctx, bool* outIsInstalled)
{
    (void)ctx;
    if (outIsInstalled != NULL)
    {
        *outIsInstalled = false;
    }
    return ADUC_RESULT2_SUCCESS;
}

/* ─── Vtable ──────────────────────────────────────────────────────────────── */

static const ADUC_StepHandlerVtable s_vtable = {
    .structVersion = 1,
    .Evaluate = SimHandler_Evaluate,
    .Acquire = SimHandler_Acquire,
    .Preprocess = SimHandler_Preprocess,
    .Execute = SimHandler_Execute,
    .Validate = SimHandler_Validate,
    .Postprocess = SimHandler_Postprocess,
    .GetResult = SimHandler_GetResult,
    .Cancel = SimHandler_Cancel,
    .Release = SimHandler_Release,
    .IsInstalled = SimHandler_IsInstalled,
};

/* ─── Extension lifecycle ─────────────────────────────────────────────────── */

ADUC_Result2 SimHandler_Initialize(const ADUC_ExtensionContext* ctx)
{
    s_extCtx = ctx;
    ADUC_EXT_LOG_INFO(ctx, SIM_COMP, 1000, "Gen2 Simulator Step Handler initialized");
    return ADUC_RESULT2_SUCCESS;
}

void SimHandler_Uninitialize(void)
{
    if (s_extCtx != NULL)
    {
        ADUC_EXT_LOG_INFO(s_extCtx, SIM_COMP, 1099, "Gen2 Simulator Step Handler uninitialized");
    }
    s_extCtx = NULL;
}

/* ─── Extension descriptor ────────────────────────────────────────────────── */

static const char* s_capabilities[] = { "microsoft/simulator:1", NULL };

static const ADUC_ExtensionDescriptor s_descriptor = {
    .structVersion = ADUC_EXTENSION_DESCRIPTOR_VERSION,
    .id = "gen2-simulator",
    .name = "Gen2 Simulator Step Handler",
    .version = "1.0.0",
    .type = ADUC_EXT_TYPE_STEP_HANDLER,
    .minHostApiVersion = 1,
    .Initialize = SimHandler_Initialize,
    .Uninitialize = SimHandler_Uninitialize,
    .vtable = &s_vtable,
    .capabilities = s_capabilities,
};

ADUC_DECLARE_EXTENSION(s_descriptor)
