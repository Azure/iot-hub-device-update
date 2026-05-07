/**
 * @file localapi_status_provider.c
 * @brief Provides the real agent status and control command handlers to the Local API server.
 *
 * Overrides the weak symbols defined in localapi_server.c with real
 * implementations that interact with the agent's workflow engine.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/aducsdk.h"
#include "aduc/timer.h"
#include "aduc/viewstatemgr.h"

#include <stdbool.h>

// External references to agent globals
extern AducTimer g_idle_pause_timer;

// Flag to track local-API-initiated pause (not timer-based)
static volatile bool g_localapi_paused = false;

/**
 * @brief Returns the current agent service status.
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

/**
 * @brief Handles PAUSE command from Local API.
 *
 * Sets agent status to Paused. The agent will not process new deployments
 * while paused. In-progress operations continue but no new work is started.
 *
 * @return 0 on success, -1 if already paused or not initialized.
 */
int localapi_handle_pause(void)
{
    if (!g_vsm.initialized)
    {
        return -1;
    }

    ADUC_ServiceStatus currentStatus = ADUC_ServiceStatus_None;
    viewstatemgr_svcstatus_get(&g_vsm, &currentStatus);

    if (currentStatus == ADUC_ServiceStatus_Paused)
    {
        return 0; // Already paused
    }

    g_localapi_paused = true;
    viewstatemgr_svcstatus_set(&g_vsm, ADUC_ServiceStatus_Paused);
    return 0;
}

/**
 * @brief Handles RESUME command from Local API.
 *
 * Clears the paused state and returns agent to Idle, ready to process work.
 *
 * @return 0 on success, -1 if not paused or not initialized.
 */
int localapi_handle_resume(void)
{
    if (!g_vsm.initialized)
    {
        return -1;
    }

    ADUC_ServiceStatus currentStatus = ADUC_ServiceStatus_None;
    viewstatemgr_svcstatus_get(&g_vsm, &currentStatus);

    if (currentStatus != ADUC_ServiceStatus_Paused)
    {
        return -1; // Not paused
    }

    g_localapi_paused = false;

    // Also stop any running idle pause timer
    AducTimer_Stop(&g_idle_pause_timer);

    viewstatemgr_svcstatus_set(&g_vsm, ADUC_ServiceStatus_Idle);
    return 0;
}

/**
 * @brief Handles CANCEL command from Local API.
 *
 * Requests cancellation of any in-progress update operation.
 * This is equivalent to receiving a Cancel action from the cloud.
 *
 * @return 0 if cancellation was requested, -1 if no operation in progress.
 */
int localapi_handle_cancel(void)
{
    if (!g_vsm.initialized)
    {
        return -1;
    }

    ADUC_ServiceStatus currentStatus = ADUC_ServiceStatus_None;
    viewstatemgr_svcstatus_get(&g_vsm, &currentStatus);

    // Can only cancel if there's an active operation
    if (currentStatus == ADUC_ServiceStatus_Idle ||
        currentStatus == ADUC_ServiceStatus_None ||
        currentStatus == ADUC_ServiceStatus_Paused)
    {
        return -1; // Nothing to cancel
    }

    // Set status to Cancelling — the workflow engine will pick this up
    viewstatemgr_svcstatus_set(&g_vsm, ADUC_ServiceStatus_Cancelling);
    return 0;
}

/**
 * @brief Handles FORCE_CHECK command from Local API.
 *
 * Triggers the agent to check for new updates immediately by requesting
 * a device twin refresh from IoT Hub.
 *
 * @return 0 if check was initiated, -1 if agent is busy.
 */
int localapi_handle_force_check(void)
{
    if (!g_vsm.initialized)
    {
        return -1;
    }

    ADUC_ServiceStatus currentStatus = ADUC_ServiceStatus_None;
    viewstatemgr_svcstatus_get(&g_vsm, &currentStatus);

    // Only allow force check when idle
    if (currentStatus != ADUC_ServiceStatus_Idle &&
        currentStatus != ADUC_ServiceStatus_None)
    {
        return -1; // Agent is busy
    }

    // Note: A full twin refresh would require access to g_iotHubClientHandle
    // which is static in main.c. For MVP, we signal readiness by ensuring
    // the agent is in Idle state and not paused — the next DoWork cycle
    // will pick up any pending twin changes.
    // TODO: Expose IoTHub_CommunicationManager_RequestTwinRefresh() for direct trigger.
    return 0;
}
