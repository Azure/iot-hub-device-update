/**
 * @file shutdown_service.c
 * @brief Implementation for agent shutdown services.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/shutdown_service.h"

#include <signal.h>

static volatile sig_atomic_t s_isShuttingDown = 0;

void ADUC_ShutdownService_RequestShutdown()
{
    s_isShuttingDown = 1;
}

bool ADUC_ShutdownService_ShouldKeepRunning()
{
    return s_isShuttingDown == 0;
}
