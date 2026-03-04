/**
 * @file diagnostics_devicename.c
 * @brief Implements function necessary for getting and setting the devicename for the diagnostics_component
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "diagnostics_devicename.h"

#include <aduc/logging.h>
#include <azure_c_shared_utility/crt_abstractions.h> // mallocAndStrcpy_s
#include <azure_c_shared_utility/strings.h>
#include <string.h>

static STRING_HANDLE s_DiagnosticsDeviceName = NULL;

bool DiagnosticsComponent_SetDeviceName(const char* deviceId, const char* moduleId)
{
    if (deviceId == NULL)
    {
        Log_Error("DiagnosticsComponent_SetDeviceName: deviceId is NULL");
        return false;
    }

    if (s_DiagnosticsDeviceName != NULL)
    {
        if (STRING_empty(s_DiagnosticsDeviceName) != 0)
        {
            Log_Error("DiagnosticsComponent_SetDeviceName: failed to empty existing device name string");
            return false;
        }
    }
    else
    {
        s_DiagnosticsDeviceName = STRING_new();

        if (s_DiagnosticsDeviceName == NULL)
        {
            Log_Error("DiagnosticsComponent_SetDeviceName: STRING_new failed (out of memory)");
            return false;
        }
    }

    if (moduleId != NULL)
    {
        if (STRING_sprintf(s_DiagnosticsDeviceName, "%s/%s", deviceId, moduleId) != 0)
        {
            Log_Error("DiagnosticsComponent_SetDeviceName: STRING_sprintf failed for deviceId/moduleId");
            return false;
        }
        Log_Info("DiagnosticsComponent_SetDeviceName: device name set to '%s/%s'", deviceId, moduleId);
    }
    else
    {
        if (STRING_sprintf(s_DiagnosticsDeviceName, "%s", deviceId) != 0)
        {
            Log_Error("DiagnosticsComponent_SetDeviceName: STRING_sprintf failed for deviceId");
            return false;
        }
        Log_Info("DiagnosticsComponent_SetDeviceName: device name set to '%s'", deviceId);
    }

    return true;
}

bool DiagnosticsComponent_GetDeviceName(char** deviceNameHandle)
{
    if (deviceNameHandle == NULL)
    {
        Log_Error("DiagnosticsComponent_GetDeviceName: deviceNameHandle is NULL");
    }

    if (s_DiagnosticsDeviceName == NULL)
    {
        Log_Error("DiagnosticsComponent_GetDeviceName: device name has not been set");
    }

    if (mallocAndStrcpy_s(deviceNameHandle, STRING_c_str(s_DiagnosticsDeviceName)) != 0)
    {
        Log_Error("DiagnosticsComponent_GetDeviceName: mallocAndStrcpy_s failed");
        return false;
    }

    Log_Debug("DiagnosticsComponent_GetDeviceName: returning device name '%s'", STRING_c_str(s_DiagnosticsDeviceName));
    return true;
}

void DiagnosticsComponent_DestroyDeviceName(void)
{
    Log_Debug("DiagnosticsComponent_DestroyDeviceName: destroying device name");
    STRING_delete(s_DiagnosticsDeviceName);
    s_DiagnosticsDeviceName = NULL;
}
