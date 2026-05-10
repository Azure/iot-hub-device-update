/**
 * @file step_handler_vtable.h
 * @brief Unified step handler extension vtable.
 *
 * Step handlers process individual deployment steps (install firmware,
 * apply config, run script, etc.). This replaces the C++ ContentHandler
 * virtual class with a C vtable for consistency and cross-language support.
 *
 * Step lifecycle: Evaluate → Acquire → Preprocess → Execute → Validate → Postprocess → Report → Signal
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_STEP_HANDLER_VTABLE_H
#define ADUC_STEP_HANDLER_VTABLE_H

#include "aduc/extension_types.h"
#include "aduc/step_result.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Opaque step handle (handler-managed state for an in-progress step).
 */
typedef struct ADUC_StepHandleImpl* ADUC_StepHandle;

/**
 * @brief Step execution context provided by orchestrator.
 */
typedef struct ADUC_StepContext
{
    const char* workflowId;
    const char* stepId;
    uint32_t attempt;                  // Retry count (1 = first attempt)

    // Content files for this step (pre-downloaded, hash-verified)
    const struct ADUC_StepFile* files;
    size_t fileCount;

    // Handler-specific configuration (from deployment manifest)
    const char* handlerConfigJson;

    // Target component info (NULL for host-level steps)
    const char* componentId;
    const char* componentGroup;
    const ADUC_PropertyMap* componentProperties;

    // Installed criteria to verify after execution
    const char* installedCriteria;
} ADUC_StepContext;

/**
 * @brief File info for a step's content.
 */
typedef struct ADUC_StepFile
{
    const char* id;
    const char* name;
    const char* path;                  // Local path (already downloaded)
    uint64_t size;
    const char* sha256;
} ADUC_StepFile;

/**
 * @brief Legacy alias — use ADUC_OrchestratorSignal from step_result.h.
 */
typedef ADUC_OrchestratorSignal ADUC_StepSignal;

/**
 * @brief Step execution result.
 */
typedef struct ADUC_StepResult
{
    ADUC_Result2 result;               // Success/failure code
    const char* resultDetails;         // Human-readable details
    ADUC_OrchestratorSignal signal;    // Orchestrator signal
    bool rollbackPerformed;            // Whether postprocess did a rollback
    const char* outputStateJson;       // Output state (e.g., installed version)
} ADUC_StepResult;

/**
 * @brief Progress callback (step handler → orchestrator).
 */
typedef void (*ADUC_StepProgressFn)(
    const char* stepId,
    uint32_t percentComplete,
    const char* phase,                 // Current lifecycle phase name
    const char* message,
    void* ctx);

/**
 * @brief Step handler vtable.
 *
 * Extension descriptor's `vtable` field points to this when type == ADUC_EXT_TYPE_STEP_HANDLER.
 *
 * Lifecycle phases are called in order. Each phase can fail, triggering Postprocess
 * with rollback. Handlers MUST be cancellation-aware (check cancel flag periodically).
 */
typedef struct ADUC_StepHandlerVtable
{
    uint32_t structVersion;  // 1

    /**
     * @brief Evaluate whether this step can proceed on the target.
     * Check: dependencies met, device ready, no conflicts, sufficient resources.
     * @return Success to proceed, failure to skip/abort.
     */
    ADUC_Result2 (*Evaluate)(const ADUC_StepContext* ctx, ADUC_StepHandle* handle);

    /**
     * @brief Acquire any additional resources needed (beyond pre-downloaded files).
     * Most handlers can return success immediately (files already downloaded).
     */
    ADUC_Result2 (*Acquire)(ADUC_StepHandle handle);

    /**
     * @brief Preprocess: backup current state, stop services, prepare for apply.
     */
    ADUC_Result2 (*Preprocess)(ADUC_StepHandle handle);

    /**
     * @brief Execute the step (apply firmware, install package, copy file, etc.).
     * This is the main work phase.
     * @param progressFn Callback for progress updates.
     * @param progressCtx Opaque context for progress callback.
     */
    ADUC_Result2 (*Execute)(ADUC_StepHandle handle, ADUC_StepProgressFn progressFn, void* progressCtx);

    /**
     * @brief Validate the step succeeded (check installed criteria, health).
     */
    ADUC_Result2 (*Validate)(ADUC_StepHandle handle);

    /**
     * @brief Postprocess: cleanup temp files, start services.
     * If any prior phase failed, this is called with rollback=true.
     * @param rollback Whether to rollback (restore from Preprocess backup).
     */
    ADUC_Result2 (*Postprocess)(ADUC_StepHandle handle, bool rollback);

    /**
     * @brief Get the final step result (after lifecycle completes).
     */
    ADUC_StepResult (*GetResult)(ADUC_StepHandle handle);

    /**
     * @brief Cancel an in-progress step.
     * Called from a different thread. Handler should check cancel flag and bail out.
     */
    ADUC_Result2 (*Cancel)(ADUC_StepHandle handle);

    /**
     * @brief Release step handle resources.
     */
    void (*Release)(ADUC_StepHandle handle);

    /**
     * @brief Report step result to the service (telemetry, status).
     */
    ADUC_StepResultDetail (*Report)(ADUC_StepHandle handle);

    /**
     * @brief Signal phase — emit orchestrator signal after step completes.
     */
    ADUC_StepResultDetail (*Signal)(ADUC_StepHandle handle);

    /**
     * @brief Query: is the specified content already installed?
     * Used to skip steps that are already satisfied (idempotency).
     */
    ADUC_Result2 (*IsInstalled)(const ADUC_StepContext* ctx, bool* outIsInstalled);

    /**
     * @brief Get handler capabilities as a NULL-terminated string array.
     * E.g., {"firmware", "delta", NULL}. Returned pointer is handler-owned (static).
     */
    const char** (*GetCapabilities)(void);

} ADUC_StepHandlerVtable;

#ifdef __cplusplus
}
#endif

#endif // ADUC_STEP_HANDLER_VTABLE_H
