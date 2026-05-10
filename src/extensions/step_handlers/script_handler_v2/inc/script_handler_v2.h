/**
 * @file script_handler_v2.h
 * @brief Microsoft Script Handler v2 — customer-friendly script execution handler.
 *
 * Implements the "microsoft/script:2" step handler type. Customer scripts are
 * plain bash scripts: do work, exit 0 for success, non-zero for failure.
 * The handler provides the framework (env vars, exit code interpretation,
 * timeout, result capture).
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef SCRIPT_HANDLER_V2_H
#define SCRIPT_HANDLER_V2_H

#include "aduc/extension_context.h"
#include "aduc/extension_descriptor.h"
#include "aduc/step_handler_vtable.h"

#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Script exit codes with semantic meaning.
 */
#define SCRIPT_EXIT_SUCCESS             0
#define SCRIPT_EXIT_GENERAL_FAILURE     1
#define SCRIPT_EXIT_RETRYABLE           2
#define SCRIPT_EXIT_REBOOT_REQUIRED     3
#define SCRIPT_EXIT_ALREADY_INSTALLED   100

/**
 * @brief Default timeout for script execution (seconds).
 */
#define SCRIPT_DEFAULT_TIMEOUT_SEC      600

/**
 * @brief Grace period between SIGTERM and SIGKILL (seconds).
 */
#define SCRIPT_GRACE_PERIOD_SEC         5

/**
 * @brief Per-step instance state for the script handler.
 */
typedef struct ADUC_StepHandleImpl
{
    ADUC_StepContext stepCtx;
    const ADUC_ExtensionContext* extCtx;

    /* Parsed configuration from handlerConfigJson */
    char scriptFileName[256];
    char arguments[1024];
    char installedCriteriaScript[256];
    char rollbackScript[256];
    char workFolder[512];
    int timeoutSec;

    /* Runtime state */
    bool cancelled;
    pid_t childPid;
    char resultDetails[2048];
    ADUC_StepResult collectedResult;
    ADUC_OrchestratorSignal pendingSignal;
} ADUC_StepHandleImpl;

/**
 * @brief Initialize the script handler extension.
 */
ADUC_Result2 ScriptHandler_Initialize(const ADUC_ExtensionContext* ctx);

/**
 * @brief Uninitialize the script handler extension.
 */
void ScriptHandler_Uninitialize(void);

#ifdef __cplusplus
}
#endif

#endif /* SCRIPT_HANDLER_V2_H */
