/**
 * @file apt_handler_v2.h
 * @brief Microsoft APT Package Handler v2 — step handler for apt-get operations.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef APT_HANDLER_V2_H
#define APT_HANDLER_V2_H

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

#define APT_MAX_PACKAGES 64
#define APT_MAX_OPTION_LEN 256
#define APT_MAX_PATH_LEN 512
#define APT_MAX_ACTION_LEN 32
#define APT_OUTPUT_BUF_SIZE 4096
#define APT_RESULT_DETAILS_SIZE 2048
#define APT_DEFAULT_TIMEOUT_SEC 600
#define APT_VERSION_LEN 128

/**
 * @brief Parsed APT handler configuration from handlerProperties JSON.
 */
typedef struct AptHandlerConfig
{
    char* packages[APT_MAX_PACKAGES];
    size_t packageCount;
    char action[APT_MAX_ACTION_LEN];
    char options[APT_MAX_OPTION_LEN];
    char sourceList[APT_MAX_PATH_LEN];
} AptHandlerConfig;

/**
 * @brief Per-package version record for rollback support.
 */
typedef struct AptPackageVersion
{
    char name[APT_VERSION_LEN];
    char version[APT_VERSION_LEN];
    bool wasInstalled;
} AptPackageVersion;

/**
 * @brief Per-step instance state for the APT handler.
 */
typedef struct ADUC_StepHandleImpl
{
    ADUC_StepContext stepCtx;
    const ADUC_ExtensionContext* extCtx;
    bool cancelled;

    AptHandlerConfig config;
    AptPackageVersion originalVersions[APT_MAX_PACKAGES];
    size_t originalVersionCount;

    ADUC_StepResult collectedResult;
    char resultDetails[APT_RESULT_DETAILS_SIZE];
} ADUC_StepHandleImpl;

ADUC_Result2 AptHandler_Initialize(const ADUC_ExtensionContext* ctx);
void AptHandler_Uninitialize(void);

#ifdef __cplusplus
}
#endif

#endif /* APT_HANDLER_V2_H */
