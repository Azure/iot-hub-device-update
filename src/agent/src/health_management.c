/**
 * @file health_management.c
 * @brief Implements functions that determine whether ADU Agent can function properly.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#include "aduc/health_management.h"
#include "aduc/config_utils.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h"
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

// TODO (Nox): Consider consolidate all connectivity related code into a Connection Manager library.

_Bool GetConnectionInfoFromIdentityService(ADUC_ConnectionInfo* info);

_Bool GetConnectionInfoFromConnectionString(ADUC_ConnectionInfo* info, const char* connectionString);

/**
 * @brief Checks whether we can obtain a device or module connection string.
 * 
 * @return true if connection string can be obtained.
 */
_Bool IsConnectionInfoValid(const ADUC_LaunchArguments* launchArgs, ADUC_ConfigInfo* config)
{
    _Bool validInfo = false;

    ADUC_ConnectionInfo info = {};

    if (launchArgs->connectionString != NULL)
    {
        validInfo = true;
        goto done;
    }

    //TODO(Nox): [MCU work in progress] Currently only supporting one agent (host device), so only reading the first agent
    //in the configuration file. Later it will fork different processes to process multiple agents.
    const ADUC_AgentInfo* agent = ADUC_ConfigInfo_GetAgent(config, 0);
    if (agent == NULL)
    {
        Log_Error("ADUC_ConfigInfo_GetAgent failed to get the agent information.");
        goto done;
    }
    if (strcmp(agent->connectionType, "AIS") == 0)
    {
        validInfo = GetConnectionInfoFromIdentityService(&info);
    }
    else if (strcmp(agent->connectionType, "string") == 0)
    {
        validInfo = GetConnectionInfoFromConnectionString(&info, agent->connectionData);
    }
    else
    {
        Log_Error("The connection type %s is not supported", agent->connectionType);
    }

done:
    ADUC_ConnectionInfo_DeAlloc(&info);
    return validInfo;
}

/**
 * @brief Helper function for simulating an unhealthy state.
 * 
 * @return true if an ADU configuration file contains simulateUnhealthyState value (any value).
 */
_Bool IsSimulatingUnhealthyState(ADUC_ConfigInfo* config)
{
    return config->simulateUnhealthyState;
}

/**
 * @brief Performs necessary checks to determine whether ADU Agent can function properly.
 * 
 * Currently, we are performing the following:
 *     - Implicitly check that agent process launched successfully.
 *     - Check that we can obtain the connection info.
 * 
 * @return true if all checks passed.
 */
_Bool HealthCheck(const ADUC_LaunchArguments* launchArgs)
{
    _Bool isHealthy = false;

    ADUC_ConfigInfo config = {};
    if (!ADUC_ConfigInfo_Init(&config, ADUC_CONF_FILE_PATH))
    {
        Log_Warn("Cannot read configuration file: %s", ADUC_CONF_FILE_PATH);
    }

    if (!IsConnectionInfoValid(launchArgs, &config))
    {
        Log_Error("Invalid connection info.");
        goto done;
    }

    // TODO (Nox): 31374905: Improve adu-agent 'health-check' logic.
#ifdef ADUC_PLATFORM_SIMULATOR
    if (IsSimulatingUnhealthyState(&config))
    {
        Log_Error("Simulating an unhealthy state.");
        goto done;
    }
#endif

    isHealthy = true;
done:
    Log_Info("Health check %s.", isHealthy ? "passed" : "failed");
    ADUC_ConfigInfo_UnInit(&config);
    return isHealthy;
}
