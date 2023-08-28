
/**
 * @file shutdown_service.c
 * @brief Implementation for agent shutdown services.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/shutdown_service.h"

static bool s_isShuttingDown = false;

void ADUC_ShutdownService_RequestShutdown()
{
    s_isShuttingDown = true;
}

bool ADUC_ShutdownService_ShouldKeepRunning()
{
    return !s_isShuttingDown;
}
