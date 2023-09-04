/**
 * @file client_handle_helper.h
 * @brief Declares an abstract interface for communicating through the ModuleClient or DeviceClient libraries.
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#ifndef CLIENT_HANDLE_HELPER_H
#define CLIENT_HANDLE_HELPER_H

#include "aduc/client_handle.h"
#include <aduc/adu_types.h>
#include <aduc/c_utils.h>
#include <azureiot/iothub_client_core_common.h>
#include <stdbool.h>
#include <stdlib.h>
#include <umock_c/umock_c_prod.h>

EXTERN_C_BEGIN

/**
 * @brief Wrapper function for the Device and Module CreateFromConnectionString functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle the clientHandle to be set by the createFromConnectionString function
 * @param connectionString connectionString that will be used to create the client connection
 * @param protocol the protocol to use to create the client connection
 * @returns true on success false on failure
 */
MOCKABLE_FUNCTION(
    ,
    bool,
    ClientHandle_CreateFromConnectionString,
    ADUC_ClientHandle*,
    iotHubClientHandle,
    ADUC_ConnType,
    type,
    const char*,
    connectionString,
    IOTHUB_CLIENT_TRANSPORT_PROVIDER,
    protocol)

/**
 * @brief Wrapper function for the Device and Module SetConnectionStatusCallback function
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle the clientHandle to be used for the operation
 * @param connectionStatusCallback the callback for the connection status
 * @param userContextCallback context for the callback usually a tracking data structure
 * @returns a value of IOTHUB_CLIENT_RESULT
 */
MOCKABLE_FUNCTION(
    ,
    IOTHUB_CLIENT_RESULT,
    ClientHandle_SetConnectionStatusCallback,
    ADUC_ClientHandle,
    iotHubClientHandle,
    IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK,
    connectionStatusCallback,
    void*,
    userContextCallback)

/**
 * @brief Wrapper for the Device and Module SendEventAsync functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle clientHandle to be used for the operation
 * @param eventMessageHandle Message handle to be sent to the IotHub
 * @param eventConfirmationCallback Callback to be used once the message has been sent
 * @param userContextCallback Parameter to be passed to @p eventConfirmationCallback once the message has been sent
 * @returns a value of IOTHUB_CLIENT_RESULT
 */
MOCKABLE_FUNCTION(
    ,
    IOTHUB_CLIENT_RESULT,
    ClientHandle_SendEventAsync,
    ADUC_ClientHandle,
    iotHubClientHandle,
    IOTHUB_MESSAGE_HANDLE,
    eventMessageHandle,
    IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK,
    eventConfirmationCallback,
    void*,
    userContextCallback);

/**
 * @brief Wrapper for the Device and Module DoWork functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle the clientHandle to be used for the operation
 */
MOCKABLE_FUNCTION(, void, ClientHandle_DoWork, ADUC_ClientHandle, iotHubClientHandle);

/**
 * @brief Wrapper for the Device and Module SetOption functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle ADUC_ClientHandle to be used for the operation
 * @param optionName Name of the option to be set
 * @param value Value of the option to be set
 * @returns a value of IOTHUB_CLIENT_RESULT
 */
MOCKABLE_FUNCTION(
    ,
    IOTHUB_CLIENT_RESULT,
    ClientHandle_SetOption,
    ADUC_ClientHandle,
    iotHubClientHandle,
    const char*,
    optionName,
    const void*,
    value);

/**
 * @brief Wrapper for the device or model GetTwinAsync functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle The clientHandle to be used for the operation
 * @param deviceTwinCallback Callback for when the function completes
 * @param userContextCallback A parameter to @p deviceTwinCallback
 * @returns a value of IOTHUB_CLIENT_RESULT
 */
MOCKABLE_FUNCTION(
    ,
    IOTHUB_CLIENT_RESULT,
    ClientHandle_GetTwinAsync,
    ADUC_ClientHandle,
    iotHubClientHandle,
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK,
    deviceTwinCallback,
    void*,
    userContextCallback)

/**
 * @brief Wrapper for the Device and Module SetClientTwinCallback functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle The clientHandle to be used for the operation
 * @param deviceTwinCallback Callback for when the function completes
 * @param userContextCallback A parameter to @p deviceTwinCallback
 * @returns a value of IOTHUB_CLIENT_RESULT
 */
MOCKABLE_FUNCTION(
    ,
    IOTHUB_CLIENT_RESULT,
    ClientHandle_SetClientTwinCallback,
    ADUC_ClientHandle,
    iotHubClientHandle,
    IOTHUB_CLIENT_DEVICE_TWIN_CALLBACK,
    deviceTwinCallback,
    void*,
    userContextCallback)

/**
 * @brief Wrapper for the Device and Module SendReportedState functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle the clientHandle to be used for the operation
 * @param reportedState reportedState to send to the IotHub
 * @param size the size of @p reportedState
 * @param reportedStateCallback callback for when the call is completed
 * @param userContextCallback the parameter that will be passed to @p reportedStateCallback
 * @returns a value of IOTHUB_CLIENT_RESULT
 */
MOCKABLE_FUNCTION(
    ,
    IOTHUB_CLIENT_RESULT,
    ClientHandle_SendReportedState,
    ADUC_ClientHandle,
    iotHubClientHandle,
    const unsigned char*,
    reportedState,
    size_t,
    size,
    IOTHUB_CLIENT_REPORTED_STATE_CALLBACK,
    reportedStateCallback,
    void*,
    userContextCallback)

/**
 * @brief Wrapper for the Device and Module SetDeviceMethodCallback functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle the clientHandle to be used for the operation
 * @param deviceMethodCallback callback for  when the device method is set
 * @param userContextCallback the argument that will be passed to @p deviceMethodCallback
 * @returns a value of IOTHUB_CLIENT_RESULT
 */
MOCKABLE_FUNCTION(
    ,
    IOTHUB_CLIENT_RESULT,
    ClientHandle_SetDeviceMethodCallback,
    ADUC_ClientHandle,
    iotHubClientHandle,
    IOTHUB_CLIENT_DEVICE_METHOD_CALLBACK_ASYNC,
    deviceMethodCallback,
    void*,
    userContextCallback)

/**
 * @brief Wrapper for the Device and Module Destroy functions
 * @details Uses either the device or module function depending on what the client type has been set to.
 * @param iotHubClientHandle the clientHandle to be used for the operation
 */
MOCKABLE_FUNCTION(, void, ClientHandle_Destroy, ADUC_ClientHandle, iotHubClientHandle)

EXTERN_C_END
#endif // CLIENT_HANDLE_HELPER_H
