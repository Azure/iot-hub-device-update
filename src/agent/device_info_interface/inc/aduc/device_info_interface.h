/**
 * @file device_info_interface.h
 * @brief Methods to communicate with "dtmi:azure:DeviceManagement:DeviceInformation;1" interface.
 *
 *        DeviceInfo only reports properties defining the device and does not accept requested properties or commands.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */
#ifndef ADUC_DEVICE_INFO_INTERFACE_H
#define ADUC_DEVICE_INFO_INTERFACE_H

#include <aduc/c_utils.h>
#include <aduc/client_handle.h>
#include <azureiot/iothub_client_core_common.h>

#include <stdbool.h>

EXTERN_C_BEGIN

/**
 * @brief Handle for DeviceInfo component to communicate to service.
 */
extern ADUC_ClientHandle g_iotHubClientHandleForDeviceInfoComponent;
//
// Registration/Unregistration
//

/**
 * @brief Initialize the interface.
 *
 * @param[out] componentContext Optional context object to use in related calls.
 * @param argc Count of arguments in @p argv
 * @param argv Command line parameters.
 * @return _Bool True on success.
 */
_Bool DeviceInfoInterface_Create(void** componentContext, int argc, char** argv);

/**
 * @brief Called after connected to IoTHub (device client handler is valid).
 *
 * @param context Context object from Create.
 */
void DeviceInfoInterface_Connected(void* componentContext);

/**
 * @brief Uninitialize the interface.
 *
 * @param componentContext Optional context object which was returned from Create.
 */
void DeviceInfoInterface_Destroy(void** componentContext);

//
// Reporting
//

/**
 * @brief Report any changed DeviceInfo properties up to server.
 */
void DeviceInfoInterface_ReportChangedPropertiesAsync();

EXTERN_C_END

#endif // ADUC_DEVICE_INFO_INTERFACE_H
