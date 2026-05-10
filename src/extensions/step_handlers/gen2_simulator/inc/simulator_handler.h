/**
 * @file simulator_handler.h
 * @brief Gen2 Simulator Step Handler — demonstrates the step handler vtable pattern.
 *
 * This handler simulates all lifecycle phases with configurable delays and
 * failure injection, useful for testing the workflow engine.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef GEN2_SIMULATOR_HANDLER_H
#define GEN2_SIMULATOR_HANDLER_H

#include "aduc/extension_context.h"
#include "aduc/extension_descriptor.h"
#include "aduc/step_handler_vtable.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Parsed simulator configuration from handlerConfigJson.
 */
typedef struct SimHandlerConfig
{
    bool simulateFailure;
    char failInPhase[32];
    uint32_t delayMs;
    bool simulateReboot;
} SimHandlerConfig;

/**
 * @brief Per-step instance state.
 */
typedef struct ADUC_StepHandleImpl
{
    ADUC_StepContext stepCtx;
    SimHandlerConfig config;
    const ADUC_ExtensionContext* extCtx;
    ADUC_StepResult collectedResult;
    bool cancelled;
} ADUC_StepHandleImpl;

ADUC_Result2 SimHandler_Initialize(const ADUC_ExtensionContext* ctx);
void SimHandler_Uninitialize(void);

#ifdef __cplusplus
}
#endif

#endif // GEN2_SIMULATOR_HANDLER_H
