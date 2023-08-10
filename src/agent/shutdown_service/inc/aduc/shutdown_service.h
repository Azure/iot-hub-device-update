/**
 * @file shutdown_service.h
 * @brief Header for agent shutdown services.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_SHUTDOWN_SERVICE
#define ADUC_SHUTDOWN_SERVICE

#include <aduc/c_utils.h>
#include <stdbool.h>

EXTERN_C_BEGIN

/**
 * @brief Requests the agent to shutdown.
 */
void ADUC_ShutdownService_RequestShutdown();

/**
 * @brief Whether agent should keep running.
 *
 * @return false if a shutdown has been requested.
 */
bool ADUC_ShutdownService_ShouldKeepRunning();

EXTERN_C_END

#endif //ADUC_SHUTDOWN_SERVICE
