/**
 * @file handler_registry.h
 * @brief Handler registry for managing extension handlers
 * 
 * @copyright Copyright (C) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See LICENSE file in the project root for license information.
 */

#ifndef ADUC_HANDLER_REGISTRY_H
#define ADUC_HANDLER_REGISTRY_H

#include "aduc/content_handler.h"
#include "aduc/result.h"
#include "aduc/types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the handler registry
 * @return ADUC_Result_t indicating success or failure
 */
ADUC_Result_t ADUC_HandlerRegistry_Initialize(void);

/**
 * @brief Cleanup the handler registry
 */
void ADUC_HandlerRegistry_Cleanup(void);

/**
 * @brief Register a handler
 * @param name The handler name
 * @param libraryPath The path to the handler library
 * @return ADUC_Result_t indicating success or failure
 */
ADUC_Result_t ADUC_HandlerRegistry_Register(const char* name, const char* libraryPath);

/**
 * @brief Load a handler by name
 * @param name The handler name
 * @param logLevel The log level to pass to the handler
 * @return Pointer to content handler, or NULL on failure
 */
ADUC_ContentHandler* ADUC_HandlerRegistry_LoadHandler(const char* name, ADUC_LogLevel logLevel);

#ifdef __cplusplus
}
#endif

#endif // ADUC_HANDLER_REGISTRY_H