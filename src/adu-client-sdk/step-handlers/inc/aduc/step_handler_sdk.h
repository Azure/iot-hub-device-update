/**
 * @file step_handler_sdk.h
 * @brief Main Step Handler SDK header - includes all necessary interfaces
 *
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#ifndef ADUC_STEP_HANDLER_SDK_H
#define ADUC_STEP_HANDLER_SDK_H

// Core SDK components
#include "aduc/result.h"
#include "aduc/types.h"
#include "aduc/exports.h"
#include "aduc/workflow_data.h"

// Step Handler specific components
#include "aduc/content_handler.h"

/**
 * @brief Step Handler SDK Version
 */
#define ADUC_STEP_HANDLER_SDK_VERSION_MAJOR 1
#define ADUC_STEP_HANDLER_SDK_VERSION_MINOR 0
#define ADUC_STEP_HANDLER_SDK_VERSION_PATCH 0

/**
 * @brief Step Handler SDK Version String
 */
#define ADUC_STEP_HANDLER_SDK_VERSION_STRING "1.0.0"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Get Step Handler SDK version
 *
 * @return const char* Version string
 */
ADUC_SDK_EXPORT const char* ADUC_StepHandlerSDK_GetVersion(void);

/**
 * @brief Helper function to create a basic content handler structure
 *
 * @return ADUC_ContentHandler* Pointer to allocated handler structure, or NULL on failure
 */
ADUC_SDK_EXPORT ADUC_ContentHandler* ADUC_ContentHandler_Create(void);

/**
 * @brief Helper function to free a content handler structure
 *
 * @param handler The handler to free
 */
ADUC_SDK_EXPORT void ADUC_ContentHandler_Free(ADUC_ContentHandler* handler);

#ifdef __cplusplus
}
#endif

#endif // ADUC_STEP_HANDLER_SDK_H
