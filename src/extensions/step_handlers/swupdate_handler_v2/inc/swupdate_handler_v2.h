/**
 * @file swupdate_handler_v2.h
 * @brief Microsoft SWUpdate Handler v2 — step handler for SWUpdate firmware installs.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef SWUPDATE_HANDLER_V2_H
#define SWUPDATE_HANDLER_V2_H

#include "aduc/extension_context.h"
#include "aduc/extension_descriptor.h"
#include "aduc/step_handler_vtable.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SWU_MAX_PATH_LEN 512
#define SWU_MAX_VERSION_LEN 128
#define SWU_MAX_HWREV_LEN 64
#define SWU_OUTPUT_BUF_SIZE 4096
#define SWU_RESULT_DETAILS_SIZE 2048
#define SWU_DEFAULT_TIMEOUT_SEC 900

/**
 * @brief Parsed SWUpdate handler configuration from handlerProperties JSON.
 */
typedef struct SwuHandlerConfig
{
    char swuFileName[SWU_MAX_PATH_LEN];
    char hwRevision[SWU_MAX_HWREV_LEN];
    bool rebootRequired;
    char verifyCommand[SWU_MAX_PATH_LEN];
    char expectedVersion[SWU_MAX_VERSION_LEN];
} SwuHandlerConfig;

/**
 * @brief Per-step instance state for the SWUpdate handler.
 */
typedef struct ADUC_StepHandleImpl
{
    ADUC_StepContext stepCtx;
    const ADUC_ExtensionContext* extCtx;
    bool cancelled;

    SwuHandlerConfig config;
    char swuFilePath[SWU_MAX_PATH_LEN];
    uint64_t swuFileSize;

    ADUC_StepResult collectedResult;
    char resultDetails[SWU_RESULT_DETAILS_SIZE];
} ADUC_StepHandleImpl;

ADUC_Result2 SwuHandler_Initialize(const ADUC_ExtensionContext* ctx);
void SwuHandler_Uninitialize(void);

#ifdef __cplusplus
}
#endif

#endif /* SWUPDATE_HANDLER_V2_H */
