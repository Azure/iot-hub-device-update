/**
 * @file diagnostics_devicename.h
 * @brief Header file describing the methods for setting the static global device name for use in the diagnostic component
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef DIAGNOSTICS_DEVICENAMES_H
#    define DIAGNOSTICS_DEVICENAMES_UTILS_H

#    include <aduc/c_utils.h>
#    include <stdbool.h>

EXTERN_C_BEGIN

/**
 * @brief Sets the static global device name with the concatentation of @p deviceId and @p moduleId
 * @details Format for device name is <device-id>/<module-id> with a max size of 256 characters
 * @param deviceId the deviceId to be used to create the device name
 * @param moduleId the moduleId to be used to create the device name
 * @returns true on success; false on failure
 */
bool DiagnosticsComponent_SetDeviceName(const char* deviceId, const char* moduleId);

/**
 * @brief Initializes @p deviceNameHandle with the value stored in g_diagnosticsDeviceName
 * @param deviceNameHandle handle to be initialized with the device name
 * @returns true on success; false on failure
 */
bool DiagnosticsComponent_GetDeviceName(char** deviceNameHandle);

/**
 * @brief Free's the currently allocated DeviceName if it has been set
 */
void DiagnosticsComponent_DestroyDeviceName(void);

EXTERN_C_END

#endif // DIAGNOSTICS_DEVICENAMES_UTILS_H
