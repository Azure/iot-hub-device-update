/**
 * @file health_management.c
 * @brief Implements functions that determine whether ADU Agent can function properly.
 *
 * @copyright Copyright (c) 2019, Microsoft Corporation.
 */
#include "aduc/health_management.h"
#include "aduc/logging.h"
#include "aduc/string_c_utils.h"
#include <ctype.h>
#include <stdbool.h>

// TODO (Nox): Consider consolidate all connectivity related code into a Connection Manager library.

_Bool GetConnectionInfoFromADUConfigFile(ADUC_ConnectionInfo* info);

#ifdef ADUC_PROVISION_WITH_EIS
_Bool GetConnectionInfoFromIdentityService(ADUC_ConnectionInfo* info);
#endif // ADUC_PROVISION_WITH_EIS

/**
 * @brief Checks whether we can obtain a device or module connection string.
 * 
 * @return true if connection string can be obtained.
 */
_Bool IsConnectionInfoValid(const ADUC_LaunchArguments* launchArgs)
{
    _Bool validInfo = false;

    ADUC_ConnectionInfo info = { ADUC_AuthType_NotSet, ADUC_ConnType_NotSet, NULL, NULL, NULL, NULL };

    if (launchArgs->connectionString != NULL)
    {
        return true;
    }

#ifndef ADUC_PROVISION_WITH_EIS
    validInfo = GetConnectionInfoFromADUConfigFile(&info);
#else
    validInfo = GetConnectionInfoFromIdentityService(&info);
#endif

    ADUC_ConnectionInfo_DeAlloc(&info);
    return validInfo;
}

/**
 * @brief Helper function for simulating an unhealthy state.
 * 
 * @return true if an ADU configuration file contains simulate_unhealthy_state value (any value).
 */
_Bool IsSimulatingUnhealthyState()
{
    char value[20];
    if (ReadDelimitedValueFromFile(ADUC_CONF_FILE_PATH, "simulate_unhealthy_state", value, ARRAY_SIZE(value)))
    {
        return true;
    }

    return false;
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

    if (!IsConnectionInfoValid(launchArgs))
    {
        Log_Error("Invalid connection info.");
        goto done;
    }

    // TODO (Nox): 31374905: Improve adu-agent 'health-check' logic.

    if (IsSimulatingUnhealthyState())
    {
        Log_Error("Simulating an unhealthy state.");
        goto done;
    }

    isHealthy = true;
done:
    Log_Info("Health check %s.", isHealthy ? "passed" : "failed");
    return isHealthy;
}
