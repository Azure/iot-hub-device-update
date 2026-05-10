/**
 * @file step_result.c
 * @brief Helper functions for ADUC_StepResultDetail construction and inspection.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#include "aduc/step_result.h"

#include "aduc/platform.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ─── Internal helpers ─────────────────────────────────────────────────────── */

static uint64_t get_monotonic_ms(void)
{
#ifdef _WIN32
    /* Use QueryPerformanceCounter for high-resolution monotonic time on Windows */
    LARGE_INTEGER freq, count;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&count);
    return (uint64_t)(count.QuadPart * 1000 / freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
#endif
}

static char* safe_strdup(const char* s)
{
    return (s != NULL) ? strdup(s) : NULL;
}

/* ─── Construction helpers ─────────────────────────────────────────────────── */

ADUC_StepResultDetail ADUC_StepResult_Success(ADUC_StepPhase phase)
{
    uint64_t now = get_monotonic_ms();
    ADUC_StepResultDetail result;
    memset(&result, 0, sizeof(result));
    result.resultCode = ADUC_RESULT2_SUCCESS;
    result.phase = phase;
    result.signal = ADUC_SIGNAL_CONTINUE;
    result.startTimeMs = now;
    result.endTimeMs = now;
    return result;
}

ADUC_StepResultDetail ADUC_StepResult_SuccessWithSignal(
    ADUC_StepPhase phase, ADUC_OrchestratorSignal signal)
{
    ADUC_StepResultDetail result = ADUC_StepResult_Success(phase);
    result.signal = signal;
    return result;
}

ADUC_StepResultDetail ADUC_StepResult_Failure(
    ADUC_StepPhase phase,
    ADUC_Result2 errorCode,
    const char* resultDetails,
    const char* errorSource)
{
    uint64_t now = get_monotonic_ms();
    ADUC_StepResultDetail result;
    memset(&result, 0, sizeof(result));
    result.resultCode = errorCode;
    result.phase = phase;
    result.signal = ADUC_SIGNAL_ABORT_DEPLOYMENT;
    result.outcome = ADUC_Outcome_Failed;
    result.failureOrigin = ADUC_FailureOrigin_AgentCore;
    result.resultDetails = safe_strdup(resultDetails);
    result.errorSource = safe_strdup(errorSource);
    result.startTimeMs = now;
    result.endTimeMs = now;
    return result;
}

ADUC_StepResultDetail ADUC_StepResult_Abort(
    ADUC_StepPhase phase,
    ADUC_Result2 errorCode,
    const char* resultDetails)
{
    return ADUC_StepResult_Failure(phase, errorCode, resultDetails, NULL);
}

ADUC_StepResultDetail ADUC_StepResult_Retry(
    ADUC_StepPhase phase,
    uint32_t retryCount,
    uint32_t maxRetries,
    uint32_t retryDelayMs)
{
    uint64_t now = get_monotonic_ms();
    ADUC_StepResultDetail result;
    memset(&result, 0, sizeof(result));
    result.resultCode = ADUC_RESULT2_SUCCESS;
    result.phase = phase;
    result.signal = ADUC_SIGNAL_RETRY_PHASE;
    result.retryCount = retryCount;
    result.maxRetries = maxRetries;
    result.retryDelayMs = retryDelayMs;
    result.startTimeMs = now;
    result.endTimeMs = now;
    return result;
}

/* ─── Inspection helpers ───────────────────────────────────────────────────── */

bool ADUC_StepResult_IsSuccess(const ADUC_StepResultDetail* result)
{
    if (result == NULL)
    {
        return false;
    }
    return result->resultCode.code == 0;
}

bool ADUC_StepResult_ShouldStop(const ADUC_StepResultDetail* result)
{
    if (result == NULL)
    {
        return false;
    }
    return result->signal == ADUC_SIGNAL_ABORT_DEPLOYMENT
        || result->signal == ADUC_SIGNAL_ROLLBACK;
}

/* ─── ToString helpers ─────────────────────────────────────────────────────── */

const char* ADUC_StepPhase_ToString(ADUC_StepPhase phase)
{
    switch (phase)
    {
        case ADUC_STEP_PHASE_EVALUATE:    return "Evaluate";
        case ADUC_STEP_PHASE_ACQUIRE:     return "Acquire";
        case ADUC_STEP_PHASE_PREPROCESS:  return "Preprocess";
        case ADUC_STEP_PHASE_EXECUTE:     return "Execute";
        case ADUC_STEP_PHASE_VALIDATE:    return "Validate";
        case ADUC_STEP_PHASE_POSTPROCESS: return "Postprocess";
        case ADUC_STEP_PHASE_REPORT:      return "Report";
        case ADUC_STEP_PHASE_SIGNAL:      return "Signal";
        default:                          return "Unknown";
    }
}

const char* ADUC_OrchestratorSignal_ToString(ADUC_OrchestratorSignal signal)
{
    switch (signal)
    {
        case ADUC_SIGNAL_CONTINUE:          return "Continue";
        case ADUC_SIGNAL_SKIP_REMAINING:    return "SkipRemaining";
        case ADUC_SIGNAL_SKIP_DEPENDENT:    return "SkipDependent";
        case ADUC_SIGNAL_DEFER_REBOOT:      return "DeferReboot";
        case ADUC_SIGNAL_IMMEDIATE_REBOOT:  return "ImmediateReboot";
        case ADUC_SIGNAL_RETRY_PHASE:       return "RetryPhase";
        case ADUC_SIGNAL_ABORT_DEPLOYMENT:  return "AbortDeployment";
        case ADUC_SIGNAL_ROLLBACK:          return "Rollback";
        default:                            return "Unknown";
    }
}

/* ─── Timing ───────────────────────────────────────────────────────────────── */

uint64_t ADUC_StepResult_ElapsedMs(const ADUC_StepResultDetail* result)
{
    if (result == NULL || result->endTimeMs < result->startTimeMs)
    {
        return 0;
    }
    return result->endTimeMs - result->startTimeMs;
}

/* ─── Cleanup ──────────────────────────────────────────────────────────────── */

void ADUC_StepResult_Free(ADUC_StepResultDetail* result)
{
    if (result == NULL)
    {
        return;
    }
    /* resultDetails and errorSource are strdup'd copies owned by the result. */
    free((void*)result->resultDetails);
    free((void*)result->errorSource);
    memset(result, 0, sizeof(*result));
}
