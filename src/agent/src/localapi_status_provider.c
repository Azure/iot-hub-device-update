/**
 * @file localapi_status_provider.c
 * @brief Provides the real agent status to the Local API server.
 *
 * Overrides the weak symbol `localapi_get_current_status()` defined in
 * localapi_server.c with a real implementation that reads from the
 * view state manager.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/viewstatemgr.h"

/**
 * @brief Returns the current agent service status.
 *
 * This function overrides the weak default in localapi_server.c so that
 * Local API clients receive the real workflow status.
 *
 * @return Current ADUC_ServiceStatus value (cast to int).
 */
int localapi_get_current_status(void)
{
    ADUC_ServiceStatus status = ADUC_ServiceStatus_Idle;
    if (g_vsm.initialized)
    {
        viewstatemgr_svcstatus_get(&g_vsm, &status);
    }
    return (int)status;
}
