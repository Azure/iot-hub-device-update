/**
 * @file diagnostics_devicename.c
 * @brief Implements function necessary for getting and setting the devicename for the diagnostics_component
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "diagnostics_devicename.h"

#include <azure_c_shared_utility/crt_abstractions.h>
#include <azure_c_shared_utility/strings.h>
#include <string.h>

static STRING_HANDLE s_DiagnosticsDeviceName = NULL;

_Bool DiagnosticsComponent_SetDeviceName(const char* deviceId, const char* moduleId)
{
    if (deviceId == NULL)
    {
        return false;
    }

    if (s_DiagnosticsDeviceName != NULL)
    {
        if (STRING_empty(s_DiagnosticsDeviceName) != 0)
        {
            return false;
        }
    }
    else
    {
        s_DiagnosticsDeviceName = STRING_new();

        if (s_DiagnosticsDeviceName == NULL)
        {
            return false;
        }
    }

    if (moduleId != NULL)
    {
        if (STRING_sprintf(s_DiagnosticsDeviceName, "%s/%s", deviceId, moduleId) != 0)
        {
            return false;
        }
    }
    else
    {
        if (STRING_sprintf(s_DiagnosticsDeviceName, "%s", deviceId) != 0)
        {
            return false;
        }
    }

    return true;
}

_Bool DiagnosticsComponent_GetDeviceName(char** deviceNameHandle)
{
    if (mallocAndStrcpy_s(deviceNameHandle, STRING_c_str(s_DiagnosticsDeviceName)) != 0)
    {
        return false;
    }
    return true;
}

void DiagnosticsComponent_DestroyDeviceName(void)
{
    STRING_delete(s_DiagnosticsDeviceName);
    s_DiagnosticsDeviceName = NULL;
}
