/**
 * @file step_result.h
 * @brief Step result types and orchestrator signal definitions.
 *
 * When a step handler completes a lifecycle phase, it returns an
 * ADUC_StepResultDetail containing the phase, success/failure info,
 * an orchestrator signal, and optional progress/retry metadata.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_STEP_RESULT_H
#define ADUC_STEP_RESULT_H

#include "aduc/extension_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Step lifecycle phase (matches the 8-phase model).
 */
typedef enum ADUC_StepPhase
{
    ADUC_STEP_PHASE_EVALUATE = 0,
    ADUC_STEP_PHASE_ACQUIRE,
    ADUC_STEP_PHASE_PREPROCESS,
    ADUC_STEP_PHASE_EXECUTE,
    ADUC_STEP_PHASE_VALIDATE,
    ADUC_STEP_PHASE_POSTPROCESS,
    ADUC_STEP_PHASE_REPORT,
    ADUC_STEP_PHASE_SIGNAL,
    ADUC_STEP_PHASE_COUNT
} ADUC_StepPhase;

/**
 * @brief Orchestrator signal — tells the workflow engine what to do
 *        after a step handler returns.
 */
typedef enum ADUC_OrchestratorSignal
{
    ADUC_SIGNAL_CONTINUE = 0,       /**< Proceed to next phase/step normally */
    ADUC_SIGNAL_SKIP_REMAINING,     /**< Skip remaining phases for this step (mark success) */
    ADUC_SIGNAL_SKIP_DEPENDENT,     /**< Skip steps that depend on this one */
    ADUC_SIGNAL_DEFER_REBOOT,       /**< Continue but schedule reboot after all steps */
    ADUC_SIGNAL_IMMEDIATE_REBOOT,   /**< Reboot now (before continuing) */
    ADUC_SIGNAL_RETRY_PHASE,        /**< Retry the current phase (with backoff) */
    ADUC_SIGNAL_ABORT_DEPLOYMENT,   /**< Abort the entire deployment */
    ADUC_SIGNAL_ROLLBACK,           /**< Trigger rollback of completed steps */
} ADUC_OrchestratorSignal;

/**
 * @brief Optional progress information for reporting.
 */
typedef struct ADUC_StepProgress
{
    uint32_t percentComplete;   /**< 0-100 */
    uint64_t bytesTransferred;  /**< For download progress */
    uint64_t bytesTotal;        /**< Total expected bytes */
    const char* statusMessage;  /**< Human-readable status (owned by caller) */
} ADUC_StepProgress;

/**
 * @brief Extended result with structured error info, signal, progress,
 *        retry metadata, and timing.
 */
typedef struct ADUC_StepResultDetail
{
    ADUC_Result2 resultCode;        /**< Structured error code */
    ADUC_StepPhase phase;           /**< Which phase produced this result */
    ADUC_OrchestratorSignal signal; /**< What the orchestrator should do */

    /* Outcome and origin for v2 protocol reporting */
    ADUC_Outcome outcome;           /**< Terminal outcome (v2 protocol) */
    ADUC_Origin origin;             /**< Advisory failure source hint (v2 protocol) */

    /* Error details (NULL if success) */
    const char* resultDetails;      /**< Human-readable error (owned by result, freed on destroy) */
    const char* errorSource;        /**< Component that failed (e.g., "swupdate", "curl") */

    /* Progress snapshot at time of result */
    ADUC_StepProgress progress;

    /* Retry info (valid when signal == RETRY_PHASE) */
    uint32_t retryCount;            /**< How many retries so far */
    uint32_t maxRetries;            /**< Max retries before giving up */
    uint32_t retryDelayMs;          /**< Suggested delay before retry */

    /* Timing */
    uint64_t startTimeMs;           /**< Phase start time (monotonic ms) */
    uint64_t endTimeMs;             /**< Phase end time (monotonic ms) */
} ADUC_StepResultDetail;

/* ─── Helper functions ─────────────────────────────────────────────────────── */

/**
 * @brief Create a success result for a given phase.
 */
ADUC_StepResultDetail ADUC_StepResult_Success(ADUC_StepPhase phase);

/**
 * @brief Create a success result with a specific orchestrator signal.
 */
ADUC_StepResultDetail ADUC_StepResult_SuccessWithSignal(
    ADUC_StepPhase phase, ADUC_OrchestratorSignal signal);

/**
 * @brief Create a failure result with error details.
 */
ADUC_StepResultDetail ADUC_StepResult_Failure(
    ADUC_StepPhase phase,
    ADUC_Result2 errorCode,
    const char* resultDetails,
    const char* errorSource);

/**
 * @brief Create a failure result with ABORT signal.
 */
ADUC_StepResultDetail ADUC_StepResult_Abort(
    ADUC_StepPhase phase,
    ADUC_Result2 errorCode,
    const char* resultDetails);

/**
 * @brief Create a retry result with backoff parameters.
 */
ADUC_StepResultDetail ADUC_StepResult_Retry(
    ADUC_StepPhase phase,
    uint32_t retryCount,
    uint32_t maxRetries,
    uint32_t retryDelayMs);

/**
 * @brief Check if a result indicates success.
 */
bool ADUC_StepResult_IsSuccess(const ADUC_StepResultDetail* result);

/**
 * @brief Check if the result signal requires stopping the workflow.
 */
bool ADUC_StepResult_ShouldStop(const ADUC_StepResultDetail* result);

/**
 * @brief Get phase name as a string literal (for logging).
 */
const char* ADUC_StepPhase_ToString(ADUC_StepPhase phase);

/**
 * @brief Get signal name as a string literal (for logging).
 */
const char* ADUC_OrchestratorSignal_ToString(ADUC_OrchestratorSignal signal);

/**
 * @brief Get elapsed time in milliseconds.
 */
uint64_t ADUC_StepResult_ElapsedMs(const ADUC_StepResultDetail* result);

/**
 * @brief Free any owned strings in the result and zero the struct.
 */
void ADUC_StepResult_Free(ADUC_StepResultDetail* result);

#ifdef __cplusplus
}
#endif

#endif /* ADUC_STEP_RESULT_H */
